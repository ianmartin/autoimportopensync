#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <check.h>

#include <glib.h>
#include <gmodule.h>

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

START_TEST (sync_setup)
{
  char *testbed = setup_testbed("sync_setup");
  OSyncEnv *osync = osync_env_new();
  osync_env_initialize(osync);
  OSyncGroup *group = osync_group_load(osync, "configs/group");
  fail_unless(group != NULL, NULL);
  fail_unless(osync_num_groups(osync) == 1, NULL);
  destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_setup_false)
{
  char *testbed = setup_testbed("sync_setup_false");
  OSyncEnv *osync = osync_env_new();
  osync_env_initialize(osync);
  OSyncGroup *group = osync_group_load(osync, "configs/group");
  fail_unless(group == NULL, NULL);
  fail_unless(osync_num_groups(osync) == 0, NULL);
  destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_setup_init)
{
  char *testbed = setup_testbed("sync_setup_init");
  OSyncEnv *osync = osync_env_new();
  osync_env_initialize(osync);
  OSyncGroup *group = osync_group_load(osync, "configs/group");
  fail_unless(group != NULL, NULL);
  fail_unless(osync_num_groups(osync) == 1, NULL);
  destroy_testbed(testbed);
}
END_TEST

int num_connected;
int num_disconnected;

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

