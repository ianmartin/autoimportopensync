#include "evolution_sync.h"

static void evo_addr_removed_cb(EBookView *book_view, GList *ids, gpointer data)
{
	printf("removed cn\n");
}

static void evo_addr_added_cb(EBookView *book_view, GList *cards, gpointer data)
{
	printf("added cn\n");
}

static void evo_addr_changed_cb(EBookView *book_view, GList *cards, gpointer data)
{
	printf("changed cn\n");
}

static void evo_addr_seqcompl_cb(EBookView *book_view, EBookViewStatus status, gpointer data)
{
	printf("seqcompl cn\n");
}

static void evo_addressbook_view_cb(EBook *book, EBookStatus status, EBookView *book_view,  gpointer data)
{
	OSyncContext *ctx = data;
	
	if (status == E_BOOK_STATUS_SUCCESS) {
		/*if (conn->addr_mode == EVO_ADDR_MODE_GETVIEW)
			conn->ebookview = book_view;*/
		g_object_ref (G_OBJECT (book_view));
		g_signal_connect (G_OBJECT (book_view), "card_changed", G_CALLBACK (evo_addr_changed_cb), ctx);
		g_signal_connect (G_OBJECT (book_view), "card_added", G_CALLBACK (evo_addr_added_cb), ctx);
		g_signal_connect (G_OBJECT (book_view), "card_removed", G_CALLBACK (evo_addr_removed_cb), ctx);
		g_signal_connect (G_OBJECT (book_view), "sequence_complete", G_CALLBACK (evo_addr_seqcompl_cb), ctx);
	}
}


void evo_addressbook_opened_cb(EBook *book, EBookStatus status, gpointer data)
{
	printf("evo_addressbook_opened_cb\n");
	OSyncContext *ctx = data;
  
	if (status == E_BOOK_STATUS_SUCCESS) {
		//env->dbs_waiting--;
		e_book_get_book_view(book, "(contains \"full_name\" \"\")", evo_addressbook_view_cb, ctx);
	} else {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to addressbook");
		g_object_unref(G_OBJECT(book));
		evo_continue(ctx);
	}
	
	/*if (env->dbs_waiting == 0)
		osync_context_report_success(ctx);*/
}

/*static void
book_open_cb (EBook *book, EBookStatus status, gpointer closure)
{
        EAddrConduitContext *ctxt = (EAddrConduitContext*)closure;

        if (status == E_BOOK_STATUS_SUCCESS) {
                e_book_get_cursor (book, "(contains \"full_name\" \"\")", cursor_cb, ctxt);
        } else {
                WARN (_("EBook not loaded\n"));
                gtk_main_quit();
        }
}*/

void *evo_addrbook_open(OSyncContext *ctx)
{
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	osync_debug("EVO-SYNC", 0, "Addressbook: Loading %s", env->addressbook_path);
	
	sleep(3);
	
	env->addressbook = e_book_new();
	if (!env->addressbook) {
		osync_debug("EVO-SYNC", 0, "Evolution plugin: Could not connect to Evolution!");
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Evolution plugin: Could not connect to Evolution!");
		return NULL;
	}

	osync_debug("EVO-SYNC", 0, "Addressbook loading `%s'...\n", env->addressbook_path);
	e_book_load_uri(env->addressbook, env->addressbook_path, evo_addressbook_opened_cb, ctx);
	printf("done loading addressbook\n");
	return NULL;
}

void evo_addrbook_get_changes(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 0, "start: %s", __func__);
	/*evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
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
					evo2_report_change(ctx, "contact", "vcard", data, datasize, uid, CHANGE_ADDED);
					break;
				case E_BOOK_CHANGE_CARD_MODIFIED:
					vcard = ebc->contact->parent;
					data = e_vcard_to_string(&vcard, EVC_FORMAT_VCARD_30);
					datasize = strlen(data) + 1;
					evo2_report_change(ctx, "contact", "vcard", data, datasize, uid, CHANGE_MODIFIED);
					break;
				case E_BOOK_CHANGE_CARD_DELETED:
					evo2_report_change(ctx, "contact", "vcard", NULL, 0, uid, CHANGE_DELETED);
					break;
			}
			g_free(uid);
		}
	} else {

	}*/
	osync_debug("EVO2-SYNC", 0, "end: %s", __func__);
}

osync_bool evo_addrbook_get_all(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 0, "slow_sync for contact");
	/*EBookQuery *query = e_book_query_any_field_contains("");
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
	e_book_query_unref(query);*/
	return FALSE;
}

static osync_bool evo_addrbook_modify(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("EVO2-SYNC", 0, "start: %s", __func__);
	/*evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
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
	osync_context_report_success(ctx);*/
	osync_debug("EVO2-SYNC", 0, "end: %s", __func__);
	return TRUE;
}

void evo_addrbook_setup(OSyncPluginInfo *info)
{
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard21");
	osync_plugin_set_commit_objformat(info, "contact", "vcard21", evo_addrbook_modify);
	//osync_plugin_set_access_objformat(info, "contact", "vcard", evo_addrbook_modify);
}
