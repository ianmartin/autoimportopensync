/*
 * tomboy-sync - A plugin for the opensync framework
 * Copyright (C) 2008  Bjoern Ricks <bjoern.ricks@googlemail.com>
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

#include "config.h"

#include "tomboy_sync.h"
#include "tomboy_sync_file.h"

/** Report files on a directory
 */
void osync_tomboysync_file_report_dir(OSyncTomboyDir *directory, OSyncPluginInfo *info, const char *subdir, OSyncContext *ctx)
{
	GError *gerror = NULL;
	const char *de = NULL;
	char *path = NULL;
	GDir *dir = NULL;
	OSyncError *error = NULL;

	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, directory, subdir, ctx);

	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);

	path = g_build_filename(directory->homedir_path,".tomboy", NULL);
	osync_trace(TRACE_INTERNAL, "path %s", path);

	dir = g_dir_open(path, 0, &gerror);
	if (!dir) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to open directory %s: %s", path, gerror ? gerror->message : "None");
		osync_context_report_osyncwarning(ctx, error);
		osync_error_unref(&error);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
		return;
	}
	GPatternSpec *pattern = g_pattern_spec_new("*.note");
	char *uuid = NULL;
	while ((de = g_dir_read_name(dir))) {
		/* only report note files */
		if (!g_pattern_match_string(pattern, de)) {
			continue;
		}
		char *filename = g_build_filename(path, de, NULL);
		int length = strlen(de) - 5; // remove .note
		if (length < 1) {
			osync_trace(TRACE_ERROR, "%s - %s", __func__,"");
			continue;
		}
		uuid = g_strndup(de,length);

		if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
//			g_print("report file %s with uuid %s\n", filename, uuid);
			
			char *data;
			unsigned int size;
			OSyncError *error = NULL;
			if (!osync_file_read(filename, &data, &size, &error)) {
				osync_context_report_osyncwarning(ctx, error);
				osync_error_unref(&error);
				g_free(filename);
				g_free(uuid);
				continue;
			}

			OSyncChange *change = osync_change_new(&error);
			if (!change) {
				osync_context_report_osyncwarning(ctx, error);
				osync_error_unref(&error);
				g_free(filename);
				continue;
			}

			osync_change_set_uid(change, uuid);

			char *hash = osync_tomboysync_generate_hash(data, size);
			if (!hash) {
				osync_trace(TRACE_ERROR, "could not create hash for file %s.", filename);
				continue;
			}
			osync_change_set_hash(change, hash);
			g_free(hash);

			OSyncChangeType type = osync_hashtable_get_changetype(directory->hashtable, change);
			osync_change_set_changetype(change, type);
			/* if change is modified (other hashvalue then in the hashtable) write the new value to the hashtable */
			osync_hashtable_update_change(directory->hashtable, change);

			if (type == OSYNC_CHANGE_TYPE_UNMODIFIED) {
				g_free(filename);
				g_free(uuid);
				osync_change_unref(change);
				continue;
			}

			/* create new data for the change */
			OSyncData *odata = NULL;
			OSyncObjFormat *tomboynoteformat = osync_format_env_find_objformat(formatenv, "tomboy-note");

			odata = osync_data_new((char *)data, size, tomboynoteformat, &error);
			if (!odata) {
				osync_change_unref(change);
				osync_context_report_osyncwarning(ctx, error);
				osync_error_unref(&error);
				g_free(data);
				g_free(filename);
				g_free(uuid);
				continue;
			}
			osync_data_set_objtype(odata, osync_objtype_sink_get_name(directory->sink));
			osync_change_set_data(change, odata);
			osync_data_unref(odata);
			/* report change */
			osync_context_report_change(ctx, change);

			osync_change_unref(change);
		}

		g_free(filename);
		g_free(uuid);

	}

	g_dir_close(dir);
	g_free(path);
	g_pattern_spec_free(pattern);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_tomboysync_file_get_changes(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncTomboyDir *dir = osync_objtype_sink_get_userdata(sink);
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	OSyncError *error = NULL;

	/* is slowsync necessary */
	if (osync_objtype_sink_get_slowsync(dir->sink)) {
		osync_trace(TRACE_INTERNAL, "Slow sync requested");
		/* prepare hashtable for slowsync (flush entries) */
		if (!osync_hashtable_slowsync(dir->hashtable, &error))
		{
			osync_context_report_osyncerror(ctx, error);
			osync_trace(TRACE_EXIT_ERROR, "%s - %s", __func__, osync_error_print(&error));
			osync_error_unref(&error);
			return;
		}
	}

	osync_trace(TRACE_INTERNAL, "get_changes for %s", osync_objtype_sink_get_name(sink));

	/* report new and changed items */
	osync_tomboysync_file_report_dir(dir, info, NULL, ctx);

	/* get all items which weren't changed in osync_tomboysync_file_report_dir. These items have to be deleted */
	OSyncList *u, *uids = osync_hashtable_get_deleted(dir->hashtable);
	for (u = uids; u; u = u->next) {
		OSyncChange *change = osync_change_new(&error);
		if (!change) {
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

		const char *uid = u->data;
		osync_change_set_uid(change, uid);
		osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);

		OSyncObjFormat *fileformat = osync_format_env_find_objformat(formatenv, "tomboy-note");

		OSyncData *odata = osync_data_new(NULL, 0, fileformat, &error);
		if (!odata) {
			osync_change_unref(change);
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

		osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));
		osync_change_set_data(change, odata);
		osync_data_unref(odata);

		osync_context_report_change(ctx, change);

		osync_hashtable_update_change(dir->hashtable, change);

		osync_change_unref(change);
	}
	osync_list_free(uids);

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*
 *
 */
