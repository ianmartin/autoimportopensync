#include "evolution_sync.h"

osync_bool evo2_addrbook_open(evo_environment *env)
{
	ESourceList *sources;
	ESource *source;
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
	
	env->adressbook = e_book_new(source, NULL);
	
	if(!env->adressbook) {
		osync_debug("EVO2-SYNC", 1, "failed new open addressbook\n");
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
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			if (!e_book_remove_contact(env->adressbook, uid, NULL)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to delete contact");
				return FALSE;
			}
			break;
		case CHANGE_ADDED:
			contact = e_contact_new_from_vcard(osync_change_get_data(change));
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

static OSyncFormatFunctions vcard_functions = {
	.commit_change = evo2_addrbook_modify,
	.access = evo2_addrbook_modify,
};

void evo2_addrbook_setup(OSyncPluginInfo *info)
{
	osync_plugin_register_accepted_objtype(info, "contact");
	osync_plugin_register_accepted_objformat(info, "contact", "vcard", &vcard_functions);
}
