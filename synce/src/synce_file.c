/*
 * file-sync - A plugin for the opensync framework
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
#define	NEW_TIME

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

static time_t DOSFS_FileTimeToUnixTime(const FILETIME *filetime, DWORD *remainder);

/*
 * Do this by ourselves.
 * The FILETIME structure is a 64-bit value representing the number of 100-nanosecond intervals
 * since January 1, 1601, 0:00. 'remainder' is the nonnegative number of 100-ns intervals
 * corresponding to the time fraction smaller than 1 second that couldn't be stored in the
 * time_t value.
 */
static time_t CeTimeToUnixTime(FILETIME t)
{
	DWORD	r;

	return DOSFS_FileTimeToUnixTime(&t, &r);
}

/*
 * A hash appears to be a string.
 * The file-sync plugin allocates new memory in this occasion so I guess that's the right
 * thing to do.
 */
static char *FileHash(fileFormat *p)
{
#if 1
	if (p)
		return g_strdup_printf("%ld-%d", p->last_mod, p->size);
	return NULL;
#else
	/*
	 * This is a slow method, but it'll produce human-readable results.
	 * Not very useful.
	 */
	char		ts[50] = {0};
	struct tm	*time_struct = NULL;

	if (!p)
		return "";

	time_struct = localtime(&p->last_mod);
	strftime(ts, sizeof(ts), "%F", time_struct);
	return g_strdup_printf("%ld %s", (long int)p->size, ts);
#endif
}

/*
 * In this pass of set_data(), do not pass real file contents, but only pass the file name
 * to the opensync engine in the fileFormat structure's data field.
 * See also the comments for synceFileGetData().
 */
static osync_bool FilesReportFileChange(OSyncContext *ctx, const char *dir,
		CE_FIND_DATA *entry, OSyncError **error)
{
	synce_plugin_environment	*env;
	OSyncChange			*change;
	char				path[MAX_PATH], *hash, *lpath;
	WCHAR				*wfn;
	HANDLE				h;
	fileFormat			*ff;
	time_t				t1, t2;

	lpath = wstr_to_current(entry->cFileName);
	snprintf(path, MAX_PATH, "%s\\%s", dir, lpath);
	wfn = wstr_from_current(path);

	fprintf(stderr, "FilesReportFileChange(%s)\n", path);

	/* Start moved up piece of code */
	/* Assemble a structure with data about this file */
	h = CeCreateFile(wfn, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	ff = osync_try_malloc0(sizeof(fileFormat), error);
	if (!ff) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	ff->userid = 0;
	ff->groupid = 0;

	/* Select the latest time */
	t1 = CeTimeToUnixTime(entry->ftLastWriteTime);
	t2 = CeTimeToUnixTime(entry->ftCreationTime);
	ff->last_mod = (t1 < t2) ? t2 : t1;

	ff->size = CeGetFileSize(h, NULL);
	ff->mode = 0777;			/* Fake */

	CeCloseHandle(h);

	/* Set the hash */
	hash = FileHash(ff);

	ff->data = strdup(path);
	/* End moved up piece of code */

	if (ctx) {
		env = (synce_plugin_environment *)osync_context_get_plugin_data(ctx);
		change = osync_change_new();

		osync_change_set_member(change, env->member);
		osync_change_set_uid(change, path);	/* Pass the full file name as UID */
		osync_change_set_objformat_string(change, "file");	/* Copied from file-sync */

		/* Should do the file handling here, but it was moved up for debugging */

		osync_change_set_hash(change, hash);
		/*
		 * Pass the structure to OpenSync
		 * The final parameter is FALSE to indicate a get_data() call will fetch
		 * the file contents if necessary later on.
		 */
		osync_change_set_data(change, (char *)ff, sizeof(fileFormat), FALSE);

		if (osync_hashtable_detect_change(env->hashtable, change)) {
#if 0
			osync_change_set_changetype(change, CHANGE_ADDED);	/* ?? */
#endif
			osync_context_report_change(ctx, change);
			osync_hashtable_update_hash(env->hashtable, change);
		}
		g_free(hash);
	}

	wstr_free_string(wfn);
	free(lpath);
	return TRUE;
}

osync_bool FilesFindAllFromDirectory(OSyncContext *ctx, const char *dir, OSyncError **error)
{
	WCHAR		*wd;
	CE_FIND_DATA	*find_data = NULL;
	int		i;
	DWORD		file_count;
	char		path[MAX_PATH];

	if (! dir)
		return TRUE;
	snprintf(path, MAX_PATH, "%s\\*", dir);

	wd = wstr_from_current(path);
	if (CeFindAllFiles(wd,
			FAF_ATTRIBUTES|FAF_LASTWRITE_TIME|FAF_NAME|FAF_SIZE_LOW|FAF_OID,
			&file_count, &find_data)) {
		for (i=0; i<file_count; i++) {
			CE_FIND_DATA	*entry = &find_data[i];

			/* Report this file to the OpenSync */
			if (! FilesReportFileChange(ctx, dir, entry, error)) {
				/* Failure */
				return FALSE;
			}


			/* Recursive call for directories */
			if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				snprintf(path, MAX_PATH, "%s\\%s",
						dir, wstr_to_current(entry->cFileName));
				(void) /* FIX ME */ FilesFindAllFromDirectory(ctx, path, error);
			}
		}
	}

	CeRapiFreeBuffer(find_data);
	wstr_free_string(wd);
	return TRUE;
}

