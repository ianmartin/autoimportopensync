#include "support.h"

static void conv_vcal(const char *filename)
{
	OSyncError *error = NULL;
	OSyncEnv *env = init_env();
	
	OSyncFormatEnv *conv_env = osync_conv_env_new(env);
	fail_unless(conv_env != NULL, NULL);

	char *buffer;
	int size;
	
	fail_unless(osync_file_read(filename, &buffer, &size, &error), NULL);
	
	OSyncChange *change = osync_change_new();
	osync_change_set_uid(change, filename);		
	osync_change_set_data(change, buffer, size + 1, TRUE);
	osync_change_set_conv_env(change, conv_env);
	
	osync_change_set_objformat_string(change, "plain");

	OSyncObjFormat *sourceformat = osync_change_detect_objformat(conv_env, change, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_change_set_objformat(change, sourceformat);
	osync_change_set_objtype(change, osync_objformat_get_objtype(sourceformat));
	
	OSyncObjFormat *targetformat = NULL;
	if (!strcmp(osync_objformat_get_name(sourceformat), "vtodo10"))
		targetformat = osync_conv_find_objformat(conv_env, "vtodo20");
	
	if (!strcmp(osync_objformat_get_name(sourceformat), "vtodo20"))
		targetformat = osync_conv_find_objformat(conv_env, "vtodo10");

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

START_TEST (conv_vcal_evolution2_full1)
{
	conv_vcal("data/vtodos/evolution2/todo-full1.vcf");
}
END_TEST

Suite *vcal_suite(void)
{
	Suite *s = suite_create("VCal");
	//Suite *s2 = suite_create("VCal");
	
	create_case(s, "conv_vcal_evolution2_full1", conv_vcal_evolution2_full1);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = vcal_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