START_TEST (sync_setup_connect)
{
	char *testbed = setup_testbed("sync_setup_connect");
	num_connected = 0;
	num_disconnected = 0;
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync);
	OSyncGroup *group = osync_group_load(osync, "configs/group");
	OSyncError *error = NULL;
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	osync_engine_set_memberstatus_callback(engine, member_status);
	mark_point();
	osync_engine_init(engine, &error);
	mark_point();
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	mark_point();
	osync_engine_finalize(engine);
	  
	fail_unless(num_connected == 2, NULL);
	fail_unless(num_disconnected == 2, NULL);
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_init_error)
{
	char *testbed = setup_testbed("sync_init_error");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync);
	OSyncGroup *group = osync_group_load(osync, "configs/group");
	OSyncError *error = NULL;
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(error == NULL, NULL);
	osync_engine_set_memberstatus_callback(engine, member_status);
	mark_point();
	osync_engine_init(engine, &error);

	fail_unless(error != NULL, NULL);
	fail_unless(error->type == OSYNC_ERROR_MISCONFIGURATION, NULL);
	printf("message: %s\n", error->message);
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_new)
{
	char *testbed = setup_testbed("sync_easy_new");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group");
	fail_unless(group != NULL, NULL);
	fail_unless(osync_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncError *error = NULL;
  	OSyncEngine *engine = osync_engine_new(group, &error);
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
	fail_unless(osync_mapping_num_entries(mapping) == 2, NULL);
	mark_point();
	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	OSyncMember *member = osync_change_get_member(change);
	//fail_unless(osync_member_get_id(member) == 1, NULL);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	//fail_unless(osync_member_get_id(member) == 2, NULL);
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
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_new_del)
{
	char *testbed = setup_testbed("sync_easy_new_del");
	
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group");
	fail_unless(group != NULL, NULL);
	fail_unless(osync_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncError *error = NULL;
  	OSyncEngine *engine = osync_engine_new(group, &error);
  	mark_point();
  	fail_unless(engine != NULL, NULL);
	osync_engine_init(engine, &error);
	mark_point();
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	system("rm data1/testdata");
	
	mark_point();
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	osync_engine_finalize(engine);
	mark_point();
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 0, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
	mark_point();
    osync_mappingtable_close(maptable);
	
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
	
	destroy_testbed(testbed);
}
END_TEST

int num_conflicts;
int num_written;
int num_read;

void conflict_handler(OSyncEngine *engine, OSyncMapping *mapping)
{
	num_conflicts++;
	
	fail_unless(osync_mapping_num_entries(mapping) == 2, NULL);

	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	osync_mapping_set_masterentry(mapping, change);
}

void conflict_handler_duplication(OSyncEngine *engine, OSyncMapping *mapping)
{
	num_conflicts++;
	
	fail_unless(osync_mapping_num_entries(mapping) == 2, NULL);
	
	osync_mapping_duplicate(engine, mapping);
}

void entry_status(MSyncChangeUpdate *status)
{
	switch (status->type) {
		case CHANGE_RECEIVED:
			num_read++;
			break;
		case CHANGE_SENT:
			num_written++;
			break;
		default:
			printf("Unknown status\n");
	}
}

START_TEST (sync_easy_conflict)
{
	char *testbed = setup_testbed("sync_easy_conflict");
	num_conflicts = 0;
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group");
	fail_unless(group != NULL, NULL);
	fail_unless(osync_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncError *error = NULL;
  	OSyncEngine *engine = osync_engine_new(group, &error);
  	osync_engine_set_conflict_callback(engine, conflict_handler);
  	mark_point();
  	fail_unless(engine != NULL, NULL);
	osync_engine_init(engine, &error);
	mark_point();
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	osync_engine_finalize(engine);
	system("diff -x \".*\" data1 data2");
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	//system("diff -x \".*\" data1 comp_data");
	//fail_unless(!system("test \"x$(diff -x \".*\" data2 comp_data)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 1, NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_new_mapping)
{
	char *testbed = setup_testbed("sync_easy_new_mapping");
	num_conflicts = 0;
	num_written = 0;
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group");
	fail_unless(group != NULL, NULL);
	fail_unless(osync_num_groups(osync) == 1, NULL);
	mark_point();
	
	OSyncError *error = NULL;
  	OSyncEngine *engine = osync_engine_new(group, &error);
  	osync_engine_set_conflict_callback(engine, conflict_handler);
  	osync_engine_set_changestatus_callback(engine, entry_status);
  	mark_point();
  	fail_unless(engine != NULL, NULL);
	osync_engine_init(engine, &error);
	mark_point();
	
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 0, NULL);
	
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 1, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
	mark_point();
	OSyncMapping *mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 2, NULL);
	mark_point();
	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	OSyncMember *member = osync_change_get_member(change);
	//fail_unless(osync_member_get_id(member) == 1, NULL);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	//member = osync_change_get_member(change);
	//fail_unless(osync_member_get_id(member) == 2, NULL);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
    mark_point();
    osync_mappingtable_close(maptable);
    char *hash = NULL;
    char *uid = NULL;
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
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_conflict_duplicate)
{
	char *testbed = setup_testbed("sync_easy_conflict_duplicate");
	num_conflicts = 0;
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync);
	OSyncGroup *group = osync_group_load(osync, "configs/group");
	
	OSyncError *error = NULL;
  	OSyncEngine *engine = osync_engine_new(group, &error);
  	osync_engine_set_conflict_callback(engine, conflict_handler_duplication);
	osync_engine_init(engine, &error);

	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	
	fail_unless(num_conflicts == 1, NULL);
	system("diff -x \".*\" data1 data2");
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	//fail_unless(!system("test \"x$(diff -x \".*\" data1 comp_data)\" = \"x\""), NULL);
	
	mark_point();
	OSyncMappingTable *maptable = osync_mappingtable_new(group);
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 2, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
	mark_point();
	OSyncMapping *mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 2, NULL);
	mark_point();
	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	OSyncMember *member = osync_change_get_member(change);
	//fail_unless(osync_member_get_id(member) == 2, NULL);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	//fail_unless(osync_member_get_id(member) == 1, NULL);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mapping = osync_mappingtable_nth_mapping(maptable, 1);
	fail_unless(osync_mapping_num_entries(mapping) == 2, NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 0);
	member = osync_change_get_member(change);
	//fail_unless(osync_member_get_id(member) == 1, NULL);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	//fail_unless(osync_member_get_id(member) == 2, NULL);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata-dupe"), NULL);
	
    mark_point();
    osync_mappingtable_close(maptable);
    char *hash = NULL;
    char *uid = NULL;
	mark_point();
	member = osync_member_from_id(group, 1);
	OSyncHashTable *table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 2, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid) || !strcmp("testdata-dupe", uid), NULL);
	fail_unless(osync_hashtable_nth_entry(table, 1, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata-dupe", uid) || !strcmp("testdata", uid), NULL);
	
	member = osync_member_from_id(group, 2);
	table = osync_hashtable_new();
	mark_point();
    osync_hashtable_load(table, member);
    fail_unless(osync_hashtable_num_entries(table) == 2, NULL);
	fail_unless(osync_hashtable_nth_entry(table, 0, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata", uid) || !strcmp("testdata-dupe", uid), NULL);
	fail_unless(osync_hashtable_nth_entry(table, 1, &uid, &hash), NULL);
	fail_unless(!strcmp("testdata-dupe", uid) || !strcmp("testdata", uid), NULL);
	fail_unless(num_conflicts == 1, NULL);
	
	system("rm -f data1/testdata-dupe");
	num_conflicts = 0;
	
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	osync_engine_finalize(engine);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	
	mark_point();
	maptable = osync_mappingtable_new(group);
	mark_point();
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	mark_point();
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 1, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
	mark_point();
	mapping = osync_mappingtable_nth_mapping(maptable, 0);
	fail_unless(osync_mapping_num_entries(mapping) == 2, NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 0);
	member = osync_change_get_member(change);
	//fail_unless(osync_member_get_id(member) == 2, NULL);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	//fail_unless(osync_member_get_id(member) == 1, NULL);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	
    mark_point();
    osync_mappingtable_close(maptable);
    hash = NULL;
    uid = NULL;
	mark_point();
	member = osync_member_from_id(group, 1);
	table = osync_hashtable_new();
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

	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_conflict_duplicate)
{
	char *testbed = setup_testbed("sync_conflict_duplicate");
	num_conflicts = 0;
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync);
	OSyncGroup *group = osync_group_load(osync, "configs/group");

	OSyncError *error = NULL;
  	OSyncEngine *engine = osync_engine_new(group, &error);
  	osync_engine_set_conflict_callback(engine, conflict_handler_duplication);
	osync_engine_init(engine, &error);

	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	
	system("diff -x \".*\" data1 data2");
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
	mark_point();
	OSyncMappingTable *maptable = engine->maptable;
	//osync_mappingtable_new(group);
	//osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	//osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 3, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
	//osync_mappingtable_close(maptable);
	
	fail_unless(!system("rm -f data1/testdata-dupe data2/testdata-dupe-dupe"), NULL);
	num_conflicts = 0;
	
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	osync_engine_finalize(engine);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	
	mark_point();
	maptable = osync_mappingtable_new(group);
	osync_mappingtable_set_dbpath(maptable, osync_group_get_configdir(group));
	osync_mappingtable_load(maptable);
	mark_point();
	fail_unless(osync_mappingtable_num_mappings(maptable) == 1, NULL);
	fail_unless(osync_mappingtable_num_unmapped(maptable) == 0, NULL);
	osync_mappingtable_close(maptable);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_conflict_duplicate2)
{
	char *testbed = setup_testbed("sync_conflict_duplicate2");
	num_conflicts = 0;
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync);
	OSyncGroup *group = osync_group_load(osync, "configs/group");
	
	OSyncError *error = NULL;
  	OSyncEngine *engine = osync_engine_new(group, &error);
  	osync_engine_set_conflict_callback(engine, conflict_handler_duplication);
	osync_engine_init(engine, &error);

	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	//fail_unless(!system("test \"x$(diff -x \".*\" data1 comp_data)\" = \"x\""), NULL);
	
	system("rm -f data1/testdata");
	sleep(2); //so the hash changes
	system("cp new_data data2/testdata");
	
	num_conflicts = 0;
	mark_point();
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	osync_engine_finalize(engine);
	system("diff -x \".*\" data1 data2");
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 1, NULL);
	
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
	fail_unless(mapping != NULL, NULL);
	fail_unless(osync_mapping_num_entries(mapping) == 2, NULL);
	mark_point();
	OSyncChange *change = osync_mapping_nth_entry(mapping, 0);
	OSyncMember *member = osync_change_get_member(change);
	//fail_unless(osync_member_get_id(member) == 2, NULL);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	mark_point();
	change = osync_mapping_nth_entry(mapping, 1);
	member = osync_change_get_member(change);
	//fail_unless(osync_member_get_id(member) == 1, NULL);
	fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), "file"), NULL);
	fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data"), NULL);
	fail_unless(!strcmp(osync_change_get_uid(change), "testdata"), NULL);
	
    mark_point();
    osync_mappingtable_close(maptable);
    char *hash = NULL;
    char *uid = NULL;
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

	fail_unless(!system("test \"x$(diff -x \".*\" data1 comp_data)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_conflict_deldel)
{
	char *testbed = setup_testbed("sync_conflict_deldel");
	num_conflicts = 0;
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync);
	OSyncGroup *group = osync_group_load(osync, "configs/group");
	
	OSyncError *error = NULL;
  	OSyncEngine *engine = osync_engine_new(group, &error);
  	osync_engine_set_conflict_callback(engine, conflict_handler_duplication);
	osync_engine_init(engine, &error);

	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);

	system("rm -f data1/testdata");
	system("rm -f data2/testdata");
	
	num_conflicts = 0;
	mark_point();
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	osync_engine_finalize(engine);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	
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
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

static void conflict_handler_random(OSyncEngine *engine, OSyncMapping *mapping)
{
	printf("random conflict handler\n");
	num_conflicts++;
	int num = osync_mapping_num_entries(mapping);
	int choosen = g_random_int_range(0, num);
	OSyncChange *change = osync_mapping_nth_entry(mapping, choosen);
	osync_mapping_set_masterentry(mapping, change);
}

START_TEST (sync_moddel)
{
	char *testbed = setup_testbed("sync_moddel");
	num_conflicts = 0;
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync);
	OSyncGroup *group = osync_group_load(osync, "configs/group");
	
	OSyncError *error = NULL;
  	OSyncEngine *engine = osync_engine_new(group, &error);
  	osync_engine_set_conflict_callback(engine, conflict_handler_random);
	osync_engine_init(engine, &error);

	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	
	sleep(2);
	system("cp new_data1 data1/testdata");
	system("cp new_data2 data2/testdata");
	
	mark_point();
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 1, NULL);
	
	system("rm -f data2/testdata");
	
	mark_point();
	num_conflicts = 0;
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	mark_point();
	osync_engine_finalize(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(num_conflicts == 0, NULL);
	
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
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_easy_dualdel)
{
	char *testbed = setup_testbed("sync_easy_dualdel");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync);
	OSyncGroup *group = osync_group_load(osync, "configs/group");
	
	OSyncError *error = NULL;
  	OSyncEngine *engine = osync_engine_new(group, &error);
  	osync_engine_set_conflict_callback(engine, conflict_handler_duplication);
	osync_engine_init(engine, &error);

	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);

	system("rm -f data1/testdata");
	system("rm -f data1/testdata2");
	
	mark_point();
	osync_engine_synchronize(engine, &error);
	mark_point();
	osync_engine_wait_sync_end(engine);
	osync_engine_finalize(engine);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	
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
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *env_suite(void)
{
	Suite *s = suite_create("Sync");
	TCase *tc_setup = tcase_create("setup");
	//TCase *tc_setup2 = tcase_create("setup2");
	suite_add_tcase (s, tc_setup);
	tcase_add_test(tc_setup, sync_setup);
	tcase_add_test(tc_setup, sync_setup_false);
	tcase_add_test(tc_setup, sync_setup_init);
	tcase_add_test(tc_setup, sync_init_error);
	tcase_add_test(tc_setup, sync_setup_connect);
	tcase_add_test(tc_setup, sync_easy_new);
	tcase_add_test(tc_setup, sync_easy_new_del);
	tcase_add_test(tc_setup, sync_easy_conflict);
	tcase_add_test(tc_setup, sync_easy_new_mapping);
	tcase_add_test(tc_setup, sync_easy_conflict_duplicate);
	tcase_add_test(tc_setup, sync_easy_dualdel);
	tcase_add_test(tc_setup, sync_conflict_duplicate2);
	tcase_add_test(tc_setup, sync_conflict_deldel);
	tcase_add_test(tc_setup, sync_moddel);
	tcase_add_test(tc_setup, sync_conflict_duplicate);
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
