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
#include "tomboy_sync_dbus.h"

#ifdef ENABLE_DBUS

char * osync_tomboysync_noteuri_to_uuid(char *noteuri) {
	GString *guid;
	char *uuid;
	
	guid = g_string_new(noteuri);
	guid = g_string_erase(guid, 0, 14); // delete note://tomboy/
	uuid = g_string_free(guid,FALSE);
	return uuid;
}

void osync_tomboysync_dbus_get_changes(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx) {
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, ctx);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncTomboyDir *dir = osync_objtype_sink_get_userdata(sink);
	OSyncTomboyEnv *tomboyenv = (OSyncTomboyEnv*)userdata;
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	OSyncError *error = NULL;
	OSyncData *odata = NULL;
	OSyncObjFormat *tomboynoteformat = NULL;
	GError *gerror = NULL;
	char **name_list;
	char **name_list_ptr;
	char *uuid = NULL;
	int size;

	/* is slowsync necessary */
	if (osync_objtype_sink_get_slowsync(dir->sink)) {
		osync_trace(TRACE_INTERNAL, "Slow sync requested");
		/* prepare hashtable for slowsync (flush entries) */
		if (!osync_hashtable_slowsync(dir->hashtable, &error)) {
			osync_context_report_osyncerror(ctx, error);
			osync_trace(TRACE_EXIT_ERROR, "%s - %s", __func__, osync_error_print(&error));
			osync_error_unref(&error);
			return;
		}
	}

	tomboynoteformat = osync_format_env_find_objformat(formatenv, "tomboy-note");
	
	osync_trace(TRACE_INTERNAL, "get_changes for %s", osync_objtype_sink_get_name(sink));

	/* report new and changed items */
	if (!dbus_g_proxy_call(tomboyenv->proxy, "ListAllNotes", &gerror, G_TYPE_INVALID, G_TYPE_STRV, &name_list, G_TYPE_INVALID)) {
		if (gerror->domain == DBUS_GERROR && gerror->code == DBUS_GERROR_REMOTE_EXCEPTION) {
			osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Caught remote method dbus exception %s: %s",	dbus_g_error_get_name(gerror), gerror->message);
		}
		else {
			osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Error: %s\n", gerror->message);
		}
		osync_context_report_osyncerror(ctx, error);
		osync_trace(TRACE_EXIT_ERROR, "%s - %s", __func__, osync_error_print(&error));
		g_error_free(gerror);
		osync_error_unref(&error);
		return;
	}
	for (name_list_ptr = name_list; *name_list_ptr; name_list_ptr++) {
		char *noteuri = *name_list_ptr;
		char *buffer;
		uuid = osync_tomboysync_noteuri_to_uuid(noteuri);
		OSyncChange *change = osync_change_new(&error);
		if (!change) {
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			g_free(uuid);
			continue;
		}

		osync_change_set_uid(change, uuid);
		if (!dbus_g_proxy_call(tomboyenv->proxy, "GetNoteCompleteXml", &gerror,G_TYPE_STRING, noteuri, G_TYPE_INVALID, G_TYPE_STRING, &buffer, G_TYPE_INVALID)) {
			if (gerror->domain == DBUS_GERROR && gerror->code == DBUS_GERROR_REMOTE_EXCEPTION) {
				osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Caught remote method dbus exception %s: %s",	dbus_g_error_get_name(gerror), gerror->message);
			}
			else {
				osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Error: %s\n", gerror->message);
			}
			osync_context_report_osyncerror(ctx, error);
			osync_trace(TRACE_EXIT_ERROR, "%s - %s", __func__, osync_error_print(&error));
			g_error_free(gerror);
			osync_error_unref(&error);
			return;
		}
		if (!strcmp(buffer,"")) {
			osync_trace(TRACE_ERROR, "could not receive data from dbus for uri\"%s\"", noteuri);
			continue;
		}
		size = strlen(buffer) + 1;
		char *hash = osync_tomboysync_generate_hash(buffer, size);
		if (!hash) {
			osync_trace(TRACE_ERROR, "could not create hash for uri %s.", noteuri);
			continue;
		}
		osync_change_set_hash(change, hash);
		g_free(hash);

		OSyncChangeType type = osync_hashtable_get_changetype(dir->hashtable, change);
		osync_change_set_changetype(change, type);
		/* if change is modified (other hashvalue then in the hashtable) write the new value to the hashtable */
		osync_hashtable_update_change(dir->hashtable, change);

		if (type == OSYNC_CHANGE_TYPE_UNMODIFIED) {
			g_free(uuid);
			osync_change_unref(change);
			continue;
		}

		/* create new data for the change */
		odata = osync_data_new(buffer, size, tomboynoteformat, &error);
		if (!odata) {
			osync_change_unref(change);
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			g_free(buffer);
			g_free(uuid);
			continue;
		}
		osync_data_set_objtype(odata, osync_objtype_sink_get_name(dir->sink));
		osync_change_set_data(change, odata);
		osync_data_unref(odata);
		/* report change */
		osync_context_report_change(ctx, change);

		osync_change_unref(change);
	}
	g_strfreev(name_list);

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

		odata = osync_data_new(NULL, 0, tomboynoteformat, &error);
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

