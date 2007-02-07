/*
 * @file file-sync - A plugin for the opensync framework
 *
 * Copyright © 2005 Danny Backx <dannybackx@users.sourceforge.net>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */
#include <rapi.h>
#include <synce_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <glib.h>

#include <opensync/opensync.h>
#include <opensync/file.h>

#include <rra/appointment.h>
#include <rra/contact.h>
#include <rra/task.h>
#include <rra/matchmaker.h>
#include <rra/uint32vector.h>
#include <rra/syncmgr.h>
#include <rapi.h>
#include <synce.h>
#include "synce_plugin.h"

static time_t DOSFS_FileTimeToUnixTime(const FILETIME *filetime,
                                       DWORD *remainder);

/*
 * Do this by ourselves.  The FILETIME structure is a 64-bit value
 * representing the number of 100-nanosecond intervals since January
 * 1, 1601, 0:00. 'remainder' is the nonnegative number of 100-ns
 * intervals corresponding to the time fraction smaller than 1 second
 * that couldn't be stored in the time_t value.
 */
static time_t CeTimeToUnixTime(FILETIME t)
{
	DWORD	r;

	return DOSFS_FileTimeToUnixTime(&t, &r);
}

/** Report to opensync library information about given file: its uid
    (path with the root directory stripped), format (file) and hash
    (currently modification date and size). For performance reasons,
    no file content is passed at this stage. opensync library will
    fetch it later if needed.
 */
static osync_bool
report_file_change(OSyncContext *ctx, const char *dir,
		CE_FIND_DATA *entry, OSyncError **error)
{
	SyncePluginPtr	*env;
	char				path[MAX_PATH], *hash, *lpath;
	WCHAR				*wfn;
	fileFormat			*ff;
	time_t				t1, t2;

	lpath = wstr_to_current(entry->cFileName);
	snprintf(path, MAX_PATH, "%s\\%s", dir, lpath);
	wfn = wstr_from_current(path);

        ff = g_malloc0(sizeof(fileFormat));
	ff->userid = 0;
	ff->groupid = 0;
	ff->mode = 0777;			/* Fake */

	if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		ff->mode |= S_IFDIR;	/* Convert Windows knowledge into Unix */
	} else {
	}

	/* Select the latest time */
	t1 = CeTimeToUnixTime(entry->ftLastWriteTime);
	t2 = CeTimeToUnixTime(entry->ftCreationTime);
	ff->last_mod = (t1 < t2) ? t2 : t1;

	if (ctx) {
		int	l;
                HANDLE h;
                unsigned size;
		env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
                OSyncChange	*change = osync_change_new();
		osync_change_set_member(change, env->member);
                /* One is the separating backslash as appended above with snprintf() */
                l = strlen(env->config_file) + 1;
		osync_change_set_uid(change, path + l);
                osync_change_set_objformat_string(change, "file");
                /* Get file size needed for hash */
                h = CeCreateFile(wfn, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL, 0);
                if (h) {
                  size = CeGetFileSize(h, NULL);
                  CeCloseHandle(h);
                } else size = 0;
                hash = g_strdup_printf("%ld-%u", ff->last_mod, size);
		osync_change_set_hash(change, hash);
                fprintf(stderr, "%s(%s) hash %s\n", __func__, path, hash);
                g_free(hash);
		/*
		 * Pass the structure to OpenSync
		 * The final parameter is FALSE to indicate a get_data() call will fetch
		 * the file contents if necessary later on.
		 */
		osync_change_set_data(change, (char *)ff, sizeof(fileFormat), FALSE);

		if (osync_hashtable_detect_change(env->hashtable, change)) {
			osync_context_report_change(ctx, change);
			osync_hashtable_update_hash(env->hashtable, change);
		}
	}

	wstr_free_string(wfn);
	free(lpath);
	return TRUE;
}