/*
 * This function gets the command for the SynCE plugin to add, modify or delete a file.
 *
 * Reply to the OpenSync framework with
 *	osync_context_report_success(ctx);
 *
 */
extern osync_bool synceFileCommit(OSyncContext *ctx, OSyncChange *change)
{
	synce_plugin_environment	*env = (synce_plugin_environment *)osync_context_get_plugin_data(ctx);
	HANDLE				h;
	fileFormat			*ff;
	WCHAR				*wfn;
	DWORD				wr;

	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);	
		
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
#if 0
			uint32_t id=0;
			id=strtol(osync_change_get_uid(change),NULL,16);
			osync_debug("SYNCE-SYNC", 4, "deleting contact id: %08x",id);
			fprintf(stderr, "%s: DELETED %s\n", __func__, osync_change_get_uid(change));
			osync_debug("SYNCE-SYNC", 4, "done");
#endif
			break;
		case CHANGE_ADDED:
			fprintf(stderr, "%s: ADDED %s\n", __func__, osync_change_get_uid(change));
			ff = (fileFormat *)osync_change_get_data(change);
			/*
			 * We need to add a new file.
			 * Check whether it already exists by using CREATE_NEW.
			 * Fail if it already existed.
			 */
			wfn = wstr_from_current(osync_change_get_uid(change));
			h = CeCreateFile(wfn, GENERIC_WRITE, 0 /* Don't share */, NULL,
					CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
			if (h == 0) {
				DWORD	e = CeGetLastError();
				char	*s = synce_strerror(e);
				osync_context_report_error(ctx, 1, "CeCreateFile : %s", s);
				wstr_free_string(wfn);
				return FALSE;
			}

			/*
			 * Succeeded in creating the file -> now write the contents.
			 */
			if (CeWriteFile(h, ff->data, ff->size, &wr, NULL) == 0) {
				DWORD	e = CeGetLastError();
				char	*s = synce_strerror(e);
				osync_context_report_error(ctx, 1, "CeWriteFile : %s", s);
				wstr_free_string(wfn);
				return FALSE;
			}
			fprintf(stderr, "Wrote %d bytes\n", wr);

			/*
			 * All done
			 */
			wstr_free_string(wfn);
			CeCloseHandle(h);

			break;
		case CHANGE_MODIFIED:
#if 0
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id,id;

			object=osync_change_get_data(change);
			id=strtol(osync_change_get_uid(change),NULL,16);			

			osync_debug("SYNCE-SYNC", 4, "updating contact id %08x",id);
			fprintf(stderr, "%s: MODIFIED %s\n", __func__, osync_change_get_uid(change));

			osync_change_set_hash(change, FileHash(NULL));

			osync_debug("SYNCE-SYNC", 4, "done");
#endif
			break;
		default:
			osync_debug("SYNCE-SYNC", 4, "Unknown change type");
			fprintf(stderr, "%s: ?? %s\n", __func__, osync_change_get_uid(change));
	}

	/* Answer the call */
	osync_context_report_success(ctx);
	osync_hashtable_update_hash(env->hashtable, change);

	return TRUE;
}

/*
 * File get_changeinfo : report the changes to the OpenSync engine.
 */
