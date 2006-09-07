#include "support.h"

#include <opensync/opensync-mapping.h>
#include <opensync/opensync-archive.h>
#include <opensync/opensync-helper.h>

char *olddir = NULL;

static void reset_env(void)
{
	unsetenv("CONNECT_ERROR");
	unsetenv("CONNECT_TIMEOUT");
	unsetenv("INIT_NULL");
	unsetenv("GET_CHANGES_ERROR");
	unsetenv("GET_CHANGES_TIMEOUT");
	unsetenv("GET_CHANGES_TIMEOUT2");
	unsetenv("COMMIT_ERROR");
	unsetenv("COMMIT_TIMEOUT");
	unsetenv("SYNC_DONE_ERROR");
	unsetenv("SYNC_DONE_TIMEOUT");
	unsetenv("DISCONNECT_ERROR");
	unsetenv("DISCONNECT_TIMEOUT");
}

char *setup_testbed(char *fkt_name)
{
	
	setuid(65534);
	char *testbed = g_strdup_printf("%s/testbed.XXXXXX", g_get_tmp_dir());
	mkdtemp(testbed);
	
	char *command = NULL;
	if (fkt_name) {
		command = g_strdup_printf("cp -R "OPENSYNC_TESTDATA"/%s/* %s", fkt_name, testbed);
		if (system(command))
			abort();
		g_free(command);
	}
	
	/*command = g_strdup_printf("cp -R ../osplugin/osplugin %s", testbed);
	if (system(command))
		abort();
	g_free(command);*/
	
	command = g_strdup_printf("cp libmocksync.so %s", testbed);
	if (system(command))
		abort();
	g_free(command);
	
	command = g_strdup_printf("cp libmockformat.so %s", testbed);
	if (system(command))
		abort();
	g_free(command);
	
	command = g_strdup_printf("cp -R ../formats/.libs/*.so %s", testbed);
	if (system(command))
		abort();
	g_free(command);
	
	command = g_strdup_printf("cp -R ../formats/vformats-xml/.libs/*.so %s", testbed);
	if (system(command))
		abort();
	g_free(command);
	
	command = g_strdup_printf("chmod -R 700 %s", testbed);
	if (system(command))
		abort();
	g_free(command);
		
	olddir = g_get_current_dir();
	if (chdir(testbed))
		abort();
	
	osync_trace(TRACE_INTERNAL, "Seting up %s at %s", fkt_name, testbed);
	printf(".");
	fflush(NULL);
	reset_env();
	return testbed;
}

void destroy_testbed(char *path)
{
	char *command = g_strdup_printf("rm -rf %s", path);
	if (olddir) {
		chdir(olddir);
		g_free(olddir);
	}
	system(command);
	g_free(command);
	osync_trace(TRACE_INTERNAL, "Tearing down %s", path);
	g_free(path);
}

/*

void conflict_handler_choose_modified(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	fail_unless(osengine_mapping_num_changes(mapping) == GPOINTER_TO_INT(user_data), NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);
	
	int i;
	for (i = 0; i < osengine_mapping_num_changes(mapping); i++) {
		OSyncChange *change = osengine_mapping_nth_change(mapping, i);
		if (change->changetype == CHANGE_MODIFIED) {
			osengine_mapping_solve(engine, mapping, change);
			return;
		}
	}
	fail();
}

void conflict_handler_ignore(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	num_conflicts++;
	if (user_data)
		fail_unless(osengine_mapping_num_changes(mapping) == GPOINTER_TO_INT(user_data), NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);
	
	osengine_mapping_ignore_conflict(engine, mapping);
}*/

void create_case(Suite *s, const char *name, void (*function)(void))
{
	TCase *tc_new = tcase_create(name);
	tcase_set_timeout(tc_new, 0);
	suite_add_tcase (s, tc_new);
	tcase_add_test(tc_new, function);
}

