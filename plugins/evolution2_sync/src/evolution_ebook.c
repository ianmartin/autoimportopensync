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
 
#include "evolution_sync.h"

osync_bool evo2_addrbook_open(evo_environment *env)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	GError *error = NULL;
	ESourceList *sources = NULL;
	ESource *source = NULL;

	if (!env->adressbook_path) {
		osync_debug("EVO2-SYNC", 0, "no addressbook path error");
		return FALSE;
	}

	printf("about to read adressbooks\n");
  	if (!e_book_get_addressbooks(&sources, &error)) {
  		if (error) {
  			osync_debug("EVO2-SYNC", 0, "Report error %s", error->message);
  		}
		osync_debug("EVO2-SYNC", 0, "Report error");
		return FALSE;
	}
	printf("read adressbooks\n");
	source = evo2_find_source(sources, env->adressbook_path);
	if (!source) {
		osync_debug("EVO2-SYNC", 0, "Error2\n");
		return FALSE;
	}
	printf("found adressbooks\n");
	env->adressbook = e_book_new(source, &error);
	printf("made new adressbooks\n");
	if(!env->adressbook) {
		osync_debug("EVO2-SYNC", 1, "failed new open addressboo %sk\n", error->message);
		return FALSE;
	}
	printf("new adressbooks\n");
	if (!e_book_open(env->adressbook, TRUE, NULL)) {
		osync_debug("EVO2-SYNC", 1, "Could not load addressbook\n");
		return FALSE;
		//FIXME Free
	}
	
	osync_debug("EVO2-SYNC", 4, "end: %s", __func__);
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
		if (!e_book_get_changes(env->adressbook, env->change_id, &changes, NULL)) {
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
		if (!e_book_get_contacts(env->adressbook, query, &changes, NULL)) {
			osync_context_send_log(ctx, "Unable to open contacts");
			printf("unable to get contacts\n");
			return;
		} 
		for (l = changes; l; l = l->next) {
			EContact *contact = E_CONTACT(l->data);
			vcard = contact->parent;
			char *data = e_vcard_to_string(&vcard, EVC_FORMAT_VCARD_30);
			char *uid = e_contact_get_const(contact, E_CONTACT_UID);
			int datasize = strlen(data) + 1;
			evo2_report_change(ctx, "contact", "vcard30", data, datasize, uid, CHANGE_ADDED);
		}
		e_book_query_unref(query);
	}
	osync_debug("EVO2-SYNC", 4, "end: %s", __func__);
}

static osync_bool evo2_addrbook_modify(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
	char *uid = osync_change_get_uid(change);
	EContact *contact;
	
	printf("Trying to commit %s with changtype %i\n", osync_change_get_uid(change), osync_change_get_changetype(change));
	printf("VCARD is:\n%s\n", osync_change_get_data(change));
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			printf("trying to delete contact\n");
			if (!e_book_remove_contact(env->adressbook, uid, NULL)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to delete contact");
				printf("unable to delete contact\n");
				return FALSE;
			}
			break;
		case CHANGE_ADDED:
			contact = e_contact_new_from_vcard(osync_change_get_data(change));
			printf("trying to add contact %p\n", contact);
			if (e_book_add_contact(env->adressbook, contact, NULL)) {
				uid = e_contact_get_const(contact, E_CONTACT_UID);
				osync_change_set_uid(change, uid);
			} else {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to add contact");
				printf("unable to add contact\n");
				return FALSE;
			}
			break;
		case CHANGE_MODIFIED:
			
			contact = e_contact_new_from_vcard(osync_change_get_data(change));
			e_contact_set(contact, E_CONTACT_UID, g_strdup(osync_change_get_uid(change)));
			printf("trying to mod contact %p\n", contact);
			e_contact_set(contact, E_CONTACT_UID, uid);
			if (e_book_commit_contact(env->adressbook, contact, NULL)) {
				uid = e_contact_get_const (contact, E_CONTACT_UID);
				printf("new uid after modding %s\n", uid);
				if (uid)
					osync_change_set_uid(change, uid);
			} else {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to modify contact");
				printf("unable to mod contact\n");
				return FALSE;
			}
			break;
		default:
			printf("Error\n");
	}
	osync_context_report_success(ctx);
	osync_debug("EVO2-SYNC", 4, "end: %s", __func__);
	return TRUE;
}

void evo2_addrbook_setup(OSyncPluginInfo *info)
{
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard30");
	osync_plugin_set_commit_objformat(info, "contact", "vcard30", evo2_addrbook_modify);
	osync_plugin_set_access_objformat(info, "contact", "vcard30", evo2_addrbook_modify);
}
