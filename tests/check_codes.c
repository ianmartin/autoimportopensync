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
	
	synchronize_once(engine, NULL);
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

START_TEST (single_init_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("INIT_NULL", "2", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	
	fail_unless(!osync_engine_init(engine, &error), NULL);
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

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
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 2, NULL);
	fail_unless(num_connected == 0, NULL);
	fail_unless(num_disconnected == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_two_connect_error)
{
	char *testbed = setup_testbed("sync_easy_new");
	
	g_setenv("CONNECT_ERROR", "1", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 1, NULL);
	fail_unless(num_connected == 1, NULL);
	fail_unless(num_disconnected == 1, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (two_of_three_connect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("CONNECT_ERROR", "5", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 2, NULL);
	fail_unless(num_connected == 1, NULL);
	fail_unless(num_disconnected == 1, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (two_of_three_connect_error2)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("CONNECT_ERROR", "6", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 2, NULL);
	fail_unless(num_connected == 1, NULL);
	fail_unless(num_disconnected == 1, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (three_of_three_connect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("CONNECT_ERROR", "7", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 3, NULL);
	fail_unless(num_connected == 0, NULL);
	fail_unless(num_disconnected == 0, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_connect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");

	g_setenv("CONNECT_ERROR", "2", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 1, NULL);
	fail_unless(num_connected == 2, NULL);
	fail_unless(num_disconnected == 2, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (no_connect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("CONNECT_ERROR", "0", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successfull == 1, NULL);
	
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_connect_timeout)
{
	char *testbed = setup_testbed("sync_easy_new");
	
	g_setenv("CONNECT_TIMEOUT", "2", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 1, NULL);
	fail_unless(num_connected == 1, NULL);
	fail_unless(num_disconnected == 1, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_connect_timeout)
{
	char *testbed = setup_testbed("sync_easy_new");
	
	g_setenv("CONNECT_TIMEOUT", "3", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 2, NULL);
	fail_unless(num_connected == 0, NULL);
	fail_unless(num_disconnected == 0, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("CONNECT_TIMEOUT", "2", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 1, NULL);
	fail_unless(num_connected == 2, NULL);
	fail_unless(num_disconnected == 2, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (timeout_and_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("CONNECT_TIMEOUT", "2", TRUE);
	g_setenv("CONNECT_ERROR", "4", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 2, NULL);
	fail_unless(num_connected == 1, NULL);
	fail_unless(num_disconnected == 1, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_get_changes_error)
{
	char *testbed = setup_testbed("sync_easy_conflict");
	
	g_setenv("GET_CHANGES_ERROR", "2", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 2, NULL);
	fail_unless(num_disconnected == 2, NULL);
	fail_unless(num_member_get_changes_errors == 1, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_get_changes_error)
{
	char *testbed = setup_testbed("sync_easy_conflict");
	
	g_setenv("GET_CHANGES_ERROR", "3", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 2, NULL);
	fail_unless(num_disconnected == 2, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_read == 0, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (two_of_three_get_changes_error)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	g_setenv("GET_CHANGES_ERROR", "5", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_get_changes_error)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	g_setenv("GET_CHANGES_ERROR", "1", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_get_changes_errors == 1, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_get_changes_timeout)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	g_setenv("GET_CHANGES_TIMEOUT", "1", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_get_changes_errors == 1, NULL);
	fail_unless(num_member_sent_changes == 2, NULL);
	fail_unless(num_read == 2, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (get_changes_timeout_and_error)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	g_setenv("GET_CHANGES_TIMEOUT", "3", TRUE);
	g_setenv("GET_CHANGES_ERROR", "4", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_read == 0, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (get_changes_timeout_sleep)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	g_setenv("GET_CHANGES_TIMEOUT2", "7", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_read == 0, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_commit_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("COMMIT_ERROR", "4", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 1, NULL);
	fail_unless(num_written_errors == 1, NULL);
	fail_unless(num_mapping_errors == 1, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_commit_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("COMMIT_ERROR", "6", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_written_errors == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_commit_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("COMMIT_TIMEOUT", "4", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 1, NULL);
	fail_unless(num_written_errors == 1, NULL);
	fail_unless(num_mapping_errors == 1, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_commit_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("COMMIT_TIMEOUT", "6", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_written_errors == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (commit_timeout_and_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("COMMIT_TIMEOUT", "4", TRUE);
	g_setenv("COMMIT_ERROR", "2", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_written_errors == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (commit_timeout_and_error2)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("COMMIT_TIMEOUT", "2", TRUE);
	g_setenv("COMMIT_ERROR", "4", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_written_errors == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (commit_error_modify)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	g_setenv("COMMIT_TIMEOUT", "2", TRUE);
	g_setenv("COMMIT_ERROR", "4", TRUE);
	
	sleep(2);
	
	system("cp newdata2 data1/testdata");
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_written_errors == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data2 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (commit_error_delete)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	g_setenv("COMMIT_TIMEOUT", "2", TRUE);
	g_setenv("COMMIT_ERROR", "4", TRUE);
	
	sleep(2);
	
	system("rm -f data1/testdata");
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_written_errors == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data2 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_sync_done_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("SYNC_DONE_ERROR", "4", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_member_sync_done_errors == 1, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_sync_done_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("SYNC_DONE_ERROR", "6", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_member_sync_done_errors == 2, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (triple_sync_done_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("SYNC_DONE_ERROR", "7", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_member_sync_done_errors == 3, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_sync_done_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("SYNC_DONE_TIMEOUT", "4", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_member_sync_done_errors == 1, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_sync_done_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("SYNC_DONE_TIMEOUT", "6", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_member_sync_done_errors == 2, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_done_timeout_and_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("SYNC_DONE_TIMEOUT", "5", TRUE);
	g_setenv("SYNC_DONE_ERROR", "2", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 3, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_member_sync_done_errors == 3, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_disconnect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("DISCONNECT_ERROR", "4", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 2, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_member_disconnect_errors == 1, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successfull == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_disconnect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("DISCONNECT_ERROR", "6", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 1, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_member_disconnect_errors == 2, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successfull == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (triple_disconnect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("DISCONNECT_ERROR", "7", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 0, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_member_disconnect_errors == 3, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successfull == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_disconnect_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("DISCONNECT_TIMEOUT", "4", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 2, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_member_disconnect_errors == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successfull == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_disconnect_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("DISCONNECT_TIMEOUT", "6", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 1, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_member_disconnect_errors == 2, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successfull == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (disconnect_timeout_and_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("DISCONNECT_TIMEOUT", "5", TRUE);
	g_setenv("DISCONNECT_ERROR", "2", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 0, NULL);
	fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_member_disconnect_errors == 3, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successfull == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (get_changes_disconnect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	g_setenv("DISCONNECT_TIMEOUT", "1", TRUE);
	g_setenv("DISCONNECT_ERROR", "2", TRUE);
	g_setenv("GET_CHANGES_TIMEOUT", "6", TRUE);
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_configdir(osync, NULL);
	osync_env_initialize(osync, NULL);
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, (void *)3);
	osync_engine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 0, NULL);
	fail_unless(num_connected == 3, NULL);
	fail_unless(num_disconnected == 1, NULL);
	fail_unless(num_member_sent_changes == 1, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_member_disconnect_errors == 2, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osync_engine_finalize(engine);
	mark_point();
	osync_engine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *multisync_suite(void)
{
	Suite *s = suite_create("Error Codes");
	//Suite *s2 = suite_create("Error Codes");
	create_case(s, "single_init_error", single_init_error);
	create_case(s, "dual_connect_error", dual_connect_error);
	create_case(s, "one_of_two_connect_error", one_of_two_connect_error);
	create_case(s, "two_of_three_connect_error", two_of_three_connect_error);
	create_case(s, "two_of_three_connect_error2", two_of_three_connect_error2);
	create_case(s, "three_of_three_connect_error", three_of_three_connect_error);
	create_case(s, "one_of_three_connect_error", one_of_three_connect_error);
	create_case(s, "no_connect_error", no_connect_error);
	create_case(s, "single_connect_timeout", single_connect_timeout);
	create_case(s, "dual_connect_timeout", dual_connect_timeout);
	create_case(s, "one_of_three_timeout", one_of_three_timeout);
	create_case(s, "timeout_and_error", timeout_and_error);
	create_case(s, "single_get_changes_error", single_get_changes_error);
	create_case(s, "dual_get_changes_error", dual_get_changes_error);
	create_case(s, "two_of_three_get_changes_error", two_of_three_get_changes_error);
	create_case(s, "one_of_three_get_changes_error", one_of_three_get_changes_error);
	create_case(s, "one_of_three_get_changes_timeout", one_of_three_get_changes_timeout);
	create_case(s, "get_changes_timeout_and_error", get_changes_timeout_and_error);
	create_case(s, "get_changes_timeout_sleep", get_changes_timeout_sleep);
	create_case(s, "single_commit_error", single_commit_error);
	create_case(s, "dual_commit_error", dual_commit_error);
	create_case(s, "single_commit_timeout", single_commit_timeout);
	create_case(s, "dual_commit_timeout", dual_commit_timeout);
	create_case(s, "commit_timeout_and_error", commit_timeout_and_error);
	create_case(s, "commit_timeout_and_error2", commit_timeout_and_error2);
	create_case(s, "commit_error_modify", commit_error_modify);
	create_case(s, "commit_error_delete", commit_error_delete);
	create_case(s, "single_sync_done_error", single_sync_done_error);
	create_case(s, "dual_sync_done_error", dual_sync_done_error);
	create_case(s, "triple_sync_done_error", triple_sync_done_error);
	create_case(s, "single_sync_done_timeout", single_sync_done_timeout);
	create_case(s, "dual_sync_done_timeout", dual_sync_done_timeout);
	create_case(s, "sync_done_timeout_and_error", sync_done_timeout_and_error);
	create_case(s, "single_disconnect_error", single_disconnect_error);
	create_case(s, "dual_disconnect_error", dual_disconnect_error);
	create_case(s, "triple_disconnect_error", triple_disconnect_error);
	create_case(s, "single_disconnect_timeout", single_disconnect_timeout);
	create_case(s, "dual_disconnect_timeout", dual_disconnect_timeout);
	create_case(s, "disconnect_timeout_and_error", disconnect_timeout_and_error);
	create_case(s, "get_changes_disconnect_error", get_changes_disconnect_error);
	
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
