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

#include "mock_sync.h"
#include "mock_format.h"

static osync_bool mock_get_error(long long int memberid, const char *domain)
{
        const char *env = g_getenv(domain);

        if (!env)
                return FALSE;

        int num = atoi(env);
        int mask = 1 << (memberid - 1);
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

static void free_dir(OSyncFileDir *dir)
{
	if (dir->path)
		g_free(dir->path);
		
	if (dir->objtype)
		g_free(dir->objtype);
	
	g_free(dir);
}

static void free_env(mock_env *env)
{
	while (env->directories) {
		OSyncFileDir *dir = env->directories->data;
		
		if (dir->sink)
			osync_objtype_sink_unref(dir->sink);
		
		free_dir(dir);
		env->directories = g_list_remove(env->directories, dir);
	}
	
	g_free(env);
}

static osync_bool mock_parse_directory(mock_env *env, OSyncPluginInfo *info, xmlNode *cur, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, env, info, cur, error);

	OSyncFileDir *dir = osync_try_malloc0(sizeof(OSyncFileDir), error);
	if (!dir)
		goto error;
	dir->env = env;

	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	
	while (cur != NULL) {
		char *str = (char*)xmlNodeGetContent(cur);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"path")) {
				dir->path = g_strdup(str);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"objtype")) {
				dir->objtype = g_strdup(str);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"objformat")) {
				dir->objformat = osync_format_env_find_objformat(formatenv, str);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"recursive")) {
				dir->recursive = (g_ascii_strcasecmp(str, "TRUE") == 0);
			}
			xmlFree(str);
		}
		cur = cur->next;
	}

	if (!dir->path) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Path not set");
		goto error_free_dir;
	}

	osync_trace(TRACE_INTERNAL, "Got directory %s with objtype %s", dir->path, dir->objtype);

	env->directories = g_list_append(env->directories, dir);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_dir:
	free_dir(dir);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/*Load the state from a xml file and return it in the conn struct*/
static osync_bool mock_parse_settings(mock_env *env, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, info, error);
	const char *data = osync_plugin_info_get_config(info);
	xmlDoc *doc = NULL;
	xmlNode *cur = NULL;

	doc = xmlParseMemory(data, strlen(data) + 1);
	if (!doc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to parse settings");
		goto error;
	}

	cur = xmlDocGetRootElement(doc);

	if (!cur) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get root element of the settings");
		goto error_free_doc;
	}

	if (xmlStrcmp(cur->name, (xmlChar*)"config")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Config valid is not valid");
		goto error_free_doc;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		char *str = (char*)xmlNodeGetContent(cur);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"directory")) {
				if (!mock_parse_directory(env, info, cur->xmlChildrenNode, error))
					goto error_free_doc;
			}
			xmlFree(str);
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_doc:
	xmlFreeDoc(doc);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static char *mock_generate_hash(struct stat *buf)
{
	char *hash = g_strdup_printf("%i-%i", (int)buf->st_mtime, (int)buf->st_ctime);
	return hash;
}

