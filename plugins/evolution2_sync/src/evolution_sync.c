#include "evolution_sync.h"

void evo_debug(evo_environment *env, int level, char *message, ...)
{
	va_list arglist;
	char *buffer;
	int debuglevel = env->debuglevel;
	debuglevel = 20;
	if (level > debuglevel) { return; }
	va_start(arglist, message);
	g_vasprintf(&buffer, message, arglist);

	switch (level) {
		case 0:
			//Error
			printf("[evo2-sync] ERROR: %s\n", buffer);
			break;
		case 1:
			//Warning
			printf("[evo2-sync] WARNING: %s\n", buffer);
			break;
		case 2:
			//Information
			printf("[evo2-sync] INFORMATION: %s\n",  buffer);
			break;
		case 3:
			//debug
			printf("[evo2-sync] DEBUG: %s\n", buffer);
			break;
		case 4:
			//fulldebug
			printf("[evo2-sync] FULL DEBUG: %s\n", buffer);
			break;
	}
	va_end(arglist);
}

char *get_config(char *path)
{
	char *filename = NULL;
	
	filename = g_strdup_printf ("%s/evo2-sync.conf", path);
	struct stat filestats;
	stat(filename, &filestats);
	char *buffer = g_malloc0(sizeof(char *) * filestats.st_size);
	FILE *file = fopen(filename, "r");
	int offset = 0;
	for (offset = 0; offset += fread(buffer + offset,1024, 1, file););
	g_strstrip(buffer);
	fclose(file);
	g_free(filename);
	return g_strdup(buffer);
}

void store_config(char *data, char *path)
{
	char *filename = NULL;
	filename = g_strdup_printf ("%s/evo2-sync.conf", path);
	FILE *file = fopen(filename, "w");
	fputs(data, file);
	fclose(file);
	g_free(filename);
}

evo_environment *initialize(OSyncMember *member, char *path)
{
	evo_environment *env = g_malloc0(sizeof(evo_environment));
	char *filename = NULL;
	filename = g_strdup_printf ("%s/evo2-sync.conf", path);
	load_evo_settings(env, filename);
	env->member = member;
	env->change_id = g_strdup_printf ("test-id3");
	g_free(filename);
	return env;
}

ESource *find_source(ESourceList *list, char *uri)
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

void connect(OSyncContext *ctx)
{
	
	evo_debug(env, 3, "start: sync_connect");
	ESource *source;
	
	if (osync_member_needs_objtype(env->member, "contact")) {
		if (!evo2_addrbook_open(env)) {
			osync_call_set_error(call, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to open directory");
			osync_call_answer(call);
			return;
		}
	}
	
	if (formats & OSYNC_VCALENDAR) {
		if (!evo2_calendar_open(env)) {
			osync_call_set_error(call, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to open directory");
			osync_call_answer(call);
			return;
		}
	}
	
	if (formats & OSYNC_VTODO) {
		if (!evo2_tasks_open(env)) {
			osync_call_set_error(call, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to open directory");
			osync_call_answer(call);
			return;
		}
	}
	
	osync_call_set_success(call);
	srand(time(NULL));
	evo_debug(env, 3, "end: sync_connect");
	osync_call_answer(call);
	return;
}

OSyncChangeType evo_get_data(void *object, OSyncChangeFormat format, char **data, int *datasize, const char **uid)
{
	char *buffer;
	EBookChange *ebc;
	ECalChange *ecc;
	switch (format) {
		case OSYNC_VCALENDAR:
		case OSYNC_VTODO:
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
		case OSYNC_VCARD:
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
		default:
			return CHANGE_UNKNOWN;
	}
}



void evo_report_changes(GList *changes, OSyncCall *call, OSyncChangeFormat format)
{
	GList *l;
	for (l = changes; l != NULL; l = l->next) {
		const char *uid = NULL;
		char *data = NULL;
		int datasize = 0;
		OSyncChangeType type = evo_get_data(l->data, format, &data, &datasize, &uid);
		if (type == CHANGE_UNKNOWN)
			continue;
		OSyncChange *change = osplg_change_new();
		osync_change_set_uid(change, uid);
		osync_change_set_format(change, format);
		printf("Setting changetype to %i\n", type);
		osync_change_set_changetype(change, type);
		osync_change_set_data(change, data, datasize);
		osync_context_report_change(call, change);
	}
}

void get_changeinfo(evo_environment *env, OSyncCall *call, OSyncFormats formats)
{
	GList *changes;

	if (formats & OSYNC_VCARD) {
		if (!e_book_get_changes(env->adressbook, env->change_id, &changes, NULL)) {
			osync_call_set_error(call, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to open directory");
			osync_call_answer(call);
			return;
		}
		evo_report_changes(changes, call, OSYNC_VCARD);
	}
	
	if (formats & OSYNC_VCALENDAR) {
		if (!e_cal_get_changes(env->calendar, env->change_id, &changes, NULL)) {
			osync_call_set_error(call, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to open directory");
			osync_call_answer(call);
			return;
		}
		evo_report_changes(changes, call, OSYNC_VCALENDAR);
	}
	
	if (formats & OSYNC_VTODO) {
		if (!e_cal_get_changes(env->tasks, env->change_id, &changes, NULL)) {
			osync_call_set_error(call, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to open directory");
			osync_call_answer(call);
			return;
		}
		evo_report_changes(changes, call, OSYNC_VTODO);
	}
	
	evo_debug(env, 2, "Done searching for changes");
	osync_call_set_success(call);
	osync_call_answer(call);
}
 
void add_change(evo_environment *env, OSyncChange *change, OSyncCall *call)
{
	GError *error = NULL;
	evo_debug(env, 2, "start: syncobj_modify");
	
	osync_call_set_success(call);
	switch (osync_change_get_format(change)) {
		case OSYNC_VCARD:
			if (!evo2_addrbook_modify(env, change))
				osync_call_set_error(call, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to write");
			break;
		case OSYNC_VCALENDAR:
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

void disconnect(evo_environment *env, OSyncCall *call)
{
	GList *changes;
	
	if (env->adressbook)
		e_book_get_changes(env->adressbook, env->change_id, &changes, NULL);
	if (env->calendar)
		e_cal_get_changes(env->calendar, env->change_id, &changes, NULL);
	if (env->tasks)
		e_cal_get_changes(env->tasks, env->change_id, &changes, NULL);
	//e_cal_free_change_list (changed);
	evo_debug(env, 2, "start: disconnect");
	osync_call_set_success(call);
	osync_call_answer(call);
}

int type() {
	return 5;
}

char *name() {
	return "evo2-sync";
}
