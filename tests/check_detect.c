#include <check.h>
#include <glib.h>
#include <gmodule.h>
#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>
#include <stdlib.h>
#include <string.h>

START_TEST (conv_env_detect_smart)
{
	OSyncEnv *osync = init_env();
	OSyncFormatEnv *env = osync_conv_env_new(osync);
	OSyncObjType *type = osync_env_register_objtype(env, "Type1", FALSE);
	OSyncObjFormat *format1 = osync_env_register_objformat(type, "Format1", NULL);
	osync_env_format_set_detect_func(format1, detect_format1);
	mark_point();
	OSyncChange *change = osync_change_new();
	osync_change_set_objformat(change, format1);
	osync_change_set_data(change, "test", 5, TRUE);
	
	fail_unless(osync_conv_detect_change_format(env, change), NULL);
	fail_unless(change->objtype == type, NULL);
	fail_unless(g_list_length(change->objformats) == 1, NULL);
}
END_TEST

START_TEST (conv_env_detect_smart2)
{
	OSyncEnv *osync = init_env();
	OSyncFormatEnv *env = osync_conv_env_new(osync);
	OSyncObjType *type = osync_env_register_objtype(env, "Type1", FALSE);
	OSyncObjFormat *format1 = osync_env_register_objformat(type, "Format1", NULL);
	OSyncObjFormat *format2 = osync_env_register_objformat(type, "Format2", NULL);

	mark_point();
	OSyncChange *change = osync_change_new();

}
END_TEST

START_TEST (conv_env_detect_smart_no)
{
	OSyncEnv *osync = init_env();
	OSyncFormatEnv *env = osync_conv_env_new(osync);
	OSyncObjType *type = osync_env_register_objtype(env, "Type1", FALSE);
	OSyncObjFormat *format1 = osync_env_register_objformat(type, "Format1", NULL);
	osync_env_format_set_detect_func(format1, detect_format1_no);
	mark_point();
	OSyncChange *change = osync_change_new();
	osync_change_set_objformat(change, format1);
	osync_change_set_data(change, "test", 5, TRUE);
	
	fail_unless(!osync_conv_detect_change_format(env, change), NULL);
	fail_unless(change->objtype == NULL, NULL);
	fail_unless(g_list_length(change->objformats) == 1, NULL);
}
END_TEST

Suite *env_suite(void)
{
	Suite *s = suite_create("Detect");
	TCase *tc_smart = tcase_create("smart");
	TCase *tc_data = tcase_create("data");
	suite_add_tcase (s, tc_smart);
	suite_add_tcase (s, tc_data);
	
	tcase_add_test(tc_smart, conv_env_detect_smart);
	tcase_add_test(tc_smart, conv_env_detect_smart2);
	tcase_add_test(tc_smart, conv_env_detect_smart_no);
	return s;
}

int main(void)
{
	int nf;
	
	Suite *s = env_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_set_fork_status (sr, CK_NOFORK);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
