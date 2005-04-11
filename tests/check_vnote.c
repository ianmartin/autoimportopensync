#include "support.h"

static void conv_vnote(const char *filename)
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
	
	OSyncObjFormat *targetformat = osync_conv_find_objformat(conv_env, "xml-note");
	fail_unless(targetformat != NULL, NULL);
	
	OSyncChange *newchange = osync_change_copy(change, &error);
	fail_unless(newchange != NULL, NULL);
	
	//Convert to
	fail_unless(osync_change_convert(conv_env, change, targetformat, &error), NULL);
	
	//Compare old to new
	fail_unless(osync_change_compare(newchange, change) == CONV_DATA_SAME, NULL);
	
	//Convert back
	fail_unless(osync_change_convert(conv_env, change, sourceformat, &error), NULL);

	//Detect the output again
	osync_change_set_objformat_string(change, "plain");
	fail_unless(osync_change_detect_objformat(conv_env, change, &error) == sourceformat, NULL);
	
	//Compare again
	fail_unless(osync_change_compare(newchange, change) == CONV_DATA_SAME, NULL);
	
	osync_conv_env_free(conv_env);
	osync_env_finalize(env, NULL);
	osync_env_free(env);
	
	destroy_testbed(testbed);
}

static void compare_vnote(const char *lfilename, const char *rfilename, OSyncConvCmpResult result)
{
	char *command1 = g_strdup_printf("cp %s/%s lfile", g_get_current_dir(), lfilename);
	char *command2 = g_strdup_printf("cp %s/%s rfile", g_get_current_dir(), rfilename);
	char *testbed = setup_testbed(NULL);
	system(command1);
	g_free(command1);
	system(command2);
	g_free(command2);
	
	OSyncError *error = NULL;
	OSyncEnv *env = init_env();
	
	OSyncFormatEnv *conv_env = osync_conv_env_new(env);
	fail_unless(conv_env != NULL, NULL);

	char *buffer;
	int size;
	
	fail_unless(osync_file_read("lfile", &buffer, &size, &error), NULL);
	
	OSyncChange *lchange = osync_change_new();
	osync_change_set_uid(lchange, "lfile");
	osync_change_set_data(lchange, buffer, size + 1, TRUE);
	osync_change_set_conv_env(lchange, conv_env);
	osync_change_set_objformat_string(lchange, "plain");

	OSyncObjFormat *sourceformat = osync_change_detect_objformat(conv_env, lchange, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_change_set_objformat(lchange, sourceformat);
	osync_change_set_objtype(lchange, osync_objformat_get_objtype(sourceformat));
	
	fail_unless(osync_file_read("rfile", &buffer, &size, &error), NULL);
	
	OSyncChange *rchange = osync_change_new();
	osync_change_set_uid(rchange, "rfile");
	osync_change_set_data(rchange, buffer, size + 1, TRUE);
	osync_change_set_conv_env(rchange, conv_env);
	osync_change_set_objformat_string(rchange, "plain");

	sourceformat = osync_change_detect_objformat(conv_env, rchange, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_change_set_objformat(rchange, sourceformat);
	osync_change_set_objtype(rchange, osync_objformat_get_objtype(sourceformat));
	
	fail_unless(osync_change_compare(lchange, rchange) == result, NULL);
	
	osync_conv_env_free(conv_env);
	osync_env_finalize(env, NULL);
	osync_env_free(env);
	destroy_testbed(testbed);
}

static time_t vnote_get_revision(const char *filename)
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
	
	OSyncObjFormat *targetformat = osync_conv_find_objformat(conv_env, "xml-note");
	fail_unless(targetformat != NULL, NULL);
	
	fail_unless(osync_change_convert(conv_env, change, targetformat, &error), NULL);
	
	time_t time = osync_change_get_revision(change, &error);
	
	osync_conv_env_free(conv_env);
	osync_env_finalize(env, NULL);
	osync_env_free(env);
	
	destroy_testbed(testbed);
	return time;
}

START_TEST (conv_vnote1)
{
	conv_vnote("data/vnotes/vnote1.vnt");
}
END_TEST

START_TEST (conv_vnote2)
{
	conv_vnote("data/vnotes/vnote2.vnt");
}
END_TEST

START_TEST (conv_vnote3)
{
	conv_vnote("data/vnotes/vnote3.vnt");
}
END_TEST

START_TEST (conv_vnote_minimal)
{
	conv_vnote("data/vnotes/vnote-minimal.vnt");
}
END_TEST

START_TEST (get_revision1)
{
	fail_unless(vnote_get_revision("data/vnotes/vnote1.vnt") == 1112742000, NULL);
}
END_TEST

START_TEST (get_revision2)
{
	fail_unless(vnote_get_revision("data/vnotes/vnote2.vnt") == 1112745661, NULL);
}
END_TEST

START_TEST (get_revision3)
{
	fail_unless(vnote_get_revision("data/vnotes/vnote3.vnt") == 1112742000, NULL);
}
END_TEST

START_TEST (get_revision4)
{
	fail_unless(vnote_get_revision("data/vnotes/vnote-minimal.vnt") == -1, NULL);
}
END_TEST

START_TEST (compare_vnote_same1)
{
	compare_vnote("data/vnotes/vnote1.vnt", "data/vnotes/vnote1.vnt", CONV_DATA_SAME);
}
END_TEST

START_TEST (compare_vnote_same2)
{
	compare_vnote("data/vnotes/vnote1.vnt", "data/vnotes/vnote1-same.vnt", CONV_DATA_SAME);
}
END_TEST

START_TEST (compare_vnote_similar1)
{
	compare_vnote("data/vnotes/vnote1.vnt", "data/vnotes/vnote1-similar.vnt", CONV_DATA_SIMILAR);
}
END_TEST

START_TEST (compare_vnote_mismatch1)
{
	compare_vnote("data/vnotes/vnote1.vnt", "data/vnotes/vnote2.vnt", CONV_DATA_MISMATCH);
}
END_TEST

START_TEST (compare_vnote_mismatch2)
{
	compare_vnote("data/vnotes/vnote1.vnt", "data/vnotes/vnote-minimal.vnt", CONV_DATA_MISMATCH);
}
END_TEST

Suite *vnote_suite(void)
{
	Suite *s = suite_create("VNote");
	//Suite *s2 = suite_create("VNote");
	
	create_case(s, "conv_vnote1", conv_vnote1);
	create_case(s, "conv_vnote2", conv_vnote2);
	create_case(s, "conv_vnote3", conv_vnote3);
	create_case(s, "conv_vnote_minimal", conv_vnote_minimal);
	
	create_case(s, "get_revision1", get_revision1);
	create_case(s, "get_revision2", get_revision2);
	create_case(s, "get_revision3", get_revision3);
	create_case(s, "get_revision4", get_revision4);
	
	create_case(s, "compare_vnote_same1", compare_vnote_same1);
	create_case(s, "compare_vnote_same2", compare_vnote_same2);
	create_case(s, "compare_vnote_similar1", compare_vnote_similar1);
	create_case(s, "compare_vnote_mismatch1", compare_vnote_mismatch1);
	create_case(s, "compare_vnote_mismatch2", compare_vnote_mismatch2);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = vnote_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