void osync_tomboysync_dbus_commit_change(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change) {
	osync_trace(TRACE_ENTRY, "%s", __func__);
	
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncTomboyEnv *tomboyenv = (OSyncTomboyEnv*)userdata;
	OSyncTomboyDir *dir = osync_objtype_sink_get_userdata(sink);
	OSyncData *odata = NULL;
	
	int size;
	char *hash = NULL;
	char *buffer = NULL;

	if (!osync_tomboysync_dbus_write(userdata, info, ctx, change)) {
		/*TODO report error to context? */
		osync_trace(TRACE_EXIT_ERROR, "%s", __func__);
		return;
	}

	if (osync_change_get_changetype(change) != OSYNC_CHANGE_TYPE_DELETED) {
		odata = osync_change_get_data(change);
		osync_data_get_data(odata, &buffer, &size);
		hash = osync_tomboysync_generate_hash(buffer, size);
		osync_change_set_hash(change, hash);
	}
	osync_hashtable_update_change(dir->hashtable, change);
	osync_context_report_success(ctx);
	g_free(hash);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

void osync_tomboysync_dbus_sync_done(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx) {
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, ctx);

	OSyncError *error = NULL;
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncTomboyDir *dir = osync_objtype_sink_get_userdata(sink);
	OSyncTomboyEnv *tomboyenv = (OSyncTomboyEnv*)userdata;

	char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
	char *path_field = g_strdup_printf("path_%s", osync_objtype_sink_get_name(sink));
	osync_anchor_update(anchorpath, path_field, dir->homedir_path);
	g_free(anchorpath);
	g_free(path_field);

	if (!osync_hashtable_save(dir->hashtable, &error))
		goto error;

	osync_context_report_success(ctx);

	g_object_unref(tomboyenv->proxy);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	g_object_unref(tomboyenv->proxy);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
	return;	
}

osync_bool osync_tomboysync_dbus_write(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change) {
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, userdata, info, ctx, change);

	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncTomboyDir *dir = osync_objtype_sink_get_userdata(sink);
	OSyncTomboyEnv *tomboyenv = (OSyncTomboyEnv*)userdata;
	OSyncError *error = NULL;
	OSyncData *odata = NULL;
	GError *gerror = NULL;
	char *buffer = NULL;
	char *uuid = NULL;
	char* noteuri = NULL;
	unsigned int size = 0;

	uuid = (char*)osync_change_get_uid(change);
	noteuri = g_strdup_printf("note://tomboy/%s",uuid);
	
	gboolean returnval;

	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_DELETED:
			/* delete note */
			if (!dbus_g_proxy_call(tomboyenv->proxy, "DeleteNote", &gerror,	G_TYPE_STRING, noteuri, G_TYPE_INVALID, G_TYPE_BOOLEAN, &returnval, G_TYPE_INVALID)) {
				if (gerror->domain == DBUS_GERROR && gerror->code == DBUS_GERROR_REMOTE_EXCEPTION) {
					osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Caught remote method dbus exception %s: %s",	dbus_g_error_get_name(gerror), gerror->message);
				}
				else {
					osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Error: %s\n", gerror->message);
				}
				g_error_free(gerror);
				goto error;
			}
			if (!returnval) {
				osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Could not delete note with uri %s via dbus\n", noteuri);
				goto error;
			}
			break;
		case OSYNC_CHANGE_TYPE_ADDED:
			if (!dbus_g_proxy_call(tomboyenv->proxy, "CreateNote", &gerror,	G_TYPE_INVALID, G_TYPE_STRING, &noteuri, G_TYPE_INVALID)) {
				if (gerror->domain == DBUS_GERROR && gerror->code == DBUS_GERROR_REMOTE_EXCEPTION) {
					osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Caught remote method dbus exception %s: %s",	dbus_g_error_get_name(gerror), gerror->message);
				}
				else {
					osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Error: %s\n", gerror->message);
				}
				g_error_free(gerror);
				goto error;
			}
			uuid = osync_tomboysync_noteuri_to_uuid(noteuri); 
			osync_change_set_uid(change, uuid);
			/* No break. Continue below */
		case OSYNC_CHANGE_TYPE_MODIFIED:
			odata = osync_change_get_data(change);
			g_assert(odata);
			osync_data_get_data(odata, &buffer, &size);
			g_assert(buffer);
			osync_trace(TRACE_INTERNAL, "try to set content of uri %s with size %d to \"%s\"", noteuri, size, buffer);
			if (!dbus_g_proxy_call(tomboyenv->proxy, "SetNoteCompleteXml", &gerror,	G_TYPE_STRING, noteuri, G_TYPE_STRING, buffer, G_TYPE_INVALID, G_TYPE_BOOLEAN, &returnval, G_TYPE_INVALID)) {
				if (gerror->domain == DBUS_GERROR && gerror->code == DBUS_GERROR_REMOTE_EXCEPTION) {
					osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Caught remote method dbus exception %s: %s",	dbus_g_error_get_name(gerror), gerror->message);
				}
				else {
					osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Error: %s\n", gerror->message);
				}
				g_error_free(gerror);
				goto error;
			}
			if (!returnval) {
				osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Could not set content of note with uri %s via dbus\n", noteuri);
				goto error;
			}
			break;
		default:
			break;
	}
	g_free(noteuri);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:
	g_free(noteuri);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
	return FALSE;
}

