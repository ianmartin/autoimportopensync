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
#include "mock_format.h"

#define fail(x) abort()

#define fail_unless(condition, msg) do { \
		if (!condition) fail(msg);       \
	} while (0)

#if 0
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
		osync_error_unref(&error);
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
	
	if (osync_member_get_slow_sync(env->member, "data"))
		osync_hashtable_set_slow_sync(env->hashtable, "data");

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
#endif

/*Load the state from a xml file and return it in the conn struct*/
static osync_bool mock_parse_settings(mock_env *env, const char *data, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, data, error);
	xmlDocPtr doc;
	xmlNodePtr cur;

	//set defaults
	env->path = NULL;

	doc = xmlParseMemory(data, strlen(data) + 1);

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

static char *mock_generate_hash(struct stat *buf)
{
	char *hash = g_strdup_printf("%i-%i", (int)buf->st_mtime, (int)buf->st_ctime);
	return hash;
}

static osync_bool discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);
	
	OSyncObjTypeSink *sink = osync_plugin_info_find_objtype(info, "file");
	if (!sink) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find sink for file");
		goto error;
	}
	osync_objtype_sink_set_available(sink, TRUE);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static void connect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	mock_env *env = (mock_env *)data;
	GError *direrror = NULL;
	OSyncError *error = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);

	osync_trace(TRACE_INTERNAL, "The configdir: %s", osync_plugin_info_get_configdir(info));
	char *tablepath = g_strdup_printf("%s/hashtable.db", osync_plugin_info_get_configdir(info));
	env->hashtable = osync_hashtable_new(tablepath, "file", &error);
	g_free(tablepath);
	
	if (!env->hashtable)
		goto error;

	env->dir = g_dir_open(env->path, 0, &direrror);
	if (direrror) {
		//Unable to open directory
		osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to open directory %s", env->path);
		g_error_free(direrror);
	} else {
		osync_context_report_success(ctx);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	mock_env *env = data;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	
	g_dir_close(env->dir);
	env->dir = NULL;
	
	osync_hashtable_free(env->hashtable);
	env->hashtable = NULL;
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void get_changes(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	
	mock_env *env = data;

	OSyncError *error = NULL;
	const char *de = NULL;
	int i = 0;
	
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	osync_assert(formatenv);
	OSyncObjFormat *objformat = osync_format_env_find_objformat(formatenv, "mockformat1");
	osync_assert(objformat);
	
	while ((de = g_dir_read_name(env->dir))) {
		char *filename = g_build_filename(env->path, de, NULL);
		osync_hashtable_report(env->hashtable, de);
		
		if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
			
			struct stat buf;
			stat(filename, &buf);
			char *hash = mock_generate_hash(&buf);
			
			OSyncChangeType type = osync_hashtable_get_changetype(env->hashtable, de, hash);
			if (type == OSYNC_CHANGE_TYPE_UNMODIFIED) {
				g_free(hash);
				g_free(filename);
				continue;
			}
			osync_hashtable_update_hash(env->hashtable, type, de, hash);
			
			/* Report normal files */
			OSyncChange *change = osync_change_new(&error);
			if (!change)
				goto error;
			
			osync_change_set_uid(change, de);
			osync_change_set_hash(change, hash);
			osync_change_set_changetype(change, type);
			
			OSyncFile *file = osync_try_malloc0(sizeof(OSyncFile), &error);
			if (!file) {
				g_free(filename);
				goto error;
			}
			file->path = g_strdup(de);
			
			OSyncError *error = NULL;
			if (!osync_file_read(filename, &(file->data), &(file->size), &error)) {
				g_free(filename);
				goto error;
			}
			
			OSyncData *odata = osync_data_new((char *)file, sizeof(OSyncFile), objformat, &error);
			if (!odata)
				goto error;
			
			osync_data_set_objtype(odata, "file");
			osync_change_set_data(change, odata);
			osync_data_unref(odata);
	
			osync_context_report_change(ctx, change);
			
			osync_change_unref(change);
			g_free(hash);
			
		}
		g_free(filename);
	}
	
	char **uids = osync_hashtable_get_deleted(env->hashtable);
	for (i = 0; uids[i]; i++) {
		OSyncChange *change = osync_change_new(&error);
		if (!change)
			goto error;
		
		osync_change_set_uid(change, uids[i]);
		osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);
		
		OSyncData *odata = osync_data_new(NULL, 0, objformat, &error);
		if (!odata)
			goto error;
		
		osync_data_set_objtype(odata, "file");
		osync_change_set_data(change, odata);
		osync_data_unref(odata);
		
		osync_context_report_change(ctx, change);
		
		osync_hashtable_update_hash(env->hashtable, osync_change_get_changetype(change), osync_change_get_uid(change), NULL);
	
		osync_change_unref(change);
		g_free(uids[i]);
	}
	g_free(uids);
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void commit_change(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	mock_env *env = data;
	OSyncData *odata = NULL;
	
	char *filename = NULL;
	OSyncError *error = NULL;
	filename = g_strdup_printf ("%s/%s", env->path, osync_change_get_uid(change));
	char *hash = NULL;
			
	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_DELETED:
			if (!remove(filename) == 0) {
				osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to write");
				g_free(filename);
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to write", __func__);
				return;
			}
			break;
		case OSYNC_CHANGE_TYPE_ADDED:
			if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
				osync_context_report_error(ctx, OSYNC_ERROR_EXISTS, "Entry already exists");
				g_free(filename);
				osync_trace(TRACE_EXIT_ERROR, "%s: Entry already exists", __func__);
				return;
			}
			/* No break. Continue below */
		case OSYNC_CHANGE_TYPE_MODIFIED:
			//FIXME add permission and ownership for file-sync
			odata = osync_change_get_data(change);
			
			char *buffer = NULL;
			unsigned int size = 0;
			osync_data_get_data(odata, &buffer, &size);
			
			OSyncFile *file = (OSyncFile *)buffer;
			
			if (!osync_file_write(filename, file->data, file->size, 0700, &error)) {
				osync_context_report_osyncerror(ctx, error);
				g_free(filename);
				osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
				return;
			}
			
			struct stat buf;
			stat(filename, &buf);
			hash = mock_generate_hash(&buf);
			break;
		default:
			fail("no changetype given");
	}
	g_free(filename);

	osync_hashtable_update_hash(env->hashtable, osync_change_get_changetype(change), osync_change_get_uid(change), hash);
	g_free(hash);
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void *initialize(OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);

	mock_env *env = osync_try_malloc0(sizeof(mock_env), error);
	if (!env)
		goto error;
	
	osync_trace(TRACE_INTERNAL, "The config: %s", osync_plugin_info_get_config(info));
	
	if (!mock_parse_settings(env, osync_plugin_info_get_config(info), error)) {
		g_free(env);
		return NULL;
	}

	OSyncObjTypeSink *sink = osync_objtype_sink_new("file", error);
	if (!sink)
		goto error;
	
	osync_objtype_sink_add_objformat(sink, "file");
	
	OSyncObjTypeSinkFunctions functions;
	memset(&functions, 0, sizeof(functions));
	functions.connect = connect;
	functions.disconnect = disconnect;
	functions.get_changes = get_changes;
	functions.commit = commit_change;
	
	osync_objtype_sink_set_functions(sink, functions);
	osync_plugin_info_add_objtype(info, sink);
	osync_objtype_sink_unref(sink);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void finalize(void *data)
{
	mock_env *env = data;

	g_free(env->path);
	
	g_free(env);
}

void get_sync_info(OSyncPluginEnv *env)
{
	OSyncError *error = NULL;
	OSyncPlugin *plugin = osync_plugin_new(&error);
	if (!plugin)
		goto error;
	
	
	osync_plugin_set_name(plugin, "mock-sync");
	osync_plugin_set_longname(plugin, "Mock Sync Plugin");
	osync_plugin_set_description(plugin, "This is a pseudo plugin");
	
	osync_plugin_set_initialize(plugin, initialize);
	osync_plugin_set_finalize(plugin, finalize);
	osync_plugin_set_discover(plugin, discover);
	
	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);
	
	return;
	
error:
	osync_trace(TRACE_ERROR, "Unable to register: %s", osync_error_print(&error));
	osync_error_unref(&error);
}

int get_version(void)
{
	return 1;
}
