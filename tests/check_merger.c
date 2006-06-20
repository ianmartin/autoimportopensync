#include "support.h"

#include <opensync/opensync-merger.h>
#include "opensync/opensync-format.h"
#include "formats/vformats-xml/vformat.c"
#include "formats/vformats-xml/xmlformat-vcard.c"

VFormat *vcard_new_from_string (const char *str);

START_TEST (merger_setup)
{
	char *testbed = setup_testbed("merger");

	char *buffer;
	int i, size;
	OSyncError *error = NULL;
	OSyncXMLFormat *xmlformat, *xmlformat_full;
	OSyncXMLField *cur;
	OSyncCapabilities *capabilities;
	
	fail_unless(osync_file_read("contact.xml", &buffer, &size, &error), NULL);
	xmlformat = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat != NULL, NULL);
	osync_xmlformat_sort(xmlformat);

	fail_unless(osync_file_read("contact-full.xml", &buffer, &size, &error), NULL);
	xmlformat_full = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat != NULL, NULL);
	osync_xmlformat_sort(xmlformat_full);
	
	capabilities = osync_capabilities_new();
	if(!osync_capabilities_read_xml(capabilities, "capabilities.xml", &error))
	{
		printf("%s\n", osync_error_print(&error));
	}
	osync_capabilities_sort(capabilities);

	osync_xmlformat_merging(xmlformat, capabilities, xmlformat_full);

	
	cur = osync_xmlformat_get_first_field(xmlformat);
	while(cur != NULL)
	{
		int keys, i;
		printf("%s\n", osync_xmlfield_get_name(cur));
		keys = osync_xmlfield_get_key_count(cur);
		for(i=0; i < keys; i++)
		{
			printf("\t%s\n", osync_xmlfield_get_nth_key_value(cur, i));
		}
		cur = osync_xmlfield_get_next(cur);
	}

	osync_xmlformat_assemble(xmlformat, &buffer, &size);
	printf("\n%s\n", buffer);
	
	printf("search for name\n");
	OSyncXMLFieldList *res = osync_xmlformat_search_field(xmlformat, "Name");
	size = osync_xmlfieldlist_getLength(res);
	for(i=0; i < size; i++)
	{
		cur = osync_xmlfieldlist_item(res, i);
		printf("found %s :)\n", osync_xmlfield_get_name(cur));
	}
	printf("\n");

	destroy_testbed(testbed);
}
END_TEST


START_TEST (merger_conv_vcard)
{
	char *testbed = setup_testbed("vcards");
	
	/* the never used functions */
	if(0){
		conv_xmlformat_to_vcard(NULL, NULL, 0, NULL, NULL, NULL, NULL, 0);
		compare_contact(NULL, NULL);
		print_contact(NULL);
		destroy_xmlformat(NULL, 0);
		fin_vcard_to_xmlformat(NULL);
		init_xmlformat_to_vcard();
		fin_xmlformat_to_vcard(NULL);
	}
	
	char *buffer;
	int size;
	OSyncError *error = NULL;
	OSyncXMLFormat *xmlformat = NULL;

	fail_unless(osync_file_read( "evolution2/evo2-full1.vcf", &buffer, &size, &error), NULL);
	
	OSyncXMLFormat** tmp = &xmlformat;
	conv_vcard_to_xmlformat(buffer, size,
							(char **)tmp,  (unsigned int *) &size,
							FALSE, NULL, &error);
	
	osync_xmlformat_assemble(xmlformat, &buffer, &size);
	printf("%s", buffer);
								
		
	destroy_testbed(testbed);
}
END_TEST

Suite *filter_suite(void)
{
	Suite *s = suite_create("Merger");
	create_case(s, "merger_setup", merger_setup);
	create_case(s, "merger_conv_vcard", merger_conv_vcard);
	return s;
}

int main(void)
{
	int nf;

	Suite *s = filter_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
