#include "support.h"

#include <opensync/opensync-group.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-engine.h>
#include <opensync/opensync-mapping.h>

START_TEST (sync_setup_connect)
{
	char *testbed = setup_testbed("sync");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	//fail_unless(num_engine_end_conflicts == 0, NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_new)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 1, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	//fail_unless(num_mapping_written == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 1);
	g_free(path);
	check_mapping(maptable, 1, 1, 2, "testdata");
	check_mapping(maptable, 2, 1, 2, "testdata");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_new_del)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 1);
	g_free(path);
	check_mapping(maptable, 1, 1, 2, "testdata");
	check_mapping(maptable, 2, 1, 2, "testdata");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);
	
	reset_counters();
	system("rm data1/testdata");

	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 1, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	//fail_unless(num_mapping_written == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	maptable = mappingtable_load(path, "mockobjtype1", 0);
	g_free(path);
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_conflict)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	system("cp testdata comp_data");
	system("cp new_data1 data2/testdata");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(2));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 2, NULL);
	fail_unless(num_change_written == 1, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	//fail_unless(num_mapping_written == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1/testdata comp_data)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data2/testdata comp_data)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 1);
	g_free(path);
	check_mapping(maptable, 1, 1, 2, "testdata");
	check_mapping(maptable, 2, 1, 2, "testdata");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_new_mapping)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	system("cp testdata data2/testdata");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(2));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 2, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	//fail_unless(num_mapping_written == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 1);
	g_free(path);
	check_mapping(maptable, 1, 1, 2, "testdata");
	check_mapping(maptable, 2, 1, 2, "testdata");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);
	
	reset_counters();
	system("rm data1/testdata");
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 1, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	//fail_unless(num_mapping_written == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	maptable = mappingtable_load(path, "mockobjtype1", 0);
	g_free(path);
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_conflict_duplicate)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	system("cp new_data1 data2/testdata");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_duplicate, GINT_TO_POINTER(2));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 2, NULL);
	fail_unless(num_change_written == 3, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	//fail_unless(num_mapping_written == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 2);
	g_free(path);
	check_mapping(maptable, 1, -1, 2, "testdata");
	check_mapping(maptable, 1, -1, 2, "testdata-dupe");
	check_mapping(maptable, 2, -1, 2, "testdata");
	check_mapping(maptable, 1, -1, 2, "testdata-dupe");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 2);
	g_free(path);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 2);
	g_free(path);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
	osync_hashtable_free(table);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	system("rm -f data1/testdata-dupe");
	
	reset_counters();
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 1, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	//fail_unless(num_mapping_written == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);


	path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	maptable = mappingtable_load(path, "mockobjtype1", 1);
	g_free(path);
	check_mapping(maptable, 1, -1, 2, "testdata");
	check_mapping(maptable, 2, -1, 2, "testdata");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);
	
	destroy_testbed(testbed);
}
END_TEST

#if 0
START_TEST (sync_conflict_duplicate)
{
	char *testbed = setup_testbed("sync_conflict_duplicate");
	num_conflicts = 0;
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);

	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_conflict_callback(engine, conflict_handler_duplication, GINT_TO_POINTER(2));
	osengine_init(engine, &error);

	synchronize_once(engine, NULL);
	
	system("diff -x \".*\" data1 data2");
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 3, 0);
	check_mapping(maptable, 1, -1, 2, "testdata");
	check_mapping(maptable, 1, -1, 2, "testdata-dupe");
	check_mapping(maptable, 1, -1, 2, "testdata-dupe-dupe");
	check_mapping(maptable, 2, -1, 2, "testdata");
	check_mapping(maptable, 2, -1, 2, "testdata-dupe");
	check_mapping(maptable, 2, -1, 2, "testdata-dupe-dupe");
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
	osengine_finalize(engine);
	osengine_free(engine);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	
	maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, 0, 2, "testdata");
	check_mapping(maptable, 2, 0, 2, "testdata");
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
#endif

