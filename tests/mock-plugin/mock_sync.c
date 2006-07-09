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

#define fail(x) abort()

#define fail_unless(condition, msg) do { \
		if (!condition) fail(msg);       \
	} while (0)

int mock_custom_function(mock_env *env, int input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, env, input, error);
	
	fail_unless(input == 1, NULL);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return 2;
}

/*Load the state from a xml file and return it in the conn struct*/
static osync_bool mock_parse_settings(mock_env *env, char *data, int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i)", __func__, env, data, size);
	xmlDocPtr doc;
	xmlNodePtr cur;

	//set defaults
	env->path = NULL;

	doc = xmlParseMemory(data, size);

	if (!doc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to parse settings");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	cur = xmlDocGetRootElement(doc);

	if (!cur) {
		xmlFreeDoc(doc);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get root element of the settings");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (xmlStrcmp(cur->name, (xmlChar*)"config")) {
		xmlFreeDoc(doc);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Config valid is not valid");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		char *str = (char*)xmlNodeGetContent(cur);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"path")) {
				env->path = g_strdup(str);
			}
			xmlFree(str);
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

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
	int configsize;
	mock_env *env = g_malloc0(sizeof(mock_env));

	if (!osync_member_get_config(member, &configdata, &configsize, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	if (!mock_parse_settings(env, configdata, configsize, error)) {
		g_free(env);
		return NULL;
	}
	
	//Rewrite the batch commit functions so we can disable them if necessary
	if (!mock_get_error(member, "BATCH_COMMIT")) {
		OSyncObjFormatSink *fmtsink = member->format_sinks->data;
		osync_trace(TRACE_INTERNAL, "Disabling batch_commit on %p:%s: %i", fmtsink, fmtsink->format ? fmtsink->format->name : "None", g_list_length(member->format_sinks));
		OSyncFormatFunctions *functions = &(fmtsink->functions);
		functions->batch_commit = NULL;
	}
	
	env->member = member;
	env->hashtable = osync_hashtable_new();

	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;
}

static void mock_connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	mock_env *env = (mock_env *)osync_context_get_plugin_data(ctx);

	env->committed_all = TRUE;

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

			osync_change_set_objformat_string(change, "mockformat");
			
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
			
			if (mock_get_error(env->member, "SLOW_REPORT"))
				sleep(1);
			
			if (osync_hashtable_detect_change(env->hashtable, change)) {
				osync_context_report_change(ctx, change);
				osync_hashtable_update_hash(env->hashtable, change);
			}
			g_free(hash);
			
			
		}
	}
	g_dir_close(dir);
	osync_hashtable_report_deleted(env->hashtable, ctx, "data");
	
	fail_unless(env->committed_all == TRUE, NULL);
	env->committed_all = FALSE;
	
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
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);
	
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
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to write", __func__);
				return FALSE;
			}
			break;
		case CHANGE_ADDED:
			if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
				osync_debug("FILE-SYNC", 0, "File %s already exists", filename);
				osync_context_report_error(ctx, OSYNC_ERROR_EXISTS, "Entry already exists");
				g_free(filename);
				osync_trace(TRACE_EXIT_ERROR, "%s: Entry already exists", __func__);
				return FALSE;
			}
			/* No break. Continue below */
		case CHANGE_MODIFIED:
			//FIXME add permission and ownership for file-sync
			if (!osync_file_write(filename, osync_change_get_data(change), osync_change_get_datasize(change), 0700, &error)) {
				osync_debug("FILE-SYNC", 0, "Unable to write to file %s", filename);
				osync_context_report_osyncerror(ctx, &error);
				g_free(filename);
				osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
				return FALSE;
			}
			
			struct stat buf;
			stat(filename, &buf);
			char *hash = mock_generate_hash(&buf);
			osync_change_set_hash(change, hash);
			break;
		default:
			fail("no changetype given");
	}
	g_free(filename);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

static osync_bool mock_commit_change(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	osync_debug("FILE-SYNC", 3, "Writing change %s with changetype %i", osync_change_get_uid(change), osync_change_get_changetype(change));
	mock_env *env = (mock_env *)osync_context_get_plugin_data(ctx);
	
	fail_unless(env->committed_all == FALSE, NULL);
	
	if (mock_get_error(env->member, "COMMIT_ERROR")) {
		osync_context_report_error(ctx, OSYNC_ERROR_EXPECTED, "Triggering COMMIT_ERROR error");
		return FALSE;
	}
	if (mock_get_error(env->member, "COMMIT_TIMEOUT"))
		return FALSE;
	
	if (!mock_access(ctx, change))
		return FALSE;

	osync_hashtable_update_hash(env->hashtable, change);
	osync_context_report_success(ctx);
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
	
	if (!g_getenv("NO_COMMITTED_ALL_CHECK"))
		fail_unless(env->committed_all == TRUE, NULL);
	env->committed_all = FALSE;
	
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

static void mock_batch_commit(OSyncContext *context, OSyncContext **contexts, OSyncChange **changes)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, context, contexts, changes);
	mock_env *env = (mock_env *)osync_context_get_plugin_data(context);
	
	fail_unless(env->committed_all == FALSE, NULL);
	env->committed_all = TRUE;
	
	int i;
	for (i = 0; contexts[i]; i++) {
		if (mock_access(contexts[i], changes[i])) {
			osync_hashtable_update_hash(env->hashtable, changes[i]);
			osync_context_report_success(contexts[i]);
		}
	}
	
	if (g_getenv("NUM_BATCH_COMMITS")) {
		int req = atoi(g_getenv("NUM_BATCH_COMMITS"));
		fail_unless(req == i, NULL);
	}
		
	if (mock_get_error(env->member, "COMMITTED_ALL_ERROR")) {
		osync_context_report_error(context, OSYNC_ERROR_EXPECTED, "Triggering COMMITTED_ALL_ERROR error");
		return;
	}
	
	osync_context_report_success(context);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void mock_committed_all(OSyncContext *context)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, context);
	mock_env *env = (mock_env *)osync_context_get_plugin_data(context);
	
	fail_unless(env->committed_all == FALSE, NULL);
	env->committed_all = TRUE;
	
	if (mock_get_error(env->member, "COMMITTED_ALL_ERROR")) {
		osync_context_report_error(context, OSYNC_ERROR_EXPECTED, "Triggering COMMITTED_ALL_ERROR error");
		osync_trace(TRACE_EXIT_ERROR, "%s: Reporting error", __func__);
		return;
	}
	
	osync_context_report_success(context);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void get_info(OSyncEnv *env)
{
	OSyncPluginInfo *info = osync_plugin_new_info(env);
	
	info->name = "file-sync";
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
	info->timeouts.commit_timeout = 15;
	
	
	if (g_getenv("NO_TIMEOUTS")) {
		info->timeouts.disconnect_timeout = 0;
		info->timeouts.connect_timeout = 0;
		info->timeouts.sync_done_timeout = 0;
		info->timeouts.get_changeinfo_timeout = 0;
		info->timeouts.get_data_timeout = 0;
		info->timeouts.commit_timeout = 0;
	}
	
	if (g_getenv("IS_AVAILABLE"))
		info->functions.is_available = mock_is_available;
	
	osync_plugin_set_batch_commit_objformat(info, "data", "mockformat", mock_batch_commit);
	osync_plugin_set_commit_objformat(info, "data", "mockformat", mock_commit_change);
	osync_plugin_set_committed_all_objformat(info, "data", "mockformat", mock_committed_all);
}
