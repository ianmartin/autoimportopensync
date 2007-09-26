#include "support.h"

START_TEST (multisync_easy_new)
{
	char *testbed = setup_testbed("multisync_easy_new");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncEngine *engine = init_engine(group);
	
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	osengine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
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
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncEngine *engine = init_engine(group);
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_written == 2, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_engine_end_conflicts = 1, NULL);
	
	sleep(2);
	system("cp newdata data3/testdata");
	
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	osengine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_read == 1, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
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
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncEngine *engine = init_engine(group);
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	
	sleep(2);
	system("cp newdata data1/testdata");
	system("cp newdata data3/testdata");
	
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	osengine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_written == 1, NULL);
	fail_unless(num_read == 2, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
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
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncEngine *engine = init_engine(group);
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	
	sleep(2);
	system("cp newdata data1/testdata");
	system("cp newdata data2/testdata");
	system("cp newdata data3/testdata");
	
	synchronize_once(engine, NULL);
	osengine_finalize(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_written == 0, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_engine_end_conflicts = 1, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
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
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncEngine *engine = init_engine(group);
	
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_written == 1, NULL);
	fail_unless(num_read == 2, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
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
	
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
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
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncEngine *engine = init_engine(group);
	
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_read == 3, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
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
	
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
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
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_duplication, NULL);
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);

	system("rm -f data2/testdata");
	
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 2, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
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
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_duplication, NULL);
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	system("rm -f data1/testdata");
	system("rm -f data3/testdata");
	
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 1, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
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
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_duplication, NULL);
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	system("rm -f data1/testdata");
	system("rm -f data2/testdata");
	system("rm -f data3/testdata");
	
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 0, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
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
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(2));

	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	system("rm -f data3/testdata");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
	hashtable_load(group, 1, 0);
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

START_TEST (multisync_conflict_data_choose2)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(3));

	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	system("rm -f data3/testdata");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine, NULL);;
	osengine_finalize(engine);
	
	mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
	hashtable_load(group, 1, 0);
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

START_TEST (multisync_conflict_changetype_choose)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data1/testdata");
	system("cp newdata data3/testdata");
	
	num_written = 0;
	num_read = 0;
	num_conflicts = 0;
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
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
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
	hashtable_load(group, 1, 0);
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

START_TEST (multisync_conflict_changetype_choose2)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_choose_deleted, GINT_TO_POINTER(3));
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data1/testdata");
	system("cp newdata data3/testdata");
	
	synchronize_once(engine, NULL);
	osengine_finalize(engine);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
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

START_TEST (multisync_conflict_hybrid_choose)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data1/testdata");
	system("cp newdata data3/testdata");
	system("cp newdata2 data2/testdata");
	
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
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
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
	hashtable_load(group, 1, 0);
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

START_TEST (multisync_conflict_hybrid_choose2)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_choose_deleted, GINT_TO_POINTER(3));
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data1/testdata");
	system("cp newdata data3/testdata");
	system("cp newdata2 data2/testdata");
	
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);

	OSyncMappingTable *maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
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

START_TEST (multisync_conflict_data_duplicate)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_duplication, GINT_TO_POINTER(2));

	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 5, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata-dupe", "mockformat", "data");
    mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	system("rm -f data3/testdata");
	
	synchronize_once(engine, NULL);
	
	maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata-dupe", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 1);
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	system("rm -f data2/testdata-dupe");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
	hashtable_load(group, 1, 0);
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

START_TEST (multisync_conflict_data_duplicate2)
{
	char *testbed = setup_testbed("multisync_conflict_data_duplicate2");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_duplication, GINT_TO_POINTER(3));

	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 8, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 3, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata-dupe-dupe", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata-dupe-dupe", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata-dupe-dupe", "mockformat", "data");
    mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 3);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
    check_hash(table, "testdata-dupe-dupe");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 3);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
    check_hash(table, "testdata-dupe-dupe");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 3);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
    check_hash(table, "testdata-dupe-dupe");
	osync_hashtable_close(table);

	system("rm -f data3/testdata data3/testdata-dupe-dupe");
	
	synchronize_once(engine, NULL);
	
	maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata-dupe", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 1);
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	system("rm -f data2/testdata-dupe");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
	hashtable_load(group, 1, 0);
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

START_TEST (multisync_conflict_changetype_duplicate)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_duplication, GINT_TO_POINTER(3));
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data1/testdata");
	system("cp newdata data3/testdata");
	
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
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
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
	hashtable_load(group, 1, 0);
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

