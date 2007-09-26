#include "support.h"

#include <opensync/opensync-mapping.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>

START_TEST (mapping_new)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncMappingTable *table = osync_mapping_table_new(&error);
	fail_unless(table != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_mapping_table_ref(table);
	osync_mapping_table_unref(table);
	osync_mapping_table_unref(table);
	
	destroy_testbed(testbed);
}
END_TEST

#if 0
TODO: port to new 0.30 API   
START_TEST (mapping_table_add_view)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncMappingTable *table = osync_mapping_table_new(&error);
	fail_unless(table != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncMappingView *view = osync_mapping_view_new(&error);
	fail_unless(view != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_mapping_table_add_view(table, view);
	osync_mapping_view_unref(view);
	
	view = osync_mapping_view_new(&error);
	fail_unless(view != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_mapping_table_add_view(table, view);
	osync_mapping_view_unref(view);
	
	osync_mapping_table_unref(table);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (mapping_view_add_entry)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncMappingView *view = osync_mapping_view_new(&error);
	fail_unless(view != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncChange *change = osync_change_new(&error);
	fail_unless(change != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_change_set_uid(change, "uid");
	
	fail_unless(osync_mapping_view_num_unmapped(view) == 0, NULL);
	
	OSyncMappingEntry *entry = osync_mapping_view_nth_unmapped(view, 0);
	fail_unless(entry == NULL, NULL);
	
	fail_unless(osync_mapping_view_add_entry(view, change, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_mapping_view_num_unmapped(view) == 1, NULL);
	
	entry = osync_mapping_view_nth_unmapped(view, 0);
	fail_unless(entry != NULL, NULL);
	fail_unless(osync_mapping_entry_get_change(entry) == change, NULL);
	
	osync_mapping_view_remove_unmapped(view, entry);
	fail_unless(osync_mapping_view_num_unmapped(view) == 0, NULL);
	
	osync_mapping_view_unref(view);
	
	destroy_testbed(testbed);
}
END_TEST
#endif

START_TEST (mapping_compare)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	
	OSyncFormatEnv *formatenv = osync_format_env_new(&error);
	fail_unless(formatenv != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_format_env_load_plugins(formatenv, testbed, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format = osync_format_env_find_objformat(formatenv, "mockformat1");
	fail_unless(format != NULL, NULL);
	
	/*OSyncChange *change = osync_change_new(&error);
	fail_unless(change != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_change_set_uid(change, "uid");*/
	
	OSyncData *data1 = osync_data_new("test", 5, format, &error);
	fail_unless(data1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncMapping *mapping = osync_mapping_new(&error);
	fail_unless(mapping != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncMappingEntry *entry = osync_mapping_entry_new(&error);
	fail_unless(entry != NULL, NULL);
	fail_unless(error == NULL, NULL);
	//osync_mapping_entry_update(entry, change);
	
	
	destroy_testbed(testbed);
}
END_TEST

Suite *client_suite(void)
{
	Suite *s = suite_create("Mapping");
//	Suite *s2 = suite_create("Mapping");
	
	create_case(s, "mapping_new", mapping_new);
// XXX: port to 0.30 API	
//	create_case(s, "mapping_table_add_view", mapping_table_add_view);
//	create_case(s, "mapping_view_add_entry", mapping_view_add_entry);
	create_case(s, "mapping_compare", mapping_compare);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = client_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