START_TEST (sync_conflict_duplicate2)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	system("cp testdata comp_data");
	system("cp new_data1 data2/testdata");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_duplicate, GINT_TO_POINTER(2));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 2, NULL);
	fail_unless(num_change_written == 3, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	//fail_unless(num_mapping_written == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 2);
	g_free(path);
	check_mapping(maptable, 1, -1, 2, "testdata");
	check_mapping(maptable, 1, -1, 2, "testdata-dupe");
	check_mapping(maptable, 2, -1, 2, "testdata");
	check_mapping(maptable, 1, -1, 2, "testdata-dupe");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 2);
	g_free(path);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 2);
	g_free(path);
    check_hash(table, "testdata");
    check_hash(table, "testdata-dupe");
	osync_hashtable_free(table);
	
	
	system("rm -f data1/testdata");
	system("rm -f data2/testdata-dupe");
	sleep(2);
	system("cp new_data1 data2/testdata");
	system("cp new_data1 comp_data");
	
	reset_counters();
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 3, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 2, NULL);
	//fail_unless(num_mapping_written == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1/testdata comp_data)\" = \"x\""), NULL);
	
	path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	maptable = mappingtable_load(path, "mockobjtype1", 1);
	g_free(path);
	check_mapping(maptable, 1, -1, 2, "testdata");
	check_mapping(maptable, 2, -1, 2, "testdata");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_conflict_delay)
{
	char *testbed = setup_testbed("sync");
	create_random_file("data1/testdata1");
	create_random_file("data1/testdata2");
	create_random_file("data1/testdata3");
	
	create_random_file("data2/testdata1");
	create_random_file("data2/testdata2");
	create_random_file("data2/testdata3");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_delay, GINT_TO_POINTER(2));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 6, NULL);
	fail_unless(num_change_written == 3, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 3, NULL);
	//fail_unless(num_mapping_written == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 3, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 3);
	g_free(path);
	check_mapping(maptable, 1, -1, 2, "testdata1");
	check_mapping(maptable, 2, -1, 2, "testdata1");
	check_mapping(maptable, 1, -1, 2, "testdata2");
	check_mapping(maptable, 2, -1, 2, "testdata2");
	check_mapping(maptable, 1, -1, 2, "testdata3");
	check_mapping(maptable, 2, -1, 2, "testdata3");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 3);
	g_free(path);
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
    check_hash(table, "testdata3");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 3);
	g_free(path);
    check_hash(table, "testdata1");
    check_hash(table, "testdata2");
    check_hash(table, "testdata3");
	osync_hashtable_free(table);
	
	system("rm -f data1/testdata1");
	system("rm -f data2/testdata2");
	system("rm -f data1/testdata3");
	
	reset_counters();
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 3, NULL);
	fail_unless(num_change_written == 3, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 3, NULL);
	//fail_unless(num_mapping_written == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	maptable = mappingtable_load(path, "mockobjtype1", 0);
	g_free(path);
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_conflict_deldel)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(2));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	reset_counters();
	system("rm data1/testdata");
	system("rm data2/testdata");
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 2, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 0);
	g_free(path);
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_moddel)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(2));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	reset_counters();
	
	sleep(2);
	system("cp new_data1 data1/testdata");
	system("cp new_data2 data2/testdata");
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 2, NULL);
	fail_unless(num_change_written == 1, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 1);
	g_free(path);
	check_mapping(maptable, 1, 1, 2, "testdata");
	check_mapping(maptable, 2, 1, 2, "testdata");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);
	
	reset_counters();
	
	system("rm data2/testdata");
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 1, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	maptable = mappingtable_load(path, "mockobjtype1", 0);
	g_free(path);
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_conflict_moddel)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_deleted, GINT_TO_POINTER(2));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	reset_counters();
	
	sleep(2);
	system("cp new_data2 data1/testdata");
	system("rm -f data2/testdata");
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 2, NULL);
	fail_unless(num_change_written == 1, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 0);
	g_free(path);
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_dualdel)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	system("cp new_data1 data1/testdata2");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_deleted, GINT_TO_POINTER(2));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 2, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 2, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 2);
	g_free(path);

	check_mapping(maptable, 2, -1, 2, "testdata2");
	check_mapping(maptable, 1, -1, 2, "testdata2");
	check_mapping(maptable, 2, -1, 2, "testdata");
	check_mapping(maptable, 1, -1, 2, "testdata");

    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 2);
	g_free(path);
    check_hash(table, "testdata");
    check_hash(table, "testdata2");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 2);
	g_free(path);
    check_hash(table, "testdata");
    check_hash(table, "testdata2");
	osync_hashtable_free(table);
	
	reset_counters();
	
	sleep(2);
	system("rm -f data1/testdata");
	system("rm -f data1/testdata2");
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 2, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 2, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	maptable = mappingtable_load(path, "mockobjtype1", 0);
	g_free(path);
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_large)
{
	char *testbed = setup_testbed("sync");
	
	create_random_file("data1/file1");
	create_random_file("data1/file2");
	create_random_file("data1/file4");
	create_random_file("data1/file5");
	create_random_file("data1/file9");
	create_random_file("data1/file10");
	
	system("cp data1/file2 data2/file2");
	create_random_file("data2/file3");
	create_random_file("data2/file4");
	system("cp data1/file5 data2/file5");
	create_random_file("data2/file6");
	create_random_file("data2/file7");
	create_random_file("data2/file8");
	create_random_file("data2/file10");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(2));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 14, NULL);
	fail_unless(num_change_written == 8, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 10, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 2, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 10);
	g_free(path);
	check_mapping(maptable, 1, -1, 2, "file1");
	check_mapping(maptable, 2, -1, 2, "file1");
	check_mapping(maptable, 1, -1, 2, "file2");
	check_mapping(maptable, 2, -1, 2, "file2");
	check_mapping(maptable, 1, -1, 2, "file3");
	check_mapping(maptable, 2, -1, 2, "file3");
	check_mapping(maptable, 1, -1, 2, "file4");
	check_mapping(maptable, 2, -1, 2, "file4");
	check_mapping(maptable, 1, -1, 2, "file5");
	check_mapping(maptable, 2, -1, 2, "file5");
	check_mapping(maptable, 1, -1, 2, "file6");
	check_mapping(maptable, 2, -1, 2, "file6");
	check_mapping(maptable, 1, -1, 2, "file7");
	check_mapping(maptable, 2, -1, 2, "file7");
	check_mapping(maptable, 1, -1, 2, "file8");
	check_mapping(maptable, 2, -1, 2, "file8");
	check_mapping(maptable, 1, -1, 2, "file9");
	check_mapping(maptable, 2, -1, 2, "file9");
	check_mapping(maptable, 1, -1, 2, "file10");
	check_mapping(maptable, 2, -1, 2, "file10");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 10);
	g_free(path);
    check_hash(table, "file1");
    check_hash(table, "file2");
    check_hash(table, "file3");
    check_hash(table, "file4");
    check_hash(table, "file5");
    check_hash(table, "file6");
    check_hash(table, "file7");
    check_hash(table, "file8");
    check_hash(table, "file9");
    check_hash(table, "file10");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 10);
	g_free(path);
    check_hash(table, "file1");
    check_hash(table, "file2");
    check_hash(table, "file3");
    check_hash(table, "file4");
    check_hash(table, "file5");
    check_hash(table, "file6");
    check_hash(table, "file7");
    check_hash(table, "file8");
    check_hash(table, "file9");
    check_hash(table, "file10");
	osync_hashtable_free(table);
	
	reset_counters();
	
	sleep(2);
	//Add left
	create_random_file("data1/file11");
	//Add right
	create_random_file("data2/file12");
	//Modify left
	create_random_file("data1/file1");
	//Modify right
	create_random_file("data2/file2");
	//Delete left
	system("rm -f data1/file3");
	//Delete right
	system("rm -f data2/file4");
	//Add left, right, same
	create_random_file("data1/file13");
	system("cp data1/file13 data2/file13");
	//Add left, right, conflict
	create_random_file("data1/file14");
	create_random_file("data2/file14");
	//Modify left, right, same
	create_random_file("data1/file5");
	system("cp data1/file5 data2/file5");
	//Modify left, right, conflict
	create_random_file("data1/file6");
	create_random_file("data2/file6");
	//Delete left, right
	system("rm -f data1/file7");
	system("rm -f data2/file7");
	//delete left, modify right
	system("rm -f data1/file8");
	create_random_file("data2/file8");
	//modify left, delete right
	create_random_file("data1/file9");
	system("rm -f data2/file9");
	
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 20, NULL);
	fail_unless(num_change_written == 10, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 13, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 4, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	maptable = mappingtable_load(path, "mockobjtype1", 10);
	g_free(path);
	check_mapping(maptable, 1, -1, 2, "file1");
	check_mapping(maptable, 2, -1, 2, "file1");
	check_mapping(maptable, 1, -1, 2, "file2");
	check_mapping(maptable, 2, -1, 2, "file2");
	check_mapping(maptable, 1, -1, 2, "file5");
	check_mapping(maptable, 2, -1, 2, "file5");
	check_mapping(maptable, 1, -1, 2, "file6");
	check_mapping(maptable, 2, -1, 2, "file6");
	check_mapping(maptable, 1, -1, 2, "file9");
	check_mapping(maptable, 2, -1, 2, "file9");
	check_mapping(maptable, 1, -1, 2, "file10");
	check_mapping(maptable, 2, -1, 2, "file10");
	check_mapping(maptable, 1, -1, 2, "file11");
	check_mapping(maptable, 2, -1, 2, "file11");
	check_mapping(maptable, 1, -1, 2, "file12");
	check_mapping(maptable, 2, -1, 2, "file12");
	check_mapping(maptable, 1, -1, 2, "file13");
	check_mapping(maptable, 2, -1, 2, "file13");
	check_mapping(maptable, 1, -1, 2, "file14");
	check_mapping(maptable, 2, -1, 2, "file14");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 10);
	g_free(path);
    check_hash(table, "file1");
    check_hash(table, "file2");
    check_hash(table, "file5");
    check_hash(table, "file6");
    check_hash(table, "file9");
    check_hash(table, "file10");
    check_hash(table, "file11");
    check_hash(table, "file12");
    check_hash(table, "file13");
    check_hash(table, "file14");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 10);
	g_free(path);
    check_hash(table, "file1");
    check_hash(table, "file2");
    check_hash(table, "file5");
    check_hash(table, "file6");
    check_hash(table, "file9");
    check_hash(table, "file10");
    check_hash(table, "file11");
    check_hash(table, "file12");
    check_hash(table, "file13");
    check_hash(table, "file14");
	osync_hashtable_free(table);
	
	
	reset_counters();
	
	system("rm -f data1/file1");
	system("rm -f data1/file2");
	system("rm -f data1/file10");
	system("rm -f data1/file11");
	system("rm -f data1/file12");
	system("rm -f data1/file13");
	system("rm -f data1/file14");
	
	
	system("rm -f data2/file5");
	system("rm -f data2/file6");
	system("rm -f data2/file9");
	system("rm -f data2/file10");
	system("rm -f data2/file11");
	system("rm -f data2/file12");
	system("rm -f data2/file13");
	system("rm -f data2/file14");
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 15, NULL);
	fail_unless(num_change_written == 5, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 10, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	maptable = mappingtable_load(path, "mockobjtype1", 0);
	g_free(path);
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 0);
	g_free(path);
	osync_hashtable_free(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

/* We want to detect a single objtype "mockobjtype1"
 * 
 * - First we send the config to the plugin
 * - Then the plugin will report the objtypes
 */
START_TEST (sync_detect_obj)
{
	char *testbed = setup_testbed("sync_multi");
	
	system("mkdir file-1");
	system("mkdir file2-1");
	system("mkdir file3-1");
	
	system("mkdir file-2");
	system("mkdir file2-2");
	system("mkdir file3-2");
	
	create_random_file("file-1/file1");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncMember *member1 = osync_group_nth_member(group, 0);
	osync_member_set_config(member1, "<config><directory><path>file-1</path><objtype>mockobjtype1</objtype><objformat>mockformat1</objformat></directory><directory><path>file2-1</path><objtype>mockobjtype2</objtype><objformat>mockformat2</objformat></directory><directory><path>file3-1</path><objtype>mockobjtype3</objtype><objformat>mockformat3</objformat></directory></config>");
	OSyncMember *member2 = osync_group_nth_member(group, 1);
	osync_member_set_config(member2, "<config><directory><path>file-2</path><objtype>mockobjtype1</objtype><objformat>mockformat1</objformat></directory><directory><path>file2-2</path><objtype>mockobjtype2</objtype><objformat>mockformat2</objformat></directory><directory><path>file3-2</path><objtype>mockobjtype3</objtype><objformat>mockformat3</objformat></directory></config>");
	
	/* Check that we dont have any discovered objtypes */
	fail_unless(osync_member_num_objtypes(member1) == 0, NULL);
	fail_unless(osync_member_num_objtypes(member2) == 0, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(2));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	/* Discover the objtypes for the members */
	fail_unless(osync_engine_discover_and_block(engine, member1, &error), NULL);
	fail_unless(osync_member_num_objtypes(member1) == 3, NULL);
	
	/* Client checks */
	fail_unless(num_client_connected == 0, NULL);
	fail_unless(num_client_main_connected == 0, NULL);
	fail_unless(num_client_read == 0, NULL);
	fail_unless(num_client_main_read == 0, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_client_main_written == 0, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	fail_unless(num_client_main_disconnected == 0, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 0, NULL);
	fail_unless(num_client_main_sync_done == 0, NULL);
	fail_unless(num_client_discovered == 1, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 0, NULL);
	fail_unless(num_engine_written == 0, NULL);
	fail_unless(num_engine_sync_done == 0, NULL);
	fail_unless(num_engine_disconnected == 0, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 0, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	
	reset_counters();
	fail_unless(osync_engine_discover_and_block(engine, member2, &error), NULL);
	fail_unless(osync_member_num_objtypes(member2) == 3, NULL);
	
	/* Client checks */
	fail_unless(num_client_connected == 0, NULL);
	fail_unless(num_client_main_connected == 0, NULL);
	fail_unless(num_client_read == 0, NULL);
	fail_unless(num_client_main_read == 0, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_client_main_written == 0, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	fail_unless(num_client_main_disconnected == 0, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 0, NULL);
	fail_unless(num_client_main_sync_done == 0, NULL);
	fail_unless(num_client_discovered == 1, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 0, NULL);
	fail_unless(num_engine_written == 0, NULL);
	fail_unless(num_engine_sync_done == 0, NULL);
	fail_unless(num_engine_disconnected == 0, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 0, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	
	fail_unless(osync_group_num_objtypes(group) == 3, NULL);
	
	reset_counters();
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 6, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 6, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 6, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 6, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 6, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	fail_unless(num_client_discovered == 0, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 1, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" file-1 file-2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 1);
	g_free(path);
	check_mapping(maptable, 1, -1, 2, "file1");
	check_mapping(maptable, 2, -1, 2, "file1");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "file1");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "file1");
	osync_hashtable_free(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_detect_obj2)
{
	char *testbed = setup_testbed("sync_multi");
	
	system("mkdir file-1");
	system("mkdir file2-1");
	system("mkdir file3-1");
	
	system("mkdir file-2");
	system("mkdir file2-2");
	system("mkdir file3-2");
	
	create_random_file("file-1/file1");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncMember *member1 = osync_group_nth_member(group, 0);
	osync_member_set_config(member1, "<config><directory><path>file-1</path><objtype>mockobjtype1</objtype><objformat>mockformat1</objformat></directory><directory><path>file2-1</path><objtype>mockobjtype2</objtype><objformat>mockformat2</objformat></directory><directory><path>file3-1</path><objtype>mockobjtype3</objtype><objformat>mockformat3</objformat></directory></config>");
	OSyncMember *member2 = osync_group_nth_member(group, 1);
	osync_member_set_config(member2, "<config><directory><path>file-2</path><objtype>mockobjtype1</objtype><objformat>mockformat1</objformat></directory></config>");
	
	/* Check that we dont have any discovered objtypes */
	fail_unless(osync_member_num_objtypes(member1) == 0, NULL);
	fail_unless(osync_member_num_objtypes(member2) == 0, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(2));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	/* Discover the objtypes for the members */
	fail_unless(osync_engine_discover_and_block(engine, member1, &error), NULL);
	fail_unless(osync_member_num_objtypes(member1) == 3, NULL);
	
	/* Client checks */
	fail_unless(num_client_connected == 0, NULL);
	fail_unless(num_client_main_connected == 0, NULL);
	fail_unless(num_client_read == 0, NULL);
	fail_unless(num_client_main_read == 0, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_client_main_written == 0, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	fail_unless(num_client_main_disconnected == 0, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 0, NULL);
	fail_unless(num_client_main_sync_done == 0, NULL);
	fail_unless(num_client_discovered == 1, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 0, NULL);
	fail_unless(num_engine_written == 0, NULL);
	fail_unless(num_engine_sync_done == 0, NULL);
	fail_unless(num_engine_disconnected == 0, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 0, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	
	reset_counters();
	fail_unless(osync_engine_discover_and_block(engine, member2, &error), NULL);
	fail_unless(osync_member_num_objtypes(member2) == 1, NULL);
	
	/* Client checks */
	fail_unless(num_client_connected == 0, NULL);
	fail_unless(num_client_main_connected == 0, NULL);
	fail_unless(num_client_read == 0, NULL);
	fail_unless(num_client_main_read == 0, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_client_main_written == 0, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	fail_unless(num_client_main_disconnected == 0, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 0, NULL);
	fail_unless(num_client_main_sync_done == 0, NULL);
	fail_unless(num_client_discovered == 1, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 0, NULL);
	fail_unless(num_engine_written == 0, NULL);
	fail_unless(num_engine_sync_done == 0, NULL);
	fail_unless(num_engine_disconnected == 0, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 0, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	
	fail_unless(osync_group_num_objtypes(group) == 1, NULL);
	
	reset_counters();
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	fail_unless(num_client_discovered == 0, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 1, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" file-1 file-2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 1);
	g_free(path);
	check_mapping(maptable, 1, -1, 2, "file1");
	check_mapping(maptable, 2, -1, 2, "file1");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "file1");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "file1");
	osync_hashtable_free(table);
	
	destroy_testbed(testbed);
}
END_TEST

/* sync_slowsync_connect is intendet to check if a slow-sync got triggered
   for all members/sinks when the slow-sync got requested during the connect()
   function of one of the members (e.g. anchor mismatch). Bug Ticket: #538 */
START_TEST (sync_slowsync_connect)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 1, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	//fail_unless(num_mapping_written == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	char *path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	OSyncMappingTable *maptable = mappingtable_load(path, "mockobjtype1", 1);
	g_free(path);
	check_mapping(maptable, 1, 1, 2, "testdata");
	check_mapping(maptable, 2, 1, 2, "testdata");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    OSyncHashTable *table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);

	/* 2nd Sync with new path config for #1 member - this is needed
	 * to trigger a slow-sync within the connect() plugin call. */
	reset_counters();
	system("mkdir data3");
	system("cp configs/new_path.conf configs/group/1/mock-sync.conf");
	
	group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	/* Client checks */
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_main_read == 2, NULL);
	fail_unless(num_client_written == 2, NULL);
	fail_unless(num_client_main_written == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_sync_done == 2, NULL);
	fail_unless(num_client_main_sync_done == 2, NULL);
	
	/* Client checks */
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_written == 1, NULL);
	fail_unless(num_engine_sync_done == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 1, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 1, NULL);
	//fail_unless(num_mapping_written == 1, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data3 data2)\" = \"x\""), NULL);
	
	path = g_strdup_printf("%s/configs/group/archive.db", testbed);
	maptable = mappingtable_load(path, "mockobjtype1", 1);
	g_free(path);
	check_mapping(maptable, 1, 1, 2, "testdata");
	check_mapping(maptable, 2, 1, 2, "testdata");
    osync_mapping_table_close(maptable);
    osync_mapping_table_unref(maptable);
    
	path = g_strdup_printf("%s/configs/group/1/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);

	path = g_strdup_printf("%s/configs/group/2/hashtable.db", testbed);
    table = hashtable_load(path, "mockobjtype1", 1);
	g_free(path);
    check_hash(table, "testdata");
	osync_hashtable_free(table);

	destroy_testbed(testbed);
}
END_TEST

Suite *env_suite(void)
{
	Suite *s = suite_create("Sync");
//	Suite *s2 = suite_create("Sync");
//	Suite *s3 = suite_create("Sync"); // really broken...

	create_case(s, "sync_setup_connect", sync_setup_connect);
	create_case(s, "sync_easy_new", sync_easy_new);
	create_case(s, "sync_easy_new_del", sync_easy_new_del);
	create_case(s, "sync_easy_conflict", sync_easy_conflict);
	create_case(s, "sync_easy_new_mapping", sync_easy_new_mapping);
	create_case(s, "sync_easy_conflict_duplicate", sync_easy_conflict_duplicate);
	create_case(s, "sync_conflict_duplicate2", sync_conflict_duplicate2);
	create_case(s, "sync_conflict_delay", sync_conflict_delay);
	create_case(s, "sync_conflict_deldel", sync_conflict_deldel);
	create_case(s, "sync_moddel", sync_moddel);
	create_case(s, "sync_conflict_moddel", sync_conflict_moddel);
	create_case(s, "sync_easy_dualdel", sync_easy_dualdel);
	create_case(s, "sync_large", sync_large);

	create_case(s, "sync_detect_obj", sync_detect_obj);
	create_case(s, "sync_detect_obj2", sync_detect_obj2);

	create_case(s, "sync_slowsync_connect", sync_slowsync_connect);

	//stateless sync
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = env_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