void osync_tomboysync_file_commit_change(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncTomboyDir *dir = osync_objtype_sink_get_userdata(sink);

	char *filename = NULL;
	char* tomboydir = NULL;

	if (!osync_tomboysync_file_write(data, info, ctx, change)) {
		osync_trace(TRACE_EXIT_ERROR, "%s", __func__);
		return;
	}

	tomboydir = g_build_filename(dir->homedir_path, ".tomboy", NULL);
	filename = g_strdup_printf("%s%s%s.%s", tomboydir, G_DIR_SEPARATOR_S, osync_change_get_uid(change), "note");
	char *hash = NULL;

	if (osync_change_get_changetype(change) != OSYNC_CHANGE_TYPE_DELETED) {
		char *data;
		int size;
		OSyncData *odata = osync_change_get_data(change);
		osync_data_get_data(odata, &data, &size);
		hash = osync_tomboysync_generate_hash(data, size);
		osync_change_set_hash(change, hash);
	}
	g_free(filename);
	g_free(tomboydir);

	osync_hashtable_update_change(dir->hashtable, change);
	g_free(hash);

	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_tomboysync_file_sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);

	OSyncError *error = NULL;

	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncTomboyDir *dir = osync_objtype_sink_get_userdata(sink);

	char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
	char *path_field = g_strdup_printf("path_%s", osync_objtype_sink_get_name(sink));
	osync_anchor_update(anchorpath, path_field, dir->homedir_path);
	g_free(anchorpath);
	g_free(path_field);

	if (!osync_hashtable_save(dir->hashtable, &error))
		goto error;

	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
	return;
}

/* write a single entry */
osync_bool osync_tomboysync_file_write(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, data, info, ctx, change);

	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncTomboyDir *dir = osync_objtype_sink_get_userdata(sink);
	OSyncError *error = NULL;
	OSyncData *odata = NULL;
	char *buffer = NULL;
	char *uuid = NULL;
	unsigned int size = 0;

	uuid = (char*)osync_change_get_uid(change);
	char *tomboydir = g_build_filename(dir->homedir_path, ".tomboy", NULL);
	char *filename = g_strdup_printf("%s%s%s.%s", tomboydir, G_DIR_SEPARATOR_S, uuid, "note");

	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_DELETED:
			/* delete file */