static void mock_connect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	OSyncError *error = NULL;
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncFileDir *dir = osync_objtype_sink_get_userdata(sink);

        dir->committed_all = TRUE;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	
	if (mock_get_error(info->memberid, "CONNECT_ERROR")) {
		osync_context_report_error(ctx, OSYNC_ERROR_EXPECTED, "Triggering CONNECT_ERROR error");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, "Triggering CONNECT_ERROR error");
		return;
	}

	if (mock_get_error(info->memberid, "CONNECT_TIMEOUT")) {
		/* Don't report context back ... let it timeout! */
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, "Triggering CONNECT_TIMEOUT error");
		return;
	}

	osync_trace(TRACE_INTERNAL, "The configdir: %s", osync_plugin_info_get_configdir(info));
	char *tablepath = g_strdup_printf("%s/hashtable.db", osync_plugin_info_get_configdir(info));
	dir->hashtable = osync_hashtable_new(tablepath, osync_objtype_sink_get_name(sink), &error);
	g_free(tablepath);
	
	if (!dir->hashtable)
		goto error;

	char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
	char *path_field = g_strdup_printf("path_%s", osync_objtype_sink_get_name(sink));
	if (!osync_anchor_compare(anchorpath, path_field, dir->path))
		osync_objtype_sink_set_slowsync(dir->sink, TRUE);
	g_free(anchorpath);
	g_free(path_field);
	
	if (!g_file_test(dir->path, G_FILE_TEST_IS_DIR)) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "%s is not a directory", dir->path);
		goto error;
	}
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void mock_disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncFileDir *dir = osync_objtype_sink_get_userdata(sink);

	if (!g_getenv("NO_COMMITTED_ALL_CHECK"))
		fail_unless(dir->committed_all == TRUE, NULL);
	dir->committed_all = FALSE;

	if (mock_get_error(info->memberid, "DISCONNECT_ERROR")) {
		osync_context_report_error(ctx, OSYNC_ERROR_EXPECTED, "Triggering DISCONNECT_ERROR error");
		return;
	}
	if (mock_get_error(info->memberid, "DISCONNECT_TIMEOUT"))
		return;

	
	if (dir->hashtable) {
		osync_hashtable_free(dir->hashtable);
		dir->hashtable = NULL;
	}
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}


//typedef void (* OSyncSinkWriteFn) 
//typedef void (* OSyncSinkCommittedAllFn) (void *data, OSyncPluginInfo *info, OSyncContext *ctx);


static osync_bool mock_read(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, data, info, ctx, change);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncFileDir *dir = osync_objtype_sink_get_userdata(sink);
	OSyncError *error = NULL;
	
	char *filename = g_strdup_printf("%s/%s", dir->path, osync_change_get_uid(change));
	
	OSyncFileFormat *file = osync_try_malloc0(sizeof(OSyncFileFormat), &error);
	if (!file)
		goto error;
	file->path = g_strdup(osync_change_get_uid(change));
	
	struct stat filestats;
	stat(filename, &filestats);
	file->userid = filestats.st_uid;
	file->groupid = filestats.st_gid;
	file->mode = filestats.st_mode;
	file->last_mod = filestats.st_mtime;
			
	if (!osync_file_read(filename, &(file->data), &(file->size), &error))
		goto error_free_file;
	
	OSyncData *odata = osync_data_new((char *)file, sizeof(OSyncFileFormat), dir->objformat, &error);
	if (!odata)
		goto error_free_data;

	osync_trace(TRACE_INTERNAL, "odata: %p", odata);
	
	osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));
	osync_change_set_data(change, odata);
	osync_data_unref(odata);
	
	osync_context_report_success(ctx);
	
	g_free(filename);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_data:
	g_free(file->data);
error_free_file:
	g_free(file->path);
	g_free(file);
error:
	g_free(filename);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
	return FALSE;
}

