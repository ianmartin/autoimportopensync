/*
 * mock-sync - A mock-plugin for the opensync framework
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
 
#include "mock_sync.h"

static osync_bool mock_get_error(OSyncMember *member, const char *domain)
{
	const char *env = g_getenv(domain);
	if (!env)
		return FALSE;
	
	int num = atoi(env);
	int mask = 1 << (osync_member_get_id(member) - 1);
	if (num & mask) {
		char *chancestr = g_strdup_printf("%s_PROB", domain);
		const char *chance = g_getenv(chancestr);
		g_free(chancestr);
		if (!chance)
			return TRUE;
		int prob = atoi(chance);
		if (prob >= g_random_int_range(0, 100))
			return TRUE;
	}
	return FALSE;
}

static void *mock_initialize(OSyncMember *member, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);

	if (mock_get_error(member, "INIT_NULL")) {
		osync_error_set(error, OSYNC_ERROR_EXPECTED, "Triggering INIT_NULL error");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}

	char *configdata;
	if (!osync_member_get_config(member, &configdata, NULL, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	mock_env *env = (mock_env *)configdata;
	
	env->member = member;
	env->hashtable = osync_hashtable_new();

	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;
}

static void mock_connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	mock_env *env = (mock_env *)osync_context_get_plugin_data(ctx);

	if (mock_get_error(env->member, "CONNECT_ERROR")) {
		osync_context_report_error(ctx, OSYNC_ERROR_EXPECTED, "Triggering CONNECT_ERROR error");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, "Triggering CONNECT_ERROR error");
		return;
	}
	
	if (mock_get_error(env->member, "CONNECT_TIMEOUT")) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, "Triggering CONNECT_TIMEOUT error");
		return;
	}
	
	OSyncError *error = NULL;
	if (!osync_hashtable_load(env->hashtable, env->member, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
		osync_error_free(&error);
		return;
	}
	
	if (!osync_anchor_compare(env->member, "path", env->path))
		osync_member_set_slow_sync(env->member, "data", TRUE);
	
	GError *direrror = NULL;

	env->dir = g_dir_open(env->path, 0, &direrror);
	if (direrror) {
		//Unable to open directory
		osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to open directory %s", env->path);
		g_error_free (direrror);
	} else {
		osync_context_report_success(ctx);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static char *mock_generate_hash(struct stat *buf)
{
	char *hash = g_strdup_printf("%i-%i", (int)buf->st_mtime, (int)buf->st_ctime);
	return hash;
}

static void mock_get_changeinfo(OSyncContext *ctx)
{
	mock_env *env = (mock_env *)osync_context_get_plugin_data(ctx);
	
	if (mock_get_error(env->member, "GET_CHANGES_ERROR")) {
		osync_context_report_error(ctx, OSYNC_ERROR_EXPECTED, "Triggering GET_CHANGES_ERROR error");
		return;
	}
	if (mock_get_error(env->member, "GET_CHANGES_TIMEOUT"))
		return;
	if (mock_get_error(env->member, "GET_CHANGES_TIMEOUT2"))
		sleep(8);
	
	if (osync_member_get_slow_sync(env->member, "data")) {
		osync_debug("FILE-SYNC", 3, "Slow sync requested");
		osync_hashtable_set_slow_sync(env->hashtable, "data");
	}

	GDir *dir;
	GError *gerror = NULL;
	const char *de = NULL;
	
	dir = g_dir_open(env->path, 0, &gerror);
	if (!dir) {
		osync_trace(TRACE_EXIT_ERROR, "mock_report_dir: Unable to open directory %s: %s", env->path, gerror ? gerror->message : "None");
		return;
	}
	while ((de = g_dir_read_name(dir))) {
		char *filename = g_build_filename(env->path, de, NULL);
		if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
			/* Report normal files */
			OSyncChange *change = osync_change_new();
			osync_change_set_member(change, env->member);
			osync_change_set_uid(change, de);

			osync_change_set_objformat_string(change, "file");
			
			struct stat buf;
			stat(filename, &buf);
			char *hash = mock_generate_hash(&buf);
			osync_change_set_hash(change, hash);
			
			if (mock_get_error(env->member, "ONLY_INFO")) {
				osync_change_set_data(change, NULL, 0, FALSE);			
			} else {
				char *data = NULL;
				int size = 0;
				OSyncError *error = NULL;
				if (!osync_file_read(filename, &data, &size, &error)) {
					osync_context_report_osyncerror(ctx, &error);
					g_free(filename);
					return;
				}
				
				osync_change_set_data(change, data, size, TRUE);
			}
			
			if (osync_hashtable_detect_change(env->hashtable, change)) {
				osync_context_report_change(ctx, change);
				osync_hashtable_update_hash(env->hashtable, change);
			}
			g_free(hash);
		}
	}
	g_dir_close(dir);

	osync_context_report_success(ctx);
}