START_TEST (multisync_conflict_changetype_duplicate2)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_duplication, GINT_TO_POINTER(3));
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data2/testdata");
	system("rm -f data3/testdata");
	system("cp newdata2 data1/testdata");
	
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, 0, 3, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
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
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
	hashtable_load(group, 1, 0);
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

START_TEST (multisync_conflict_hybrid_duplicate)
{
	char *testbed = setup_testbed("multisync_conflict_changetype_choose");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_duplication, GINT_TO_POINTER(3));
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data2/testdata");
	system("cp newdata data3/testdata");
	system("cp newdata2 data1/testdata");
	
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 5, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata-dupe", "mockformat", "data");
    mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	system("rm -f data1/testdata data2/testdata-dupe");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
	hashtable_load(group, 1, 0);
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

START_TEST (multisync_multi_conflict)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_duplication, GINT_TO_POINTER(3));
	
	system("cp newdata data3/testdata1");
	system("cp newdata1 data2/testdata2");
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 3, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata2", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata2", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata2", "mockformat", "data");
    mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 3);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 3);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 3);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
	osync_hashtable_close(table);
		
	//Change statuses
	fail_unless(num_read == 3, NULL);
	fail_unless(num_read_info == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 6, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_recv_errors == 0, NULL);
	
	//Member statuses
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_comitted_all == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_member_get_changes_errors == 0, NULL);
	fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_member_disconnect_errors == 0, NULL);
	fail_unless(num_member_comitted_all_errors == 0, NULL);
	
	//Engine statuses
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successfull == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	

	
	sleep(2);
	
	system("rm -f data2/testdata");
	system("cp newdata data3/testdata");

	system("cp newdata3 data1/testdata1");
	system("cp newdata4 data3/testdata1");
	
	system("cp newdata data1/testdata2");
	system("cp newdata5 data3/testdata2");
	system("rm -f data2/testdata2");
	
	synchronize_once(engine, NULL);

	fail_unless(num_read == 7, NULL);
	fail_unless(num_conflicts == 3, NULL);
	fail_unless(num_written == 12, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	maptable = mappingtable_load(group, 5, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1-dupe", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1-dupe", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1-dupe", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata2", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata2", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata2", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata2-dupe", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata2-dupe", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata2-dupe", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 5);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
    check_hash(table, "testdata1-dupe");
    check_hash(table, "testdata2-dupe");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 5);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
    check_hash(table, "testdata1-dupe");
    check_hash(table, "testdata2-dupe");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 5);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
    check_hash(table, "testdata1-dupe");
    check_hash(table, "testdata2-dupe");
	osync_hashtable_close(table);
	
	system("rm -f data1/*");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
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

START_TEST (multisync_delayed_conflict_handler)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_delay, GINT_TO_POINTER(3));
	
	system("cp newdata data3/testdata1");
	system("cp newdata1 data2/testdata2");
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 3, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata2", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata2", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata2", "mockformat", "data");
    mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 3);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 3);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 3);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
	osync_hashtable_close(table);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 6, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	
	sleep(2);
	
	system("rm -f data2/testdata");
	system("cp newdata data3/testdata");

	system("cp newdata3 data1/testdata1");
	system("rm -f data2/testdata1");
	
	system("cp newdata data1/testdata2");
	system("rm -f data3/testdata2");
	system("rm -f data2/testdata2");
	
	synchronize_once(engine, NULL);

	fail_unless(num_read == 7, NULL);
	fail_unless(num_conflicts == 3, NULL);
	fail_unless(num_written == 6, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	maptable = mappingtable_load(group, 3, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata2", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata2", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata2", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 3);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 3);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 3);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
	osync_hashtable_close(table);
	
	system("rm -f data1/*");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
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

START_TEST (multisync_delayed_slow)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_delay, GINT_TO_POINTER(3));
	
	system("cp newdata data3/testdata1");
	setenv("SLOW_REPORT", "2", TRUE);
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 4, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	
	sleep(2);
	
	system("cp newdata data3/testdata");

	system("cp newdata3 data1/testdata1");
	system("rm -f data2/testdata1");
	
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 4, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	system("rm -f data1/*");
	
	mark_point();
	num_conflicts = 0;
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 0);
	osync_hashtable_close(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	unsetenv("SLOW_REPORT");
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_conflict_ignore)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_ignore, GINT_TO_POINTER(3));
	
	system("cp newdata data3/testdata1");
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 4, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data2/testdata");
	system("cp newdata data3/testdata");

	system("cp newdata3 data1/testdata1");
	system("cp newdata2 data2/testdata1");
	system("cp newdata1 data3/testdata1");
	
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 5, NULL);
	fail_unless(num_conflicts == 2, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	synchronize_once(engine, NULL);

	fail_unless(num_read == 5, NULL);
	fail_unless(num_conflicts == 2, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 5, NULL);
	fail_unless(num_conflicts == 2, NULL);
	fail_unless(num_written == 4, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	system("rm -f data1/*");

	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
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

START_TEST (multisync_conflict_ignore2)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_ignore, NULL);
	
	system("cp newdata data3/testdata1");
	system("cp newdata1 data3/testdata");
	
	synchronize_once(engine, NULL);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	system("cp newdata2 data2/testdata");
	
	osengine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(3));
	synchronize_once(engine, NULL);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	system("rm -f data1/*");

	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
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

