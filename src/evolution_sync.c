#include "evolution_sync.h"

static void *evo2_initialize(OSyncMember *member)
{
	char *configdata = NULL;
	int configsize = 0;
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = g_malloc0(sizeof(evo_environment));
	if (!env)
		goto error_ret;
	if (!osync_member_get_config(member, &configdata, &configsize))
		goto error_free;
	if (!evo2_parse_settings(env, configdata, configsize))
		goto error_free_data;
	env->member = member;
	OSyncGroup *group = osync_member_get_group(member);
	char *name = osync_group_get_name(group);
	printf("evo2 init %s\n", name);
	env->change_id = g_strdup_printf(name);
	
	g_free(configdata);
	return (void *)env;
	
	error_free_data:
		g_free(configdata);
	error_free:
		g_free(env);
	error_ret:
		return NULL;
}

static ESource *find_source(ESourceList *list, char *uri)
{
	GSList *g;
	for (g = e_source_list_peek_groups (list); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		GSList *s;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			ESource *source = E_SOURCE (s->data);
			if (!strcmp(e_source_get_uri(source), uri))
				return source;
		}
	}
	return NULL;
}

static void evo2_connect(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	ESource *source;
	osync_bool open_any = FALSE;
	
	if (osync_member_needs_objtype(env->member, "contact") &&  env->adressbook_path && strlen(env->adressbook_path)) {
		if (evo2_addrbook_open(env)) {
			open_any = TRUE;
			if (!osync_anchor_compare(env->member, "contact", env->adressbook_path))
				osync_member_request_slow_sync(env->member, "contact");
		} else {
			osync_context_send_warning(ctx, "Unable to open addressbook");
		}
	}
	
	if (osync_member_needs_objtype(env->member, "calendar") &&  env->calendar_path && strlen(env->calendar_path)) {
		if (evo2_calendar_open(env)) {
			open_any = TRUE;
			if (!osync_anchor_compare(env->member, "calendar", env->calendar_path))
				osync_member_request_slow_sync(env->member, "calendar");
		} else {
			osync_context_send_warning(ctx, "Unable to open calendar");
		}
	}
	
	if (osync_member_needs_objtype(env->member, "tasks") && env->tasks_path && strlen(env->tasks_path)) {
		if (evo2_tasks_open(env)) {
			open_any = TRUE;
			if (!osync_anchor_compare(env->member, "tasks", env->tasks_path))
				osync_member_request_slow_sync(env->member, "tasks");
		} else {
			osync_context_send_warning(ctx, "Unable to open tasks");
		}
	}
	
	srand(time(NULL));
	if (!open_any) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to open anything");
		return;
	}
	osync_context_report_success(ctx);
}

static OSyncChangeType evo2_get_data(void *object, char *objtype, char **data, int *datasize, const char **uid)
{
	char *buffer;
	EBookChange *ebc;
	ECalChange *ecc;
	if (!strcmp(objtype, "calendar") || !strcmp(objtype, "todo")) {
		ecc = (ECalChange *)object;
		e_cal_component_commit_sequence (ecc->comp);
		e_cal_component_strip_errors(ecc->comp);
		*data = e_cal_component_get_as_string (ecc->comp);
		*datasize = strlen(*data) + 1;
		e_cal_component_get_uid(ecc->comp, uid);
		switch (ecc->type) {
			case E_CAL_CHANGE_ADDED:
				return CHANGE_ADDED;
			case E_CAL_CHANGE_MODIFIED:
				return CHANGE_MODIFIED;
			case E_CAL_CHANGE_DELETED:
				return CHANGE_DELETED;
		}
	} else {
		ebc = (EBookChange *)object;
		EVCard vcard = ebc->contact->parent;
		*data = e_vcard_to_string(&vcard, EVC_FORMAT_VCARD_30);
		*uid = e_contact_get_const(ebc->contact, E_CONTACT_UID);
		*datasize = strlen(*data) + 1;
		switch (ebc->change_type) {
			case E_BOOK_CHANGE_CARD_ADDED:
				return CHANGE_ADDED;
			case E_BOOK_CHANGE_CARD_MODIFIED:
				return CHANGE_MODIFIED;
			case E_BOOK_CHANGE_CARD_DELETED:
				return CHANGE_DELETED;
		}
	}
}

static void evo2_report_changes(OSyncMember *member, GList *changes, OSyncContext *ctx, char *objtypestr, char *objformatstr)
{
	GList *l;
	for (l = changes; l; l = l->next) {
		const char *uid = NULL;
		char *data = NULL;
		int datasize = 0;
		OSyncChangeType type = evo_get_data(l->data, objtypestr, &data, &datasize, &uid);
		
		OSyncFormatEnv *env = osync_member_get_format_env(member);
		OSyncObjType *objtype = osync_conv_find_objtype(env, objtypestr);
		OSyncFormatType *objformat = osync_conv_find_objformat(env, objformatstr);
		
		OSyncChange *change = osync_change_new();
		osync_change_set_uid(change, uid);
		osync_change_set_objtype(change, objtype);
		osync_change_set_objformat(change, objformat);
		osync_change_set_changetype(change, type);
		osync_change_set_data(change, data, datasize, TRUE);
		osync_context_report_change(ctx, change);
	}
}

