#include "support.h"

#include "opensync/group/opensync_group_internals.h"

START_TEST (single_init_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("INIT_NULL", "2", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	/* this is a bug - please see sml_fail_unless for details */
	fail_unless(osync_engine_initialize(engine, &error), osync_error_print(&error));
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_connect_error)
{
	char *testbed = setup_testbed("sync_easy_new");
	
	setenv("CONNECT_ERROR", "3", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 0, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_two_connect_error)
{
	char *testbed = setup_testbed("sync_easy_new");
	
	setenv("CONNECT_ERROR", "1", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 1, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	//fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (two_of_three_connect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("CONNECT_ERROR", "5", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 1, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	//fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (two_of_three_connect_error2)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("CONNECT_ERROR", "6", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 1, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	//fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (three_of_three_connect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("CONNECT_ERROR", "7", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 3, NULL);
	fail_unless(num_client_connected == 0, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	//fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_connect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");

	setenv("CONNECT_ERROR", "2", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	//fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (no_connect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("CONNECT_ERROR", "0", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_connect_timeout)
{
	char *testbed = setup_testbed("sync_easy_new");
	
	setenv("CONNECT_TIMEOUT", "2", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 1, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	//fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_connect_timeout)
{
	char *testbed = setup_testbed("sync_easy_new");
	
	setenv("CONNECT_TIMEOUT", "3", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 0, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	//fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("CONNECT_TIMEOUT", "2", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	//fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (timeout_and_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("CONNECT_TIMEOUT", "2", TRUE);
	setenv("CONNECT_ERROR", "4", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 1, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	//fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_get_changes_error)
{
	char *testbed = setup_testbed("sync_easy_conflict");
	
	setenv("GET_CHANGES_ERROR", "2", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	//fail_unless(num_member_get_changes_errors == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_get_changes_error)
{
	char *testbed = setup_testbed("sync_easy_conflict");
	
	setenv("GET_CHANGES_ERROR", "3", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	//fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_client_read == 0, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (two_of_three_get_changes_error)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	setenv("GET_CHANGES_ERROR", "5", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_get_changes_error)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	setenv("GET_CHANGES_ERROR", "1", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_get_changes_errors == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_get_changes_timeout)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	setenv("GET_CHANGES_TIMEOUT", "1", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_get_changes_errors == 1, NULL);
	//fail_unless(num_member_sent_changes == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (get_changes_timeout_and_error)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	setenv("GET_CHANGES_TIMEOUT", "3", TRUE);
	setenv("GET_CHANGES_ERROR", "4", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_client_read == 0, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (get_changes_timeout_sleep)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	setenv("GET_CHANGES_TIMEOUT2", "7", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 0, NULL);
	fail_unless(num_client_read == 0, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_commit_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("COMMIT_ERROR", "4", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 1, NULL);
	//fail_unless(num_written_errors == 1, NULL);
	fail_unless(num_mapping_errors == 1, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_commit_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("COMMIT_ERROR", "6", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_written_errors == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_commit_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("COMMIT_TIMEOUT", "4", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 1, NULL);
	//fail_unless(num_written_errors == 1, NULL);
	fail_unless(num_mapping_errors == 1, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_commit_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("COMMIT_TIMEOUT", "6", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_written_errors == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
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
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_written_errors == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
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
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_written_errors == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (commit_error_modify)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	setenv("COMMIT_TIMEOUT", "2", TRUE);
	setenv("COMMIT_ERROR", "4", TRUE);
	
	sleep(2);
	
	osync_testing_system_abort("cp newdata2 data1/testdata");
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_written_errors == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data2 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (commit_error_delete)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	setenv("COMMIT_TIMEOUT", "2", TRUE);
	setenv("COMMIT_ERROR", "4", TRUE);
	
	sleep(2);
	
	osync_testing_system_abort("rm -f data1/testdata");
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_written_errors == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data2 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (committed_all_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("COMMITTED_ALL_ERROR", "3", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (committed_all_batch_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("BATCH_COMMIT", "7", TRUE);
	setenv("COMMITTED_ALL_ERROR", "3", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_sync_done_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("SYNC_DONE_ERROR", "4", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_member_sync_done_errors == 1, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_sync_done_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("SYNC_DONE_ERROR", "6", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	//fail_unless(num_member_sync_done_errors == 2, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (triple_sync_done_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("SYNC_DONE_ERROR", "7", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	//fail_unless(num_member_sync_done_errors == 3, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_sync_done_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("SYNC_DONE_TIMEOUT", "4", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	//fail_unless(num_member_sync_done_errors == 1, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_sync_done_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("SYNC_DONE_TIMEOUT", "6", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	//fail_unless(num_member_sync_done_errors == 2, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
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
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	//fail_unless(num_member_sync_done_errors == 3, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_disconnect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_ERROR", "4", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_member_sync_done_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_disconnect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_ERROR", "6", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	//fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (triple_disconnect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_ERROR", "7", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 3, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	//fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_disconnect_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_TIMEOUT", "4", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	//fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_disconnect_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_TIMEOUT", "6", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	//fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	
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
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 3, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	//fail_unless(num_member_sent_changes == 3, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 2, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	//fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (get_changes_disconnect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_TIMEOUT", "1", TRUE);
	setenv("DISCONNECT_ERROR", "2", TRUE);
	setenv("GET_CHANGES_TIMEOUT", "6", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	//fail_unless(num_member_sent_changes == 1, NULL);
	fail_unless(num_client_read == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	//fail_unless(num_written_errors == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	//fail_unless(num_conflicts == 0, NULL);
	//fail_unless(num_member_sync_done_errors == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	mark_point();
	osync_error_unref(&error);
	mark_point();
	osync_engine_finalize(engine, &error);
	mark_point();
	osync_engine_unref(engine);
	
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
	create_case(s, "committed_all_error", committed_all_error);
	create_case(s, "committed_all_batch_error", committed_all_batch_error);
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
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
