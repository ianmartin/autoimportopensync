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

static osync_bool evo2_addrbook_modify(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
	char *uid = osync_change_get_uid(change);
	EContact *contact;
	
	printf("Trying to commit changtype %i\n", osync_change_get_changetype(change));
	printf("VCARD is:\n%s\n", osync_change_get_data(change));
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			if (!e_book_remove_contact(env->adressbook, uid, NULL)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to delete contact");
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
}