static void mock_get_data(OSyncContext *ctx, OSyncChange *change)
{
	mock_env *env = (mock_env *)osync_context_get_plugin_data(ctx);

	if (mock_get_error(env->member, "GET_DATA_ERROR")) {
		osync_context_report_error(ctx, OSYNC_ERROR_EXPECTED, "Triggering GET_DATA_ERROR error");
		return;
	}
	if (mock_get_error(env->member, "GET_DATA_TIMEOUT"))
		return;

	char *filename = g_strdup_printf("%s/%s", env->path, osync_change_get_uid(change));
	char *data = NULL;
	int size = 0;
	OSyncError *error = NULL;
	if (!osync_file_read(filename, &data, &size, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		g_free(filename);
		return;
	}
	
	osync_change_set_data(change, data, size, TRUE);
	g_free(filename);
	
	osync_context_report_success(ctx);
}

static void mock_read(OSyncContext *ctx, OSyncChange *change)
{
	mock_env *env = (mock_env *)osync_context_get_plugin_data(ctx);
	
	char *filename = g_strdup_printf("%s/%s", env->path, osync_change_get_uid(change));
	
	char *data = NULL;
	int size = 0;
	OSyncError *error = NULL;
	if (!osync_file_read(filename, &data, &size, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		g_free(filename);
		return;
	}
	
	osync_change_set_data(change, data, size, TRUE);

	g_free(filename);
	
	osync_context_report_success(ctx);
}

static osync_bool mock_access(OSyncContext *ctx, OSyncChange *change)
{
	/*TODO: Create directory for file, if it doesn't exist */
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	mock_env *env = (mock_env *)osync_context_get_plugin_data(ctx);
	
	char *filename = NULL;
	OSyncError *error = NULL;
	filename = g_strdup_printf ("%s/%s", env->path, osync_change_get_uid(change));
	
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
			if (!osync_file_write(filename, osync_change_get_data(change), osync_change_get_datasize(change), 0700, &error)) {
				osync_debug("FILE-SYNC", 0, "Unable to write to file %s", filename);
				osync_context_report_osyncerror(ctx, &error);
				g_free(filename);
				return FALSE;
			}
			
			struct stat buf;
			stat(filename, &buf);
			char *hash = mock_generate_hash(&buf);
			osync_change_set_hash(change, hash);
			break;
		default:
			osync_debug("FILE-SYNC", 0, "Unknown change type");
	}
	osync_context_report_success(ctx);
	g_free(filename);
	osync_debug("FILE-SYNC", 4, "end: %s", __func__);
	return TRUE;
}

static osync_bool mock_commit_change(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	osync_debug("FILE-SYNC", 3, "Writing change %s with changetype %i", osync_change_get_uid(change), osync_change_get_changetype(change));
	mock_env *env = (mock_env *)osync_context_get_plugin_data(ctx);
	
	if (mock_get_error(env->member, "COMMIT_ERROR")) {
		osync_context_report_error(ctx, OSYNC_ERROR_EXPECTED, "Triggering COMMIT_ERROR error");
		return FALSE;
	}
	if (mock_get_error(env->member, "COMMIT_TIMEOUT"))
		return FALSE;
	
	if (!mock_access(ctx, change))
		return FALSE;

	osync_hashtable_update_hash(env->hashtable, change);
	osync_debug("FILE-SYNC", 4, "end: %s", __func__);
	return TRUE;
}

static void mock_sync_done(OSyncContext *ctx)
{
	osync_debug("FILE-SYNC", 3, "start: %s", __func__);
	mock_env *env = (mock_env *)osync_context_get_plugin_data(ctx);
	
	if (mock_get_error(env->member, "SYNC_DONE_ERROR")) {
		osync_context_report_error(ctx, OSYNC_ERROR_EXPECTED, "Triggering SYNC_DONE_ERROR error");
		return;
	}
	if (mock_get_error(env->member, "SYNC_DONE_TIMEOUT"))
		return;
	
	osync_anchor_update(env->member, "path", env->path);
	osync_context_report_success(ctx);
	osync_debug("FILE-SYNC", 3, "end: %s", __func__);
}

static void mock_disconnect(OSyncContext *ctx)
{
	osync_debug("FILE-SYNC", 3, "start: %s", __func__);
	mock_env *env = (mock_env *)osync_context_get_plugin_data(ctx);
	
	if (mock_get_error(env->member, "DISCONNECT_ERROR")) {
		osync_context_report_error(ctx, OSYNC_ERROR_EXPECTED, "Triggering DISCONNECT_ERROR error");
		return;
	}
	if (mock_get_error(env->member, "DISCONNECT_TIMEOUT"))
		return;
	
	g_dir_close(env->dir);
	osync_hashtable_close(env->hashtable);
	osync_context_report_success(ctx);
	osync_debug("FILE-SYNC", 3, "end: %s", __func__);
}

static void mock_finalize(void *data)
{
	osync_debug("FILE-SYNC", 3, "start: %s", __func__);
	mock_env *env = (mock_env *)data;
	osync_hashtable_free(env->hashtable);

	g_free(env->path);
	g_free(env);
}

static osync_bool mock_is_available(OSyncError **error)
{
	if (g_getenv("IS_NOT_AVAILABLE")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "file-sync plugin is not available");
		return FALSE;
	}
	return TRUE;
}

