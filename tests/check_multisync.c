#include "support.h"

START_TEST (multisync_easy_new)
{
	char *testbed = setup_testbed("multisync_easy_new");
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, "dont_load_groups_on_initialize");
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	mark_point();
	fail_unless(engine != NULL, NULL);
	fail_unless(osync_engine_init(engine, &error), NULL);
	
	synchronize_once(engine);
	osync_engine_finalize(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_written == 2, NULL);
	fail_unless(num_read == 1, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "file", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "file", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "file", "data");
    osync_mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_easy_mod)
{
	char *testbed = setup_testbed("multisync_easy_new");
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, "dont_load_groups_on_initialize");
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	mark_point();
	fail_unless(engine != NULL, NULL);
	fail_unless(osync_engine_init(engine, &error), NULL);
	
	synchronize_once(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_written == 2, NULL);
	fail_unless(num_read == 1, NULL);
	
	sleep(2);
	system("cp newdata data3/testdata");
	
	synchronize_once(engine);
	osync_engine_finalize(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_written == 2, NULL);
	fail_unless(num_read == 1, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "file", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "file", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "file", "data");
    osync_mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_dual_mod)
{
	char *testbed = setup_testbed("multisync_easy_new");
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, "dont_load_groups_on_initialize");
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	mark_point();
	fail_unless(engine != NULL, NULL);
	fail_unless(osync_engine_init(engine, &error), NULL);
	
	synchronize_once(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_written == 2, NULL);
	fail_unless(num_read == 1, NULL);
	
	sleep(2);
	system("cp newdata data1/testdata");
	system("cp newdata data3/testdata");
	
	synchronize_once(engine);
	osync_engine_finalize(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_written == 1, NULL);
	fail_unless(num_read == 2, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "file", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "file", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "file", "data");
    osync_mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_triple_mod)
{
	char *testbed = setup_testbed("multisync_easy_new");
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, "dont_load_groups_on_initialize");
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	mark_point();
	fail_unless(engine != NULL, NULL);
	fail_unless(osync_engine_init(engine, &error), NULL);
	
	synchronize_once(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_written == 2, NULL);
	fail_unless(num_read == 1, NULL);
	
	sleep(2);
	system("cp newdata data1/testdata");
	system("cp newdata data2/testdata");
	system("cp newdata data3/testdata");
	
	synchronize_once(engine);
	osync_engine_finalize(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_written == 0, NULL);
	fail_unless(num_read == 3, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "file", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "file", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "file", "data");
    osync_mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_dual_new)
{
	char *testbed = setup_testbed("multisync_easy_new_partial");
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, "dont_load_groups_on_initialize");
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	mark_point();
	fail_unless(engine != NULL, NULL);
	fail_unless(osync_engine_init(engine, &error), NULL);
	synchronize_once(engine);

	fail_unless(num_written == 1, NULL);
	fail_unless(num_read == 2, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "file", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "file", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "file", "data");
    osync_mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	system("rm -f data2/testdata");
	
	synchronize_once(engine);
	osync_engine_finalize(engine);
	
	maptable = mappingtable_load(group, 0, 0);
    osync_mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 0);
	osync_hashtable_close(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST


START_TEST (multisync_triple_new)
{
	char *testbed = setup_testbed("multisync_easy_new_partial2");
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, "dont_load_groups_on_initialize");
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	num_written = 0;
	num_read = 0;
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	mark_point();
	fail_unless(engine != NULL, NULL);
	fail_unless(osync_engine_init(engine, &error), NULL);
	synchronize_once(engine);

	fail_unless(num_written == 0, NULL);
	fail_unless(num_read == 3, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "file", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "file", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "file", "data");
    osync_mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	system("rm -f data1/testdata");
	
	synchronize_once(engine);
	osync_engine_finalize(engine);
	
	maptable = mappingtable_load(group, 0, 0);
    osync_mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 0);
	osync_hashtable_close(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_easy_del)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = osync_engine_new(group, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_duplication, NULL);
	osync_engine_init(engine, NULL);
	
	synchronize_once(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);

	system("rm -f data2/testdata");
	
	synchronize_once(engine);
	osync_engine_finalize(engine);
	
	fail_unless(num_read == 1, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 2, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 0, 0);
    osync_mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 0);
	osync_hashtable_close(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_dual_del)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = osync_engine_new(group, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_duplication, NULL);
	osync_engine_init(engine, NULL);
	
	synchronize_once(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	system("rm -f data1/testdata");
	system("rm -f data3/testdata");
	
	synchronize_once(engine);
	osync_engine_finalize(engine);
	
	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 1, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 0, 0);
    osync_mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 0);
	osync_hashtable_close(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_triple_del)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = osync_engine_new(group, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_duplication, NULL);
	osync_engine_init(engine, NULL);
	
	synchronize_once(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	system("rm -f data1/testdata");
	system("rm -f data2/testdata");
	system("rm -f data3/testdata");
	
	synchronize_once(engine);
	osync_engine_finalize(engine);
	
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 0, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 0, 0);
    osync_mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 0);
	osync_hashtable_close(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_conflict_data_choose)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose");
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, "dont_load_groups_on_initialize");
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	num_written = 0;
	num_read = 0;
	num_conflicts = 0;
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, (void *)2);
	mark_point();
	fail_unless(engine != NULL, NULL);
	fail_unless(osync_engine_init(engine, &error), NULL);
	synchronize_once(engine);
	
	char *uid;
    char *hash;

	fail_unless(num_written == 2, NULL);
	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 1, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
	mark_point();
	OSyncMapping *mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	OSyncMember *member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	system("rm -f data3/testdata");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine);
	osync_engine_finalize(engine);
	
	mark_point();
	maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 0, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
    
    member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_conflict_data_choose2)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, "dont_load_groups_on_initialize");
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	num_written = 0;
	num_read = 0;
	num_conflicts = 0;
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, (void *)3);
	mark_point();
	fail_unless(engine != NULL, NULL);
	fail_unless(osync_engine_init(engine, &error), NULL);
	synchronize_once(engine);
	
	char *uid;
    char *hash;

	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 1, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
	mark_point();
	OSyncMapping *mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	OSyncMember *member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	system("rm -f data3/testdata");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine);;
	osync_engine_finalize(engine);
	
	mark_point();
	maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 0, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
    
    member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_conflict_changetype_choose)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = osync_engine_new(group, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, NULL);
	
	synchronize_once(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data1/testdata");
	system("cp newdata data3/testdata");
	
	num_written = 0;
	num_read = 0;
	num_conflicts = 0;
	synchronize_once(engine);
	
	char *uid;
    char *hash;

	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 1, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
	mark_point();
	OSyncMapping *mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	OSyncMember *member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	system("rm -f data1/testdata");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine);
	osync_engine_finalize(engine);
	
	mark_point();
	maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 0, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
    
    member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_conflict_changetype_choose2)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = osync_engine_new(group, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_deleted, (void *)3);
	osync_engine_init(engine, NULL);
	
	synchronize_once(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data1/testdata");
	system("cp newdata data3/testdata");
	
	num_written = 0;
	num_read = 0;
	num_conflicts = 0;
	synchronize_once(engine);
	osync_engine_finalize(engine);

	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);
	
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 0, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	OSyncMember *member = osync_member_from_id(group, 1);
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
    
    member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_conflict_hybrid_choose)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = osync_engine_new(group, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, NULL);
	
	synchronize_once(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data1/testdata");
	system("cp newdata data3/testdata");
	system("cp newdata2 data2/testdata");
	
	num_written = 0;
	num_read = 0;
	num_conflicts = 0;
	synchronize_once(engine);
	
	char *uid;
    char *hash;

	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 1, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
	mark_point();
	OSyncMapping *mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	OSyncMember *member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	system("rm -f data1/testdata");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine);
	osync_engine_finalize(engine);
	
	mark_point();
	maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 0, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
    
    member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_conflict_hybrid_choose2)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = osync_engine_new(group, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_deleted, (void *)3);
	osync_engine_init(engine, NULL);
	
	synchronize_once(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data1/testdata");
	system("cp newdata data3/testdata");
	system("cp newdata2 data2/testdata");
	
	num_written = 0;
	num_read = 0;
	num_conflicts = 0;
	synchronize_once(engine);

	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);

	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 0, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	OSyncMember *member = osync_member_from_id(group, 1);
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
    
    member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_conflict_data_duplicate)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose");
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, "dont_load_groups_on_initialize");
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	num_written = 0;
	num_read = 0;
	num_conflicts = 0;
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_duplication, (void *)2);
	mark_point();
	fail_unless(engine != NULL, NULL);
	fail_unless(osync_engine_init(engine, &error), NULL);
	synchronize_once(engine);

	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 5, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 2, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);

	OSyncMapping *mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	OSyncMember *member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);

	mapping = osync_mappingtable_nth_mapping(maptable, 1);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 0);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);

    osync_mappingtable_close(maptable);
    
    //Test the hash tables
    char *uid, *uid2;
    char *hash;
    
	mark_point();
	member = osync_member_from_id(group, 1);
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 2, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(osync_hashtable_nth_entry(table, 1, &uid2, &hash), NULL);
	fail_unless((!strcmp("testdata", uid) && !strcmp("testdata-dupe", uid2)) || (!strcmp("testdata", uid2) && !strcmp("testdata-dupe", uid)), NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 2, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(osync_hashtable_nth_entry(table, 1, &uid2, &hash), NULL);
	fail_unless((!strcmp("testdata", uid) && !strcmp("testdata-dupe", uid2)) || (!strcmp("testdata", uid2) && !strcmp("testdata-dupe", uid)), NULL);
	
	member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 2, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(osync_hashtable_nth_entry(table, 1, &uid2, &hash), NULL);
	fail_unless((!strcmp("testdata", uid) && !strcmp("testdata-dupe", uid2)) || (!strcmp("testdata", uid2) && !strcmp("testdata-dupe", uid)), NULL);
	
	system("rm -f data3/testdata");
	
	synchronize_once(engine);
	
	mark_point();
	maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 1, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);

	mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 0);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);

    osync_mappingtable_close(maptable);
    
    //Test the hash tables
	mark_point();
	member = osync_member_from_id(group, 1);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata-dupe", uid), NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata-dupe", uid), NULL);
	
	member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata-dupe", uid), NULL);
	
	system("rm -f data2/testdata-dupe");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine);
	osync_engine_finalize(engine);
	
	mark_point();
	maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 0, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
    
    member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_conflict_data_duplicate2)
{
	char *testbed = setup_testbed("multisync_conflict_data_duplicate2");
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, "dont_load_groups_on_initialize");
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	num_written = 0;
	num_read = 0;
	num_conflicts = 0;
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_duplication, (void *)3);
	mark_point();
	fail_unless(engine != NULL, NULL);
	fail_unless(osync_engine_init(engine, &error), NULL);
	synchronize_once(engine);

	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 8, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 3, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);

	OSyncMapping *mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	OSyncMember *member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);

	mapping = osync_mappingtable_nth_mapping(maptable, 1);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 0);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);

	mapping = osync_mappingtable_nth_mapping(maptable, 2);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 0);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe-dupe"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe-dupe"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe-dupe"), NULL);

    osync_mappingtable_close(maptable);
    
    //Test the hash tables
    char *uid;
    char *hash;
    
	mark_point();
	member = osync_member_from_id(group, 1);
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 3, NULL);

	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 3, NULL);

	member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 3, NULL);

	system("rm -f data3/testdata data3/testdata-dupe-dupe");
	
	synchronize_once(engine);
	
	mark_point();
	maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 1, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);

	mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 0);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);

    osync_mappingtable_close(maptable);
    
    //Test the hash tables
	mark_point();
	member = osync_member_from_id(group, 1);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata-dupe", uid), NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata-dupe", uid), NULL);
	
	member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata-dupe", uid), NULL);
	
	system("rm -f data2/testdata-dupe");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine);
	osync_engine_finalize(engine);
	
	mark_point();
	maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 0, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
    
    member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_conflict_changetype_duplicate)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = osync_engine_new(group, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_duplication, (void *)3);
	osync_engine_init(engine, NULL);
	
	synchronize_once(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data1/testdata");
	system("cp newdata data3/testdata");
	
	num_written = 0;
	num_read = 0;
	num_conflicts = 0;
	synchronize_once(engine);
	
	char *uid;
    char *hash;

	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 1, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
	mark_point();
	OSyncMapping *mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	OSyncMember *member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	system("rm -f data1/testdata");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine);
	osync_engine_finalize(engine);
	
	mark_point();
	maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 0, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
    
    member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_conflict_changetype_duplicate2)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = osync_engine_new(group, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_duplication, (void *)3);
	osync_engine_init(engine, NULL);
	
	synchronize_once(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data2/testdata");
	system("rm -f data3/testdata");
	system("cp newdata2 data1/testdata");
	
	num_written = 0;
	num_read = 0;
	num_conflicts = 0;
	synchronize_once(engine);
	
	char *uid;
    char *hash;

	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 1, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
	mark_point();
	OSyncMapping *mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	OSyncMember *member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 1, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid), NULL);
	
	system("rm -f data1/testdata");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine);
	osync_engine_finalize(engine);
	
	mark_point();
	maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 0, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
    
    member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_conflict_hybrid_duplicate)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = osync_engine_new(group, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_duplication, (void *)3);
	osync_engine_init(engine, NULL);
	
	synchronize_once(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data2/testdata");
	system("cp newdata data3/testdata");
	system("cp newdata2 data1/testdata");
	
	num_written = 0;
	num_read = 0;
	num_conflicts = 0;
	synchronize_once(engine);

	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 5, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 2, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
	mark_point();
	OSyncMapping *mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	OSyncMember *member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	
	mapping = osync_mappingtable_nth_mapping(maptable, 1);
	fail_unless(osync_mapping_num_entries(mapping) == 3, NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 0);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 2);
	member = osync_change_get_member(change);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
    //Test the hash tables
    char *uid, *uid2;
    char *hash;
    
	mark_point();
	member = osync_member_from_id(group, 1);
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 2, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(osync_hashtable_nth_entry(table, 1, &uid2, &hash), NULL);
	fail_unless((!strcmp("testdata", uid) && !strcmp("testdata-dupe", uid2)) || (!strcmp("testdata", uid2) && !strcmp("testdata-dupe", uid)), NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 2, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(osync_hashtable_nth_entry(table, 1, &uid2, &hash), NULL);
	fail_unless((!strcmp("testdata", uid) && !strcmp("testdata-dupe", uid2)) || (!strcmp("testdata", uid2) && !strcmp("testdata-dupe", uid)), NULL);
	
	member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 2, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(osync_hashtable_nth_entry(table, 1, &uid2, &hash), NULL);
	fail_unless((!strcmp("testdata", uid) && !strcmp("testdata-dupe", uid2)) || (!strcmp("testdata", uid2) && !strcmp("testdata-dupe", uid)), NULL);
	
	
	system("rm -f data1/testdata data2/testdata-dupe");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine);
	osync_engine_finalize(engine);
	
	mark_point();
	maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 0, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    
	mark_point();
	member = osync_member_from_id(group, 1);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
    
    member = osync_member_from_id(group, 3);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 0, NULL);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *multisync_suite(void)
{
	Suite *s = suite_create("Multisync");
	create_case(s, "multisync_easy_new", multisync_easy_new);
	create_case(s, "multisync_dual_new", multisync_dual_new);
	create_case(s, "multisync_triple_new", multisync_triple_new);
	create_case(s, "multisync_easy_mod", multisync_easy_mod);
	create_case(s, "multisync_dual_mod", multisync_dual_mod);
	create_case(s, "multisync_triple_mod", multisync_triple_mod);
	create_case(s, "multisync_easy_del", multisync_easy_del);
	create_case(s, "multisync_dual_del", multisync_dual_del);
	create_case(s, "multisync_triple_del", multisync_triple_del);
	
	create_case(s, "multisync_conflict_data_choose", multisync_conflict_data_choose);
	create_case(s, "multisync_conflict_data_choose2", multisync_conflict_data_choose2);
	create_case(s, "multisync_conflict_changetype_choose", multisync_conflict_changetype_choose);
	create_case(s, "multisync_conflict_changetype_choose2", multisync_conflict_changetype_choose2);
	create_case(s, "multisync_conflict_hybrid_choose", multisync_conflict_hybrid_choose);
	create_case(s, "multisync_conflict_hybrid_choose2", multisync_conflict_hybrid_choose2);
	create_case(s, "multisync_conflict_data_duplicate", multisync_conflict_data_duplicate);
	create_case(s, "multisync_conflict_data_duplicate2", multisync_conflict_data_duplicate2);
	create_case(s, "multisync_conflict_changetype_duplicate", multisync_conflict_changetype_duplicate);
	create_case(s, "multisync_conflict_changetype_duplicate2", multisync_conflict_changetype_duplicate2);
	create_case(s, "multisync_conflict_hybrid_duplicate", multisync_conflict_hybrid_duplicate);

	return s;
}

int main(void)
{
	int nf;

	Suite *s = multisync_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