//			g_print("delete file %s\n", filename);
			if (!g_remove(filename) == 0) {
				osync_error_set(&error, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to delete file %s", filename);
				goto error;
			}
			break;
		case OSYNC_CHANGE_TYPE_ADDED:
//			g_print("add file %s\n", filename);
			if(!osync_tomboysync_validate_uuid(uuid)) {
				uuid = osync_tomboysync_generate_uuid();
			}
			char *testfilename = g_strdup_printf("%s%s%s.%s", tomboydir, G_DIR_SEPARATOR_S, uuid, "note");
//			g_print("add with new uid file %s\n", filename);
			while (g_file_test(filename, G_FILE_TEST_EXISTS)) {
				g_free(uuid);
				uuid = osync_tomboysync_generate_uuid();
				osync_trace(TRACE_INTERNAL, "created new uuid %s for new entry with uid %s", __NULLSTR(uuid), __NULLSTR(osync_change_get_uid(change)) );
			}
			osync_change_set_uid(change, uuid);
			g_free(filename);
			filename = testfilename;
			/* No break. Continue below */
		case OSYNC_CHANGE_TYPE_MODIFIED:
//			g_print("modified file %s\n", filename);
			odata = osync_change_get_data(change);
			g_assert(odata);
			osync_data_get_data(odata, &buffer, &size);
			g_assert(buffer);
			if (!osync_file_write(filename, buffer, size, 0 , &error)) {
				goto error;
			}
			break;
		default:
			break;
	}

	g_free(filename);
	g_free(tomboydir);
	g_free(uuid);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	g_free(filename);
	g_free(tomboydir);
	g_free(uuid);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
	return FALSE;
}

/* called after ignore.
 * read a single entry and pass it to the engine */
osync_bool osync_tomboysync_file_read(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, userdata, info, ctx, change);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncTomboyDir *dir = osync_objtype_sink_get_userdata(sink);
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	OSyncError *error = NULL;

	char *tomboydir = g_build_filename(dir->homedir_path, ".tomboy", NULL);
	char *filename = g_strdup_printf("%s%s%s.%s", tomboydir, G_DIR_SEPARATOR_S, osync_change_get_uid(change), "note");

	char *data;
	unsigned int size;

	if (!osync_file_read(filename, &data, &size, &error)) {
		osync_change_unref(change);
		osync_context_report_osyncwarning(ctx, error);
		osync_error_unref(&error);
		goto error;
	}

	OSyncData *odata = NULL;

	OSyncObjFormat *format = osync_format_env_find_objformat(formatenv, "tomboy-note");

	odata = osync_data_new(data, size, format, &error);
	if (!odata) {
		osync_change_unref(change);
		osync_context_report_osyncwarning(ctx, error);
		osync_error_unref(&error);
		goto error_free_data;
	}

	osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));
	osync_change_set_data(change, odata);
	osync_data_unref(odata);

	osync_context_report_success(ctx);

	g_free(filename);
	g_free(tomboydir);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_data:
	g_free(data);
error:
	g_free(filename);
	g_free(tomboydir);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
	return FALSE;
}

osync_bool osync_tomboysync_file_initalize(OSyncTomboyEnv *tomboyenv, OSyncError **error) {
	osync_assert(tomboyenv);
	osync_assert(tomboyenv->dir)
	char * tomboydir = g_strdup_printf("%s/.tomboy", tomboyenv->dir->homedir_path);
	if (!g_file_test(tomboydir, G_FILE_TEST_IS_DIR)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s is not a directory.", tomboydir);
		goto error;
	}
	
	return TRUE;
error:
	return FALSE;
}