static osync_bool mock_write(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, data, info, ctx, change);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncFileDir *dir = osync_objtype_sink_get_userdata(sink);
	OSyncError *error = NULL;
	OSyncData *odata = NULL;
	char *buffer = NULL;
	unsigned int size = 0;
	
	char *filename = g_strdup_printf ("%s/%s", dir->path, osync_change_get_uid(change));
			
	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_DELETED:
			if (!remove(filename) == 0) {
				osync_error_set(&error, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to write");
				goto error;
			}
			break;
		case OSYNC_CHANGE_TYPE_ADDED:
			if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
				osync_error_set(&error, OSYNC_ERROR_EXISTS, "Entry already exists");
				goto error;
			}
			/* No break. Continue below */
		case OSYNC_CHANGE_TYPE_MODIFIED:
			//FIXME add ownership for file-sync
			odata = osync_change_get_data(change);
			g_assert(odata);
			osync_data_get_data(odata, &buffer, &size);
			g_assert(buffer);
			g_assert(size == sizeof(OSyncFileFormat));
			
			OSyncFileFormat *file = (OSyncFileFormat *)buffer;
			
			if (!osync_file_write(filename, file->data, file->size, file->mode, &error))
				goto error;
			break;
		default:
			break;
	}
	
	g_free(filename);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error:
	g_free(filename);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
	return FALSE;
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
static void mock_report_dir(OSyncFileDir *directory, const char *subdir, OSyncContext *ctx, OSyncPluginInfo *info)
{
	GError *gerror = NULL;
	const char *de = NULL;
	char *path = NULL;
	GDir *dir = NULL;
	OSyncError *error = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, directory, subdir, ctx);
	
	path = g_build_filename(directory->path, subdir, NULL);
	osync_trace(TRACE_INTERNAL, "path %s", path);
	
	dir = g_dir_open(path, 0, &gerror);
	if (!dir) {
		/*FIXME: Permission errors may make files to be reported as deleted.
		 * Make fs_report_dir() able to report errors
		 */
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to open directory %s: %s", path, gerror ? gerror->message : "None");
		osync_context_report_osyncwarning(ctx, error);
		osync_error_unref(&error);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
		return;
	}

	while ((de = g_dir_read_name(dir))) {
		char *filename = g_build_filename(path, de, NULL);
		char *relative_filename = NULL;
		if (!subdir)
			relative_filename = g_strdup(de);
		else
			relative_filename = g_build_filename(subdir, de, NULL);
			
		osync_hashtable_report(directory->hashtable, relative_filename);
		osync_trace(TRACE_INTERNAL, "path2 %s %s", filename, relative_filename);
		
		if (g_file_test(filename, G_FILE_TEST_IS_DIR)) {
			/* Recurse into subdirectories */
			if (directory->recursive)
				mock_report_dir(directory, relative_filename, ctx, info);
		} else if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
			
			struct stat buf;
			stat(filename, &buf);
			char *hash = mock_generate_hash(&buf);
			
			OSyncChangeType type = osync_hashtable_get_changetype(directory->hashtable, relative_filename, hash);
			if (type == OSYNC_CHANGE_TYPE_UNMODIFIED) {
				g_free(hash);
				g_free(filename);
				g_free(relative_filename);
				continue;
			}
			osync_hashtable_update_hash(directory->hashtable, type, relative_filename, hash);
			
			/* Report normal files */
			OSyncChange *change = osync_change_new(&error);
			if (!change) {
				osync_context_report_osyncwarning(ctx, error);
				osync_error_unref(&error);
				g_free(hash);
				g_free(filename);
				g_free(relative_filename);
				continue;
			}
			
			osync_change_set_uid(change, relative_filename);
			osync_change_set_hash(change, hash);
			osync_change_set_changetype(change, type);

			g_free(hash);
			
			OSyncFileFormat *file = osync_try_malloc0(sizeof(OSyncFileFormat), &error);
			if (!file) {
				osync_change_unref(change);
				osync_context_report_osyncwarning(ctx, error);
				osync_error_unref(&error);
				g_free(filename);
				g_free(relative_filename);
				continue;
			}
			file->path = g_strdup(relative_filename);
			
			OSyncError *error = NULL;
			OSyncData *odata = NULL;

			if (mock_get_error(info->memberid, "ONLY_INFO")) {
				odata = osync_data_new(NULL, 0, directory->objformat, &error);
			} else {
				if (!osync_file_read(filename, &(file->data), &(file->size), &error)) {
					osync_change_unref(change);
					osync_context_report_osyncwarning(ctx, error);
					osync_error_unref(&error);
					g_free(filename);
					continue;
				}

				if (mock_get_error(info->memberid, "SLOW_REPORT"))
					sleep(1);
				
				odata = osync_data_new((char *)file, sizeof(OSyncFileFormat), directory->objformat, &error);
				if (!odata) {
					osync_change_unref(change);
					osync_context_report_osyncwarning(ctx, error);
					osync_error_unref(&error);
					continue;
				}

			}
			
			osync_data_set_objtype(odata, osync_objtype_sink_get_name(directory->sink));
			osync_change_set_data(change, odata);
			osync_data_unref(odata);
	
			osync_context_report_change(ctx, change);
			
			osync_change_unref(change);
		}

		g_free(filename);
		g_free(relative_filename);

	}

	g_dir_close(dir);

	g_free(path);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void mock_get_changes(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncFileDir *dir = osync_objtype_sink_get_userdata(sink);
	int i = 0;
	OSyncError *error = NULL;

	fail_unless(dir->committed_all == TRUE, NULL);
	dir->committed_all = FALSE;

	if (mock_get_error(info->memberid, "GET_CHANGES_ERROR")) {
		osync_context_report_error(ctx, OSYNC_ERROR_EXPECTED, "Triggering GET_CHANGES_ERROR error");
		osync_trace(TRACE_EXIT_ERROR, "%s - Triggering GET_CHANGES error", __func__);
		return;
	}

	if (mock_get_error(info->memberid, "GET_CHANGES_TIMEOUT")) {
		osync_trace(TRACE_EXIT, "%s - Triggering GET_CHANGES_TIMEOUT (without context report!)", __func__);
		return;
	}

	if (mock_get_error(info->memberid, "GET_CHANGES_TIMEOUT2"))
		sleep(8);
		
	if (osync_objtype_sink_get_slowsync(dir->sink)) {
		osync_trace(TRACE_INTERNAL, "Slow sync requested");
		osync_hashtable_reset(dir->hashtable);
	}
	
	osync_trace(TRACE_INTERNAL, "get_changes for %s", osync_objtype_sink_get_name(sink));

	mock_report_dir(dir, NULL, ctx, info);
	
	char **uids = osync_hashtable_get_deleted(dir->hashtable);
	for (i = 0; uids[i]; i++) {
		OSyncChange *change = osync_change_new(&error);
		if (!change) {
			g_free(uids[i]);
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}
		
		osync_change_set_uid(change, uids[i]);
		osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);
		
		OSyncData *odata = osync_data_new(NULL, 0, dir->objformat, &error);
		if (!odata) {
			g_free(uids[i]);
			osync_change_unref(change);
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}
		
		osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));
		osync_change_set_data(change, odata);
		osync_data_unref(odata);
		
		osync_context_report_change(ctx, change);
		
		osync_hashtable_update_hash(dir->hashtable, osync_change_get_changetype(change), osync_change_get_uid(change), NULL);
	
		osync_change_unref(change);
		g_free(uids[i]);
	}
	g_free(uids);
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void mock_commit_change(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncFileDir *dir = osync_objtype_sink_get_userdata(sink);
	
	char *filename = NULL;

	fail_unless(dir->committed_all == FALSE, NULL);

	if (mock_get_error(info->memberid, "COMMIT_ERROR")) {
		osync_context_report_error(ctx, OSYNC_ERROR_EXPECTED, "Triggering COMMIT_ERROR error");
		return;
	}
	if (mock_get_error(info->memberid, "COMMIT_TIMEOUT")) {
		osync_trace(TRACE_EXIT_ERROR, "COMMIT_TIMEOUT (mock-sync)!");
		return;
	}
	
	if (!mock_write(data, info, ctx, change)) {
		osync_trace(TRACE_EXIT_ERROR, "%s", __func__);
		return;
	}
	
	filename = g_strdup_printf ("%s/%s", dir->path, osync_change_get_uid(change));
	char *hash = NULL;
	
	if (osync_change_get_changetype(change) != OSYNC_CHANGE_TYPE_DELETED) {
		struct stat buf;
		stat(filename, &buf);
		hash = mock_generate_hash(&buf);
	}
	g_free(filename);

	osync_hashtable_update_hash(dir->hashtable, osync_change_get_changetype(change), osync_change_get_uid(change), hash);
	g_free(hash);
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void mock_batch_commit(void *data, OSyncPluginInfo *info, OSyncContext *context, OSyncContext **contexts, OSyncChange **changes)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, context, contexts, changes);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncFileDir *dir = osync_objtype_sink_get_userdata(sink);

        fail_unless(dir->committed_all == FALSE, NULL);
        dir->committed_all = TRUE;

        int i; 
        for (i = 0; contexts[i]; i++) {
                if (mock_write(data, info, contexts[i], changes[i])) {
			char *filename = g_strdup_printf ("%s/%s", dir->path, osync_change_get_uid(changes[i]));
			char *hash = NULL;
	
			if (osync_change_get_changetype(changes[i]) != OSYNC_CHANGE_TYPE_DELETED) {
				struct stat buf;
				stat(filename, &buf);
				hash = mock_generate_hash(&buf);
			}
			g_free(filename);

			osync_hashtable_update_hash(dir->hashtable, osync_change_get_changetype(changes[i]), osync_change_get_uid(changes[i]), hash);
                        osync_context_report_success(contexts[i]);
                }
        }

        if (g_getenv("NUM_BATCH_COMMITS")) {
                int req = atoi(g_getenv("NUM_BATCH_COMMITS"));
                fail_unless(req == i, NULL);
        }
                
        if (mock_get_error(info->memberid, "COMMITTED_ALL_ERROR")) {
                osync_context_report_error(context, OSYNC_ERROR_EXPECTED, "Triggering COMMITTED_ALL_ERROR error");
                return;
        }

        osync_context_report_success(context);

        osync_trace(TRACE_EXIT, "%s", __func__);
}

