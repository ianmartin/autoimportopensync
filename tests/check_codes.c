#include "support.h"

/*START_TEST (multisync_easy_new)
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
	osync_engine_free(engine);
	
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
END_TEST*/

START_TEST (dual_connect_error)
{
	char *testbed = setup_testbed("sync_easy_new");
	
	g_setenv("CONNECT_ERROR", "3", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	synchronize_once(engine);
	
	fail_unless(num_member_connect_errors == 2, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *multisync_suite(void)
{
	Suite *s = suite_create("Error Codes");
	//Suite *s2 = suite_create("Error Codes");
	create_case(s, "dual_connect_error", dual_connect_error);

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