OSyncMappingTable *mappingtable_load(const char *path, const char *objtype, int num_mappings)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %s, %i)", __func__, path, objtype, num_mappings);
	
	OSyncError *error = NULL;
	OSyncMappingTable *table = osync_mapping_table_new(&error);
	fail_unless(table != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncArchive *archive = osync_archive_new(path, &error);
	fail_unless(archive != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_mapping_table_load(table, archive, objtype, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_archive_unref(archive);
	
	fail_unless(osync_mapping_table_num_mappings(table) == num_mappings, NULL);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, table);
	return table;
}

void check_mapping(OSyncMappingTable *maptable, int memberid, int mappingid, int numentries, const char *uid)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %i, %i, %s)", __func__, maptable, memberid, mappingid, numentries, uid);
	
	int i = 0;
	for (i = 0; i < osync_mapping_table_num_mappings(maptable); i++) {
		OSyncMapping *mapping = osync_mapping_table_nth_mapping(maptable, i);
		OSyncMappingEntry *testentry = osync_mapping_find_entry_by_member_id(mapping, memberid);
		if ((mappingid != -1 && osync_mapping_get_id(mapping) == mappingid) || (mappingid == -1 && !strcmp(osync_mapping_entry_get_uid(testentry), uid))) {
			fail_unless(osync_mapping_num_entries(mapping) == numentries);
			int n = 0;
			for (n = 0; n < osync_mapping_num_entries(mapping); n++) {
				OSyncMappingEntry *entry = osync_mapping_nth_entry(mapping, n);
				if (osync_mapping_entry_get_member_id(entry) == memberid) {
					fail_unless(!strcmp(osync_mapping_entry_get_uid(entry), uid), NULL);
					goto out;
				}
			}
			fail(NULL);
		}
	}
	fail(NULL);

out:
	/*OSyncMember *member = osync_member_from_id(maptable->group, memberid);
	OSyncMappingView *view = osengine_mappingtable_find_view(maptable, member);
	mark_point();
	if (mappingid != -1) {
		mapping = g_list_nth_data(maptable->mappings, mappingid);
	} else {
		GList *m;
		for (m = maptable->mappings; m; m = m->next) {
			mapping = m->data;
			OSyncMappingEntry *entry = osengine_mapping_find_entry(mapping, NULL, view);
			if (!entry)
				continue;
			OSyncChange *change = entry->change;
			fail_unless(change != NULL, NULL);
			if (!strcmp(osync_change_get_uid(change), uid))
				break;
		}
	}
	fail_unless(mapping != NULL, NULL);
	fail_unless(osengine_mapping_num_changes(mapping) == numentries, "osengine_mapping_num_changes(mapping) == numentries for %s, %i: %i != %i", uid, memberid, osengine_mapping_num_changes(mapping), numentries);*/
	
	
	
	/*OSyncChange *change = osengine_mapping_find_entry(mapping, NULL, view)->change;
	fail_unless(change != NULL, NULL);
	if (format)
		fail_unless(!strcmp(osync_objformat_get_name(osync_change_get_objformat(change)), format), NULL);
	if (objecttype)
		fail_unless(!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), objecttype), NULL);
	if (uid && strcmp(osync_change_get_uid(change), uid)) {
		printf("uid mismatch: %s != %s for member %i and mapping %i\n", osync_change_get_uid(change), uid, memberid, mappingid);
		fail("uid mismatch");
	}*/
	osync_trace(TRACE_EXIT, "%s", __func__);
}

OSyncHashTable *hashtable_load(const char *path, const char *objtype, int entries)
{
	OSyncError *error = NULL;
	OSyncHashTable *table = osync_hashtable_new(path, objtype, &error);
	fail_unless(table != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
    fail_unless(osync_hashtable_num_entries(table) == entries, NULL);
    
    return table;
}

void check_hash(OSyncHashTable *table, const char *cmpuid)
{
	char *uid = NULL;
	char *hash = NULL;
	int i;
	osync_bool found = FALSE;
	for (i = 0; i < osync_hashtable_num_entries(table); i++) {
		osync_hashtable_nth_entry(table, i, &uid, &hash);
		if (!strcmp(cmpuid, uid))
			found = TRUE;
		g_free(hash);
		g_free(uid);
	}
	fail_unless(found == TRUE, NULL);
}
