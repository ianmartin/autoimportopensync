#include "support.h"

char *olddir = NULL;

static void reset_env(void)
{
	unsetenv("CONNECT_ERROR");
	unsetenv("CONNECT_TIMEOUT");
	unsetenv("INIT_NULL");
	unsetenv("GET_CHANGES_ERROR");
	unsetenv("GET_CHANGES_TIMEOUT");
	unsetenv("GET_CHANGES_TIMEOUT2");
	unsetenv("COMMIT_ERROR");
	unsetenv("COMMIT_TIMEOUT");
	unsetenv("SYNC_DONE_ERROR");
	unsetenv("SYNC_DONE_TIMEOUT");
	unsetenv("DISCONNECT_ERROR");
	unsetenv("DISCONNECT_TIMEOUT");
}

char *setup_testbed(char *fkt_name)
{
	setuid(65534);
	char *testbed = g_strdup_printf("%s/testbed.XXXXXX", g_get_tmp_dir());
	mkdtemp(testbed);
	
	char *command = NULL;
	if (fkt_name) {
		command = g_strdup_printf("cp -R %s%sdata/%s/* %s", g_getenv("srcdir") ? g_getenv("srcdir") : "", g_getenv("srcdir") ? "/" : "", fkt_name, testbed);
		if (system(command))
			abort();
		g_free(command);
	}
	
	command = g_strdup_printf("cp -R %s%smock-plugin/.libs/*.so %s", g_getenv("srcdir") ? g_getenv("srcdir") : "", g_getenv("srcdir") ? "/" : "", testbed);
	if (system(command))
		abort();
	g_free(command);
	
	command = g_strdup_printf("cp -R %s%s../formats/.libs/*.so %s", g_getenv("srcdir") ? g_getenv("srcdir") : "", g_getenv("srcdir") ? "/" : "", testbed);
	if (system(command))
		abort();
	g_free(command);
	
	command = g_strdup_printf("cp -R %s%s../formats/vformats-xml/.libs/*.so %s", g_getenv("srcdir") ? g_getenv("srcdir") : "", g_getenv("srcdir") ? "/" : "", testbed);
	if (system(command))
		abort();
	g_free(command);
	
	olddir = g_get_current_dir();
	if (chdir(testbed))
		abort();
	
	osync_debug("TEST", 4, "Seting up %s at %s", fkt_name, testbed);
	printf(".");
	fflush(NULL);
	reset_env();
	return testbed;
}

void destroy_testbed(char *path)
{
	char *command = g_strdup_printf("rm -rf %s", path);
	if (olddir)
		chdir(olddir);
	system(command);
	g_free(command);
	osync_debug("TEST", 4, "Tearing down %s\n", path);
	g_free(path);
}

void conflict_handler_choose_first(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	fail_unless(osengine_mapping_num_changes(mapping) == (int)(user_data), NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);
	
	OSyncChange *change = osengine_mapping_nth_change(mapping, 0);
	osengine_mapping_solve(engine, mapping, change);
}

void conflict_handler_choose_modified(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	fail_unless(osengine_mapping_num_changes(mapping) == (int)(user_data), NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);
	
	int i;
	for (i = 0; i < osengine_mapping_num_changes(mapping); i++) {
		OSyncChange *change = osengine_mapping_nth_change(mapping, i);
		if (change->changetype == CHANGE_MODIFIED) {
			osengine_mapping_solve(engine, mapping, change);
			return;
		}
	}
	fail();
}

void conflict_handler_choose_deleted(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	fail_unless(osengine_mapping_num_changes(mapping) == (int)(user_data), NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);

	int i;
	for (i = 0; i < osengine_mapping_num_changes(mapping); i++) {
		OSyncChange *change = osengine_mapping_nth_change(mapping, i);
		if (change->changetype == CHANGE_DELETED) {
			osengine_mapping_solve(engine, mapping, change);
			return;
		}
	}
	fail(NULL);
}

