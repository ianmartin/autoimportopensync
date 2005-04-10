/*
 * file-sync - A plugin for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
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
 
#include "file_sync.h"
#include <stdlib.h>

#ifdef HAVE_FAM
static gboolean _fam_prepare(GSource *source, gint *timeout_)
{
	*timeout_ = 1;
	return FALSE;
}

static gboolean _fam_check(GSource *source)
{
	return TRUE;
}

static gboolean _fam_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
	filesyncinfo *fsinfo = user_data;
	FAMEvent famEvent;
	if (FAMPending(fsinfo->famConn)) {
		if (FAMNextEvent(fsinfo->famConn, &famEvent) < 0) {
			osync_debug("FILE-SYNC", 1, "Error getting fam event");
		} else {
			if (famEvent.code == 1 || famEvent.code == 2 || famEvent.code == 5 || famEvent.code == 6)
				osync_member_request_synchronization(fsinfo->member);
		}
	}
	return TRUE;
}

static void fam_setup(filesyncinfo *fsinfo, GMainContext *context)
{
	GSourceFuncs *functions = g_malloc0(sizeof(GSourceFuncs));
	functions->prepare = _fam_prepare;
	functions->check = _fam_check;
	functions->dispatch = _fam_dispatch;
	functions->finalize = NULL;

	GSource *source = g_source_new(functions, sizeof(GSource));
	g_source_set_callback(source, NULL, fsinfo, NULL);
	g_source_attach(source, context);
}
#endif

static void *fs_initialize(OSyncMember *member, OSyncError **error)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);

	char *configdata;
	int configsize;
	filesyncinfo *fsinfo = g_malloc0(sizeof(filesyncinfo));
	
	if (!osync_member_get_config(member, &configdata, &configsize, error)) {
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		g_free(fsinfo);
		return NULL;
	}
	
	if (!fs_parse_settings(fsinfo, configdata, configsize, error)) {
		g_free(fsinfo);
		return NULL;
	}
	
	fsinfo->member = member;
	fsinfo->hashtable = osync_hashtable_new();

#ifdef HAVE_FAM

	fsinfo->famConn = g_malloc0(sizeof(FAMConnection));
	fsinfo->famRequest = g_malloc0(sizeof(FAMRequest));

	if (FAMOpen(fsinfo->famConn) < 0) {
		osync_debug("FILE-SYNC", 3, "Cannot connect to FAM");
	} else {
		if( FAMMonitorDirectory(fsinfo->famConn, fsinfo->path, fsinfo->famRequest, fsinfo ) < 0 ) {
			osync_debug("FILE-SYNC", 3, "Cannot monitor directory %s", fsinfo->path);
			FAMClose(fsinfo->famConn);
		} else {
			fam_setup(fsinfo, NULL);
		}
	}
#endif

	return (void *)fsinfo;
}

static void fs_connect(OSyncContext *ctx)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);

	OSyncError *error = NULL;
	if (!osync_hashtable_load(fsinfo->hashtable, fsinfo->member, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		return;
	}
	
	if (!osync_anchor_compare(fsinfo->member, "path", fsinfo->path))
		osync_member_set_slow_sync(fsinfo->member, "data", TRUE);
	
	GError *direrror = NULL;

	fsinfo->dir = g_dir_open(fsinfo->path, 0, &direrror);
	if (direrror) {
		//Unable to open directory
		osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to open directory %s", fsinfo->path);
		g_error_free (direrror);
	} else {
		osync_context_report_success(ctx);
	}
	osync_debug("FILE-SYNC", 4, "end: %s", __func__);
}

static char *fs_generate_hash(fs_fileinfo *info)
{
	char *hash = g_strdup_printf("%i-%i", (int)info->filestats.st_mtime, (int)info->filestats.st_ctime);
	return hash;
}

/** Report files on a directory
 *
 * NOTE: If 'dir' is non-empty it MUST start it a slash. This is just
 * to make easier concatenation of the paths, and we can just concatenate
 * fsinfo->path and subdir to get the complete path.
 *
 * @param dir The fsinfo->path subdirectory that should be reported. Use
 *            an empty string to report files on fsinfo->path. Should
 *            start with a slash. See note above.
 *
 */