static void mock_batch_commit(void *data, OSyncContext **contexts, OSyncChange **changes)
{
	mock_env *env = (mock_env *)data;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, contexts, changes);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void mock_committed_all(void *data)
{
	mock_env *env = (mock_env *)data;
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void get_info(OSyncPluginInfo *info)
{
	info->name = "mock-sync";
	info->longname = "Mock Plugin";
	info->description = "Mock Plugin";
	info->version = 1;
	
	info->functions.initialize = mock_initialize;
	info->functions.connect = mock_connect;
	info->functions.sync_done = mock_sync_done;
	info->functions.disconnect = mock_disconnect;
	info->functions.finalize = mock_finalize;
	info->functions.get_changeinfo = mock_get_changeinfo;
	info->functions.get_data = mock_get_data;

	osync_plugin_accept_objtype(info, "data");
	osync_plugin_accept_objformat(info, "data", "mockformat", NULL);
	
	osync_plugin_set_access_objformat(info, "data", "mockformat", mock_access);
	osync_plugin_set_read_objformat(info, "data", "mockformat", mock_read);

	//Lets reduce the timeouts a bit so the checks work faster
	info->timeouts.disconnect_timeout = 5;
	info->timeouts.connect_timeout = 5;
	info->timeouts.sync_done_timeout = 5;
	info->timeouts.get_changeinfo_timeout = 5;
	info->timeouts.get_data_timeout = 5;
	info->timeouts.commit_timeout = 5;
	
	if (g_getenv("IS_AVAILABLE"))
		info->functions.is_available = mock_is_available;
	
	if (g_getenv("BATCH_COMMIT1")) {
		osync_plugin_set_batch_commit_objformat(info, "data", "mockformat", mock_batch_commit);
	} else {
		osync_plugin_set_commit_objformat(info, "data", "mockformat", mock_commit_change);
		if (g_getenv("BATCH_COMMIT2")) 
			osync_plugin_set_committed_all_objformat(info, "data", "mockformat", mock_committed_all);
	}
}
