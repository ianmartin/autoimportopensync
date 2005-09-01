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

static time_t CeTimeToUnixTime(FILETIME t)
{
#ifdef	NEW_TIME
	TIME_FIELDS	tf;
	time_fields_from_filetime(&t, &tf);
	return 0;
#else
	time_t seconds;
	seconds = filetime_to_unix_time(&p->time_create);
	return seconds;
#endif
}

/*
 * A hash appears to be a string.
 * The file-sync plugin allocates new memory in this occasion so I guess that's the right
 * thing to do.
 */
static char *FileHash(fileFormat *p)
{
	char		ts[50] = {0};
	struct tm	*tmp;
#ifndef	NEW_TIME
	time_t seconds;
	struct tm	*time_struct = NULL;
#else
	TIME_FIELDS	write_time_fields;
#endif

	if (!p)
		return "";

#ifndef	NEW_TIME
	seconds = filetime_to_unix_time(&p->last_mod);
	time_struct = localtime(&seconds);
	strftime(ts, sizeof(ts), "%F", time_struct);
#else
	tmp = localtime(&p->last_mod);
	strftime(ts, sizeof(ts), "%F", tmp);
	sprintf(ts, "%04i-%02i-%02i %02i:%02i:%02i",
		write_time_fields.Year, write_time_fields.Month, write_time_fields.Day,
		write_time_fields.Hour, write_time_fields.Minute, write_time_fields.Second); 
#endif
	return g_strdup_printf("%ld %s", (long int)p->size, ts);
}

#if 0
static void my_wstrcpy(WCHAR *dst, WCHAR *src, int ml)
{
	int	i;

	for (i=0; i<ml && src[i]; i++)
		dst[i] = src[i];
	if (i != ml)
		dst[i] = 0;
}
#endif

#if 0
/* Read the file through SynCE */
static osync_bool FileReadFileIntoStruct(HANDLE h, fileFormat *mip)
{
	char	*buffer;
	size_t	sz, rsz;

	sz = mip->size;
	buffer = malloc(sz);

	if (!CeReadFile(h, buffer, sz, &rsz, NULL)) {
		/* Error */
		fprintf(stderr, "CeReadFile failed\n");
		return FALSE;
	}
	mip->data = buffer;
	return TRUE;
}
#endif

/*
 * In this pass of set_data(), do not pass real file contents, but only pass the file name
 * to the opensync engine in the fileFormat structure's data field.
 */