static void mock_committed_all(void *data, OSyncPluginInfo *info, OSyncContext *context)
{
        osync_trace(TRACE_ENTRY, "%s(%p)", __func__, context);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncFileDir *dir = osync_objtype_sink_get_userdata(sink);

        fail_unless(dir->committed_all == FALSE, NULL);
        dir->committed_all = TRUE;

        if (mock_get_error(info->memberid, "COMMITTED_ALL_ERROR")) {
                osync_context_report_error(context, OSYNC_ERROR_EXPECTED, "Triggering COMMITTED_ALL_ERROR error");
                osync_trace(TRACE_EXIT_ERROR, "%s: Reporting error", __func__);
                return;
        }

        osync_context_report_success(context);

        osync_trace(TRACE_EXIT, "%s", __func__);
}

static void mock_sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncFileDir *dir = osync_objtype_sink_get_userdata(sink);

	if (mock_get_error(info->memberid, "SYNC_DONE_ERROR")) {
		osync_context_report_error(ctx, OSYNC_ERROR_EXPECTED, "Triggering SYNC_DONE_ERROR error");
		return;
	}
	if (mock_get_error(info->memberid, "SYNC_DONE_TIMEOUT"))
		return;
	
	char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
	char *path_field = g_strdup_printf("path_%s", osync_objtype_sink_get_name(sink));
	osync_anchor_update(anchorpath, path_field, dir->path);
	g_free(anchorpath);
	g_free(path_field);
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/* In initialize, we get the config for the plugin. Here we also must register
 * all _possible_ objtype sinks. */
