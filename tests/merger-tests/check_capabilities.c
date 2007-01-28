#include "support.h"

#include <opensync/opensync-merger.h>

START_TEST (capabilities_new)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncCapabilities *capabilities = osync_capabilities_new(&error);
	fail_unless(capabilities != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_capabilities_ref(capabilities);
	osync_capabilities_unref(capabilities);
	
	osync_capabilities_unref(capabilities);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (capability_new)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncCapabilities *capabilities = osync_capabilities_new(&error);
	fail_unless(capabilities != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncCapability *capability = osync_capability_new(capabilities, "contact", "Name", &error);
	fail_unless(capability != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_capabilities_unref(capabilities);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (capabilities_parse)
{
	char *testbed = setup_testbed("merger");
	
	OSyncError *error = NULL;
	char* buffer;
	unsigned int size;
	fail_unless(osync_file_read("capabilities.xml", &buffer, &size, &error), NULL);

	OSyncCapabilities *capabilities = osync_capabilities_parse(buffer, size, &error);
	fail_unless(capabilities != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_capabilities_unref(capabilities);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (capabilities_sort)
{
	char *testbed = setup_testbed("merger");
	
	OSyncError *error = NULL;
	char* buffer;
	unsigned int size;
	fail_unless(osync_file_read("capabilities.xml", &buffer, &size, &error), NULL);

	OSyncCapabilities *capabilities = osync_capabilities_parse(buffer, size, &error);
	fail_unless(capabilities != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_capabilities_sort(capabilities);
	
	osync_capabilities_unref(capabilities);

	g_free(buffer);

	destroy_testbed(testbed);
}
END_TEST

Suite *capabilities_suite(void)
{
	Suite *s = suite_create("Capabilities");
	create_case(s, "capabilities_new", capabilities_new);
	create_case(s, "capability_new", capability_new);
	create_case(s, "capabilities_parse", capabilities_parse);
	create_case(s, "capabilities_sort", capabilities_sort);
	return s;
}

int main(void)
{
	int nf;

	Suite *s = capabilities_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
