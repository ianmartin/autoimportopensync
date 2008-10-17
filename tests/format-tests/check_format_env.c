#include "support.h"

#include <opensync/opensync-module.h>
#include <opensync/opensync-format.h>

START_TEST (conv_env_create)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_format_env_free(env);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_env_register_objformat)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_format_env_register_objformat(env, format);
	
	osync_objformat_unref(format);
	
	osync_format_env_free(env);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_env_register_objformat_count)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_format_env_num_objformats(env) == 0, NULL);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	
	fail_unless(osync_format_env_num_objformats(env) == 1, NULL);
	fail_unless(osync_format_env_nth_objformat(env, 0) == format, NULL);
	
	osync_format_env_free(env);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_env_objformat_find)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	
	fail_unless(osync_format_env_find_objformat(env, "format") == format, NULL);
	
	osync_format_env_free(env);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_env_objformat_find_false)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	
	fail_unless(osync_format_env_find_objformat(env, "format2") == NULL, NULL);
	
	osync_format_env_free(env);
	
	destroy_testbed(testbed);
}
END_TEST

osync_bool convert_func(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	return TRUE;
}

START_TEST (conv_env_register_converter)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("format1", "objtype", &error);
	fail_unless(format1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format1);
	
	OSyncObjFormat *format2 = osync_objformat_new("format2", "objtype", &error);
	fail_unless(format2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format2);
	
	OSyncFormatConverter *converter = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_format_env_register_converter(env, converter);

	osync_converter_unref(converter);
	
	osync_format_env_free(env);
	
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_env_register_converter_count)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("format1", "objtype", &error);
	fail_unless(format1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format1);
	
	OSyncObjFormat *format2 = osync_objformat_new("format2", "objtype", &error);
	fail_unless(format2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format2);
	
	OSyncFormatConverter *converter = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_format_env_num_converters(env) == 0, NULL);
	
	osync_format_env_register_converter(env, converter);

	fail_unless(osync_format_env_num_converters(env) == 1, NULL);
	fail_unless(osync_format_env_nth_converter(env, 0) == converter, NULL);
	
	osync_converter_unref(converter);
	
	osync_format_env_free(env);
	
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_env_converter_find)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("format1", "objtype", &error);
	fail_unless(format1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format1);
	
	OSyncObjFormat *format2 = osync_objformat_new("format2", "objtype", &error);
	fail_unless(format2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format2);
	
	OSyncFormatConverter *converter = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_format_env_register_converter(env, converter);
	osync_converter_unref(converter);

	fail_unless(osync_format_env_find_converter(env, format1, format2) == converter, NULL);
	
	osync_format_env_free(env);
	
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_env_converter_find_false)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("format1", "objtype", &error);
	fail_unless(format1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format1);
	
	OSyncObjFormat *format2 = osync_objformat_new("format2", "objtype", &error);
	fail_unless(format2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format2);
	
	OSyncObjFormat *format3 = osync_objformat_new("format3", "objtype", &error);
	fail_unless(format3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format3);
	
	OSyncFormatConverter *converter = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_format_env_register_converter(env, converter);
	osync_converter_unref(converter);

	fail_unless(osync_format_env_find_converter(env, format1, format3) == NULL, NULL);
	fail_unless(osync_format_env_find_converter(env, format2, format1) == NULL, NULL);
	
	osync_format_env_free(env);
	
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	osync_objformat_unref(format3);
	
	destroy_testbed(testbed);
}
END_TEST

osync_bool filter_hook(OSyncData *data, const char *config)
{
	return TRUE;
}

START_TEST (conv_env_register_filter)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncCustomFilter *filter = osync_custom_filter_new("format", "objtype", "name", filter_hook, &error);
	fail_unless(filter != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_format_env_register_filter(env, filter);

	osync_custom_filter_unref(filter);
	
	osync_format_env_free(env);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_env_register_filter_count)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncCustomFilter *filter = osync_custom_filter_new("format", "objtype", "name", filter_hook, &error);
	fail_unless(filter != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_format_env_num_filters(env) == 0, NULL);
	
	osync_format_env_register_filter(env, filter);

	fail_unless(osync_format_env_num_filters(env) == 1, NULL);
	fail_unless(osync_format_env_nth_filter(env, 0) == filter, NULL);
	
	osync_custom_filter_unref(filter);
	
	osync_format_env_free(env);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_env_load_plugins)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	char *curdir = g_get_current_dir();
	fail_unless(osync_format_env_load_plugins(env, curdir, &error), NULL);
	fail_unless(error == NULL, NULL);
	g_free(curdir);
	
	osync_format_env_free(env);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_env_plugin)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncModule *module = osync_module_new(&error);
	fail_unless(module != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	char *curdir = g_get_current_dir();
	char *path = g_strdup_printf("%s/mock-format.%s", curdir, G_MODULE_SUFFIX);
	fail_unless(osync_module_load(module, path, &error), NULL);
	fail_unless(error == NULL, NULL);
	g_free(path);
	g_free(curdir);
	
	fail_unless(osync_module_get_format_info(module, env, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_module_get_conversion_info(module, env, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_format_env_num_objformats(env) == 3, NULL);
	fail_unless(osync_format_env_num_converters(env) == 2, NULL);
	
	osync_format_env_free(env);
	
	osync_module_free(module);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *format_env_suite(void)
{
	Suite *s = suite_create("Format-Env");
//	Suite *s2 = suite_create("Format-Env");
	
	create_case(s, "conv_env_create", conv_env_create);
	
	create_case(s, "conv_env_register_objformat", conv_env_register_objformat);
	create_case(s, "conv_env_register_objformat_count", conv_env_register_objformat_count);
	create_case(s, "conv_env_objformat_find", conv_env_objformat_find);
	create_case(s, "conv_env_objformat_find_false", conv_env_objformat_find_false);
	
	create_case(s, "conv_env_register_converter", conv_env_register_converter);
	create_case(s, "conv_env_register_converter_count", conv_env_register_converter_count);
	create_case(s, "conv_env_converter_find", conv_env_converter_find);
	create_case(s, "conv_env_converter_find_false", conv_env_converter_find_false);
	
	create_case(s, "conv_env_register_filter", conv_env_register_filter);
	create_case(s, "conv_env_register_filter_count", conv_env_register_filter_count);

	create_case(s, "conv_env_load_plugins", conv_env_load_plugins);
	create_case(s, "conv_env_plugin", conv_env_plugin);
	
	return s;
}

int main(void)
{
	int nf;
	
	Suite *s = format_env_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