static osync_bool
FilesFindAllFromDirectory(OSyncContext *ctx, const char *dir,
                          OSyncError **error)
{
	WCHAR		*wd;
	CE_FIND_DATA	*find_data = NULL;
	int		i;
	DWORD		file_count;
	char		path[MAX_PATH];

	if (! dir)
		return TRUE;
        fprintf(stderr, "%s(%s)\n", __func__, dir);
	snprintf(path, MAX_PATH, "%s\\*", dir);

	wd = wstr_from_current(path);
        if (CeFindAllFiles(wd,
                           FAF_ATTRIBUTES|FAF_LASTWRITE_TIME|FAF_NAME|FAF_SIZE_LOW,
                           &file_count, &find_data)) {
                for (i=0; i<file_count; i++) {
			CE_FIND_DATA	*entry = &find_data[i];

			/* Do NOT report directories */
			if ((entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				/*
				 * Report this file to the OpenSync.
				 */
                          if (! report_file_change(ctx, dir, entry, error)) {
					/* Failure */
                                        wstr_free_string(wd);
					return FALSE;
				}
			}

			/* Recursive call for directories */
			if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				snprintf(path, MAX_PATH, "%s\\%s",
						dir, wstr_to_current(entry->cFileName));
				if (! FilesFindAllFromDirectory(ctx, path, error)) {
					/* Failure, pass on the word */
                                        wstr_free_string(wd);
					return FALSE;
				}
			}
		}
	}

	CeRapiFreeBuffer(find_data);
	wstr_free_string(wd);
	return TRUE;
}

static gchar*
get_complete_file_name(const char *config_prefix, const char *rest)
{
        char *s, *lfn;

        /* Concatenate our root and the local file name */
        lfn = g_strdup_printf("%s\\%s", config_prefix, rest);

        /* Translate slashes into backslashes */
        for (s=lfn; *s; s++)
                if (*s == '/')
                        *s = '\\';

        /* fprintf(stderr, "Full fn(%s)\n", lfn); */
        return lfn;
}
/**
 * This function gets the command for the SynCE plugin to add, modify
 * or delete a file.
 *
 * Reply to the OpenSync framework with
 *	osync_context_report_success(ctx);
 *
 */
