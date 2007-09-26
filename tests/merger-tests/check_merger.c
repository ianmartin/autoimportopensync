#include "support.h"

#include <opensync/opensync-merger.h>

START_TEST (merger_new)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncCapabilities *capabilities = osync_capabilities_new(&error);
	fail_unless(capabilities != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncMerger *merger = osync_merger_new(capabilities, &error);
	fail_unless(merger != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_merger_ref(merger);
	osync_merger_unref(merger);
	
	osync_merger_unref(merger);
	osync_capabilities_unref(capabilities);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (merger_merge)
{
	char *testbed = setup_testbed("merger");

	char *buffer;
	unsigned int size;
	OSyncError *error = NULL;
	OSyncXMLFormat *xmlformat, *xmlformat_entire;
	OSyncCapabilities *capabilities;

	fail_unless(osync_file_read("contact.xml", &buffer, &size, &error), NULL);
	xmlformat = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat != NULL, NULL);
	fail_unless(error == NULL, NULL);
	g_free(buffer);
	osync_xmlformat_sort(xmlformat);
	
	fail_unless(osync_file_read("contact-full.xml", &buffer, &size, &error), NULL);
	xmlformat_entire = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat_entire != NULL, NULL);
	fail_unless(error == NULL, NULL);
	g_free(buffer);
	osync_xmlformat_sort(xmlformat_entire);
	
	fail_unless(osync_file_read("capabilities.xml", &buffer, &size, &error), NULL);
	capabilities = osync_capabilities_parse(buffer, size, &error);
	fail_unless(capabilities != NULL, NULL);
	fail_unless(error == NULL, NULL);
	g_free(buffer);
	osync_capabilities_sort(capabilities);
	
	OSyncMerger *merger = osync_merger_new(capabilities, &error);
	fail_unless(merger != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_merger_merge(merger, xmlformat, xmlformat_entire);
	osync_merger_unref(merger);
	
	osync_capabilities_unref(capabilities);
	osync_xmlformat_unref(xmlformat);
	osync_xmlformat_unref(xmlformat_entire);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (merger_demerge)
{
	char *testbed = setup_testbed("merger");

	char *buffer;
	unsigned int size;
	OSyncError *error = NULL;
	OSyncXMLFormat *xmlformat, *xmlformat_entire;
	OSyncCapabilities *capabilities;

	fail_unless(osync_file_read("contact.xml", &buffer, &size, &error), NULL);
	xmlformat = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat != NULL, NULL);
	fail_unless(error == NULL, NULL);
	g_free(buffer);
	osync_xmlformat_sort(xmlformat);
	
	fail_unless(osync_file_read("contact-full.xml", &buffer, &size, &error), NULL);
	xmlformat_entire = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat_entire != NULL, NULL);
	fail_unless(error == NULL, NULL);
	g_free(buffer);
	osync_xmlformat_sort(xmlformat_entire);
	
	fail_unless(osync_file_read("capabilities.xml", &buffer, &size, &error), NULL);
	capabilities = osync_capabilities_parse(buffer, size, &error);
	fail_unless(capabilities != NULL, NULL);
	fail_unless(error == NULL, NULL);
//printf("\n%s", buffer);
	g_free(buffer);
	osync_capabilities_sort(capabilities);

//osync_xmlformat_assemble(xmlformat, &buffer, &size); printf("\n%s", buffer); g_free(buffer);
//osync_xmlformat_assemble(xmlformat_entire, &buffer, &size); printf("\n%s", buffer); g_free(buffer);
	
	OSyncMerger *merger = osync_merger_new(capabilities, &error);
	fail_unless(merger != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_merger_merge(merger, xmlformat, xmlformat_entire);
//osync_xmlformat_assemble(xmlformat, &buffer, &size); printf("\nMERGED:\n%s", buffer); g_free(buffer);
	osync_merger_demerge(merger, xmlformat);
//osync_xmlformat_assemble(xmlformat, &buffer, &size); printf("\nDEMERGED:\n%s", buffer); g_free(buffer);

	osync_merger_unref(merger);
	
	osync_capabilities_unref(capabilities);
	osync_xmlformat_unref(xmlformat);
	osync_xmlformat_unref(xmlformat_entire);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *filter_suite(void)
{
	Suite *s = suite_create("Merger");
	create_case(s, "merger_new", merger_new);
	create_case(s, "merger_merge", merger_merge);
	create_case(s, "merger_demerge", merger_demerge);
	return s;
}

int main(void)
{
	int nf;

	Suite *s = filter_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
