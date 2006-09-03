#include "support.h"

#include <opensync/opensync-merger.h>

START_TEST (xmlformat_new)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncXMLFormat *xmlformat = osync_xmlformat_new("contact", &error);
	fail_unless(xmlformat != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_xmlformat_ref(xmlformat);
	osync_xmlformat_unref(xmlformat);
	
	osync_xmlformat_unref(xmlformat);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (xmlfield_new)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncXMLFormat *xmlformat = osync_xmlformat_new("contact", &error);
	fail_unless(xmlformat != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Name", &error);
	fail_unless(xmlfield != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_xmlformat_unref(xmlformat);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (xmlformat_parse)
{
	char *testbed = setup_testbed("merger");
	
	OSyncError *error = NULL;
	char* buffer;
	unsigned int size;
	fail_unless(osync_file_read("capabilities.xml", &buffer, &size, &error), NULL);

	OSyncXMLFormat *xmlformat = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_xmlformat_unref(xmlformat);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (xmlformat_sort)
{
	char *testbed = setup_testbed("merger");
	
	OSyncError *error = NULL;
	char* buffer;
	unsigned int size;
	fail_unless(osync_file_read("capabilities.xml", &buffer, &size, &error), NULL);

	OSyncXMLFormat *xmlformat = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_xmlformat_sort(xmlformat);
	
	osync_xmlformat_unref(xmlformat);

	destroy_testbed(testbed);
}
END_TEST

Suite *xmlformat_suite(void)
{
	Suite *s = suite_create("XMLFormat");
	create_case(s, "xmlformat_new", xmlformat_new);
	create_case(s, "xmlfield_new", xmlfield_new);
	create_case(s, "xmlformat_parse", xmlformat_parse);
	create_case(s, "xmlformat_sort", xmlformat_sort);
	return s;
}

int main(void)
{
	int nf;

	Suite *s = xmlformat_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