osync_bool
synce_file_commit(OSyncContext *ctx, OSyncChange *change)
{
	SyncePluginPtr	*env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	HANDLE		h;
	fileFormat	*ff;
	WCHAR		*wfn;
	DWORD		wr, e, opt;
        char		*p, *s, *fn, *lfn, *str;
	OSyncChangeType	ct;

	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);	
        if(!env->config_file) {
                osync_context_report_error(ctx, 1,
                                           "<file> parameter not set");
                return FALSE;
        }
		
	ff = (fileFormat *)osync_change_get_data(change);
	fn = (char *)osync_change_get_uid(change);
	ct = osync_change_get_changetype(change);
        lfn = get_complete_file_name(env->config_file, fn);
	wfn = wstr_from_current(lfn);

	switch (ct) {
		case CHANGE_DELETED:
                        fprintf(stderr, "%s: DELETED %s\n", __func__,
                                osync_change_get_uid(change));
			if (ff == NULL) {
				/* Something went wrong, e.g. the file isn't there. */
				wstr_free_string(wfn);
                                g_free(lfn);
				/* FIX ME is this a valid return value for this case ? */
				return TRUE;
			}
			if (S_ISREG(ff->mode)) {	/* Regular file */
				if (CeDeleteFile(wfn) == 0) {
					e = CeGetLastError();
					s = synce_strerror(e);
					osync_context_report_error(ctx, 1,
							"CeDeleteFile(%s) : %s", fn, s);
					wstr_free_string(wfn);
                                        g_free(lfn);
					return FALSE;
				}
			} else if (S_ISDIR(ff->mode)) {	/* Directory */
				/* Removing directories... should we do this ? */
			}
			break;
		case CHANGE_ADDED:
		case CHANGE_MODIFIED:
			/* First check whether the directory tree exists. */
			for (s=p=lfn+1; *p; p++) {
				if (*p == '\\') {
					WCHAR	*w;
					DWORD	cnt;
					CE_FIND_DATA	*find_data = NULL;

					/* Make a dir name */
					*p = '\0';
					w = wstr_from_current(lfn);

					/* Check whether this dir exists */
					if (CeFindAllFiles(w, FAF_FOLDERS_ONLY|FAF_ATTRIBUTES,
							&cnt, &find_data) == 0) {
						/* FIX ME what does failure mean here ? */
						e = CeGetLastError();
						s = synce_strerror(e);
						osync_context_report_error(ctx, 1,
							"CeFindAllFiles(%s) : %s",
							lfn, s);
						wstr_free_string(w);
						wstr_free_string(wfn);
                                                g_free(lfn);
						return FALSE;
					}
					if (cnt == 0) {
						/* Create it */
						fprintf(stderr, "Yow create(%s)\n", lfn);
						if (CeCreateDirectory(w, NULL) == 0) {
							e = CeGetLastError();
							s = synce_strerror(e);
							osync_context_report_error(ctx, 1,
								"CeCreateDirectory(%s) : %s",
								lfn, s);
							wstr_free_string(w);
							wstr_free_string(wfn);
							free(lfn);
							return FALSE;
						}
					}
					/* Restore string, forward loop */
					*p = '\\';
					s = p+1;
					wstr_free_string(w);
					CeRapiFreeBuffer(find_data);
				}
			}

			/* Go on treating the file */
			if (ct == CHANGE_ADDED) {
				str = "ADDED";
				/*
				 * We need to add a new file.
				 * Check whether it already exists by using CREATE_NEW.
				 * Fail if it already existed.
				 */
				opt = CREATE_NEW;
			} else {
				str = "MODIFIED";
				/*
				 * Modify an existing file.
				 * The CeCreateFile() call fails by using this option if the
				 * file doesn't exist. The call will truncate the existing file
				 * so it becomes empty.
				 */
				opt = TRUNCATE_EXISTING;
			}
                        fprintf(stderr, "%s: %s %s\n", __func__, str,
                                osync_change_get_uid(change));

			if (S_ISDIR(ff->mode)) {	/* Directory */
				/* FIX ME */
				if (CeCreateDirectory(wfn, NULL) == 0) {
					e = CeGetLastError();
					s = synce_strerror(e);
					osync_context_report_error(ctx, 1,
							"CeCreateDirectory(%s) : %s", fn, s);
					wstr_free_string(wfn);
                                        g_free(lfn);
					return FALSE;
				}
			} else if (S_ISREG(ff->mode)) {	/* Regular file */
				h = CeCreateFile(wfn, GENERIC_WRITE, 0 /* Don't share */, NULL,
						opt, FILE_ATTRIBUTE_NORMAL, 0);
				if (h == 0) {
					e = CeGetLastError();
					s = synce_strerror(e);
					osync_context_report_error(ctx, 1,
							"CeCreateFile(%s) : %s", fn, s);
					wstr_free_string(wfn);
                                        g_free(lfn);
					return FALSE;
				}

				/*
				 * Succeeded in creating the file -> now write the contents.
				 */
				if (CeWriteFile(h, ff->data, ff->size, &wr, NULL) == 0) {
					e = CeGetLastError();
					s = synce_strerror(e);
					osync_context_report_error(ctx, 1,
							"CeWriteFile(%s, sz %d) : %s",
							lfn, ff->size, s);
					CeCloseHandle(h);
					wstr_free_string(wfn);
                                        g_free(lfn);
					return FALSE;
				}
				CeCloseHandle(h);
			}

			/*
			 * All done
			 */
			break;
		default:
			osync_debug("SYNCE-SYNC", 4, "Unknown change type");
			fprintf(stderr, "%s: ?? %s\n", __func__, osync_change_get_uid(change));
	}

	/* Answer the call */
	osync_context_report_success(ctx);
	osync_hashtable_update_hash(env->hashtable, change);

        wstr_free_string(wfn);
        g_free(lfn);
	return TRUE;
}

/*
 * File get_changeinfo : report the changes to the OpenSync engine.
 */
osync_bool
synce_file_get_changeinfo(OSyncContext *ctx, OSyncError **error)
{
	SyncePluginPtr	*env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);

	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);

        /* Detect whether we need to do a slow sync - this is supported by
           the hash table */
	if (osync_member_get_slow_sync(env->member, "data"))
		osync_hashtable_set_slow_sync(env->hashtable, "data");

	/* SynCE : check if RRA is connected */
	if (!env->syncmgr || !rra_syncmgr_is_connected(env->syncmgr)){
		/* not connected, exit */
		osync_context_report_error(ctx, 1, "not connected to device, exit.");
		return FALSE;
	}

	osync_debug("SynCE-file", 4, "checking files");

	/*
	 * We're supporting sync of exactly one directory.
	 */
        if (env->config_file && 
            ! FilesFindAllFromDirectory(ctx, env->config_file, error)) {
		osync_context_report_error(ctx, 1, "Error while checking for files");
		return FALSE;
	}

	osync_hashtable_report_deleted(env->hashtable, ctx, "data");

        /* Don't report via osync_context_report_success(ctx) here, our
         * caller will already be doing that.
	 */
	return TRUE;
}