void conflict_handler_duplication(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	fail_unless(osengine_mapping_num_changes(mapping) == (int)(user_data), NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);
	
	osengine_mapping_duplicate(engine, mapping);
}

void conflict_handler_ignore(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	if (user_data)
		fail_unless(osengine_mapping_num_changes(mapping) == (int)(user_data), NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);
	
	osengine_mapping_ignore_conflict(engine, mapping);
}


void conflict_handler_random(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	fail_unless(osengine_mapping_num_changes(mapping) == (int)(user_data), NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);

	int num = osengine_mapping_num_changes(mapping);
	int choosen = g_random_int_range(0, num);
	OSyncChange *change = osengine_mapping_nth_change(mapping, choosen);
	osengine_mapping_solve(engine, mapping, change);
}

static void solve_conflict(OSyncMapping *mapping)
{
	sleep(5);
	
	OSyncEngine *engine = mapping->table->engine;
	
	int i;
	for (i = 0; i < osengine_mapping_num_changes(mapping); i++) {
		OSyncChange *change = osengine_mapping_nth_change(mapping, i);
		if (change->changetype == CHANGE_MODIFIED) {
			osengine_mapping_solve(engine, mapping, change);
			return;
		}
	}
}

void conflict_handler_delay(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	fail_unless(osengine_mapping_num_changes(mapping) == (int)(user_data), NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);
	
	g_thread_create ((GThreadFunc)solve_conflict, mapping, TRUE, NULL);
}

void entry_status(OSyncEngine *engine, OSyncChangeUpdate *status, void *user_data)
{
	switch (status->type) {
		case CHANGE_RECEIVED:
			num_read++;
			break;
		case CHANGE_RECEIVED_INFO:
			num_read_info++;
			break;
		case CHANGE_SENT:
			num_written++;
			break;
		case CHANGE_WRITE_ERROR:
			fail_unless(osync_error_is_set(&(status->error)), NULL);
			osync_debug("TEST", 4, "CHANGE_WRITE_ERROR: %s", status->error->message);
			num_written_errors++;
			break;
		default:
			printf("Unknown status\n");
	}
}

void member_status(OSyncMemberUpdate *status, void *user_data)
{
	mark_point();
	switch (status->type) {
		case MEMBER_CONNECTED:
			num_connected++;
			break;
		case MEMBER_DISCONNECTED:
			num_disconnected++;
			break;
		case MEMBER_SENT_CHANGES:
			num_member_sent_changes++;
			break;
		case MEMBER_CONNECT_ERROR:
			fail_unless(osync_error_is_set(&(status->error)), NULL);
			osync_debug("TEST", 4, "MEMBER_CONNECT_ERROR: %s", status->error->message);
			num_member_connect_errors++;
			break;
		case MEMBER_GET_CHANGES_ERROR:
			fail_unless(osync_error_is_set(&(status->error)), NULL);
			osync_debug("TEST", 4, "MEMBER_CONNECT_ERROR: %s", status->error->message);
			num_member_get_changes_errors++;
			break;
		case MEMBER_SYNC_DONE_ERROR:
			fail_unless(osync_error_is_set(&(status->error)), NULL);
			osync_debug("TEST", 4, "MEMBER_SYNC_DONE_ERROR: %s", status->error->message);
			num_member_sync_done_errors++;
			break;
		case MEMBER_DISCONNECT_ERROR:
			fail_unless(osync_error_is_set(&(status->error)), NULL);
			osync_debug("TEST", 4, "MEMBER_DISCONNECT_ERROR: %s", status->error->message);
			num_member_disconnect_errors++;
			break;
		default:
			printf("Unknown status\n");
	}
}