static void *mock_initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);

	if (mock_get_error(info->memberid, "INIT_NULL")) {
		osync_error_set(error, OSYNC_ERROR_EXPECTED, "Triggering INIT_NULL error");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}

	mock_env *env = osync_try_malloc0(sizeof(mock_env), error);
	if (!env)
		goto error;
	
	osync_trace(TRACE_INTERNAL, "The config: %s", osync_plugin_info_get_config(info));
	
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	
	if (!mock_parse_settings(env, info, error))
		goto error_free_env;
	
	/* Now we register the objtypes that we can sync. This plugin is special. It can
	 * synchronize any objtype we configure it to sync and where a conversion
	 * path to the file format can be found */
	GList *o = env->directories;
	for (; o; o = o->next) {
		OSyncFileDir *dir = o->data;
		/* We register the given objtype here */
		OSyncObjTypeSink *sink = osync_objtype_sink_new(dir->objtype, error);
		if (!sink)
			goto error_free_env;
		
		dir->sink = sink;
		
		osync_objtype_sink_add_objformat(sink, osync_objformat_get_name(dir->objformat));
		
		/* All sinks have the same functions of course */
		OSyncObjTypeSinkFunctions functions;
		memset(&functions, 0, sizeof(functions));
		functions.connect = mock_connect;
		functions.disconnect = mock_disconnect;
		functions.get_changes = mock_get_changes;

		//Rewrite the batch commit functions so we can enable them if necessary
		if (mock_get_error(info->memberid, "BATCH_COMMIT")) {
			osync_trace(TRACE_INTERNAL, "Enabling batch_commit on %p:%s", sink, osync_objtype_sink_get_name(sink) ? osync_objtype_sink_get_name(sink) : "None");
	                functions.batch_commit = mock_batch_commit; 
		} else {
			functions.committed_all = mock_committed_all;
			functions.commit = mock_commit_change;
		}
		functions.read = mock_read;
		functions.write = mock_write;
		functions.sync_done = mock_sync_done;
		
		/* We pass the OSyncFileDir object to the sink, so we dont have to look it up
		 * again once the functions are called */
		osync_objtype_sink_set_functions(sink, functions, dir);
		osync_plugin_info_add_objtype(info, sink);

		/*
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
		*/
	}

	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;