START_TEST(multisync_easy_new_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	multisync_easy_new();
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_triple_del_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	multisync_triple_del();
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_conflict_hybrid_choose2_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	multisync_conflict_hybrid_choose2();
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_delayed_conflict_handler_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	multisync_delayed_conflict_handler();
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_delayed_slow_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	multisync_delayed_slow();
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_conflict_ignore_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	multisync_conflict_ignore();
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_conflict_ignore2_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	setenv("NO_TIMEOUTS", "7", TRUE);
	multisync_conflict_ignore2();
	unsetenv("BATCH_COMMIT");
	unsetenv("NO_TIMEOUTS");
}
END_TEST

START_TEST(multisync_conflict_hybrid_duplicate_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	multisync_conflict_hybrid_duplicate();
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_multi_conflict_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	multisync_multi_conflict();
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_zero_changes_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_duplication, GINT_TO_POINTER(3));
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	setenv("NUM_BATCH_COMMITS", "0", TRUE);
	
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);

	system("rm -f data1/*");
		
	unsetenv("NUM_BATCH_COMMITS");
	
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
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
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_conflict_hybrid_choose2_b2)
{
	setenv("BATCH_COMMIT", "2", TRUE);
	multisync_conflict_hybrid_choose2();
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_delayed_conflict_handler_b2)
{
	setenv("BATCH_COMMIT", "2", TRUE);
	multisync_delayed_conflict_handler();
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_conflict_ignore_b2)
{
	setenv("BATCH_COMMIT", "2", TRUE);
	multisync_conflict_ignore();
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_multi_conflict_b2)
{
	setenv("BATCH_COMMIT", "2", TRUE);
	multisync_multi_conflict();
	unsetenv("BATCH_COMMIT");
}
END_TEST

Suite *multisync_suite(void)
{
	Suite *s = suite_create("Multisync");
	//Suite *s2 = suite_create("Multisync");
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
	create_case(s, "multisync_multi_conflict", multisync_multi_conflict);

	create_case(s, "multisync_delayed_conflict_handler", multisync_delayed_conflict_handler);
	create_case(s, "multisync_delayed_slow", multisync_delayed_slow);
	
	create_case(s, "multisync_conflict_ignore", multisync_conflict_ignore);
	create_case(s, "multisync_conflict_ignore2", multisync_conflict_ignore2);
	
	create_case(s, "multisync_easy_new_b", multisync_easy_new_b);
	create_case(s, "multisync_triple_del_b", multisync_triple_del_b);
	create_case(s, "multisync_conflict_hybrid_choose2_b", multisync_conflict_hybrid_choose2_b);
	create_case(s, "multisync_delayed_conflict_handler_b", multisync_delayed_conflict_handler_b);
	create_case(s, "multisync_delayed_slow_b", multisync_delayed_slow_b);
	create_case(s, "multisync_conflict_ignore_b", multisync_conflict_ignore_b);
	create_case(s, "multisync_conflict_ignore2_b", multisync_conflict_ignore2_b);
	create_case(s, "multisync_conflict_hybrid_duplicate_b", multisync_conflict_hybrid_duplicate_b);
	create_case(s, "multisync_multi_conflict_b", multisync_multi_conflict_b);
	create_case(s, "multisync_zero_changes_b", multisync_zero_changes_b);
	
	create_case(s, "multisync_conflict_hybrid_choose2_b2", multisync_conflict_hybrid_choose2_b2);
	create_case(s, "multisync_delayed_conflict_handler_b2", multisync_delayed_conflict_handler_b2);
	create_case(s, "multisync_conflict_ignore_b2", multisync_conflict_ignore_b2);
	create_case(s, "multisync_multi_conflict_b2", multisync_multi_conflict_b2);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = multisync_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
