#include <check.h>
#include "opensync.h"
#include "engine.h"
#include "engine_internals.h"

char *olddir = NULL;

char *setup_testbed(char *fkt_name)
{
	setuid(65534);
	char *testbed = g_strdup_printf("%s/testbed.XXXXXX", g_get_tmp_dir());
	mkdtemp(testbed);
	char *command = g_strdup_printf("cp -a data/%s/* %s", fkt_name, testbed);
	if (system(command))
		abort();
	olddir = g_get_current_dir();
	if (chdir(testbed))
		abort();
	g_free(command);
	printf("Seting up %s at %s\n", fkt_name, testbed);
	return testbed;
}

void destroy_testbed(char *path)
{
	char *command = g_strdup_printf("rm -rf %s", path);
	if (olddir)
		chdir(olddir);
	system(command);
	g_free(command);
	printf("Tearing down %s\n", path);
	g_free(path);
}

int num_conflicts;
int num_written;
int num_read;

void conflict_handler(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	
	fail_unless(osync_mapping_num_entries(mapping) == 2, NULL);

	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	osengine_mapping_solve(engine, mapping, change);
}

void conflict_handler_duplication(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	
	fail_unless(osync_mapping_num_entries(mapping) == 2, NULL);
	
	osync_mapping_duplicate(engine, mapping);
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

START_TEST (multisync_easy_new)
{
	char *testbed = setup_testbed("multisync_easy_new");
	OSyncEnv *osync = osync_env_new();
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
	mark_point();
	fail_unless(osync_engine_synchronize(engine, &error), NULL);
	mark_point();
	
	osync_engine_wait_sync_end(engine);
	osync_engine_finalize(engine);
	
	char *uid;
    char *hash;

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	fail_unless(num_written == 2, NULL);
	fail_unless(num_read == 1, NULL);
	
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
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (multisync_easy_new_partial)
{
	char *testbed = setup_testbed("multisync_easy_new_partial");
	OSyncEnv *osync = osync_env_new();
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
	mark_point();
	fail_unless(osync_engine_synchronize(engine, &error), NULL);
	mark_point();
	
	osync_engine_wait_sync_end(engine);
	
	char *uid;
    char *hash;

	fail_unless(num_written == 1, NULL);
	fail_unless(num_read == 2, NULL);

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
	
	system("rm -f data2/testdata");
	
	mark_point();
	num_conflicts = 0;
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	mark_point();
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


START_TEST (multisync_easy_new_partial2)
{
	char *testbed = setup_testbed("multisync_easy_new_partial2");
	OSyncEnv *osync = osync_env_new();
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
	mark_point();
	fail_unless(osync_engine_synchronize(engine, &error), NULL);
	mark_point();
	
	osync_engine_wait_sync_end(engine);
	
	char *uid;
    char *hash;

	fail_unless(num_written == 0, NULL);
	fail_unless(num_read == 3, NULL);

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
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	mark_point();
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
	TCase *tc_setup = tcase_create("setup");
	//TCase *tc_setup2 = tcase_create("setup2");
	suite_add_tcase (s, tc_setup);
	tcase_add_test(tc_setup, multisync_easy_new);
	tcase_add_test(tc_setup, multisync_easy_new_partial);
	tcase_add_test(tc_setup, multisync_easy_new_partial2);
	
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