error_free_env:
	free_env(env);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void mock_finalize(void *data)
{
	mock_env *env = data;

	free_env(env);
}

/* Here we actually tell opensync which sinks are available. For this plugin, we
 * go through the list of directories and enable all, since all have been configured */
static osync_bool mock_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);
	
	mock_env *env = (mock_env *)data;
	GList *o = env->directories;
	for (; o; o = o->next) {
		OSyncFileDir *dir = o->data;
		osync_objtype_sink_set_available(dir->sink, TRUE);
	}
	
	OSyncVersion *version = osync_version_new(error);
	osync_version_set_plugin(version, "mock-sync");
	//osync_version_set_vendor(version, "version");
	//osync_version_set_modelversion(version, "version");
	//osync_version_set_firmwareversion(version, "firmwareversion");
	//osync_version_set_softwareversion(version, "softwareversion");
	//osync_version_set_hardwareversion(version, "hardwareversion");
	osync_plugin_info_set_version(info, version);
	osync_version_unref(version);

	/* we can set here the capabilities, but for the file-sync
	 * plugin they are static and shipped with opensync */

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
		goto error;
	
	osync_plugin_set_name(plugin, "mock-sync");
	osync_plugin_set_longname(plugin, "File Synchronization Plugin");
	osync_plugin_set_description(plugin, "Plugin to synchronize files on the local filesystem");
	
	osync_plugin_set_initialize(plugin, mock_initialize);
	osync_plugin_set_finalize(plugin, mock_finalize);
	osync_plugin_set_discover(plugin, mock_discover);
	
	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);

	plugin = osync_plugin_new(error);
	if (!plugin)
		goto error;
	
	osync_plugin_set_name(plugin, "mock-sync-external");
	osync_plugin_set_longname(plugin, "Mock Synchronization Plugin with Start Type External");
	osync_plugin_set_description(plugin, "Plugin to synchronize files on the local filesystem for unit tests");
	osync_plugin_set_start_type(plugin, OSYNC_START_TYPE_EXTERNAL);
	
	osync_plugin_set_initialize(plugin, mock_initialize);
	osync_plugin_set_finalize(plugin, mock_finalize);
	osync_plugin_set_discover(plugin, mock_discover);
	
	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);

	
	return TRUE;
	
error:
	osync_trace(TRACE_ERROR, "Unable to register: %s", osync_error_print(error));
	osync_error_unref(error);
	return FALSE;
}

int get_version(void)
{
	return 1;
}