static void fs_report_dir(filesyncinfo *fsinfo, const char *subdir, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "fs_report_dir(%p, %s, %p)", fsinfo, subdir, ctx);
	const char *de = NULL;
	
	char *path = g_build_filename(fsinfo->path, subdir, NULL);
	osync_trace(TRACE_INTERNAL, "path %s", path);
		
	GDir *dir;
	GError *gerror = NULL;

	dir = g_dir_open(path, 0, &gerror);
	if (!dir) {
		/*FIXME: Permission errors may make files to be reported as deleted.
		 * Make fs_report_dir() able to report errors
		 */
		osync_trace(TRACE_EXIT_ERROR, "fs_report_dir: Unable to open directory %s: %s", path, gerror ? gerror->message : "None");
		return;
	}

	while ((de = g_dir_read_name(dir))) {
		char *filename = g_build_filename(path, de, NULL);
		char *relative_filename = NULL;
		if (!subdir)
			relative_filename = g_strdup(de);
		else
			relative_filename = g_build_filename(subdir, de, NULL);
			
		osync_trace(TRACE_INTERNAL, "path2 %s %s", filename, relative_filename);
		
		if (g_file_test(filename, G_FILE_TEST_IS_DIR)) {
			/* Recurse into subdirectories */
			if (fsinfo->recursive)
				fs_report_dir(fsinfo, relative_filename, ctx);
		} else if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
			/* Report normal files */
			OSyncChange *change = osync_change_new();
			osync_change_set_member(change, fsinfo->member);
			osync_change_set_uid(change, relative_filename);

			osync_change_set_objformat_string(change, "file");
			
			fs_fileinfo *info = g_malloc0(sizeof(fs_fileinfo));
			stat(filename, &info->filestats);
			
			char *hash = fs_generate_hash(info);
			osync_change_set_hash(change, hash);
			
			osync_change_set_data(change, (char *)info, sizeof(fs_fileinfo), FALSE);			

			if (osync_hashtable_detect_change(fsinfo->hashtable, change)) {
				osync_context_report_change(ctx, change);
				osync_hashtable_update_hash(fsinfo->hashtable, change);
			}
			g_free(hash);
		}

		g_free(relative_filename);
		g_free(filename);
	}
	g_dir_close(dir);

	g_free(path);
	osync_trace(TRACE_EXIT, "fs_report_dir");
}

static void fs_get_changeinfo(OSyncContext *ctx)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	
	if (osync_member_get_slow_sync(fsinfo->member, "data")) {
		osync_debug("FILE-SYNC", 3, "Slow sync requested");
		osync_hashtable_set_slow_sync(fsinfo->hashtable, "data");
	}

	if (fsinfo->dir) {
		fs_report_dir(fsinfo, NULL, ctx);
		osync_hashtable_report_deleted(fsinfo->hashtable, ctx, "data");
	}
	osync_context_report_success(ctx);
	
	osync_debug("FILE-SYNC", 4, "end: %s", __func__);
}

