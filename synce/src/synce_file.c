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

// #include "pcommon.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <opensync/opensync.h>

#include <rra/appointment.h>
#include <rra/contact.h>
#include <rra/task.h>
#include <rra/matchmaker.h>
#include <rra/uint32vector.h>
#include <rra/syncmgr.h>
#include <rapi.h>
#include "synce_plugin.h"


/* From pcommon.h */
void convert_to_backward_slashes(char* path);
bool is_remote_file(const char* filename);
WCHAR* adjust_remote_path(WCHAR* old_path, bool free_path);

/* End pcommon.h copy */

/* Start pcommon.c copy */
#define WIDE_BACKSLASH   htole16('\\')

WCHAR* adjust_remote_path(WCHAR* old_path, bool free_path)
{
	WCHAR wide_backslash[2];
	WCHAR path[MAX_PATH];

	wide_backslash[0] = WIDE_BACKSLASH;
	wide_backslash[1] = '\0';

	/* Nothing to adjust if we have an absolute path */
	if (WIDE_BACKSLASH == old_path[0])
		return old_path;

	if (!CeGetSpecialFolderPath(CSIDL_PERSONAL, sizeof(path), path)) {
		fprintf(stderr, "Unable to get the \"My Documents\" path.\n");
		return NULL;
	}

	wstr_append(path, wide_backslash, sizeof(path));
	wstr_append(path, old_path, sizeof(path));

	if (free_path)
		wstr_free_string(old_path);

	synce_trace_wstr(path);
	return wstrdup(path);
}

void convert_to_backward_slashes(char* path)
{
	while (*path) {
		if ('/' == *path)
			*path = '\\';
		path++;
	}
}
/* End pcommon.c copy */

static bool numeric_file_attributes = false;
static bool show_hidden_files = false;

#ifdef	TEST_FILE
static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-a] [-d LEVEL] [-h] [-n] [DIRECTORY]\n"
			"\n"
			"\t-a        Show all files including those marked as hidden\n"
			"\t-d LEVEL  Set debug log level\n"
			"\t              0 - No logging (default)\n"
			"\t              1 - Errors only\n"
			"\t              2 - Errors and warnings\n"
			"\t              3 - Everything\n"
			"\t-h         Show this help message\n"
			"\t-n         Show numeric value for file attributes\n"
			"\tDIRECTORY  The remote directory where you want to list files\n",
			name);
}
#endif

#if 0
static bool handle_parameters(int argc, char** argv, char** path)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;

	while ((c = getopt(argc, argv, "ad:hn")) != -1)
	{
		switch (c)
		{
			case 'a':
				show_hidden_files = true;
				break;
				
			case 'd':
				log_level = atoi(optarg);
				break;

			case 'n':
				numeric_file_attributes = true;
				break;
			
			case 'h':
			default:
				show_usage(argv[0]);
				return false;
		}
	}

	synce_log_set_level(log_level);

	/* TODO: handle more than one path */
	if (optind < argc)
		*path = strdup(argv[optind++]);

	return true;
}
#endif

static void print_attribute(CE_FIND_DATA* entry, DWORD attribute, int c)
{
	if (entry->dwFileAttributes & attribute)
		putchar(c);
	else
		putchar('-');
}

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

static bool list_matching_files(WCHAR* wide_path)
{
	bool success = false;
	BOOL result;
	CE_FIND_DATA* find_data = NULL;
	DWORD file_count = 0;
	unsigned i;

	synce_trace_wstr(wide_path);
	wide_path = adjust_remote_path(wide_path, true);
	synce_trace_wstr(wide_path);

	result = CeFindAllFiles(wide_path,
		(show_hidden_files ? 0 : FAF_ATTRIB_NO_HIDDEN)
		| FAF_ATTRIBUTES|FAF_LASTWRITE_TIME|FAF_NAME|FAF_SIZE_LOW|FAF_OID,
		&file_count, &find_data);

	if (!result)
		goto exit;

	for (i = 0; i < file_count; i++)
		print_entry(find_data + i);

	success = true;

exit:
	CeRapiFreeBuffer(find_data);

	return success;
}

static const char *wildcards = "*.*";

bool list_directory(WCHAR* directory)
{
	WCHAR path[MAX_PATH];
	WCHAR* tmp = NULL;

	if (!directory) {
		synce_error("Directory is NULL. How did that happen?");
		return false;
	}

	synce_trace_wstr(directory);
	wstrcpy(path, directory);
	synce_trace_wstr(path);
	wstr_append(path, (tmp = wstr_from_current(wildcards)), sizeof(path));
	wstr_free_string(tmp);
	return list_matching_files(path);
}

osync_bool FilesReportFileChange(OSyncContext *ctx, const char *dir, CE_FIND_DATA *entry)
{
	plugin_environment	*env = (plugin_environment *)osync_context_get_plugin_data(ctx);
	OSyncChange		*change = osync_change_new();
	char			path[MAX_PATH];

	sprintf(path, "%s\\%s", dir, wstr_to_current(entry->cFileName));
	fprintf(stderr, "Report(%s)\n", path);

	osync_change_set_member(change, env->member);
	osync_change_set_uid(change, path);
	osync_change_set_objformat_string(change, "data");
	osync_change_set_data(change, NULL, 0, TRUE);	/* FIX ME */
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
	wd = adjust_remote_path(wd, true);
	if (CeFindAllFiles(wd, (show_hidden_files ? 0 : FAF_ATTRIB_NO_HIDDEN)
			| FAF_ATTRIBUTES|FAF_LASTWRITE_TIME|FAF_NAME|FAF_SIZE_LOW|FAF_OID,
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
	return TRUE;
}

/*
 * File get_changeinfo : report the changes to the OpenSync engine.
 */
extern osync_bool file_get_changeinfo(OSyncContext *ctx)
{
	plugin_environment *env = (plugin_environment *)osync_context_get_plugin_data(ctx);

	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);

	/* SynCE : check if RRA is connected */
	if (!env->syncmgr || !rra_syncmgr_is_connected(env->syncmgr)){
		/* not connected, exit */
		osync_context_report_error(ctx, 1, "not connected to device, exit.");
		return FALSE;
	}

	osync_debug("SynCE-file", 4, "checking files");

	/*
	 * FIX ME
	 * The list of directories to replicate should come from configuration,
	 * for now it's hardcoded to something useful to me.
	 */
	if (! FilesFindAllFromDirectory(ctx, "\\Storage Card")) {
		osync_context_report_error(ctx, 1, "Error while checking for files (storage card)");
		return FALSE;
	}
	if (! FilesFindAllFromDirectory(ctx, "\\My Documents")) {
		osync_context_report_error(ctx, 1, "Error while checking for files (my docs)");
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

