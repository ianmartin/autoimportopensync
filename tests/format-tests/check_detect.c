#include "support.h"

static osync_bool detect(const char *data, int size)
{
	return TRUE;
}

static osync_bool detect_false(const char *data, int size)
{
	return FALSE;
}

START_TEST (conv_env_detect_smart)
{
	OSyncError *error = NULL;

	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(error == NULL);

	OSyncObjFormat *format1 = osync_objformat_new("Format1", "Type1", &error);
	OSyncObjFormat *format2 = osync_objformat_new("Format2", "Type1", &error);
	fail_unless(error == NULL);

	OSyncFormatConverter *conv = osync_converter_new_detector(format2, format1, detect, &error);
	fail_unless(error == NULL);

	osync_format_env_register_converter(env, conv);

	mark_point();

	OSyncData *data = osync_data_new("test", 5, format2, &error);
	fail_unless(error == NULL);

	OSyncObjFormat *result = osync_format_env_detect_objformat(env, data);
	fail_unless(result == format1);
	fail_unless(osync_data_get_objformat(data) == format2);
	
	osync_data_unref(data);
	osync_format_env_free(env);
}
END_TEST

START_TEST (conv_env_detect_different_objtype)
{
	OSyncError *error = NULL;

	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(error == NULL);

	OSyncObjFormat *format1 = osync_objformat_new("Format1", "Type1", &error);
	// Different objtype!
	OSyncObjFormat *format2 = osync_objformat_new("Format2", "Type2", &error);
	fail_unless(error == NULL);

	OSyncFormatConverter *conv = osync_converter_new_detector(format2, format1, detect, &error);
	fail_unless(error == NULL);

	osync_format_env_register_converter(env, conv);

	mark_point();

	OSyncData *data = osync_data_new("test", 5, format2, &error);
	fail_unless(error == NULL);

	OSyncObjFormat *result = osync_format_env_detect_objformat(env, data);
	fail_unless(result == format1);
	fail_unless(osync_data_get_objformat(data) == format2);
	
	osync_data_unref(data);
	osync_format_env_free(env);
}
END_TEST


START_TEST (conv_env_detect_smart_no)
{
	OSyncError *error = NULL;

	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(error == NULL);

	OSyncObjFormat *format1 = osync_objformat_new("Format1", "Type1", &error);
	OSyncObjFormat *format2 = osync_objformat_new("Format2", "Type1", &error);
	fail_unless(error == NULL);

	OSyncFormatConverter *conv = osync_converter_new_detector(format2, format1, detect_false, &error);
	fail_unless(error == NULL);

	osync_format_env_register_converter(env, conv);

	mark_point();

	OSyncData *data = osync_data_new("test", 5, format2, &error);
	fail_unless(error == NULL);

	OSyncObjFormat *result = osync_format_env_detect_objformat(env, data);
	fail_unless(!result);
	fail_unless(osync_data_get_objformat(data) == format2);
	
	osync_data_unref(data);
	osync_format_env_free(env);
}
END_TEST

Suite *env_suite(void)
{
	Suite *s = suite_create("Detect");

	create_case(s, "conv_env_detect_smart", conv_env_detect_smart);
	create_case(s, "conv_env_detect_different_objtype", conv_env_detect_different_objtype);
	create_case(s, "conv_env_detect_smart_no", conv_env_detect_smart_no);

	return s;
}

int main(void)
{
	int nf;
	
	Suite *s = env_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
