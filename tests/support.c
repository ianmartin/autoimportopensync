#include "support.h"

char *olddir = NULL;

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
			if (osync_change_has_data(status->change))
				num_read++;
			break;
		case CHANGE_SENT:
			num_written++;
			break;
		default:
			printf("Unknown status\n");
	}
}

void member_status(MSyncMemberUpdate *status)
{
	switch (status->type) {
		case MEMBER_CONNECTED:
			num_connected++;
			break;
		case MEMBER_DISCONNECTED:
			num_disconnected++;
			break;
		default:
			printf("Unknown status\n");
	}
}

void synchronize_once(OSyncEngine *engine)
{
	num_connected = 0;
	num_disconnected = 0;
	num_conflicts = 0;
	num_written = 0;
	num_read = 0;
	mark_point();
	osync_engine_sync_and_block(engine, NULL);
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
	osync_mappingtable_load(maptable);
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
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
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
