#include "support.h"

static void conv_vcard(const char *filename)
{
	OSyncError *error = NULL;
	OSyncEnv *env = init_env();
	
	OSyncFormatEnv *conv_env = osync_conv_env_new(env);
	fail_unless(conv_env != NULL, NULL);

	char *buffer;
	int size;
	
	char *tmp = g_strdup_printf("ls %s", filename);
	system(tmp);
	fail_unless(osync_file_read(filename, &buffer, &size, &error), NULL);
	
	OSyncChange *change = osync_change_new();
	osync_change_set_uid(change, filename);		
	osync_change_set_data(change, buffer, size, TRUE);
	osync_change_set_conv_env(change, conv_env);
	
	osync_change_set_objformat_string(change, "plain");

	OSyncObjFormat *sourceformat = osync_change_detect_objformat(conv_env, change, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_change_set_objformat(change, sourceformat);
	osync_change_set_objtype(change, osync_objformat_get_objtype(sourceformat));
	
	OSyncObjFormat *targetformat = NULL;
	if (!strcmp(osync_objformat_get_name(sourceformat), "vcard21"))
		targetformat = osync_conv_find_objformat(conv_env, "vcard30");
	
	if (!strcmp(osync_objformat_get_name(sourceformat), "vcard30"))
		targetformat = osync_conv_find_objformat(conv_env, "vcard21");

	fail_unless(targetformat != NULL, NULL);
	
	OSyncChange *newchange = osync_change_copy(change, &error);
	fail_unless(newchange != NULL, NULL);
	
	//Convert to
	fail_unless(osync_change_convert(conv_env, change, targetformat, &error), NULL);
	
	//Compare old to new
	fail_unless(osync_change_compare(newchange, change) == CONV_DATA_SAME, NULL);
	
	//Convert back
	fail_unless(osync_change_convert(conv_env, change, targetformat, &error), NULL);
	
	//Compare again
	fail_unless(osync_change_compare(newchange, change) == CONV_DATA_SAME, NULL);
	
	osync_conv_env_free(conv_env);
	osync_env_finalize(env, NULL);
	osync_env_free(env);
}

START_TEST (conv_vcard_evolution2)
{
	conv_vcard("data/vcards/evolution2/evo2-full1.vcf");
}
END_TEST

Suite *vcard_suite(void)
{
	Suite *s = suite_create("Vcard");
	//Suite *s2 = suite_create("ConvVcard");
	create_case(s, "conv_vcard_evolution2", conv_vcard_evolution2);

	return s;
}

int main(void)
{
	int nf;

	Suite *s = vcard_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
