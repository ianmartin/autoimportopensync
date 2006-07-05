#include "support.h"

#include <opensync/opensync-merger.h>
#include "opensync/opensync-format.h"
#include "formats/vformats-xml/vformat.c"
#include "formats/vformats-xml/xmlformat-vcard.c"

VFormat *vcard_new_from_string (const char *str);

START_TEST (merger_setup)
{
	char *testbed = setup_testbed("merger");
	printf("\n");

	char *buffer;
	int i, size;
	OSyncError *error = NULL;
	OSyncXMLFormat *xmlformat, *xmlformat_full;
	OSyncXMLField *cur;
	OSyncCapabilities *capabilities;
printf("---------------------------\n");
	fail_unless(osync_file_read("contact.xml", &buffer, &size, &error), NULL);
	xmlformat = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat != NULL, NULL);
	osync_xmlformat_sort(xmlformat);
	osync_xmlformat_assemble(xmlformat, &buffer, &size);
	printf("%s", buffer);
printf("---------------------------\n");
	fail_unless(osync_file_read("contact-full.xml", &buffer, &size, &error), NULL);
	xmlformat_full = osync_xmlformat_parse(buffer, size, &error);
	fail_unless(xmlformat != NULL, NULL);
	osync_xmlformat_sort(xmlformat_full);
	osync_xmlformat_assemble(xmlformat_full, &buffer, &size);
	printf("%s", buffer);
printf("---------------------------\n");
	fail_unless(osync_file_read("capabilities.xml", &buffer, &size, &error), NULL);
	printf("%s", buffer);
	capabilities = osync_capabilities_parse(buffer, size, &error);
	printf("---------------------------\n");
	fail_unless(capabilities != NULL, NULL);
	printf("---------------------------\n");
	osync_capabilities_sort(capabilities);
	printf("%s", buffer);
printf("---------------------------\n");
	osync_xmlformat_merging(xmlformat, capabilities, xmlformat_full);

	
//	cur = osync_xmlformat_get_first_field(xmlformat);
//	while(cur != NULL)
//	{
//		int keys, i;
//		printf("%s\n", osync_xmlfield_get_name(cur));
//		keys = osync_xmlfield_get_key_count(cur);
//		for(i=0; i < keys; i++)
//		{
//			printf("\t%s\n", osync_xmlfield_get_nth_key_value(cur, i));
//		}
//		cur = osync_xmlfield_get_next(cur);
//	}
	osync_xmlformat_assemble(xmlformat, &buffer, &size);
	printf("\n%s\n", buffer);
	
	printf("search for \"Name\"\n");
	OSyncXMLFieldList *res = osync_xmlformat_search_field(xmlformat, "Name", NULL);
	size = osync_xmlfieldlist_get_length(res);
	for(i=0; i < size; i++)
	{
		cur = osync_xmlfieldlist_item(res, i);
		printf("found %s :)\n", osync_xmlfield_get_name(cur));
	}
	printf("\n\n");

	destroy_testbed(testbed);
}
END_TEST


START_TEST (merger_conv_vcard)
{
	char *command = g_strdup_printf("cp %s/../opensync/merger/xmlformat.xsd .", g_get_current_dir());
	char *testbed = setup_testbed("vcards");
	printf("\n");
	system(command);
	g_free(command);
	
	/* the never used functions */
	if(0){
		conv_xmlformat_to_vcard(NULL, 0, NULL, 0, NULL, NULL, NULL, 0);
		compare_contact(NULL, 0, NULL, 0);
		print_contact(NULL, 0);
		destroy_contact(NULL, 0);
		get_revision(NULL, 0, NULL);
		get_version();
		get_format_info(NULL);
		get_conversion_info(NULL);
	}
	
	char *buffer;
	int size;
	OSyncError *error = NULL;
	OSyncXMLFormat *xmlformat = NULL;

	fail_unless(osync_file_read( "evolution2/evo2-full1.vcf", &buffer, &size, &error), NULL);
	printf("%s\n\n", buffer);
	
	osync_bool booollllllll;
	conv_vcard_to_xmlformat(buffer, size,
							(char **)&xmlformat,  (unsigned int *) &size,
							&booollllllll, "VCARD_EXTENSION=Evolution", &error);
	osync_xmlformat_sort(xmlformat);
	osync_xmlformat_assemble(xmlformat, &buffer, &size);
	printf("%s", buffer);
//	printf("\nChecking against Schema....");
//	fail_unless(osync_xmlformat_validate(xmlformat), NULL);
//	printf("is valid :-)\n");

	conv_xmlformat_to_vcard((char*)xmlformat, size,
							&buffer, (unsigned int *)&size,
							&booollllllll, "VCARD_EXTENSION=Evolution",	&error, VFORMAT_CARD_30);
	printf("%s", buffer);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (merger_validate)
{
	char *command = g_strdup_printf("cp %s/../opensync/merger/xmlformat.xsd .", g_get_current_dir());
	char *testbed = setup_testbed("merger");
	printf("\n");
	system(command);
	g_free(command);

	char *buffer;
	int size;
	OSyncError *error = NULL;
	OSyncXMLFormat *xmlformat;

	fail_unless(osync_file_read("contact.xml", &buffer, &size, &error), NULL);
	xmlformat = osync_xmlformat_parse(buffer, size, &error);
	osync_xmlformat_assemble(xmlformat, &buffer, &size);
	printf("%s", buffer);
	
	printf("\nChecking unsorted contact against Schema....\n");
	fail_unless(!osync_xmlformat_validate(xmlformat), NULL);
	printf("is not valid\n");
	


	destroy_testbed(testbed);
}
END_TEST

Suite *filter_suite(void)
{
	Suite *s = suite_create("Merger");
	create_case(s, "merger_setup", merger_setup);
	create_case(s, "merger_conv_vcard", merger_conv_vcard);
	create_case(s, "merger_validate", merger_validate);
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