static void fs_get_data(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	fs_fileinfo *file_info = (fs_fileinfo *)osync_change_get_data(change);
	
	char *filename = g_strdup_printf("%s/%s", fsinfo->path, osync_change_get_uid(change));
	OSyncError *error = NULL;
	if (!osync_file_read(filename, &file_info->data, &file_info->size, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		g_free(filename);
		return;
	}
	
	osync_change_set_data(change, (char *)file_info, sizeof(fs_fileinfo), TRUE);
	g_free(filename);
	
	osync_context_report_success(ctx);
	osync_debug("FILE-SYNC", 4, "end: %s", __func__);
}

static void fs_read(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	
	char *filename = g_strdup_printf("%s/%s", fsinfo->path, osync_change_get_uid(change));

	fs_fileinfo *info = g_malloc0(sizeof(fs_fileinfo));
	stat(filename, &info->filestats);
	
	OSyncError *error = NULL;
	if (!osync_file_read(filename, &info->data, &info->size, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		g_free(filename);
		return;
	}
		
	osync_change_set_data(change, (char *)info, sizeof(fs_fileinfo), TRUE);

	g_free(filename);
	
	osync_context_report_success(ctx);
	osync_debug("FILE-SYNC", 4, "end: %s", __func__);
}

static osync_bool fs_access(OSyncContext *ctx, OSyncChange *change)
{
	/*TODO: Create directory for file, if it doesn't exist */
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	fs_fileinfo *file_info = (fs_fileinfo *)osync_change_get_data(change);

	char *filename = NULL;
	OSyncError *error = NULL;
	filename = g_strdup_printf ("%s/%s", fsinfo->path, osync_change_get_uid(change));
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			if (!remove(filename) == 0) {
				osync_debug("FILE-SYNC", 0, "Unable to remove file %s", filename);
				osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to write");
				g_free(filename);
				return FALSE;
			}
			break;
		case CHANGE_ADDED:
			if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
				osync_debug("FILE-SYNC", 0, "File %s already exists", filename);
				osync_context_report_error(ctx, OSYNC_ERROR_EXISTS, "Entry already exists");
				g_free(filename);
				return FALSE;
			}
			/* No break. Continue below */
		case CHANGE_MODIFIED:
			//FIXME add permission and ownership for file-sync
			if (!osync_file_write(filename, file_info->data, file_info->size, 0700, &error)) {
				osync_debug("FILE-SYNC", 0, "Unable to write to file %s", filename);
				osync_context_report_osyncerror(ctx, &error);
				g_free(filename);
				return FALSE;
			}
			stat(filename, &file_info->filestats);
			osync_change_set_hash(change, fs_generate_hash(file_info));
			break;
		default:
			osync_debug("FILE-SYNC", 0, "Unknown change type");
	}
	osync_context_report_success(ctx);
	g_free(filename);
	osync_debug("FILE-SYNC", 4, "end: %s", __func__);
	return TRUE;
}

static osync_bool fs_commit_change(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	osync_debug("FILE-SYNC", 3, "Writing change %s with changetype %i", osync_change_get_uid(change), osync_change_get_changetype(change));
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	
	if (!fs_access(ctx, change))
		return FALSE;

	osync_hashtable_update_hash(fsinfo->hashtable, change);
	osync_debug("FILE-SYNC", 4, "end: %s", __func__);
	return TRUE;
}

static void fs_sync_done(OSyncContext *ctx)
{
	osync_debug("FILE-SYNC", 3, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	
	//osync_hashtable_forget(fsinfo->hashtable);
	osync_anchor_update(fsinfo->member, "path", fsinfo->path);
	osync_context_report_success(ctx);
	osync_debug("FILE-SYNC", 3, "end: %s", __func__);
}

static void fs_disconnect(OSyncContext *ctx)
{
	osync_debug("FILE-SYNC", 3, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	
	g_dir_close(fsinfo->dir);
	osync_hashtable_close(fsinfo->hashtable);
	osync_context_report_success(ctx);
	osync_debug("FILE-SYNC", 3, "end: %s", __func__);
}

static void fs_finalize(void *data)
{
	osync_debug("FILE-SYNC", 3, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)data;
	osync_hashtable_free(fsinfo->hashtable);
#ifdef HAVE_FAM
	//FAMCancelMonitor(fsinfo->famConn, fsinfo->famRequest);
	//FAMClose(fsinfo->famConn);
#endif

	g_free(fsinfo->path);
	g_free(fsinfo);
}

void get_info(OSyncPluginInfo *info)
{
	info->name = "file-sync";
	info->longname = "File Synchronization Plugin";
	info->description = "Plugin to synchronize files on the local filesystem";
	info->version = 1;
	info->is_threadsafe = TRUE;
	
	info->functions.initialize = fs_initialize;
	info->functions.connect = fs_connect;
	info->functions.sync_done = fs_sync_done;
	info->functions.disconnect = fs_disconnect;
	info->functions.finalize = fs_finalize;
	info->functions.get_changeinfo = fs_get_changeinfo;
	info->functions.get_data = fs_get_data;

	osync_plugin_accept_objtype(info, "data");
	osync_plugin_accept_objformat(info, "data", "file", NULL);
	osync_plugin_set_commit_objformat(info, "data", "file", fs_commit_change);
	osync_plugin_set_access_objformat(info, "data", "file", fs_access);
	osync_plugin_set_read_objformat(info, "data", "file", fs_read);

}