void engine_status(OSyncEngine *engine, OSyncEngineUpdate *status, void *user_data)
{
	switch (status->type) {
		case ENG_ENDPHASE_CON:
			osync_debug("TEST", 4, "All clients connected or error");
			break;
		case ENG_ENDPHASE_READ:
			osync_debug("TEST", 4, "All clients sent changes or error");
			break;
		case ENG_ENDPHASE_WRITE:
			osync_debug("TEST", 4, "All clients have writen");
			break;
		case ENG_ENDPHASE_DISCON:
			osync_debug("TEST", 4, "All clients have disconnected");
			break;
		case ENG_ERROR:
			fail_unless(osync_error_is_set(&(status->error)), NULL);
			osync_debug("TEST", 4, "ENG_ERROR: %s", status->error->message);
			num_engine_errors++;
			break;
		case ENG_SYNC_SUCCESSFULL:
			osync_debug("TEST", 4, "Sync Successfull");
			num_engine_successfull++;
			break;
		case ENG_PREV_UNCLEAN:
			osync_debug("TEST", 4, "Previous sync was unclean");
			num_engine_prev_unclean++;
			break;
		case ENG_END_CONFLICTS:
			osync_debug("TEST", 4, "End conflicts");
			num_engine_end_conflicts++;
			break;
		default:
			printf("ERROR: Unknown status type: %d\n", status->type);
	}
}

void mapping_status(OSyncMappingUpdate *status, void *user_data)
{
	switch (status->type) {
		case MAPPING_SOLVED:
			osync_debug("TEST", 4, "Mapping solved");
			break;
		case MAPPING_SYNCED:
			osync_debug("TEST", 4, "Mapping Synced");
			break;
		case MAPPING_WRITE_ERROR:
			fail_unless(osync_error_is_set(&(status->error)), NULL);
			osync_debug("TEST", 4, "MAPPING_WRITE_ERROR: %s", status->error->message);
			num_mapping_errors++;
			break;
		default:
			printf("errro\n");
	}
}

osync_bool synchronize_once(OSyncEngine *engine, OSyncError **error)
{
	num_connected = 0;
	num_disconnected = 0;
	num_conflicts = 0;
	num_written = 0;
	num_read = 0;
	num_read_info = 0;
	num_member_connect_errors = 0;
	num_member_sent_changes = 0;
	num_engine_errors = 0;
	num_engine_successfull = 0;
	num_member_get_changes_errors = 0;
	num_written_errors = 0;
	num_mapping_errors = 0;
	num_member_sync_done_errors = 0;
	num_member_disconnect_errors = 0;
	num_engine_prev_unclean = 0;
	num_engine_end_conflicts = 0;
	mark_point();
	return osengine_sync_and_block(engine, error);
}

void create_case(Suite *s, const char *name, void (*function)(void))
{
	TCase *tc_new = tcase_create(name);
	tcase_set_timeout(tc_new, 0);
	suite_add_tcase (s, tc_new);
	tcase_add_test(tc_new, function);
}

OSyncMappingTable *mappingtable_load(OSyncGroup *group, int num_mappings, int num_unmapped)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %i)", __func__, group, num_mappings, num_unmapped);
	mark_point();
	OSyncEnv *osync = init_env();
	OSyncGroup *newgroup = osync_group_load(osync, "configs/group", NULL);
	OSyncMappingTable *maptable = _osengine_mappingtable_load_group(newgroup);
	mark_point();
	fail_unless(g_list_length(maptable->mappings) == num_mappings, NULL);
	fail_unless(g_list_length(maptable->unmapped) == num_unmapped, NULL);
	osync_trace(TRACE_EXIT, "%s: %p", __func__, maptable);
	return maptable;
}

