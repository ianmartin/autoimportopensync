/*
not working tests disabled
*/
#include "support.h"


START_TEST (single_connect_timeout)
{
	char *testbed = setup_testbed("sync_easy_new");
	
	setenv("CONNECT_TIMEOUT", "2", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 1, NULL);
	fail_unless(num_connected == 1, NULL);
	fail_unless(num_disconnected == 1, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osengine_finalize(engine);
	osengine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_connect_timeout)
{
	char *testbed = setup_testbed("sync_easy_new");
	
	setenv("CONNECT_TIMEOUT", "3", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 2, NULL);
	fail_unless(num_connected == 0, NULL);
	fail_unless(num_disconnected == 0, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osengine_finalize(engine);
	osengine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("CONNECT_TIMEOUT", "2", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 1, NULL);
	fail_unless(num_connected == 2, NULL);
	fail_unless(num_disconnected == 2, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osengine_finalize(engine);
	osengine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (timeout_and_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("CONNECT_TIMEOUT", "2", TRUE);
	setenv("CONNECT_ERROR", "4", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_member_connect_errors == 2, NULL);
	fail_unless(num_connected == 1, NULL);
	fail_unless(num_disconnected == 1, NULL);
	fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successfull == 0, NULL);
	
	osync_error_free(&error);
	osengine_finalize(engine);
	osengine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_get_changes_timeout)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	setenv("GET_CHANGES_TIMEOUT", "1", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
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
	osengine_finalize(engine);
	osengine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (get_changes_timeout_and_error)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	setenv("GET_CHANGES_TIMEOUT", "3", TRUE);
	setenv("GET_CHANGES_ERROR", "4", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
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
	osengine_finalize(engine);
	osengine_free(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (get_changes_timeout_sleep)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	setenv("GET_CHANGES_TIMEOUT2", "7", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osengine_finalize(engine);
	mark_point();
	osengine_free(engine);
	
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

START_TEST (single_commit_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("COMMIT_TIMEOUT", "4", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osengine_finalize(engine);
	mark_point();
	osengine_free(engine);
	
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
	
	setenv("COMMIT_TIMEOUT", "6", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osengine_finalize(engine);
	mark_point();
	osengine_free(engine);
	
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
	
	setenv("COMMIT_TIMEOUT", "4", TRUE);
	setenv("COMMIT_ERROR", "2", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osengine_finalize(engine);
	mark_point();
	osengine_free(engine);
	
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
	
	setenv("COMMIT_TIMEOUT", "2", TRUE);
	setenv("COMMIT_ERROR", "4", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osengine_finalize(engine);
	mark_point();
	osengine_free(engine);
	
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

START_TEST (single_sync_done_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("SYNC_DONE_TIMEOUT", "4", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osengine_finalize(engine);
	mark_point();
	osengine_free(engine);
	
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
	
	setenv("SYNC_DONE_TIMEOUT", "6", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osengine_finalize(engine);
	mark_point();
	osengine_free(engine);
	
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
	
	setenv("SYNC_DONE_TIMEOUT", "5", TRUE);
	setenv("SYNC_DONE_ERROR", "2", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osengine_finalize(engine);
	mark_point();
	osengine_free(engine);
	
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

START_TEST (single_disconnect_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_TIMEOUT", "4", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osengine_finalize(engine);
	mark_point();
	osengine_free(engine);
	
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
	
	setenv("DISCONNECT_TIMEOUT", "6", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osengine_finalize(engine);
	mark_point();
	osengine_free(engine);
	
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
	
	setenv("DISCONNECT_TIMEOUT", "5", TRUE);
	setenv("DISCONNECT_ERROR", "2", TRUE);
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	osengine_init(engine, &error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_free(&error);
	mark_point();
	osengine_finalize(engine);
	mark_point();
	osengine_free(engine);
	
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

Suite *multisync_suite(void)
{
	Suite *s = suite_create("Error Codes");
	//Suite *s2 = suite_create("Error Codes");
	create_case(s, "single_connect_timeout", single_connect_timeout);
	create_case(s, "dual_connect_timeout", dual_connect_timeout);
	create_case(s, "one_of_three_timeout", one_of_three_timeout);
	create_case(s, "timeout_and_error", timeout_and_error);
	create_case(s, "one_of_three_get_changes_timeout", one_of_three_get_changes_timeout);
	create_case(s, "get_changes_timeout_and_error", get_changes_timeout_and_error);
	create_case(s, "get_changes_timeout_sleep", get_changes_timeout_sleep);
	create_case(s, "single_commit_timeout", single_commit_timeout);
	create_case(s, "dual_commit_timeout", dual_commit_timeout);
	create_case(s, "commit_timeout_and_error", commit_timeout_and_error);
	create_case(s, "commit_timeout_and_error2", commit_timeout_and_error2);
	create_case(s, "single_sync_done_timeout", single_sync_done_timeout);
	create_case(s, "dual_sync_done_timeout", dual_sync_done_timeout);
	create_case(s, "sync_done_timeout_and_error", sync_done_timeout_and_error);
	create_case(s, "single_disconnect_timeout", single_disconnect_timeout);
	create_case(s, "dual_disconnect_timeout", dual_disconnect_timeout);
	create_case(s, "disconnect_timeout_and_error", disconnect_timeout_and_error);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = multisync_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_set_fork_status (sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