/*
 * FIX ME. This function is known to sometimes fail, it sometimes gets
 * called with all zeroes in the structure pointed to by ff.
 * NOTE: pawsa never saw this problem.
 */
void
synce_file_getdata(OSyncContext *ctx, OSyncChange *change)
{
	SyncePluginPtr	*env;
	fileFormat			*ff;
	HANDLE				h;
	uint32_t			rsz;
	WCHAR				*wfn;
	int				r;
        char       *lfn;
        const char *fname;

	osync_debug("SynCE-File", 4, "start : %s", __func__);
	env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	ff = (fileFormat *)osync_change_get_data(change);

#if 0
	if (ff == NULL || ff->data == NULL || ff->size == 0) {
                osync_debug("SynCE-File", 4,
                            "%s: NULL values encountered, returning\n", __func__);
		osync_context_report_error(ctx, 1, "get_data got NULLs");
		return;
	}
#endif
	/*
	 * Directory ?
	 */
	if (S_ISDIR(ff->mode)) {
		fprintf(stderr, "YOW -> directory %s\n", ff->data);
		osync_context_report_success(ctx);
		return;
	}
        fname = osync_change_get_uid(change);
        lfn = get_complete_file_name(env->config_file, fname);
        wfn = wstr_from_current(lfn);
	/* Read the file through SynCE */
        h = CeCreateFile(wfn, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, 0);

        ff->size = CeGetFileSize(h, NULL);
        if(ff->size == 0xFFFFFFFF) {
                DWORD e = CeGetLastError();
                const char *s = synce_strerror(e);
                osync_context_report_error(ctx, 4,
                                           "\n%s: could not find out file size (%u): %s\n",
                                           __func__, (unsigned)e, s);
                CeCloseHandle(h);
                g_free(lfn);
                wstr_free_string(wfn);
                return;
        }
        ff->data = g_malloc(ff->size);
        if(ff->size>0) {
                r = CeReadFile(h, ff->data, ff->size, &rsz, NULL);
                if (r == 0) {
                        /* Error */
                        DWORD	e = CeGetLastError();
                        char	*s = synce_strerror(e);
                        osync_context_report_error(ctx, 1, "Error from CeReadFile (%d:%s)",
                                                   e, s);
                        CeCloseHandle(h);
                        return;
                }
        }

	/* Send its contents */
	osync_change_set_data(change, (char *)ff, sizeof(fileFormat), TRUE);
	CeCloseHandle(h);

	wstr_free_string(wfn);
	osync_context_report_success(ctx);
	osync_debug("SynCE-File", 4, "end : %s", __func__);
}

/* See if we can pass a list of directories found on the device.
 * Handle directory name concatenation with care.
 */
GList*
ListAllDirectories(GList *list, char *dir, int recurse)
{
	WCHAR		*wd;
	CE_FIND_DATA	*find_data = NULL;
	int		i;
	DWORD		file_count;
	char		path[MAX_PATH];
	osync_bool	isroot;

        /* Detect root directory as special case, the only one with tailing
           backslash */
	isroot = (dir[0] == '\\' && dir[1] == '\0');
	if (isroot)
		strcpy(path, "\\*");
	else {
		/* Remove trailing backslash in other cases */
		for (i=0; dir[i]; i++) ;
		if (dir[i-1] == '\\')
			dir[i-1] = '\0';

		/* Append wildcard */
		snprintf(path, MAX_PATH, "%s\\*", dir);
	}
	wd = wstr_from_current(path);
	if (CeFindAllFiles(wd, FAF_FOLDERS_ONLY | FAF_NAME | FAF_ATTRIBUTES,
			&file_count, &find_data)) {
		for (i=0; i<file_count; i++) {
			CE_FIND_DATA	*entry = &find_data[i];

			/* Recursive call for directories */
			if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				char	*r, *fn = wstr_to_current(entry->cFileName);

				if (isroot)
					r = g_strdup_printf("\\%s", fn);
				else
					r = g_strdup_printf("%s\\%s", dir, fn);
				wstr_free_string(fn);

#if 0
				fprintf(stderr, "%s -> %s\n", __func__, r);
#endif
				list = g_list_append(list, r);
				if (recurse)
					list = ListAllDirectories(list, r, recurse);
			}
		}
	} else {
		fprintf(stderr, "%s: CeFindAllFiles(%s) failure\n", __func__, dir);
	}

	CeRapiFreeBuffer(find_data);
	wstr_free_string(wd);
	return list;
}