extern osync_bool synceFileGetChangeInfo(OSyncContext *ctx, OSyncError **error)
{
	synce_plugin_environment	*env = (synce_plugin_environment *)osync_context_get_plugin_data(ctx);

	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);

	/* Detect whether we need to do a slow sync - this is supported by the hash table */
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
	if (env->config_file && ! FilesFindAllFromDirectory(ctx, env->config_file, error)) {
		osync_context_report_error(ctx, 1, "Error while checking for files");
		return FALSE;
	}

	osync_hashtable_report_deleted(env->hashtable, ctx, "data");

	/* Don't report via
	 *	osync_context_report_success(ctx)
	 * here, our caller will already be doing that.
	 */
	return TRUE;
}

#if 0
extern  bool file_callback (RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie)
{
	fprintf(stderr, "file_callback\n");
	return TRUE;
}
#endif

extern void synceFileGetData(OSyncContext *ctx, OSyncChange *change)
{
	synce_plugin_environment	*env;
	fileFormat			*ff;
	HANDLE				h;
	size_t				rsz;
	WCHAR				*wfn;
	int				r;

	osync_debug("SynCE-File", 4, "start : %s", __func__);
	env = (synce_plugin_environment *)osync_context_get_plugin_data(ctx);
	ff = (fileFormat *)osync_change_get_data(change);

#if 0
	if (ff == NULL || ff->data == NULL || ff->size == 0) {
		osync_debug("SynCE-File", 4, "%s: NULL values encountered, returning\n", __func__);
		osync_context_report_error(ctx, 1, "get_data got NULLs");
		return;
	}
#endif
	/*
	 * There appears to be a trick being used here.
	 *
	 * First report some stuff, use the data field to pass random data (e.g. the file
	 * name) to ourselves.
	 * Then, in the get_data() call, read that data and replace it with the file content.
	 */
	if (ff->data == NULL) {
		osync_context_report_error(ctx, 4, "%s: NULL values", __func__);
		return;
	}
	
	wfn = wstr_from_current(ff->data);

	/* Read the file through SynCE */
	h = CeCreateFile(wfn, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	free(ff->data);
	ff->data = malloc(ff->size);

	r = CeReadFile(h, ff->data, ff->size, &rsz, NULL);
	if (r == 0) {
		/* Error */
		DWORD	e = CeGetLastError();
		char	*s = synce_strerror(e);
		osync_context_report_error(ctx, 1, "Error from CeReadFile (%s)", s);
		CeCloseHandle(h);
		return;
	}

	/* Send its contents */
	osync_change_set_data(change, (char *)ff, sizeof(ff), TRUE);
	CeCloseHandle(h);

	wstr_free_string(wfn);
	osync_context_report_success(ctx);
	osync_debug("SynCE-File", 4, "end : %s", __func__);
}

#if 0
extern void file_read(OSyncContext *ctx, OSyncChange *change)
{
	fileFormat	*mip;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);
	mip = (fileFormat *)osync_context_get_plugin_data(ctx);

	osync_debug("SynCE-SYNC", 4, "end: %s", __func__);
}

extern osync_bool file_access(OSyncContext *ctx, OSyncChange *change)
{
	fileFormat	*ff;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);
	ff = (fileFormat *)osync_context_get_plugin_data(ctx);

	osync_debug("SynCE File", 4, "file_access(%s)\n", ff->data);
	osync_context_report_success(ctx);
	osync_debug("SynCE-SYNC", 4, "end: %s", __func__);
	return TRUE;
}
#endif

/*
 * This function is called by the configuration GUI,
 * it gives it a list of interesting directories on the PDA,
 * which the user might like to choose from to synchronize.
 *
 * Do not make this function static, as the confuration GUI needs to be able to locate
 * it using dlsym(); also don't change its name or signature without making the
 * corresponding change in the configuration program.
 */
GList * synce_list_directories(synce_plugin_environment *env, const char *basedir, OSyncError **error)
{
	GList	*ret = NULL;
	WCHAR	path[MAX_PATH];
	char	*s = NULL;
	HRESULT hr;
	int	r;

	hr = CeRapiInit();

	int	i;

	for (i=0; i<255; i++) {
		r = CeGetSpecialFolderPath(i, MAX_PATH, path);
		if (r > 0) {
			s = wstr_to_current(path);
			ret = g_list_append(ret, s);
			free(s);
		}
	}

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

//	FilesFindAllFromDirectory(NULL, "\\Storage Card");
	FilesFindAllFromDirectory(NULL, "\\My Documents\\foto", NULL);

	(void) synce_list_directories(NULL, NULL, NULL);

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

