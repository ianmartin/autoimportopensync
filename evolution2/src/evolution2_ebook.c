/*
 * evolution2_sync - A plugin for the opensync framework
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
 
#include "evolution2_sync.h"

static void evo2_ebook_connect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	OSyncError *error = NULL;
	GError *gerror = NULL;
	ESourceList *sources = NULL;
	ESource *source = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncEvoEnv *env = (OSyncEvoEnv *)data;
	
	if (!env->addressbook_path) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "no addressbook path set");
	  	goto error;
	}

	if (strcmp(env->addressbook_path, "default")) {
		if (!e_book_get_addressbooks(&sources, &gerror)) {
	  		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Error getting addressbooks: %s", gerror ? gerror->message : "None");
	  		goto error;
		}
		
		if (!(source = evo2_find_source(sources, env->addressbook_path))) {
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Error finding source \"%s\"", env->addressbook_path);
	  		goto error;
		}
		
		if (!(env->addressbook = e_book_new(source, &gerror))) {
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to alloc new addressbook: %s", gerror ? gerror->message : "None");
	  		goto error;
		}
	} else {
		osync_trace(TRACE_INTERNAL, "Opening default addressbook\n");
		if (!(env->addressbook = e_book_new_default_addressbook(&gerror))) {
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to alloc new default addressbook: %s", gerror ? gerror->message : "None");
	  		goto error;
		}
	}
	
	char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
	if (!osync_anchor_compare(anchorpath, "contact", env->addressbook_path))
		osync_objtype_sink_set_slowsync(sink, TRUE);
	g_free(anchorpath);
	
	if (!e_book_open(env->addressbook, TRUE, &gerror)) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to alloc new addressbook: %s", gerror ? gerror->message : "None");
	  	goto error_free_book;
	}
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_book:
		g_object_unref(env->addressbook);
		env->addressbook = NULL;
error:
	if (gerror)
		g_clear_error(&gerror);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void evo2_ebook_disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	OSyncEvoEnv *env = (OSyncEvoEnv *)data;
	
	if (env->addressbook) {
		g_object_unref(env->addressbook);
		env->addressbook = NULL;
	}
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void evo2_ebook_sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	OSyncEvoEnv *env = (OSyncEvoEnv *)data;

	char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
	osync_anchor_update(anchorpath, "contact", env->addressbook_path);
	g_free(anchorpath);
	
	
	GList *changes = NULL;
	e_book_get_changes(env->addressbook, env->change_id, &changes, NULL);
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void evo2_report_change(OSyncContext *ctx, OSyncObjFormat *format, char *data, unsigned int size, const char *uid, OSyncChangeType changetype)
{
	OSyncError *error = NULL;
	
	OSyncChange *change = osync_change_new(&error);
	if (!change) {
		osync_context_report_osyncwarning(ctx, error);
		osync_error_unref(&error);
		return;
	}
	
	osync_change_set_uid(change, uid);
	osync_change_set_changetype(change, changetype);
	
	OSyncData *odata = osync_data_new(data, size, format, &error);
	if (!odata) {
		osync_change_unref(change);
		osync_context_report_osyncwarning(ctx, error);
		osync_error_unref(&error);
		return;
	}
	
	osync_change_set_data(change, odata);
	osync_data_unref(odata);

	osync_context_report_change(ctx, change);
	
	osync_change_unref(change);
}

static void evo2_ebook_get_changes(void *indata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, indata, info, ctx);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncEvoEnv *env = (OSyncEvoEnv *)indata;
	OSyncError *error = NULL;
	
	GList *changes = NULL;
	EBookChange *ebc = NULL;
	EVCard vcard;
	GList *l = NULL;
	char *data = NULL;
	char *uid = NULL;
	int datasize = 0;
	GError *gerror = NULL;
	
	if (osync_objtype_sink_get_slowsync(sink) == FALSE) {
		osync_trace(TRACE_INTERNAL, "No slow_sync for contact");
		if (!e_book_get_changes(env->addressbook, env->change_id, &changes, &gerror)) {
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to alloc new default addressbook: %s", gerror ? gerror->message : "None");
			goto error;
		}
		osync_trace(TRACE_INTERNAL, "Found %i changes for change-ID %s", g_list_length(changes), env->change_id);
		
		for (l = changes; l; l = l->next) {
			ebc = (EBookChange *)l->data;
			uid = g_strdup(e_contact_get_const(ebc->contact, E_CONTACT_UID));
			e_contact_set(ebc->contact, E_CONTACT_UID, NULL);
			switch (ebc->change_type) {
				case E_BOOK_CHANGE_CARD_ADDED:
					vcard = ebc->contact->parent;
					data = e_vcard_to_string(&vcard, EVC_FORMAT_VCARD_30);
					datasize = strlen(data) + 1;
					evo2_report_change(ctx, env->contact_format, data, datasize, uid, OSYNC_CHANGE_TYPE_ADDED);
					break;
				case E_BOOK_CHANGE_CARD_MODIFIED:
					vcard = ebc->contact->parent;
					data = e_vcard_to_string(&vcard, EVC_FORMAT_VCARD_30);
					datasize = strlen(data) + 1;
					evo2_report_change(ctx, env->contact_format, data, datasize, uid, OSYNC_CHANGE_TYPE_MODIFIED);
					break;
				case E_BOOK_CHANGE_CARD_DELETED:
					evo2_report_change(ctx, env->contact_format, NULL, 0, uid, OSYNC_CHANGE_TYPE_DELETED);
					break;
			}
			g_free(uid);
		}
	} else {
		osync_trace(TRACE_INTERNAL, "slow_sync for contact");
		EBookQuery *query = e_book_query_any_field_contains("");
		if (!e_book_get_contacts(env->addressbook, query, &changes, &gerror)) {
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to get changes from addressbook: %s", gerror ? gerror->message : "None");
			goto error;
		}
		for (l = changes; l; l = l->next) {
			EContact *contact = E_CONTACT(l->data);
			vcard = contact->parent;
			char *data = e_vcard_to_string(&vcard, EVC_FORMAT_VCARD_30);
			const char *uid = e_contact_get_const(contact, E_CONTACT_UID);
			int datasize = strlen(data) + 1;
			evo2_report_change(ctx, env->contact_format, data, datasize, uid, OSYNC_CHANGE_TYPE_ADDED);
		}
		e_book_query_unref(query);
	}
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	if (gerror)
		g_clear_error(&gerror);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void evo2_ebook_modify(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, data, info, ctx, change);
	OSyncEvoEnv *env = (OSyncEvoEnv *)data;
	
	const char *uid = osync_change_get_uid(change);
	EContact *contact = NULL;
	GError *gerror = NULL;
	OSyncError *error = NULL;
	OSyncData *odata = NULL;
	char *plain = NULL;
	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_DELETED:
			if (!e_book_remove_contact(env->addressbook, uid, &gerror)) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to delete contact: %s", gerror ? gerror->message : "None");
				goto error;
			}
			break;
		case OSYNC_CHANGE_TYPE_ADDED:
			odata = osync_change_get_data(change);
			osync_data_get_data(odata, &plain, NULL);
			contact = e_contact_new_from_vcard(plain);
			e_contact_set(contact, E_CONTACT_UID, NULL);
			if (e_book_add_contact(env->addressbook, contact, &gerror)) {
				uid = e_contact_get_const(contact, E_CONTACT_UID);
				osync_change_set_uid(change, uid);
			} else {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to add contact: %s", gerror ? gerror->message : "None");
				goto error;
			}
			break;
		case OSYNC_CHANGE_TYPE_MODIFIED:
			odata = osync_change_get_data(change);
			osync_data_get_data(odata, &plain, NULL);
			
			contact = e_contact_new_from_vcard(plain);
			e_contact_set(contact, E_CONTACT_UID, g_strdup(uid));
			
			osync_trace(TRACE_INTERNAL, "ABout to modify vcard:\n%s", e_vcard_to_string(&(contact->parent), EVC_FORMAT_VCARD_30));
			
			if (e_book_commit_contact(env->addressbook, contact, &gerror)) {
				uid = e_contact_get_const (contact, E_CONTACT_UID);
				if (uid)
					osync_change_set_uid(change, uid);
			} else {
				/* try to add */
				osync_trace(TRACE_INTERNAL, "unable to mod contact: %s", gerror ? gerror->message : "None");
				
				g_clear_error(&gerror);
				if (e_book_add_contact(env->addressbook, contact, &gerror)) {
					uid = e_contact_get_const(contact, E_CONTACT_UID);
					osync_change_set_uid(change, uid);
				} else {
					osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to modify contact: %s", gerror ? gerror->message : "None");
					goto error;
				}
			}
			break;
		default:
			printf("Error\n");
	}
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	if (gerror)
		g_clear_error(&gerror);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

osync_bool evo2_ebook_initialize(OSyncEvoEnv *env, OSyncPluginInfo *info, OSyncError **error)
{
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	env->contact_format = osync_format_env_find_objformat(formatenv, "vcard30");
	
	
	env->contact_sink = osync_objtype_sink_new("contact", error);
	if (!env->contact_sink)
		return FALSE;
	
	osync_objtype_sink_add_objformat(env->contact_sink, "vcard30");
	osync_objtype_sink_add_objformat(env->contact_sink, "vcard21");
	
	/* All sinks have the same functions of course */
	OSyncObjTypeSinkFunctions functions;
	memset(&functions, 0, sizeof(functions));
	functions.connect = evo2_ebook_connect;
	functions.disconnect = evo2_ebook_disconnect;
	functions.get_changes = evo2_ebook_get_changes;
	functions.commit = evo2_ebook_modify;
	functions.sync_done = evo2_ebook_sync_done;
	
	/* We pass the OSyncFileDir object to the sink, so we dont have to look it up
	 * again once the functions are called */
	osync_objtype_sink_set_functions(env->contact_sink, functions, NULL);
	osync_plugin_info_add_objtype(info, env->contact_sink);
}