/*
 * This function is called by the configuration GUI,
 * it gives it a list of interesting directories on the PDA,
 * which the user might like to choose from to synchronize.
 *
 * Do not make this function static, as the confuration GUI needs to
 * be able to locate it using dlsym(); also don't change its name or
 * signature without making the corresponding change in the
 * configuration program.
 */
GList*
synce_list_directories(SyncePluginPtr *env, const char *basedir,
                       OSyncError **error)
{
	HRESULT hr;
	GList	*ret = NULL;
#if 0
	WCHAR	path[MAX_PATH];
	char	*s = NULL;
	int	r;
#endif

	hr = CeRapiInit();
#if 0
	int	i;

	for (i=0; i<255; i++) {
		r = CeGetSpecialFolderPath(i, MAX_PATH, path);
		if (r > 0) {
			s = wstr_to_current(path);
			ret = g_list_append(ret, s);
			free(s);
		}
	}
#endif
	/*
         * List all the directories starting with root, this appears
         * to be a much better solution The final parameter is whether
         * to recurse (0 only lists the top level dirs on the device,
         * 1 will list all directories).
	 */
	ret = ListAllDirectories(ret, "\\", 1);

	CeRapiUninit();
	return ret;
}

#ifdef	TEST_FILE
int main(int argc, char** argv)
{
	char* path = NULL;
	WCHAR* wide_path = NULL;
	HRESULT hr;
	
	hr = CeRapiInit();

	if (FAILED(hr)) {
		fprintf(stderr, "%s: Unable to initialize RAPI: %s\n", argv[0],
				synce_strerror(hr));
		goto exit;
	}

/*
	FilesFindAllFromDirectory(NULL, "\\My Documents\\foto", NULL);
	(void) synce_list_directories(NULL, NULL, NULL);
*/

{
	char	*lfn = strdup("/My documents/foto/aa/bb/yy.txt");
	char	*s, *p;
	WCHAR	*wfn;
	DWORD	e;
	HANDLE	h;

	/* Translate slashes into backslashes */
	for (s=lfn; *s; s++)
		if (*s == '/')
			*s = '\\';

    /* Convert file name into WIDE format for WinCE - this must be the
       complete file name */
	fprintf(stderr, "Wide fn(%s)\n", lfn);
	wfn = wstr_from_current(lfn);

			/* First check whether the directory tree exists. */
	for (s=p=lfn+1; *p; p++) {
		if (*p == '\\') {
			WCHAR	*w;
			DWORD	cnt;
			CE_FIND_DATA	*find_data = NULL;

			/* Make a dir name */
			*p = '\0';
			w = wstr_from_current(lfn);

			/* Check whether this dir exists */
			if (CeFindAllFiles(w, FAF_FOLDERS_ONLY|FAF_ATTRIBUTES,
					&cnt, &find_data) == 0) {
				/* FIX ME what does failure mean here ? */
				e = CeGetLastError();
				s = synce_strerror(e);
				fprintf(stderr,
					"CeFindAllFiles(%s) : %s",
					lfn, s);
				wstr_free_string(w);
				wstr_free_string(wfn);
				free(lfn);
				return FALSE;
			}
			if (cnt == 0) {
				/* Create it */
				fprintf(stderr, "Yow create(%s)\n", lfn);
				if (CeCreateDirectory(w, NULL) == 0) {
					e = CeGetLastError();
					s = synce_strerror(e);
					fprintf(stderr,
						"CeCreateDirectory(%s) : %s",
						lfn, s);
					wstr_free_string(w);
					wstr_free_string(wfn);
					free(lfn);
					return FALSE;
				}
			}
			/* Restore string, forward loop */
			*p = '\\';
			s = p+1;
			wstr_free_string(w);
			CeRapiFreeBuffer(find_data);
		}
	}
	h = CeCreateFile(wfn, GENERIC_WRITE, 0 /* Don't share */, NULL,
			CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == 0) {
		e = CeGetLastError();
		s = synce_strerror(e);
		fprintf(stderr, "CeCreateFile(%s) : %s", lfn, s);
		free(lfn);
		return FALSE;
	}

	/*
	 * Succeeded in creating the file -> now write the contents.
	 */
	DWORD wr;
	if (CeWriteFile(h, "Yow", 3, &wr, NULL) == 0) {
		e = CeGetLastError();
		s = synce_strerror(e);
		fprintf(stderr, "CeWriteFile(%s) : %s", lfn, s);
		free(lfn);
		return FALSE;
	}
	CeCloseHandle(h);
	wstr_free_string(wfn);
}
exit:
	if (path)
		free(path);

	wstr_free_string(wide_path);

	CeRapiUninit();
	return 0;
}
#endif

