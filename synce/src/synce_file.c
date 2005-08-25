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
/* $Id: pls.c,v 1.11 2005/01/13 21:37:03 twogood Exp $ */

#include <rapi.h>
#include <synce_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <opensync/opensync.h>

#include <rra/appointment.h>
#include <rra/contact.h>
#include <rra/task.h>
#include <rra/matchmaker.h>
#include <rra/uint32vector.h>
#include <rra/syncmgr.h>
#include <rapi.h>
#include "synce_plugin.h"


#if 0
static bool print_entry(CE_FIND_DATA* entry)
{
#ifndef	NEW_TIME
	time_t seconds;
	char time_string[50] = {0};
	struct tm* time_struct = NULL;
#else
	TIME_FIELDS	time_fields;
#endif
	char* filename = NULL;
	
	/*
	 * Print file attributes
	 */
	if (numeric_file_attributes)
		printf("%08x  ", entry->dwFileAttributes);
	else
		switch (entry->dwFileAttributes)
		{
#if 0
		/* This only prints one attribute at a time */
			case FILE_ATTRIBUTE_ARCHIVE:
				printf("Archive   ");
				break;

			case FILE_ATTRIBUTE_NORMAL:
				printf("Normal    ");
				break;

			case FILE_ATTRIBUTE_DIRECTORY:
				printf("Directory ");
				break;
#endif
			default:
				print_attribute(entry, FILE_ATTRIBUTE_ARCHIVE,       'A');
				print_attribute(entry, FILE_ATTRIBUTE_COMPRESSED,    'C');
				print_attribute(entry, FILE_ATTRIBUTE_DIRECTORY,     'D');
				print_attribute(entry, FILE_ATTRIBUTE_HIDDEN,        'H');
				print_attribute(entry, FILE_ATTRIBUTE_INROM,         'I');
				print_attribute(entry, FILE_ATTRIBUTE_ROMMODULE,     'M');
				print_attribute(entry, FILE_ATTRIBUTE_NORMAL,        'N');
				print_attribute(entry, FILE_ATTRIBUTE_READONLY,      'R');
				print_attribute(entry, FILE_ATTRIBUTE_SYSTEM,        'S');
				print_attribute(entry, FILE_ATTRIBUTE_TEMPORARY,     'T');
				break;
		}

	printf("  ");

	/*
	 * Size 
	 *
	 * XXX: cheating by ignoring nFileSizeHigh
	 */

	if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		printf("          ");
	else
		printf("%10u", entry->nFileSizeLow);

	printf("  ");

	/*
	 * Modification time
	 */

#ifndef	NEW_TIME
	seconds = filetime_to_unix_time(&entry->ftLastWriteTime);
	time_struct = localtime(&seconds);
	strftime(time_string, sizeof(time_string), "%c", time_struct);
	printf("%s", time_string);
#else
	time_fields_from_filetime(&entry->ftLastWriteTime, &time_fields);
	printf("%04i-%02i-%02i %02i:%02i:%02i",
			time_fields.Year, time_fields.Month, time_fields.Day,
			time_fields.Hour, time_fields.Minute, time_fields.Second); 
#endif
	printf("  ");

	/*
	 * OID
	 */

//	printf("%08x", entry->dwOID);
	
//	printf("  ");

	/*
	 * Filename
	 */

	filename = wstr_to_current(entry->cFileName);
        printf(filename);
	wstr_free_string(filename);
	if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		printf("/");

	printf("\n");
	return true;
}
#endif

