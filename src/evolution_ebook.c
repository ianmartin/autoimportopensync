#include "evolution_sync.h"

gboolean evo2_addrbook_open(evo_environment *env)
{
	ESourceList *sources;
	ESource *source;
	if (!env->adressbook_path)
		return FALSE;
	
  	if (!e_book_get_addressbooks(&sources, NULL)) {
		printf("Report error\n");
		return FALSE;
	}
	
	source = find_source(sources, env->adressbook_path);
	if (!source) {
		printf("Error2\n");
		return FALSE;
	}
	
	env->adressbook = e_book_new(source, NULL);
	
	if(!env->adressbook) {
		evo_debug(env, 1, "failed new open addressbook\n");
		return FALSE;
	}
	
	if (!e_book_open(env->adressbook, TRUE, NULL)) {
		evo_debug(env, 1, "Could not load addressbook\n");
		return FALSE;
		//FIXME Free
	}
	return TRUE;
}

gboolean evo2_addrbook_modify(evo_environment *env, OSyncChange *change)
{
	char *uid = osync_change_get_uid(change);
	EContact *contact;
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			if (e_book_remove_contact(env->adressbook, uid, NULL))
				return TRUE;
			break;
		case CHANGE_ADDED:
			contact = e_contact_new_from_vcard(osync_change_get_data(change));
			if (e_book_add_contact(env->adressbook, contact, NULL)) {
				uid = e_contact_get_const(contact, E_CONTACT_UID);
				osync_change_set_uid(change, uid);
				return TRUE;
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
				return TRUE;
			}
			break;
		default:
			printf("Error\n");
	}
	return FALSE;
}