/*
 * Copyright (c) 1993-2002 the Wine project authors
 *
 * These functions come from wine/files/dos_fs.c in release 20020228 of the
 * WINE project. See http://www.winehq.org/ for more information. 
 *
 * Licensing information for the code below:
 *
 *   http://source.winehq.org/source/LICENSE?v=wine20020228
 *
 */


/***********************************************************************
 *           DOSFS_UnixTimeToFileTime
 *
 * Convert a Unix time to FILETIME format.
 * The FILETIME structure is a 64-bit value representing the number of
 * 100-nanosecond intervals since January 1, 1601, 0:00.
 * 'remainder' is the nonnegative number of 100-ns intervals
 * corresponding to the time fraction smaller than 1 second that
 * couldn't be stored in the time_t value.
 */
#if 0
static void DOSFS_UnixTimeToFileTime( time_t unix_time, FILETIME *filetime,
                               DWORD remainder )
{
    /* NOTES:

       CONSTANTS:
       The time difference between 1 January 1601, 00:00:00 and
       1 January 1970, 00:00:00 is 369 years, plus the leap years
       from 1604 to 1968, excluding 1700, 1800, 1900.
       This makes (1968 - 1600) / 4 - 3 = 89 leap days, and a total
       of 134774 days.

       Any day in that period had 24 * 60 * 60 = 86400 seconds.

       The time difference is 134774 * 86400 * 10000000, which can be written
       116444736000000000
       27111902 * 2^32 + 3577643008
       413 * 2^48 + 45534 * 2^32 + 54590 * 2^16 + 32768

       If you find that these constants are buggy, please change them in all
       instances in both conversion functions.

       VERSIONS:
       There are two versions, one of them uses long long variables and
       is presumably faster but not ISO C. The other one uses standard C
       data types and operations but relies on the assumption that negative
       numbers are stored as 2's complement (-1 is 0xffff....). If this
       assumption is violated, dates before 1970 will not convert correctly.
       This should however work on any reasonable architecture where WINE
       will run.

       DETAILS:

       Take care not to remove the casts. I have tested these functions
       (in both versions) for a lot of numbers. I would be interested in
       results on other compilers than GCC.

       The operations have been designed to account for the possibility
       of 64-bit time_t in future UNICES. Even the versions without
       internal long long numbers will work if time_t only is 64 bit.
       A 32-bit shift, which was necessary for that operation, turned out
       not to work correctly in GCC, besides giving the warning. So I
       used a double 16-bit shift instead. Numbers are in the ISO version
       represented by three limbs, the most significant with 32 bit, the
       other two with 16 bit each.

       As the modulo-operator % is not well-defined for negative numbers,
       negative divisors have been avoided in DOSFS_FileTimeToUnixTime.

       There might be quicker ways to do this in C. Certainly so in
       assembler.

       Claus Fischer, fischer@iue.tuwien.ac.at
       */

#if SIZEOF_LONG_LONG >= 8
#  define USE_LONG_LONG 1
#else
#  define USE_LONG_LONG 0
#endif

#if USE_LONG_LONG		/* gcc supports long long type */

    long long int t = unix_time;
    t *= 10000000;
    t += 116444736000000000LL;
    t += remainder;
    filetime->dwLowDateTime  = (UINT)t;
    filetime->dwHighDateTime = (UINT)(t >> 32);

#else  /* ISO version */

    UINT a0;			/* 16 bit, low    bits */
    UINT a1;			/* 16 bit, medium bits */
    UINT a2;			/* 32 bit, high   bits */

    /* Copy the unix time to a2/a1/a0 */
    a0 =  unix_time & 0xffff;
    a1 = (unix_time >> 16) & 0xffff;
    /* This is obsolete if unix_time is only 32 bits, but it does not hurt.
       Do not replace this by >> 32, it gives a compiler warning and it does
       not work. */
    a2 = (unix_time >= 0 ? (unix_time >> 16) >> 16 :
	  ~((~unix_time >> 16) >> 16));

    /* Multiply a by 10000000 (a = a2/a1/a0)
       Split the factor into 10000 * 1000 which are both less than 0xffff. */
    a0 *= 10000;
    a1 = a1 * 10000 + (a0 >> 16);
    a2 = a2 * 10000 + (a1 >> 16);
    a0 &= 0xffff;
    a1 &= 0xffff;

    a0 *= 1000;
    a1 = a1 * 1000 + (a0 >> 16);
    a2 = a2 * 1000 + (a1 >> 16);
    a0 &= 0xffff;
    a1 &= 0xffff;

    /* Add the time difference and the remainder */
    a0 += 32768 + (remainder & 0xffff);
    a1 += 54590 + (remainder >> 16   ) + (a0 >> 16);
    a2 += 27111902                     + (a1 >> 16);
    a0 &= 0xffff;
    a1 &= 0xffff;

    /* Set filetime */
    filetime->dwLowDateTime  = (a1 << 16) + a0;
    filetime->dwHighDateTime = a2;
#endif
}
#endif