/* called after ignore.
 * read a single entry and pass it to the engine */
osync_bool osync_tomboysync_dbus_read(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change) {
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, userdata, info, ctx, change);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncTomboyDir *dir = osync_objtype_sink_get_userdata(sink);
	OSyncTomboyEnv *tomboyenv = (OSyncTomboyEnv*)userdata;
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	OSyncError *error = NULL;
	OSyncData *odata = NULL;
	GError *gerror = NULL;

	char *uuid = NULL;
	char *noteuri = NULL;
	char *buffer;
	unsigned int size;

	uuid = (char*)osync_change_get_uid(change);
	noteuri = g_strdup_printf("note://tomboy/%s",uuid);
	
	if (!dbus_g_proxy_call(tomboyenv->proxy, "GetNoteCompleteXml", &gerror,G_TYPE_STRING, noteuri, G_TYPE_INVALID, G_TYPE_STRING, &buffer, G_TYPE_INVALID)) {
		if (gerror->domain == DBUS_GERROR && gerror->code == DBUS_GERROR_REMOTE_EXCEPTION) {
			osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Caught remote method dbus exception %s: %s",	dbus_g_error_get_name(gerror), gerror->message);
		}
		else {
			osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Error: %s\n", gerror->message);
		}
		osync_context_report_osyncerror(ctx, error);
		osync_trace(TRACE_EXIT_ERROR, "%s - %s", __func__, osync_error_print(&error));
		g_error_free(gerror);
		osync_error_unref(&error);
		return;
	}
	if (!strcmp(buffer,"")) {
		osync_error_set(&error, OSYNC_ERROR_FILE_NOT_FOUND, "could not receive data from dbus for uri \"%s\"", noteuri);
		goto error_free_data;
	}
	size = strlen(buffer) + 1;
	char *hash = osync_tomboysync_generate_hash(buffer, size);
	if (!hash) {
		osync_error_set(&error, OSYNC_ERROR_FILE_NOT_FOUND,"could not create hash for uri \"%s\".", noteuri);
		goto error_free_data;
	}

	OSyncObjFormat *format = osync_format_env_find_objformat(formatenv, "tomboy-note");

	odata = osync_data_new(buffer, size, format, &error);
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

	g_free(uuid);
	g_free(noteuri);
	g_free(buffer);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_data:
	g_free(buffer);
error:
	g_free(uuid);
	g_free(noteuri);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
	return FALSE;	
}

osync_bool osync_tomboysync_dbus_initalize(OSyncTomboyEnv *tomboyenv, OSyncError **error) {
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, tomboyenv, error);
	
	osync_assert(tomboyenv);
	
	GError *gerror = NULL;
	g_type_init();

	tomboyenv->connection = dbus_g_bus_get(DBUS_BUS_SESSION, &gerror);
	if (tomboyenv->connection == NULL) {
		osync_error_set(error, OSYNC_ERROR_NO_CONNECTION, "Could not connect to dbus. Error \"%s\".", gerror->message);
		g_error_free(gerror);
		goto error;
	}
	
	tomboyenv->proxy = dbus_g_proxy_new_for_name(tomboyenv->connection, TOMBOY_DBUS_NAME, TOMBOY_DBUS_PATH, TOMBOY_DBUS_INTERFACE);
	return TRUE;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

#endif /* ENABLE_DBUS */