void mappingtable_close(OSyncMappingTable *maptable)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, maptable);
	osengine_mappingtable_close(maptable);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void check_mapping(OSyncMappingTable *maptable, int memberid, int mappingid, int numentries, const char *uid, const char *format, const char *objecttype)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %i, %i, %s, %s, %s)", __func__, maptable, memberid, mappingid, numentries, uid, format, objecttype);
	OSyncMapping *mapping = NULL;
	mark_point();
	OSyncMember *member = osync_member_from_id(maptable->group, memberid);
	OSyncMappingView *view = osengine_mappingtable_find_view(maptable, member);
	mark_point();
	if (mappingid != -1) {
		mapping = g_list_nth_data(maptable->mappings, mappingid);
	} else {
		GList *m;
		for (m = maptable->mappings; m; m = m->next) {
			mapping = m->data;
			OSyncMappingEntry *entry = osengine_mapping_find_entry(mapping, NULL, view);
			if (!entry)
				continue;
			OSyncChange *change = entry->change;
			fail_unless(change != NULL, NULL);
			if (!strcmp(osync_change_get_uid(change), uid))
				break;
		}
	}
	fail_unless(mapping != NULL, NULL);
	fail_unless(osengine_mapping_num_changes(mapping) == numentries, "osengine_mapping_num_changes(mapping) == numentries for %s, %i: %i != %i", uid, memberid, osengine_mapping_num_changes(mapping), numentries);
	mark_point();
	
	
	OSyncChange *change = osengine_mapping_find_entry(mapping, NULL, view)->change;
	fail_unless(change != NULL, NULL);
	if (format)
		fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), format), NULL);
	if (objecttype)
		fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), objecttype), NULL);
	if (uid && strcmp(osync_change_get_uid(change), uid)) {
		printf("uid mismatch: %s != %s for member %i and mapping %i\n", osync_change_get_uid(change), uid, memberid, mappingid);
		fail("uid mismatch");
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}

OSyncHashTable *hashtable_load(OSyncGroup *group, int memberid, int entries)
{
	mark_point();
	OSyncMember *member = osync_member_from_id(group, memberid);
	mark_point();
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    fail_unless(osync_hashtable_load(table, member, NULL), NULL);
    mark_point();
    fail_unless(osync_hashtable_num_entries(table) == entries, NULL);
    return table;
}

void check_hash(OSyncHashTable *table, const char *cmpuid)
{
	char *uid = NULL;
	char *hash = NULL;
	int i;
	osync_bool found = FALSE;
	for (i = 0; i < osync_hashtable_num_entries(table); i++) {
		osync_hashtable_nth_entry(table, i, &uid, &hash);
		if (!strcmp(cmpuid, uid))
			found = TRUE;
	}
	fail_unless(found == TRUE, NULL);
}

static void load_format(OSyncEnv *env, const char *name)
{
	OSyncError *error = NULL;
	char *path = g_strdup_printf("%s/%s", g_get_current_dir(), name);	
	fail_unless(osync_module_load(env, path, &error), NULL);
	g_free(path);
}

OSyncEnv *init_env(void)
{
	mark_point();
	OSyncEnv *osync = osync_env_new();
	mark_point();
	osync_env_set_option(osync, "LOAD_GROUPS", "FALSE");
	osync_env_set_option(osync, "LOAD_FORMATS", "FALSE");
	osync_env_set_option(osync, "LOAD_PLUGINS", "FALSE");
	mark_point();
	OSyncError *error = NULL;
	fail_unless(osync_env_initialize(osync, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	char *path = g_strdup_printf("%s/%s", g_get_current_dir(), "mock_sync.so");	
	fail_unless(osync_module_load(osync, path, &error), NULL);
	g_free(path);
	
	load_format(osync, "contact.so");
	load_format(osync, "data.so");
	load_format(osync, "event.so");
	load_format(osync, "note.so");
	load_format(osync, "todo.so");
	load_format(osync, "xml-vcal.so");
	load_format(osync, "xml-vcard.so");
	load_format(osync, "xml-vnote.so");
	load_format(osync, "xml-evolution.so");
	load_format(osync, "xml-kde.so");
	load_format(osync, "mockformat.so");
	
	return osync;
}

OSyncEnv *init_env_none(void)
{
	mark_point();
	OSyncEnv *osync = osync_env_new();
	mark_point();
	osync_env_set_option(osync, "LOAD_GROUPS", "FALSE");
	osync_env_set_option(osync, "LOAD_FORMATS", "FALSE");
	osync_env_set_option(osync, "LOAD_PLUGINS", "FALSE");
	mark_point();
	OSyncError *error = NULL;
	fail_unless(osync_env_initialize(osync, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	return osync;
}
