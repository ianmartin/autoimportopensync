#include "support.h"

char *olddir = NULL;

static void reset_env(void)
{
	g_unsetenv("CONNECT_ERROR");
	g_unsetenv("CONNECT_TIMEOUT");
	g_unsetenv("INIT_NULL");
	g_unsetenv("GET_CHANGES_ERROR");
	g_unsetenv("GET_CHANGES_TIMEOUT");
	g_unsetenv("GET_CHANGES_TIMEOUT2");
	g_unsetenv("COMMIT_ERROR");
	g_unsetenv("COMMIT_TIMEOUT");
	g_unsetenv("SYNC_DONE_ERROR");
	g_unsetenv("SYNC_DONE_TIMEOUT");
	g_unsetenv("DISCONNECT_ERROR");
	g_unsetenv("DISCONNECT_TIMEOUT");
}

char *setup_testbed(char *fkt_name)
{
	setuid(65534);
	char *testbed = g_strdup_printf("%s/testbed.XXXXXX", g_get_tmp_dir());
	mkdtemp(testbed);
	char *command = g_strdup_printf("cp -a %s%sdata/%s/* %s", g_getenv("srcdir") ? g_getenv("srcdir") : "", g_getenv("srcdir") ? "/" : "", fkt_name, testbed);
	if (system(command))
		abort();
	olddir = g_get_current_dir();
	if (chdir(testbed))
		abort();
	g_free(command);
	osync_debug("TEST", 4, "Seting up %s at %s\n", fkt_name, testbed);
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
	fail_unless(osync_mapping_num_entries(mapping) == (int)(user_data), NULL);

	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	osengine_mapping_solve(engine, mapping, change);
}

void conflict_handler_choose_modified(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	fail_unless(osync_mapping_num_entries(mapping) == (int)(user_data), NULL);

	int i;
	for (i = 0; i < osync_mapping_num_entries(mapping); i++) {
		OSyncChange *change = osync_mapping_nth_entry(mapping, i);
		if (change->changetype == CHANGE_MODIFIED) {
			osengine_mapping_solve(engine, mapping, change);
			return;
		}
	}
}

void conflict_handler_choose_deleted(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	fail_unless(osync_mapping_num_entries(mapping) == (int)(user_data), NULL);

	int i;
	for (i = 0; i < osync_mapping_num_entries(mapping); i++) {
		OSyncChange *change = osync_mapping_nth_entry(mapping, i);
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
	fail_unless(osync_mapping_num_entries(mapping) == (int)(user_data), NULL);
	
	osync_mapping_duplicate(engine, mapping);
}

void conflict_handler_random(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	fail_unless(osync_mapping_num_entries(mapping) == (int)(user_data), NULL);

	int num = osync_mapping_num_entries(mapping);
	int choosen = g_random_int_range(0, num);
	OSyncChange *change = osync_mapping_nth_entry(mapping, choosen);
	osengine_mapping_solve(engine, mapping, change);
}

void entry_status(OSyncEngine *engine, MSyncChangeUpdate *status, void *user_data)
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

void member_status(MSyncMemberUpdate *status, void *user_data)
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
			osync_debug("TEST", 0, "Previous sync was unclean");
			num_engine_prev_unclean++;
			break;
		default:
			printf("ERrro\n");
	}
}

void mapping_status(MSyncMappingUpdate *status, void *user_data)
{
	switch (status->type) {
		case MAPPING_SOLVED:
			osync_debug("TEST", 4, "Mapping solved");
			break;
		case MAPPING_NEW:
			osync_debug("TEST", 4, "New Mapping");
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
	mark_point();
	return osync_engine_sync_and_block(engine, error);
}

void create_case(Suite *s, const char *name, void (*function)(void))
{
	TCase *tc_new = tcase_create(name);
	suite_add_tcase (s, tc_new);
	tcase_add_test(tc_new, function);
}

OSyncMappingTable *mappingtable_load(OSyncGroup *group, int num_mappings, int num_unmapped)
{
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	fail_unless(osync_mappingtable_load(maptable, NULL), NULL);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == num_mappings, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == num_unmapped, NULL);
	return maptable;
}

void check_mapping(OSyncMappingTable *maptable, int memberid, int mappingid, int numentries, const char *uid, const char *format, const char *objecttype)
{
	OSyncMapping *mapping = NULL;
	mark_point();
	OSyncMember *member = osync_member_from_id(maptable->group, memberid);
	mark_point();
	if (mappingid != -1) {
		mapping = osync_mappingtable_nth_mapping(maptable, mappingid);
	} else {
		GList *m;
		for (m = maptable->mappings; m; m = m->next) {
			mapping = m->data;
			OSyncChange *change = osync_mapping_get_entry_by_owner(mapping, member);
			fail_unless(change != NULL, NULL);
			if (!strcmp(osync_change_get_uid(change), uid))
				break;
		}
	}
	fail_unless(mapping != NULL, NULL);
	fail_unless(osync_mapping_num_entries(mapping) == numentries, NULL);
	mark_point();
	
	
	OSyncChange *change = osync_mapping_get_entry_by_owner(mapping, member);
	fail_unless(change != NULL, NULL);
	if (format)
		fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), format), NULL);
	if (objecttype)
		fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), objecttype), NULL);
	if (uid && strcmp(osync_change_get_uid(change), uid)) {
		printf("uid mismatch: %s != %s for member %i and mapping %i\n", osync_change_get_uid(change), uid, memberid, mappingid);
		fail("uid mismatch");
	}
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
