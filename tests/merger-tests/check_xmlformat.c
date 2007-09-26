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

START_TEST (xmlformat_search_field)
{
	char *testbed = setup_testbed("merger");

	char *buffer;
	unsigned int size;
	OSyncError *error = NULL;

	fail_unless(osync_file_read("contact.xml", &buffer, (unsigned int *)(&size), &error), NULL);
	OSyncXMLFormat *xmlformat = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat != NULL, NULL);
	fail_unless(error == NULL, NULL);
	g_free(buffer);
	osync_xmlformat_sort(xmlformat);
	
	OSyncXMLFieldList *xmlfieldlist = osync_xmlformat_search_field(xmlformat, "Name", NULL);
	fail_unless(xmlfieldlist != NULL, NULL);
	
	fail_unless(size != osync_xmlfieldlist_get_length(xmlfieldlist), NULL);

	osync_xmlfieldlist_free(xmlfieldlist);
	osync_xmlformat_unref(xmlformat);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (xmlformat_compare)
{
	char *testbed = setup_testbed("xmlformats");

	char *buffer;
	unsigned int size;
	OSyncError *error = NULL;


	fail_unless(osync_file_read( "contact.xml", &buffer, &size, &error), NULL);

	OSyncXMLFormat *xmlformat = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat != NULL, NULL);
	fail_unless(error == NULL, NULL);

	OSyncXMLFormat *xmlformat2 = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat2 != NULL, NULL);
	fail_unless(error == NULL, NULL);

	g_free(buffer);

        char* keys_content[] =  {"Content", NULL};
        char* keys_name[] = {"FirstName", "LastName", NULL};
        OSyncXMLPoints points[] = {
                {"Name",                90,     keys_name},
                {"Telephone",   10,     keys_content},
                {"EMail",               10,     keys_content},
                {NULL}
        };

        osync_xmlformat_compare((OSyncXMLFormat*)xmlformat, (OSyncXMLFormat*)xmlformat2, points, 0, 100);

	osync_xmlformat_unref((OSyncXMLFormat*)xmlformat);
	osync_xmlformat_unref((OSyncXMLFormat*)xmlformat2);


	destroy_testbed(testbed);
}
END_TEST

START_TEST (xmlformat_compare_field2null)
{
	char *testbed = setup_testbed("xmlformats");

	char *buffer1;
	char *buffer2;
	unsigned int size1;
	unsigned int size2;
	OSyncError *error = NULL;


	fail_unless(osync_file_read( "contact1.xml", &buffer1, &size1, &error), NULL);

	OSyncXMLFormat *xmlformat1 = osync_xmlformat_parse(buffer1, size1, &error);
	fail_unless(xmlformat1 != NULL, NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(osync_file_read( "contact2.xml", &buffer2, &size2, &error), NULL);
	
	OSyncXMLFormat *xmlformat2 = osync_xmlformat_parse(buffer2, size2, &error);
	fail_unless(xmlformat2 != NULL, NULL);
	fail_unless(error == NULL, NULL);

	g_free(buffer1);
	g_free(buffer2);

        char* keys_content[] =  {"Content", NULL};
        char* keys_name[] = {"FirstName", "LastName", NULL};
        OSyncXMLPoints points[] = {
                {"Name",                90,     keys_name},
                {"Telephone",   10,     keys_content},
                {"EMail",               10,     keys_content},
                {NULL}
        };

        osync_xmlformat_compare((OSyncXMLFormat*)xmlformat1, (OSyncXMLFormat*)xmlformat2, points, 0, 100);

	osync_xmlformat_unref((OSyncXMLFormat*)xmlformat1);
	osync_xmlformat_unref((OSyncXMLFormat*)xmlformat2);


	destroy_testbed(testbed);
}
END_TEST

START_TEST (xmlformat_event_schema)
{
	char *testbed = setup_testbed("xmlformats");
	char *buffer;
	unsigned int size;
	OSyncError *error = NULL;

	fail_unless(osync_file_read("event.xml", &buffer, &size, &error), NULL);
	fail_unless(error == NULL, NULL);

	OSyncXMLFormat *xmlformat = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(error == NULL, NULL);

	g_free(buffer);

	fail_unless(osync_xmlformat_validate(xmlformat) != FALSE, NULL);

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

START_TEST (xmlfield_sort)
{
	char *testbed = setup_testbed("xmlformats");
	
	OSyncError *error = NULL;
	char* buffer;
	unsigned int size;
	fail_unless(osync_file_read("xmlfield_unsorted.xml", &buffer, &size, &error), NULL);

	OSyncXMLFormat *xmlformat = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncXMLField *xmlfield = osync_xmlformat_get_first_field(xmlformat);
	for (; xmlfield != NULL; xmlfield = osync_xmlfield_get_next(xmlfield)) {
		osync_xmlfield_sort(xmlfield);
	}

	xmlfield = osync_xmlformat_get_first_field(xmlformat);
	for (; xmlfield != NULL; xmlfield = osync_xmlfield_get_next(xmlfield)) {
		int count = osync_xmlfield_get_key_count(xmlfield);

		if (count > 0)
			fail_unless(!strcmp(osync_xmlfield_get_nth_key_name(xmlfield, 0), "ABCDEFG"), NULL);

		if (count > 1)
			fail_unless(!strcmp(osync_xmlfield_get_nth_key_name(xmlfield, 1), "BCDEFG"), NULL);

		if (count > 2)
			fail_unless(!strcmp(osync_xmlfield_get_nth_key_name(xmlfield, 2), "CDEFG"), NULL);

		if (count > 3)
			fail_unless(!strcmp(osync_xmlfield_get_nth_key_name(xmlfield, 3), "DEFG"), NULL);

		if (count > 4)
			fail_unless(!strcmp(osync_xmlfield_get_nth_key_name(xmlfield, 4), "EFG"), NULL);

		if (count > 5)
			fail_unless(!strcmp(osync_xmlfield_get_nth_key_name(xmlfield, 5), "FG"), NULL);

		if (count > 6)
			fail_unless(!strcmp(osync_xmlfield_get_nth_key_name(xmlfield, 6), "G"), NULL);

	}
	
	osync_xmlformat_unref(xmlformat);

	destroy_testbed(testbed);
}
END_TEST

Suite *xmlformat_suite(void)
{
	Suite *s = suite_create("XMLFormat");

	// xmlformat
	create_case(s, "xmlformat_new", xmlformat_new);
	create_case(s, "xmlformat_parse", xmlformat_parse);
	create_case(s, "xmlformat_sort", xmlformat_sort);
	create_case(s, "xmlformat_search_field", xmlformat_search_field);
	create_case(s, "xmlformat_compare", xmlformat_compare);
	create_case(s, "xmlformat_compare_field2null", xmlformat_compare_field2null);
	create_case(s, "xmlformat_event_schema", xmlformat_event_schema);

	// xmlfield
	create_case(s, "xmlfield_new", xmlfield_new);
	create_case(s, "xmlfield_sort", xmlfield_sort);

	return s;
}

int main(void)
{
	int nf;

	Suite *s = xmlformat_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
