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
#include <opensync/file.h>
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
			fam_setup(fsinfo, osync_member_get_loop(member));
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

static char *fs_generate_hash(struct stat *filestats)
{
	char *hash = g_strdup_printf("%i-%i", (int)filestats->st_mtime, (int)filestats->st_ctime);
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
			
			fileFormat *info = g_malloc0(sizeof(fileFormat));
			struct stat filestats;
			stat(filename, &filestats);
			info->userid = filestats.st_uid;
			info->groupid = filestats.st_gid;
			info->mode = filestats.st_mode;
			info->last_mod = filestats.st_mtime;
			
			char *hash = fs_generate_hash(&filestats);
			osync_change_set_hash(change, hash);
			
			OSyncError *error = NULL;
			if (!osync_file_read(filename, &info->data, &info->size, &error)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to read file");
				g_free(filename);
				return;
			}
	
			osync_change_set_data(change, (char *)info, sizeof(fileFormat), TRUE);

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
	fileFormat *file_info = (fileFormat *)osync_change_get_data(change);
	
	char *filename = g_strdup_printf("%s/%s", fsinfo->path, osync_change_get_uid(change));
	OSyncError *error = NULL;
	if (!osync_file_read(filename, &file_info->data, &file_info->size, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		g_free(filename);
		return;
	}
	
	osync_change_set_data(change, (char *)file_info, sizeof(fileFormat), TRUE);
	g_free(filename);
	
	osync_context_report_success(ctx);
	osync_debug("FILE-SYNC", 4, "end: %s", __func__);
}

static void fs_read(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	
	char *filename = g_strdup_printf("%s/%s", fsinfo->path, osync_change_get_uid(change));

	fileFormat *info = g_malloc0(sizeof(fileFormat));
	struct stat filestats;
	stat(filename, &filestats);
	info->userid = filestats.st_uid;
	info->groupid = filestats.st_gid;
	info->mode = filestats.st_mode;
	info->last_mod = filestats.st_mtime;
	
	OSyncError *error = NULL;
	if (!osync_file_read(filename, &info->data, &info->size, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		g_free(filename);
		return;
	}
		
	osync_change_set_data(change, (char *)info, sizeof(fileFormat), TRUE);

	g_free(filename);
	
	osync_context_report_success(ctx);
	osync_debug("FILE-SYNC", 4, "end: %s", __func__);
}

static osync_bool _fs_filename_is_valid(const char *filename)
{
	const char invalid[] = {'\x10', '/', '\0'}; 
	if (strpbrk(filename, invalid))
		return FALSE;
	return TRUE;
}

/** Write a change, but doesn't report success
 *
 * This function writes a change, but doesn't report success on
 * the OSyncContext object on error. This function is used by
 * fs_access() and fs_commit_change(), and allow the caller to
 * do more tasks before reporting success to opensync.
 *
 * On success, TRUE will be returned but osync_context_report_success() won't be called
 * On failure, FALSE will be returned, and osync_context_report_error() will be called
 */
static osync_bool __fs_access(OSyncContext *ctx, OSyncChange *change)
{
	/*TODO: Create directory for file, if it doesn't exist */
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	fileFormat *file_info = (fileFormat *)osync_change_get_data(change);

	char *hash = NULL;
	char *filename = NULL;
	OSyncError *error = NULL;
	filename = g_strdup_printf ("%s/%s", fsinfo->path, osync_change_get_uid(change));
	struct stat filestats;
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			if (remove(filename) != 0) {
				osync_debug("FILE-SYNC", 0, "Unable to remove file %s", filename);
				osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to write");
				g_free(filename);
				return FALSE;
			}
			break;
		case CHANGE_ADDED:
			/* It can happen, that if we sync against another plugin like evo2 or kde,
			 * that we get an invalid character in the uid for the filesystem. Since we
			 * use the uid as the filename, this is a problem. This fix replaces any uid
			 * it receives, which contains an invalid uid with a new uid which is valid.
			 * 
			 * This has some problems: we cannot compare the filename correctly if its a strange
			 * charset like utf16. Another problem appears if we have filesystems which supports
			 * different characters in filenames */
			if (!_fs_filename_is_valid(osync_change_get_uid(change))) {
				g_free(filename);
				osync_change_set_uid(change, osync_rand_str(15));
				filename = g_strdup_printf("%s/%s", fsinfo->path, osync_change_get_uid(change));
			}

			if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
				osync_debug("FILE-SYNC", 0, "File %s already exists", filename);
				osync_context_report_error(ctx, OSYNC_ERROR_EXISTS, "Entry already exists");
				g_free(filename);
				return FALSE;
			}
			
			if (!osync_file_write(filename, file_info->data, file_info->size, file_info->mode, &error)) {
				osync_debug("FILE-SYNC", 0, "Unable to write to file %s", filename);
				osync_context_report_osyncerror(ctx, &error);
				g_free(filename);
				return FALSE;
			}
			
			stat(filename, &filestats);
			
			hash = fs_generate_hash(&filestats);
			osync_change_set_hash(change, hash);
			break;
		case CHANGE_MODIFIED:
			/* Dont touch the permissions */
			if (stat(filename, &filestats) == -1)
				filestats.st_mode = 0700; //An error occured. Choose a save default value
				
			if (!osync_file_write(filename, file_info->data, file_info->size, filestats.st_mode, &error)) {
				osync_debug("FILE-SYNC", 0, "Unable to write to file %s", filename);
				osync_context_report_osyncerror(ctx, &error);
				g_free(filename);
				return FALSE;
			}
			
			if (stat(filename, &filestats) != 0) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to stat file");
				osync_context_report_osyncerror(ctx, &error);
				g_free(filename);
				return FALSE;
			}
			
			hash = fs_generate_hash(&filestats);
			osync_change_set_hash(change, hash);
			break;
		default:
			osync_debug("FILE-SYNC", 0, "Unknown change type");
	}
	g_free(filename);
	osync_debug("FILE-SYNC", 4, "end: %s", __func__);
	return TRUE;
}

static osync_bool fs_commit_change(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	osync_debug("FILE-SYNC", 3, "Writing change %s with changetype %i", osync_change_get_uid(change), osync_change_get_changetype(change));
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	
	if (!__fs_access(ctx, change))
		return FALSE;

	osync_hashtable_update_hash(fsinfo->hashtable, change);
	osync_context_report_success(ctx);
	osync_debug("FILE-SYNC", 4, "end: %s", __func__);
	return TRUE;
}

static osync_bool fs_access(OSyncContext *ctx, OSyncChange *change)
{
	if (!__fs_access(ctx, change))
		return FALSE;

	osync_context_report_success(ctx);
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

void get_info(OSyncEnv *env)
{
	OSyncPluginInfo *info = osync_plugin_new_info(env);
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