static void evo2_get_changeinfo(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
	GList *changes;

	if (env->addressbook) {
		if (osync_member_get_slow_sync(env->member, "contact")) {
			if (!e_book_get_changes(env->adressbook, env->change_id, &changes, NULL)) {
				osync_context_send_warning(ctx, "Unable to open changed contacts");
			}
		} else {
			EBookQuery *query = e_book_query_from_string("*"); //FIXME
			if (!e_book_get_contacts(env->adressbook, env->change_id, &changes, NULL)) {
				osync_context_send_warning(ctx, "Unable to open contacts");
			}
		}	
		evo2_report_changes(changes, ctx, "contact", "vcard");
	}
	
	if (env->calendar) {
		if (osync_member_get_slow_sync(env->member, "calendar")) {
			if (!e_cal_get_changes(env->calendar, env->change_id, &changes, NULL)) {
				osync_context_send_warning(ctx, "Unable to open changed calendar entries");
			}
		} else {
			/* FIXME HOW?
			
			EBookQuery *query = e_book_query_from_string("*"); //FIXME
			if (!e_book_get_contacts(env->adressbook, env->change_id, &changes, NULL)) {
				osync_context_send_warning(ctx, "Unable to open contacts");
			}
			*/
		}	
		evo2_report_changes(changes, ctx, "calendar", "vcalendar");
	}
	
	if (env->tasks) {
		if (osync_member_get_slow_sync(env->member, "todo")) {
			if (!e_cal_get_changes(env->tasks, env->change_id, &changes, NULL)) {
				osync_context_send_warning(ctx, "Unable to open changed tasks");
			}
		} else {
			/* FIXME HOW?
			
			EBookQuery *query = e_book_query_from_string("*"); //FIXME
			if (!e_book_get_contacts(env->adressbook, env->change_id, &changes, NULL)) {
				osync_context_send_warning(ctx, "Unable to open contacts");
			}
			*/
		}	
		evo2_report_changes(changes, ctx, "todo", "vtodo");
	}
	
	osync_context_report_success(ctx);
}
 
static void evo2_commit_change(OSyncContext *ctx, OSyncChange *change)
{
	GError *error = NULL;
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
	if (!strcmp("contact", osync_change_get_format(change))) {
		if (!evo2_addrbook_modify(env, change))
			osync_call_set_error(call, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to write");
	} else if {
			if (!evo2_calendar_modify(env, change))
				osync_call_set_error(call, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to write");
			break;
		case OSYNC_VTODO:
			if (!evo2_tasks_modify(env, change))
				osync_call_set_error(call, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to write");
			break;
		default:
			printf("Error2\n");
	}

	osync_call_answer(call);
}

static void evo2_sync_done(OSyncContext *ctx)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);

	GList *changes;
	
	if (env->adressbook)
		e_book_get_changes(env->adressbook, env->change_id, &changes, NULL);
	if (env->calendar)
		e_cal_get_changes(env->calendar, env->change_id, &changes, NULL);
	if (env->tasks)
		e_cal_get_changes(env->tasks, env->change_id, &changes, NULL);
	
	osync_context_report_success(ctx);
}

static void evo2_disconnect(OSyncContext *ctx)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);

	//FIXME!!
	
	osync_context_report_success(ctx);
}

static void evo2_finalize(void *data)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
	g_free(env->change_id);
	g_free(env);
}

void get_info(OSyncPluginInfo *info) {
	info->name = "evo2-sync";
	info->version = 1;
	info->is_threadsafe = FALSE;
	
	info->functions.initialize = evo2_initialize;
	info->functions.connect = evo2_connect;
	info->functions.sync_done = evo2_sync_done;
	info->functions.disconnect = evo2_disconnect;
	info->functions.finalize = evo2_finalize;

	OSyncObjType *contact = osync_conv_register_objtype(info->accepted_objtypes, "contact");
	OSyncObjFormat *vcard = osync_conv_register_objformat(contact, "vcard");
	vcard->functions.get_changeinfo = evo2_ebook_read;
	vcard->functions.get_data = NULL;
	vcard->functions.commit_change = evo2_ebook_commit;
	vcard->functions.access = evo2_ebook_commit;
	
	OSyncObjType *calendar = osync_conv_register_objtype(info->accepted_objtypes, "calendar");
	OSyncObjFormat *vcal = osync_conv_register_objformat(calendar, "vcalendar");
	vcal->functions.get_changeinfo = evo2_cal_read;
	vcal->functions.get_data = NULL;
	vcal->functions.commit_change = evo2_cal_commit;
	vcal->functions.access = evo2_cal_commit;
	
	OSyncObjType *todo = osync_conv_register_objtype(info->accepted_objtypes, "todo");
	OSyncObjFormat *vtodo = osync_conv_register_objformat(todo, "vtodo");
	vtodo->functions.get_changeinfo = evo2_tasks_read;
	vtodo->functions.get_data = NULL;
	vtodo->functions.commit_change = evo2_tasks_commit;
	vtodo->functions.access = evo2_tasks_commit;
}