/***********************************************************************
 *           DOSFS_FileTimeToUnixTime
 *
 * Convert a FILETIME format to Unix time.
 * If not NULL, 'remainder' contains the fractional part of the filetime,
 * in the range of [0..9999999] (even if time_t is negative).
 */
static time_t DOSFS_FileTimeToUnixTime( const FILETIME *filetime, DWORD *remainder )
{
    /* Read the comment in the function DOSFS_UnixTimeToFileTime. */
#if USE_LONG_LONG

    long long int t = filetime->dwHighDateTime;
    t <<= 32;
    t += (UINT)filetime->dwLowDateTime;
    t -= 116444736000000000LL;
    if (t < 0)
    {
	if (remainder) *remainder = 9999999 - (-t - 1) % 10000000;
	return -1 - ((-t - 1) / 10000000);
    }
    else
    {
	if (remainder) *remainder = t % 10000000;
	return t / 10000000;
    }

#else  /* ISO version */

    UINT a0;			/* 16 bit, low    bits */
    UINT a1;			/* 16 bit, medium bits */
    UINT a2;			/* 32 bit, high   bits */
    UINT r;			/* remainder of division */
    unsigned int carry;		/* carry bit for subtraction */
    int negative;		/* whether a represents a negative value */

    /* Copy the time values to a2/a1/a0 */
    a2 =  (UINT)filetime->dwHighDateTime;
    a1 = ((UINT)filetime->dwLowDateTime ) >> 16;
    a0 = ((UINT)filetime->dwLowDateTime ) & 0xffff;

    /* Subtract the time difference */
    if (a0 >= 32768           ) a0 -=             32768        , carry = 0;
    else                        a0 += (1 << 16) - 32768        , carry = 1;

    if (a1 >= 54590    + carry) a1 -=             54590 + carry, carry = 0;
    else                        a1 += (1 << 16) - 54590 - carry, carry = 1;

    a2 -= 27111902 + carry;

    /* If a is negative, replace a by (-1-a) */
    negative = (a2 >= ((UINT)1) << 31);
    if (negative)
    {
	/* Set a to -a - 1 (a is a2/a1/a0) */
	a0 = 0xffff - a0;
	a1 = 0xffff - a1;
	a2 = ~a2;
    }

    /* Divide a by 10000000 (a = a2/a1/a0), put the rest into r.
       Split the divisor into 10000 * 1000 which are both less than 0xffff. */
    a1 += (a2 % 10000) << 16;
    a2 /=       10000;
    a0 += (a1 % 10000) << 16;
    a1 /=       10000;
    r   =  a0 % 10000;
    a0 /=       10000;

    a1 += (a2 % 1000) << 16;
    a2 /=       1000;
    a0 += (a1 % 1000) << 16;
    a1 /=       1000;
    r  += (a0 % 1000) * 10000;
    a0 /=       1000;

    /* If a was negative, replace a by (-1-a) and r by (9999999 - r) */
    if (negative)
    {
	/* Set a to -a - 1 (a is a2/a1/a0) */
	a0 = 0xffff - a0;
	a1 = 0xffff - a1;
	a2 = ~a2;

        r  = 9999999 - r;
    }

    if (remainder) *remainder = r;

    /* Do not replace this by << 32, it gives a compiler warning and it does
       not work. */
    return ((((time_t)a2) << 16) << 16) + (a1 << 16) + a0;
#endif
}
