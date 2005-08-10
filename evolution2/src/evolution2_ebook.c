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

osync_bool evo2_addrbook_open(evo_environment *env, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "EVO2-SYNC: %s(%p)", __func__, env);
	GError *gerror = NULL;
	ESourceList *sources = NULL;
	ESource *source = NULL;

	if (!env->addressbook_path) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "no addressbook path set");
		osync_trace(TRACE_EXIT_ERROR, "EVO2-SYNC: %s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (strcmp(env->addressbook_path, "default")) {
		if (!e_book_get_addressbooks(&sources, NULL)) {
	  		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error getting addressbooks: %s", gerror ? gerror->message : "None");
			osync_trace(TRACE_EXIT_ERROR, "EVO2-SYNC: %s: %s", __func__, osync_error_print(error));
			g_clear_error(&gerror);
			return FALSE;
		}
		
		if (!(source = evo2_find_source(sources, env->addressbook_path))) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Error finding source \"%s\"", env->addressbook_path);
			osync_trace(TRACE_EXIT_ERROR, "EVO2-SYNC: %s: %s", __func__, osync_error_print(error));
			return FALSE;
		}
		
		if (!(env->addressbook = e_book_new(source, &gerror))) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to alloc new addressbook: %s", gerror ? gerror->message : "None");
			osync_trace(TRACE_EXIT_ERROR, "EVO2-SYNC: %s: %s", __func__, osync_error_print(error));
			g_clear_error(&gerror);
			return FALSE;
		}
	} else {
		if (!(env->addressbook = e_book_new_default_addressbook(&gerror))) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to alloc new default addressbook: %s", gerror ? gerror->message : "None");
			osync_trace(TRACE_EXIT_ERROR, "EVO2-SYNC: %s: %s", __func__, osync_error_print(error));
			g_clear_error(&gerror);
			return FALSE;
		}
	}
	
	if (!e_book_open(env->addressbook, TRUE, &gerror)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to alloc new addressbook: %s", gerror ? gerror->message : "None");
		g_clear_error(&gerror);
		g_object_unref(env->addressbook);
		env->addressbook = NULL;
		osync_trace(TRACE_EXIT_ERROR, "EVO2-SYNC: %s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	if (!osync_anchor_compare(env->member, "contact", env->addressbook_path))
		osync_member_set_slow_sync(env->member, "contact", TRUE);
	
	osync_trace(TRACE_EXIT, "EVO2-SYNC: %s", __func__);
	return TRUE;
}

void evo2_addrbook_get_changes(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
	GList *changes = NULL;
	EBookChange *ebc = NULL;
	EVCard vcard;
	GList *l = NULL;
	char *data = NULL;
	char *uid = NULL;
	int datasize = 0;
	
	if (osync_member_get_slow_sync(env->member, "contact") == FALSE) {
		osync_debug("EVO2-SYNC", 4, "No slow_sync for contact");
		if (!e_book_get_changes(env->addressbook, env->change_id, &changes, NULL)) {
			osync_context_send_log(ctx, "Unable to open changed contacts");
			return;
		} 
		
		for (l = changes; l; l = l->next) {
			ebc = (EBookChange *)l->data;
			uid = g_strdup(e_contact_get_const(ebc->contact, E_CONTACT_UID));
			e_contact_set(ebc->contact, E_CONTACT_UID, NULL);
			switch (ebc->change_type) {
				case E_BOOK_CHANGE_CARD_ADDED:
					vcard = ebc->contact->parent;
					data = e_vcard_to_string(&vcard, EVC_FORMAT_VCARD_30);
					datasize = strlen(data) + 1;
					evo2_report_change(ctx, "contact", "vcard30", data, datasize, uid, CHANGE_ADDED);
					break;
				case E_BOOK_CHANGE_CARD_MODIFIED:
					vcard = ebc->contact->parent;
					data = e_vcard_to_string(&vcard, EVC_FORMAT_VCARD_30);
					datasize = strlen(data) + 1;
					evo2_report_change(ctx, "contact", "vcard30", data, datasize, uid, CHANGE_MODIFIED);
					break;
				case E_BOOK_CHANGE_CARD_DELETED:
					evo2_report_change(ctx, "contact", "vcard30", NULL, 0, uid, CHANGE_DELETED);
					break;
			}
			g_free(uid);
		}
	} else {
		osync_debug("EVO2-SYNC", 4, "slow_sync for contact");
		EBookQuery *query = e_book_query_any_field_contains("");
		if (!e_book_get_contacts(env->addressbook, query, &changes, NULL)) {
			osync_context_send_log(ctx, "Unable to open contacts");
			printf("unable to get contacts\n");
			return;
		} 
		for (l = changes; l; l = l->next) {
			EContact *contact = E_CONTACT(l->data);
			vcard = contact->parent;
			char *data = e_vcard_to_string(&vcard, EVC_FORMAT_VCARD_30);
			const char *uid = e_contact_get_const(contact, E_CONTACT_UID);
			int datasize = strlen(data) + 1;
			evo2_report_change(ctx, "contact", "vcard30", data, datasize, uid, CHANGE_ADDED);
		}
		e_book_query_unref(query);
	}
	osync_debug("EVO2-SYNC", 4, "end: %s", __func__);
}

static osync_bool evo2_addrbook_modify(OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
	const char *uid = osync_change_get_uid(change);
	EContact *contact = NULL;
	GError *gerror = NULL;
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			if (!e_book_remove_contact(env->addressbook, uid, NULL)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to delete contact");
				osync_trace(TRACE_EXIT_ERROR, "%s: unable to delete contact", __func__);
				return FALSE;
			}
			break;
		case CHANGE_ADDED:
			contact = e_contact_new_from_vcard(osync_change_get_data(change));
			e_contact_set(contact, E_CONTACT_UID, NULL);
			if (e_book_add_contact(env->addressbook, contact, &gerror)) {
				uid = e_contact_get_const(contact, E_CONTACT_UID);
				osync_change_set_uid(change, uid);
			} else {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to add contact");
				osync_trace(TRACE_EXIT_ERROR, "%s: unable to add contact: %s", __func__, gerror ? gerror->message : "None");
				return FALSE;
			}
			break;
		case CHANGE_MODIFIED:
			
			contact = e_contact_new_from_vcard(osync_change_get_data(change));
			e_contact_set(contact, E_CONTACT_UID, g_strdup(uid));
			
			osync_trace(TRACE_INTERNAL, "ABout to modify vcard:\n%s", e_vcard_to_string(&(contact->parent), EVC_FORMAT_VCARD_30));
			
			if (e_book_commit_contact(env->addressbook, contact, &gerror)) {
				uid = e_contact_get_const (contact, E_CONTACT_UID);
				if (uid)
					osync_change_set_uid(change, uid);
			} else {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to modify contact");
				osync_trace(TRACE_EXIT_ERROR, "%s: unable to mod contact: %s", __func__, gerror ? gerror->message : "None");
				return FALSE;
			}
			break;
		default:
			printf("Error\n");
	}
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

void evo2_addrbook_setup(OSyncPluginInfo *info)
{
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard30", NULL);
	osync_plugin_set_commit_objformat(info, "contact", "vcard30", evo2_addrbook_modify);
	osync_plugin_set_access_objformat(info, "contact", "vcard30", evo2_addrbook_modify);
}