struct	MyCeFileInfo {
	DWORD	attrib;
	DWORD	size;
	WCHAR	*wfn;
#if 0
	LPFILETIME	time_create, time_access, time_write;
#endif
};
osync_bool FilesReportFileChange(OSyncContext *ctx, const char *dir, CE_FIND_DATA *entry)
{
	plugin_environment	*env;
	OSyncChange		*change;
	char			path[MAX_PATH];
	struct MyCeFileInfo	mi;
	WCHAR			*wfn;
	HANDLE			h;

	fprintf(stderr, "FilesReportFileChange(%s/%s)\n", dir, wstr_to_current(entry->cFileName));


	sprintf(path, "%s\\%s", dir, wstr_to_current(entry->cFileName));
	wfn = wstr_from_current(path);

	fprintf(stderr, "Report(%s)\n", path);

	if (ctx) {
		env = (plugin_environment *)osync_context_get_plugin_data(ctx);
		change = osync_change_new();

		osync_change_set_member(change, env->member);
		osync_change_set_uid(change, path);	/* Pass the full file name as UID */
		osync_change_set_objformat_string(change, "file");	/* Copied from file-sync */

		/* Assemble a structure with data about this file */
		mi.attrib = CeGetFileAttributes(wfn);
		h = CeCreateFile(wfn, GENERIC_READ, 0, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
#if 0
		/* SynCE doesn't do this yet */
		(void) CeGetFileTime(h, &mi.time_create, &mi.time_access, &mi.time_write);
#endif
		mi.size = CeGetFileSize(h, NULL);
		mi.wfn = wfn;			/* FIX ME memory leak/problem ? */
		CeCloseHandle(h);

		/*
		 * Pass the structure to OpenSync
		 * The final parameter is FALSE to indicate a get_data() call will fetch
		 * the file contents if necessary later on.
		 */
		osync_change_set_data(change, (char *)&mi, sizeof(struct MyCeFileInfo), FALSE);

		/* ??? */
		osync_change_set_changetype(change, CHANGE_ADDED);
		osync_context_report_change(ctx, change);

	}

#if 0
	/* Read the file through SynCE */
	if (1) {
		HANDLE	h;
		char	*buffer;
		size_t	sz, rsz;

		sz = 4096;
		buffer = malloc(sz);

		h = CeCreateFile(wfn, GENERIC_READ, 0, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		/*
		 * Should do this in a loop, as CeReadFile() will return success
		 * with 0 bytes read when we're done.
		 */
		rsz = 1;
		while (rsz) {
			if (!CeReadFile(h, buffer, sz, &rsz, NULL)) {
				/* Error */
				fprintf(stderr, "CeReadFile failed\n");
			}
			/* Send its contents */
			if (ctx)
				osync_change_set_data(change,
					buffer, rsz,
					(rsz == 0) ? TRUE : FALSE);	/* TRUE if complete */
		}
		fprintf(stderr, "CeReadFile ok\n");
		CeCloseHandle(h);
		wstr_free_string(wfn);

#if 0
		/* A test - write all the files you find locally */
		{
			int	fd;
			static int	cnt = 0;
			char	p[MAX_PATH];

			sprintf(p, "log/file.%03d", cnt++);
			fd = open(p, O_CREAT|O_TRUNC|O_WRONLY, 0666);
			if (fd) {
				write(fd, buffer, rsz);
				close(fd);
			}
		}
#endif
	}
#endif

	/*
	 * Cannot free this if we pass it to OpenSync.
	wstr_free_string(wfn);
	*/
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
	fprintf(stderr, "FilesFindAllFromDirectory(%s)\n", dir);
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

			fprintf(stderr, "-> (%s)\n", wstr_to_current(entry->cFileName));

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
extern osync_bool commit_file_change(OSyncContext *ctx, OSyncChange *change)
{
#if 0
	plugin_environment *env = (plugin_environment *)osync_context_get_plugin_data(ctx);
#endif
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

			osync_debug("SYNCE-SYNC", 4, "done");
			break;
		}
		default:
			osync_debug("SYNCE-SYNC", 4, "Unknown change type");
	}

	//Answer the call
	osync_context_report_success(ctx);
	return TRUE;
}

/*
 * File get_changeinfo : report the changes to the OpenSync engine.
 */
extern osync_bool file_get_changeinfo(OSyncContext *ctx)
{
	plugin_environment	*env = (plugin_environment *)osync_context_get_plugin_data(ctx);

	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);

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

	/* Don't report via
	 *	osync_context_report_success(ctx)
	 * here, our caller will already be doing that.
	 */
	return TRUE;
}

