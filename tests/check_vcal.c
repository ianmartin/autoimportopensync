#include "support.h"

static void conv_vcal(const char *filename)
{
	char *command = g_strdup_printf("cp %s/%s .", g_get_current_dir(), filename);
	char *testbed = setup_testbed(NULL);
	system(command);
	g_free(command);
	
	
	OSyncError *error = NULL;
	OSyncEnv *env = init_env();
	
	OSyncFormatEnv *conv_env = osync_conv_env_new(env);
	fail_unless(conv_env != NULL, NULL);

	char *buffer;
	int size;
	
	char *file = g_path_get_basename(filename);
	fail_unless(osync_file_read(file, &buffer, &size, &error), NULL);
	
	OSyncChange *change = osync_change_new();
	osync_change_set_uid(change, file);		
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
	
	destroy_testbed(testbed);
}


static time_t vcal_get_revision(const char *filename)
{
	char *command = g_strdup_printf("cp %s/%s .", g_get_current_dir(), filename);
	char *testbed = setup_testbed(NULL);
	system(command);
	g_free(command);
	
	
	OSyncError *error = NULL;
	OSyncEnv *env = init_env();
	
	OSyncFormatEnv *conv_env = osync_conv_env_new(env);
	fail_unless(conv_env != NULL, NULL);

	char *buffer;
	int size;
	
	char *file = g_path_get_basename(filename);
	fail_unless(osync_file_read(file, &buffer, &size, &error), NULL);
	
	OSyncChange *change = osync_change_new();
	osync_change_set_uid(change, file);
	g_free(file);
	osync_change_set_data(change, buffer, size + 1, TRUE);
	osync_change_set_conv_env(change, conv_env);
	
	osync_change_set_objformat_string(change, "plain");

	OSyncObjFormat *sourceformat = osync_change_detect_objformat(conv_env, change, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_change_set_objformat(change, sourceformat);
	
	OSyncObjFormat *targetformat = osync_conv_find_objformat(conv_env, "xml-todo");
	fail_unless(targetformat != NULL, NULL);
	
	fail_unless(osync_change_convert_extension(conv_env, change, targetformat, "evolution", &error), NULL);
	
	time_t time = osync_change_get_revision(change, &error);
	
	osync_conv_env_free(conv_env);
	osync_env_finalize(env, NULL);
	osync_env_free(env);
	
	destroy_testbed(testbed);
	return time;
}

START_TEST (conv_vcal_evolution2_full1)
{
	conv_vcal("data/vtodos/evolution2/todo-full1.vcf");
}
END_TEST

START_TEST (todo_get_revision1)
{
	fail_unless(vcal_get_revision("data/vtodos/evolution2/todo-full1.vcf") == 1110067010, NULL);
}
END_TEST

START_TEST (todo_get_revision2)
{
	fail_unless(vcal_get_revision("data/vtodos/evolution2/todo-full2.vcf") == 1110067010, NULL);
}
END_TEST

START_TEST (todo_get_revision3)
{
	fail_unless(vcal_get_revision("data/vtodos/evolution2/todo-full3.vcf") == 1110063600, NULL);
}
END_TEST

START_TEST (todo_no_revision)
{
	fail_unless(vcal_get_revision("data/vtodos/kdepim/todo-full1.vcs") == -1, NULL);
}
END_TEST

Suite *vcal_suite(void)
{
	Suite *s = suite_create("VCal");
	Suite *s2 = suite_create("VCal");
	
	create_case(s2, "conv_vcal_evolution2_full1", conv_vcal_evolution2_full1);
	
	create_case(s, "todo_get_revision1", todo_get_revision1);
	create_case(s, "todo_get_revision2", todo_get_revision2);
	create_case(s, "todo_get_revision3", todo_get_revision3);
	create_case(s, "todo_no_revision", todo_no_revision);
	
	return s2;
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
