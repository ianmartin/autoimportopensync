#include "support.h"

#include <opensync/opensync-merger.h>

#include "formats/vformats-xml/vformat.c"
#include "formats/vformats-xml/xmlformat.c"
#include "formats/vformats-xml/xmlformat-vcard.c"

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

START_TEST (xmlformat_validate)
{
	char *testbed = setup_testbed("vcards");

	char *buffer;
	unsigned int size;
	osync_bool free_input, ret;
	OSyncError *error = NULL;
	OSyncXMLFormat *xmlformat;

	fail_unless(osync_file_read( "evolution2/evo2-full1.vcf", &buffer, (unsigned int *)(&size), &error), NULL);
	ret = conv_vcard_to_xmlformat(buffer, size, (char **)&xmlformat, (unsigned int *) &size, &free_input, "VCARD_EXTENSION=Evolution", &error);
	fail_unless(ret == TRUE, NULL);
	fail_unless(error == NULL, NULL);
	if(free_input)
		g_free(buffer);

	fail_unless(osync_xmlformat_validate(xmlformat) != FALSE, NULL);

	osync_xmlformat_unref(xmlformat);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (xmlformat_compare)
{
	char *testbed = setup_testbed("vcards");

	char *buffer;
	unsigned int size;
	osync_bool free_input, ret;
	OSyncError *error = NULL;
	OSyncXMLFormat *xmlformat;
	OSyncXMLFormat *xmlformat2;


	fail_unless(osync_file_read( "evolution2/evo2-full1.vcf", &buffer, (unsigned int *)(&size), &error), NULL);
	ret = conv_vcard_to_xmlformat(buffer, size, (char **)&xmlformat, (unsigned int *) &size, &free_input, "VCARD_EXTENSION=Evolution", &error);
	fail_unless(ret == TRUE, NULL);
	fail_unless(error == NULL, NULL);

	ret = conv_vcard_to_xmlformat(buffer, size, (char **)&xmlformat2, (unsigned int *) &size, &free_input, "VCARD_EXTENSION=Evolution", &error);
	fail_unless(ret == TRUE, NULL);
	fail_unless(error == NULL, NULL);

	if(free_input)
		g_free(buffer);

        char* keys_content[] =  {"Content", NULL};
        char* keys_name[] = {"FirstName", "LastName", NULL};
        OSyncXMLPoints points[] = {
                {"Name",                90,     keys_name},
                {"Telephone",   10,     keys_content},
                {"EMail",               10,     keys_content},
                {NULL}
        };

        osync_xmlformat_compare(xmlformat, xmlformat2, points, 0, 100);

	osync_xmlformat_unref(xmlformat);
	osync_xmlformat_unref(xmlformat2);


	destroy_testbed(testbed);
}
END_TEST

START_TEST (xmlformat_event_schema)
{
	char *testbed = setup_testbed("xmlformats");
	char *buffer;
	unsigned int size;
	OSyncError *error = NULL;

	fail_unless(osync_file_read("event.xml", &buffer, (unsigned int *)&size, &error), NULL);
	fail_unless(error == NULL, NULL);

	OSyncXMLFormat *xmlformat = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(error == NULL, NULL);

	fail_unless(osync_xmlformat_validate(xmlformat) != FALSE, NULL);

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
	create_case(s, "xmlformat_search_field", xmlformat_search_field);
	create_case(s, "xmlformat_validate", xmlformat_validate);
	create_case(s, "xmlformat_compare", xmlformat_compare);
	create_case(s, "xmlformat_event_schema", xmlformat_event_schema);


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
