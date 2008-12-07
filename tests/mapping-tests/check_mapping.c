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

START_TEST (mapping_compare)
{
	char *testbed = setup_testbed(NULL);
	char *formatdir = g_strdup_printf("%s/formats",  testbed);
	
	OSyncError *error = NULL;
	
	OSyncFormatEnv *formatenv = osync_format_env_new(&error);
	fail_unless(formatenv != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_format_env_load_plugins(formatenv, formatdir, &error), NULL);
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
	
	osync_format_env_free(formatenv);
	
	g_free(formatdir);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *client_suite(void)
{
	Suite *s = suite_create("Mapping");
//	Suite *s2 = suite_create("Mapping");
	
	create_case(s, "mapping_new", mapping_new);
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
