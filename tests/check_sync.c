#include "support.h"

START_TEST (sync_setup)
{
  char *testbed = setup_testbed("sync_setup");
  OSyncEnv *osync = init_env();
  osync_group_load(osync, "configs/group", NULL);
  fail_unless(osync_env_num_groups(osync) == 1, NULL);
  destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_setup_init)
{
  char *testbed = setup_testbed("sync_setup_init");
  OSyncEnv *osync = init_env();
  OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
  fail_unless(group != NULL, NULL);
  fail_unless(osync_env_num_groups(osync) == 1, NULL);
  destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_setup_connect)
{
	char *testbed = setup_testbed("sync_setup_connect");
	num_connected = 0;
	num_disconnected = 0;
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = osync_engine_new(group, NULL);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	
	OSyncError *error = NULL;
	osync_engine_init(engine, &error);
	synchronize_once(engine, &error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	  
	fail_unless(num_connected == 2, NULL);
	fail_unless(num_disconnected == 2, NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_init_error)
{
	char *testbed = setup_testbed("sync_init_error");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	OSyncError *error = NULL;
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(error == NULL, NULL);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	mark_point();
	osync_engine_init(engine, &error);

	fail_unless(error != NULL, NULL);
	fail_unless(error->type == OSYNC_ERROR_MISCONFIGURATION, NULL);
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_new)
{
	char *testbed = setup_testbed("sync_easy_new");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	mark_point();
	fail_unless(engine != NULL, NULL);
	fail_unless(osync_engine_init(engine, &error), NULL);
	synchronize_once(engine, NULL);
	osync_engine_finalize(engine);
	osync_engine_free(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 2, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
    OSyncHashTable *table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);

	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_new_del)
{
	char *testbed = setup_testbed("sync_easy_new_del");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	mark_point();
	fail_unless(engine != NULL, NULL);
	osync_engine_init(engine, &error);
	synchronize_once(engine, NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	system("rm data1/testdata");
	
	synchronize_once(engine, NULL);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	mark_point();
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
    
    OSyncHashTable *table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);

	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_conflict)
{
	char *testbed = setup_testbed("sync_easy_conflict");
	num_conflicts = 0;
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, (void *)2);
	mark_point();
	fail_unless(engine != NULL, NULL);
	osync_engine_init(engine, &error);
	synchronize_once(engine, NULL);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	system("diff -x \".*\" data1 data2");
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 1, NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_new_mapping)
{
	char *testbed = setup_testbed("sync_easy_new_mapping");
	num_conflicts = 0;
	num_written = 0;
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	mark_point();
	fail_unless(engine != NULL, NULL);
	osync_engine_init(engine, &error);
	mark_point();
	
	synchronize_once(engine, NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 0, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 2, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
    
    OSyncHashTable *table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);

	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_conflict_duplicate)
{
	char *testbed = setup_testbed("sync_easy_conflict_duplicate");
	num_conflicts = 0;
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_conflict_callback(engine, conflict_handler_duplication, (void *)2);
	osync_engine_init(engine, &error);

	synchronize_once(engine, NULL);
	
	fail_unless(num_conflicts == 1, NULL);
	system("diff -x \".*\" data1 data2");
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 2, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 2, -1, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 2, "testdata-dupe", "mockformat", "data");
    mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
	osync_hashtable_close(table);
	
	system("rm -f data1/testdata-dupe");
	system("rm -f data2/testdata-dupe");
	
	synchronize_once(engine, NULL);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	
	maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 2, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_conflict_duplicate)
{
	char *testbed = setup_testbed("sync_conflict_duplicate");
	num_conflicts = 0;
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);

	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_conflict_callback(engine, conflict_handler_duplication, (void *)2);
	osync_engine_init(engine, &error);

	synchronize_once(engine, NULL);
	
	system("diff -x \".*\" data1 data2");
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 3, 0);
	check_mapping(maptable, 1, -1, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 2, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 1, -1, 2, "testdata-dupe-dupe", "mockformat", "data");
	check_mapping(maptable, 2, -1, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 2, "testdata-dupe", "mockformat", "data");
	check_mapping(maptable, 2, -1, 2, "testdata-dupe-dupe", "mockformat", "data");
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
	
	fail_unless(!system("rm -f data1/testdata-dupe data2/testdata-dupe-dupe"), NULL);
	
	synchronize_once(engine, NULL);
	osync_engine_finalize(engine);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	
	maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 2, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_conflict_duplicate2)
{
	char *testbed = setup_testbed("sync_conflict_duplicate2");
	num_conflicts = 0;
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_conflict_callback(engine, conflict_handler_duplication, (void *)2);
	osync_engine_init(engine, &error);

	synchronize_once(engine, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	system("rm -f data1/testdata");
	sleep(2);
	system("cp new_data data2/testdata");
	
	synchronize_once(engine, NULL);
	osync_engine_finalize(engine);
	
	system("diff -x \".*\" data1 data2");
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 1, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, 0, 2, "testdata", "mockformat", "data");
    mappingtable_close(maptable);
    
    OSyncHashTable *table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);

	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 comp_data)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_conflict_deldel)
{
	char *testbed = setup_testbed("sync_conflict_deldel");
	num_conflicts = 0;
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_conflict_callback(engine, conflict_handler_duplication, NULL);
	osync_engine_init(engine, &error);

	synchronize_once(engine, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);

	system("rm -f data1/testdata");
	system("rm -f data2/testdata");
	
	synchronize_once(engine, NULL);
	osync_engine_finalize(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
    
	OSyncHashTable *table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_moddel)
{
	char *testbed = setup_testbed("sync_moddel");
	num_conflicts = 0;
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_conflict_callback(engine, conflict_handler_random, (void *)2);
	osync_engine_init(engine, &error);

	synchronize_once(engine, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	
	sleep(2);
	system("cp new_data1 data1/testdata");
	system("cp new_data2 data2/testdata");
	
	synchronize_once(engine, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 1, NULL);
	
	system("rm -f data2/testdata");
	
	synchronize_once(engine, NULL);
	osync_engine_finalize(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
    
	OSyncHashTable *table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_conflict_moddel)
{
	char *testbed = setup_testbed("sync_moddel");
	num_conflicts = 0;
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_deleted, (void *)2);
	osync_engine_init(engine, &error);

	synchronize_once(engine, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	
	sleep(2);
	system("cp new_data2 data1/testdata");
	system("rm -f data2/testdata");
	
	synchronize_once(engine, NULL);
	osync_engine_finalize(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 1, NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
    
	OSyncHashTable *table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_dualdel)
{
	char *testbed = setup_testbed("sync_easy_dualdel");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_conflict_callback(engine, conflict_handler_duplication, NULL);
	osync_engine_init(engine, &error);

	synchronize_once(engine, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);

	system("rm -f data1/testdata");
	system("rm -f data1/testdata2");
	
	synchronize_once(engine, NULL);
	osync_engine_finalize(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
    
	OSyncHashTable *table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

//This cannot work with the mock plugin
/*START_TEST (sync_subdirs_new)
{
	char *testbed = setup_testbed("sync_subdirs_new");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	mark_point();
	
	system("rm -rf data1/.svn");
	system("rm -rf data2/.svn");
	system("rm -rf data1/subdir/.svn");
	system("rm -rf data2/subdir/.svn");
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	mark_point();
	fail_unless(engine != NULL, NULL);
	fail_unless(osync_engine_init(engine, &error), NULL);
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" -r data1 data2)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 3, 0);
	check_mapping(maptable, 1, -1, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 2, "testdata", "mockformat", "data");
	
	check_mapping(maptable, 1, -1, 2, "subdir/testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 2, "subdir/testdata", "mockformat", "data");
	
	check_mapping(maptable, 1, -1, 2, "subdir/testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 2, "subdir/testdata1", "mockformat", "data");
	
    mappingtable_close(maptable);
    
    OSyncHashTable *table = hashtable_load(group, 1, 3);
    check_hash(table, "testdata");
    check_hash(table, "subdir/testdata");
    check_hash(table, "subdir/testdata1");
	osync_hashtable_close(table);

	table = hashtable_load(group, 2, 3);
    check_hash(table, "testdata");
    check_hash(table, "subdir/testdata");
    check_hash(table, "subdir/testdata1");
	osync_hashtable_close(table);
	
	system("rm -f data2/testdata");
	system("rm -f data1/subdir/testdata");
	system("rm -f data1/subdir/testdata1");
	
	synchronize_once(engine, NULL);

	maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
    
    table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);

	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);

	fail_unless(!system("test \"x$(diff -x \".*\" -r data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data1)\" = \"xsubdir\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"xsubdir\""), NULL);
	fail_unless(!system("test \"x$(ls data1/subdir)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2/subdir)\" = \"x\""), NULL);
	
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	destroy_testbed(testbed);
}
END_TEST*/

Suite *env_suite(void)
{
	Suite *s = suite_create("Sync");
	//Suite *s2 = suite_create("Sync");
	create_case(s, "sync_setup", sync_setup);
	create_case(s, "sync_setup_init", sync_setup_init);
	create_case(s, "sync_init_error", sync_init_error);
	create_case(s, "sync_setup_connect", sync_setup_connect);
	create_case(s, "sync_easy_new", sync_easy_new);
	create_case(s, "sync_easy_new_del", sync_easy_new_del);
	create_case(s, "sync_easy_conflict", sync_easy_conflict);
	create_case(s, "sync_easy_new_mapping", sync_easy_new_mapping);
	create_case(s, "sync_easy_conflict_duplicate", sync_easy_conflict_duplicate);
	create_case(s, "sync_easy_dualdel", sync_easy_dualdel);
	create_case(s, "sync_conflict_duplicate2", sync_conflict_duplicate2);
	create_case(s, "sync_conflict_deldel", sync_conflict_deldel);
	create_case(s, "sync_moddel", sync_moddel);
	create_case(s, "sync_conflict_moddel", sync_conflict_moddel);
	create_case(s, "sync_conflict_duplicate", sync_conflict_duplicate);
	//create_case(s, "sync_subdirs_new", sync_subdirs_new);

	return s;
}

int main(void)
{
	int nf;

	Suite *s = env_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
