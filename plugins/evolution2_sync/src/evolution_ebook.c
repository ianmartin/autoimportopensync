#include "evolution_sync.h"

osync_bool evo2_addrbook_open(evo_environment *env)
{
	GError *error = NULL;
	ESourceList *sources = NULL;
	ESource *source = NULL;

	if (!env->adressbook_path)
		return FALSE;
	
  	if (!e_book_get_addressbooks(&sources, NULL)) {
		osync_debug("EVO2-SYNC", 0, "Report error\n");
		return FALSE;
	}
	
	source = evo2_find_source(sources, env->adressbook_path);
	if (!source) {
		osync_debug("EVO2-SYNC", 0, "Error2\n");
		return FALSE;
	}
	
	env->adressbook = e_book_new(source, &error);
	
	if(!env->adressbook) {
		osync_debug("EVO2-SYNC", 1, "failed new open addressboo %sk\n", error->message);
		return FALSE;
	}
	
	if (!e_book_open(env->adressbook, TRUE, NULL)) {
		osync_debug("EVO2-SYNC", 1, "Could not load addressbook\n");
		return FALSE;
		//FIXME Free
	}
	
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
	
	if (osync_member_get_slow_sync(env->member, "contact") == FALSE) {
		osync_debug("EVO2-SYNC", 4, "No slow_sync for contact");
		if (!e_book_get_changes(env->adressbook, env->change_id, &changes, NULL)) {
			osync_context_send_log(ctx, "Unable to open changed contacts");
			return;
		} 
		
		for (l = changes; l; l = l->next) {
			ebc = (EBookChange *)l->data;
			vcard = ebc->contact->parent;
			char *data = e_vcard_to_string(&vcard, EVC_FORMAT_VCARD_30);
			char *uid = e_contact_get_const(ebc->contact, E_CONTACT_UID);
			int datasize = strlen(data) + 1;
			switch (ebc->change_type) {
				case E_BOOK_CHANGE_CARD_ADDED:
					evo2_report_change(ctx, "contact", "vcard", data, datasize, uid, CHANGE_ADDED);
				case E_BOOK_CHANGE_CARD_MODIFIED:
					evo2_report_change(ctx, "contact", "vcard", data, datasize, uid, CHANGE_MODIFIED);
				case E_BOOK_CHANGE_CARD_DELETED:
					evo2_report_change(ctx, "contact", "vcard", data, datasize, uid, CHANGE_DELETED);
			}
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
			evo2_report_change(ctx, "contact", "vcard", data, datasize, uid, CHANGE_ADDED);
		}
		e_book_query_unref(query);
	}
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
	return TRUE;
}

void evo2_addrbook_setup(OSyncPluginInfo *info)
{
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard");
	osync_plugin_set_commit_objformat(info, "contact", "vcard", evo2_addrbook_modify);
	osync_plugin_set_access_objformat(info, "contact", "vcard", evo2_addrbook_modify);
}