osync_bool FilesReportFileChange(OSyncContext *ctx, const char *dir, CE_FIND_DATA *entry)
{
	synce_plugin_environment	*env;
	OSyncChange			*change;
	char				path[MAX_PATH], *hash;
	WCHAR				*wfn;
	HANDLE				h;
	fileFormat			ff;
	time_t				t1, t2;

	sprintf(path, "%s\\%s", dir, wstr_to_current(entry->cFileName));
	wfn = wstr_from_current(path);

	if (ctx) {
		env = (synce_plugin_environment *)osync_context_get_plugin_data(ctx);
		change = osync_change_new();

		osync_change_set_member(change, env->member);
		osync_change_set_uid(change, path);	/* Pass the full file name as UID */
		osync_change_set_objformat_string(change, "file");	/* Copied from file-sync */

		/* Assemble a structure with data about this file */
		h = CeCreateFile(wfn, GENERIC_READ, 0, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		ff.userid = 0;
		ff.groupid = 0;

		/* Select the latest time */
		t1 = CeTimeToUnixTime(entry->ftLastWriteTime);
		t2 = CeTimeToUnixTime(entry->ftCreationTime);
		ff.last_mod = (t1 < t2) ? t2 : t1;

		ff.size = CeGetFileSize(h, NULL);
		ff.mode = 0777;			/* Fake */

#if 0
		if (! FileReadFileIntoStruct(h, &ff)) {
			osync_context_report_error(ctx, 1,
					"Problem reading the file [%s]\n",
					path);
			return FALSE;
		}
#endif
		CeCloseHandle(h);

		/* Set the hash */
		hash = FileHash(&ff);

		ff.data = strdup(path);

		osync_change_set_hash(change, hash);
		osync_change_set_data(change, (char *)&ff, sizeof(ff), FALSE);

		if (osync_hashtable_detect_change(env->hashtable, change)) {
#if 0
			osync_change_set_changetype(change, CHANGE_ADDED);	/* ?? */
#endif
			osync_context_report_change(ctx, change);
			osync_hashtable_update_hash(env->hashtable, change);
		}
		g_free(hash);
#if 0
		/*
		 * Pass the structure to OpenSync
		 * The final parameter is FALSE to indicate a get_data() call will fetch
		 * the file contents if necessary later on.
		 */
		osync_change_set_data(change, (char *)&ff, sizeof(fileFormat), FALSE);
#endif
	}

	wstr_free_string(wfn);
	return TRUE;
}

osync_bool FilesFindAllFromDirectory(OSyncContext *ctx, const char *dir)
{
	WCHAR		*wd;
	CE_FIND_DATA	*find_data = NULL;
	int		i;
	DWORD		file_count;
	char		path[MAX_PATH];

	if (! dir)
		return TRUE;
	sprintf(path, "%s\\*", dir);

	wd = wstr_from_current(path);
	if (CeFindAllFiles(wd,
			FAF_ATTRIBUTES|FAF_LASTWRITE_TIME|FAF_NAME|FAF_SIZE_LOW|FAF_OID,
			&file_count, &find_data)) {
		for (i=0; i<file_count; i++) {
			CE_FIND_DATA	*entry = &find_data[i];

			/* Report this file to the OpenSync */
			if (! FilesReportFileChange(ctx, dir, entry)) {
				/* Failure */
				return FALSE;
			}


			/* Recursive call for directories */
			if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				sprintf(path, "%s\\%s", dir, wstr_to_current(entry->cFileName));
				(void) /* FIX ME */ FilesFindAllFromDirectory(ctx, path);
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
extern osync_bool file_commit(OSyncContext *ctx, OSyncChange *change)
{
	synce_plugin_environment *env = (synce_plugin_environment *)osync_context_get_plugin_data(ctx);
	uint32_t id=0;

	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);	
		
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			id=strtol(osync_change_get_uid(change),NULL,16);
			osync_debug("SYNCE-SYNC", 4, "deleting contact id: %08x",id);
		
			osync_debug("SYNCE-SYNC", 4, "done");
			break;
		case CHANGE_ADDED:
		{
			char *object=NULL;
#if 0
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id,id;
#endif
			
			object=osync_change_get_data(change);
			id=strtol(osync_change_get_uid(change),NULL,16);			

			osync_debug("SYNCE-SYNC", 4, "adding contact id %08x",id);

			osync_change_set_hash(change, FileHash(NULL));

			osync_debug("SYNCE-SYNC", 4, "done");
			break;
		}
		case CHANGE_MODIFIED:
		{
			char *object=NULL;
#if 0
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id,id;
#endif

			object=osync_change_get_data(change);
			id=strtol(osync_change_get_uid(change),NULL,16);			

			osync_debug("SYNCE-SYNC", 4, "updating contact id %08x",id);

			osync_change_set_hash(change, FileHash(NULL));

			osync_debug("SYNCE-SYNC", 4, "done");
			break;
		}
		default:
			osync_debug("SYNCE-SYNC", 4, "Unknown change type");
	}

	/* Answer the call */
	osync_context_report_success(ctx);
	osync_hashtable_update_hash(env->hashtable, change);

	return TRUE;
}

/*
 * File get_changeinfo : report the changes to the OpenSync engine.
 */
extern osync_bool file_get_changeinfo(OSyncContext *ctx)
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
	if (env->config_file && ! FilesFindAllFromDirectory(ctx, env->config_file)) {
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

extern void file_connect(OSyncContext *ctx)
{
	/*
	 * Don't repeat the SynCE connection making - the synce_plugin.c connect() already
	 * handles all that.
	 */
	synce_plugin_environment *env;
	OSyncError	*error = NULL;

	osync_debug("SynCE-File", 4, "start: %s", __func__);

	/*
	 * Each time you get passed a context (which is used to track calls to your plugin)
	 * you can get the data your returned in initialize via this call :
	 */
	env = (synce_plugin_environment *)osync_context_get_plugin_data(ctx);

	/* Load the hash table */
	if (!osync_hashtable_load(env->hashtable, env->member, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		return;
	}
}

extern  bool file_callback (RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie)
{
	fprintf(stderr, "file_callback\n");
	return TRUE;
}

extern void file_sync_done(OSyncContext *ctx)
{
	synce_plugin_environment	*env;

	env = (synce_plugin_environment *)osync_context_get_plugin_data(ctx);
	osync_hashtable_forget(env->hashtable);

	fprintf(stderr, "file_sync_done\n");
}

extern void file_disconnect(OSyncContext *ctx)
{
	synce_plugin_environment	*env;

	env = (synce_plugin_environment *)osync_context_get_plugin_data(ctx);
	osync_hashtable_close(env->hashtable);
}

extern void file_finalize(void *data)
{
	synce_plugin_environment	*env = (synce_plugin_environment *)data;
	osync_hashtable_free(env->hashtable);
}

extern void file_get_data(OSyncContext *ctx, OSyncChange *change)
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

	/* There appears to be a trick being used here.
	 * First report some stuff, use the data field to pass random data (e.g. the file
	 * name) to ourselved.
	 * Then, in the get_data() call, read that data and replace it with the file content.
	 */
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
	if (ctx)
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
	fileFormat	*mip;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);
	mip = (fileFormat *)osync_context_get_plugin_data(ctx);

	osync_debug("SynCE File", 4, "file_access(%s)\n", mip->wfn);
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

	FilesFindAllFromDirectory(NULL, "\\Storage Card");
	FilesFindAllFromDirectory(NULL, "\\My Documents\\foto");

	(void) synce_list_directories(NULL, NULL, NULL);

exit:
	if (path)
		free(path);

	wstr_free_string(wide_path);

	CeRapiUninit();
	return 0;
}
#endif