extern void file_connect(OSyncContext *ctx)
{
	RRA_Matchmaker* matchmaker = NULL;

	osync_debug("SynCE-File", 4, "start: %s", __func__);

	/*
	 * Each time you get passed a context (which is used to track calls to your plugin)
	 * you can get the data your returned in initialize via this call :
	 */
	plugin_environment *env = (plugin_environment *)osync_context_get_plugin_data(ctx);

	/*
	 * Now connect to your devices and report
	 * 
	 * an error via:
	 * osync_context_report_error(ctx, ERROR_CODE, "Some message");
	 * 
	 * or success via:
	 * osync_context_report_success(ctx);
	 * 
	 * You have to use one of these 2 somewhere to answer the context.
	 * 
	 */
	 
	//1 - creating matchmaker
	matchmaker = rra_matchmaker_new();
	if (!matchmaker){
		osync_context_report_error(ctx, 1, "building matchmaker");
		return;
	}
	osync_debug("SynCE-File", 4, "matchmaker built");

	//2 - setting partnership 
	if (!rra_matchmaker_set_current_partner(matchmaker, 1)){
		osync_context_report_error(ctx, 1, "set current partner");
		return;
	}
	osync_debug("SynCE-File", 4, "partner set");

        //3 -setting timezone
        if (!rra_timezone_get(&(env->timezone))){
		osync_context_report_error(ctx, 1, "getting timezone");
		return;
    	}
       	osync_debug("SynCE-File", 4, "timezone set");
	
	
	//4- creating syncmgr
	env->syncmgr = rra_syncmgr_new();

	if (!rra_syncmgr_connect(env->syncmgr))
	{
		osync_context_report_error(ctx, 1, "can't connect");
		return;
	}
    	osync_debug("SynCE-File", 4, "syncmgr created");
    	
    	/*//5- subscribe changes
	if (!synce_subscribe(env-syncmgr)){
		osync_context_report_error(ctx, 1, "can't subscribe");
		return;
  	}
  	osync_debug("SynCE-File", 4, "5- subscribed");*/

	osync_context_report_success(ctx);
	
	//you can also use the anchor system to detect a device reset
	//or some parameter change here. Check the docs to see how it works
	//char *lanchor = NULL;
	//Now you get the last stored anchor from the device
	//if (!osync_anchor_compare(env->member, "lanchor", lanchor))
	//	osync_member_set_slow_sync(env->member, "<object type to request a slow-sync>", TRUE);
}

extern  bool file_callback (RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie)
{
	fprintf(stderr, "file_callback\n");
	return TRUE;
}

extern void file_sync_done(OSyncContext *ctx)
{
	fprintf(stderr, "file_sync_done\n");
}

extern void file_get_data(OSyncContext *ctx, OSyncChange *change)
{
	plugin_environment	*env;
	struct MyCeFileInfo	*mip;
	HANDLE			h;
	char			*buffer;
	size_t			sz, rsz;


	osync_debug("SynCE-File", 4, "start : %s", __func__);
	env = (plugin_environment *)osync_context_get_plugin_data(ctx);
	mip = (struct MyCeFileInfo *)osync_change_get_data(change);

	/* Read the file through SynCE */
	sz = 4096;
	buffer = malloc(sz);

	h = CeCreateFile(mip->wfn, GENERIC_READ, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	/*
	 * Should do this in a loop, as CeReadFile() will return success
	 * with 0 bytes read when we're done.
	 */
	rsz = 1;
	while (rsz) {
		if (!CeReadFile(h, buffer, sz, &rsz, NULL)) {
			/* Error */
			osync_context_report_error(ctx, 1, "Error from CeReadFile");
			CeCloseHandle(h);
			return;
		}
		/* Send its contents */
		if (ctx)
			osync_change_set_data(change,
				buffer, rsz,
				(rsz == 0) ? TRUE : FALSE);	/* TRUE if complete */
	}
	fprintf(stderr, "CeReadFile ok\n");
	CeCloseHandle(h);

	osync_context_report_success(ctx);
	osync_debug("SynCE-File", 4, "end : %s", __func__);
}

#ifdef	TEST_FILE
static       WCHAR empty[]   = {'\0'};

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

exit:
	if (path)
		free(path);

	wstr_free_string(wide_path);

	CeRapiUninit();
	return 0;
}
#endif

