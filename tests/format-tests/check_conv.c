#include "support.h"

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

osync_bool convert_func(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void* userdata, OSyncError **error)
{
	*free_input = TRUE;
	*output = g_strdup("test");
	*outpsize = 5;
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

#if 0

static osync_bool dummyconvert(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	*free_input = TRUE;
	*output = g_strdup("test");
	*outpsize = 5;
	return TRUE;
}

static char dummy_data[1] = { 0 };

static OSyncChange *create_change(OSyncObjFormat *fmt, char *data, size_t datasize)
{
	OSyncChange *chg = osync_change_new();
	osync_change_set_objformat(chg, fmt);
	osync_change_set_data(chg, data, datasize, TRUE);
	return chg;
}
#endif

START_TEST (conv_find_path)
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

	OSyncData *data1 = osync_data_new("data", 5, format1, &error);
	fail_unless(data1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
 
	OSyncData *data2 = osync_data_new("data", 5, format2, &error);
	fail_unless(data2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
 

	OSyncFormatConverterPath *path = osync_format_env_find_path_with_detectors(env, data1, format2, NULL, &error);
	fail_unless(path != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_converter_path_num_edges(path) == 1, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 0) == converter, NULL);
	
	osync_converter_path_unref(path);
	
	// intended to fail
	path = osync_format_env_find_path_with_detectors(env, data2, format1, NULL, &error);
	fail_unless(path == NULL, NULL);
	fail_unless(error != NULL, NULL);
	
	osync_format_env_free(env);
	
	osync_data_unref(data1);
	osync_data_unref(data2);
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_find_path2)
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
	
	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format3, convert_func, &error);
	fail_unless(converter2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	OSyncData *data1 = osync_data_new("data", 5, format1, &error);
	fail_unless(data1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
 

	OSyncFormatConverterPath *path = osync_format_env_find_path_with_detectors(env, data1, format3, NULL, &error);
	fail_unless(path != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_converter_path_num_edges(path) == 2, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 0) == converter1, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 1) == converter2, NULL);
	
	osync_converter_path_unref(path);
	
	osync_format_env_free(env);
	
	osync_data_unref(data1);
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	osync_objformat_unref(format3);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_find_path_false)
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
	
	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format2, convert_func, &error);
	fail_unless(converter2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	OSyncData *data1 = osync_data_new("data", 5, format1, &error);
	fail_unless(data1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
 
	// intended to fail
	OSyncFormatConverterPath *path = osync_format_env_find_path_with_detectors(env, data1, format3, NULL, &error);
	fail_unless(path == NULL, NULL);
	fail_unless(error != NULL, NULL);
	
	osync_format_env_free(env);
	
	osync_data_unref(data1);
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	osync_objformat_unref(format3);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_find_multi_path)
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
	
	OSyncObjFormat *format4 = osync_objformat_new("format4", "objtype", &error);
	fail_unless(format4 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format4);
	
	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format4, convert_func, &error);
	fail_unless(converter2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);
	
	OSyncFormatConverter *converter3 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format3, convert_func, &error);
	fail_unless(converter3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter3);
	osync_converter_unref(converter3);
	
	OSyncFormatConverter *converter4 = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format4, convert_func, &error);
	fail_unless(converter4 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter4);
	osync_converter_unref(converter4);

	OSyncData *data1 = osync_data_new("data", 5, format1, &error);
	fail_unless(data1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
 
	OSyncFormatConverterPath *path = osync_format_env_find_path_with_detectors(env, data1, format4, NULL, &error);
	fail_unless(path != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_converter_path_num_edges(path) == 2, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 0) == converter1, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 1) == converter2, NULL);
	
	osync_converter_path_unref(path);
	
	osync_format_env_free(env);
	
	osync_data_unref(data1);
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	osync_objformat_unref(format3);
	osync_objformat_unref(format4);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_find_multi_path_with_preferred)
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
	
	OSyncObjFormat *format4 = osync_objformat_new("format4", "objtype", &error);
	fail_unless(format4 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format4);
	
	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format4, convert_func, &error);
	fail_unless(converter2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);
	
	OSyncFormatConverter *converter3 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format3, convert_func, &error);
	fail_unless(converter3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter3);
	osync_converter_unref(converter3);
	
	OSyncFormatConverter *converter4 = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format4, convert_func, &error);
	fail_unless(converter4 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter4);
	osync_converter_unref(converter4);

	OSyncData *data1 = osync_data_new("data", 5, format1, &error);
	fail_unless(data1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
 
	OSyncFormatConverterPath *path = osync_format_env_find_path_with_detectors(env, data1, format4, "format3", &error);
	fail_unless(path != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_converter_path_num_edges(path) == 2, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 0) == converter3, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 1) == converter4, NULL);
	
	osync_converter_path_unref(path);
	
	osync_format_env_free(env);
	
	osync_data_unref(data1);
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	osync_objformat_unref(format3);
	osync_objformat_unref(format4);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_find_circular_false)
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
	
	OSyncObjFormat *format4 = osync_objformat_new("format4", "objtype", &error);
	fail_unless(format4 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format4);
	
	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format3, convert_func, &error);
	fail_unless(converter2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);
	
	OSyncFormatConverter *converter3 = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format1, convert_func, &error);
	fail_unless(converter3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter3);
	osync_converter_unref(converter3);

	OSyncData *data1 = osync_data_new("data", 5, format1, &error);
	fail_unless(data1 != NULL, NULL);
	fail_unless(error == NULL, NULL);

	// intended to fail
	OSyncFormatConverterPath *path = osync_format_env_find_path_with_detectors(env, data1, format4, NULL, &error);
	fail_unless(path == NULL, NULL);
	fail_unless(error != NULL, NULL);
	
	osync_format_env_free(env);
	
	osync_data_unref(data1);
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	osync_objformat_unref(format3);
	osync_objformat_unref(format4);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_find_complex)
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
	
	OSyncObjFormat *format4 = osync_objformat_new("format4", "objtype", &error);
	fail_unless(format4 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format4);
	
	OSyncObjFormat *format5 = osync_objformat_new("format5", "objtype", &error);
	fail_unless(format5 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format5);
	
	OSyncObjFormat *format6 = osync_objformat_new("format6", "objtype", &error);
	fail_unless(format6 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format6);
	
	OSyncObjFormat *format7 = osync_objformat_new("format7", "objtype", &error);
	fail_unless(format7 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format7);
	
	OSyncObjFormat *format8 = osync_objformat_new("format8", "objtype", &error);
	fail_unless(format8 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format8);
	
	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format3, convert_func, &error);
	fail_unless(converter2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);
	
	OSyncFormatConverter *converter3 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format4, convert_func, &error);
	fail_unless(converter3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter3);
	osync_converter_unref(converter3);
	
	OSyncFormatConverter *converter4 = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format5, convert_func, &error);
	fail_unless(converter4 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter4);
	osync_converter_unref(converter4);
	
	OSyncFormatConverter *converter5 = osync_converter_new(OSYNC_CONVERTER_CONV, format4, format7, convert_func, &error);
	fail_unless(converter5 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter5);
	osync_converter_unref(converter5);
	
	OSyncFormatConverter *converter6 = osync_converter_new(OSYNC_CONVERTER_CONV, format5, format7, convert_func, &error);
	fail_unless(converter6 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter6);
	osync_converter_unref(converter6);
	
	OSyncFormatConverter *converter7 = osync_converter_new(OSYNC_CONVERTER_CONV, format7, format8, convert_func, &error);
	fail_unless(converter7 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter7);
	osync_converter_unref(converter7);
	
	OSyncFormatConverter *converter8 = osync_converter_new(OSYNC_CONVERTER_CONV, format8, format6, convert_func, &error);
	fail_unless(converter8 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter8);
	osync_converter_unref(converter8);
	
	OSyncFormatConverter *converter9 = osync_converter_new(OSYNC_CONVERTER_CONV, format5, format6, convert_func, &error);
	fail_unless(converter9 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter9);
	osync_converter_unref(converter9);

	OSyncData *data1 = osync_data_new("data", 5, format1, &error);
	fail_unless(data1 != NULL, NULL);
	fail_unless(error == NULL, NULL);

	OSyncFormatConverterPath *path = osync_format_env_find_path_with_detectors(env, data1, format6, NULL, &error);
	fail_unless(path != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_converter_path_num_edges(path) == 3, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 0) == converter2, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 1) == converter4, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 2) == converter9, NULL);
	
	osync_converter_path_unref(path);
	
	osync_format_env_free(env);
	
	osync_data_unref(data1);
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	osync_objformat_unref(format3);
	osync_objformat_unref(format4);
	osync_objformat_unref(format5);
	osync_objformat_unref(format6);
	osync_objformat_unref(format7);
	osync_objformat_unref(format8);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_find_multi_target)
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
	
	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format3, convert_func, &error);
	fail_unless(converter2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	OSyncList *targets = NULL; 
	targets = osync_list_prepend(targets, format2);
	targets = osync_list_prepend(targets, format3);

	OSyncData *data1 = osync_data_new("data", 5, format1, &error);
	fail_unless(data1 != NULL, NULL);
	fail_unless(error == NULL, NULL);

	OSyncFormatConverterPath *path = osync_format_env_find_path_formats_with_detectors(env, data1, targets, NULL, &error);
	fail_unless(path != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_converter_path_num_edges(path) == 1, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 0) == converter1, NULL);
	
	osync_converter_path_unref(path);
	
	osync_format_env_free(env);
	
	osync_data_unref(data1);
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	osync_objformat_unref(format3);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_find_multi_target2)
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
	
	OSyncObjFormat *format4 = osync_objformat_new("format4", "objtype", &error);
	fail_unless(format4 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format4);
	
	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format4, convert_func, &error);
	fail_unless(converter2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);
	
	OSyncFormatConverter *converter3 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format3, convert_func, &error);
	fail_unless(converter3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter3);
	osync_converter_unref(converter3);

	OSyncList *targets = NULL; 
	targets = osync_list_prepend(targets, format4);
	targets = osync_list_prepend(targets, format3);

	OSyncData *data1 = osync_data_new("data", 5, format1, &error);
	fail_unless(data1 != NULL, NULL);
	fail_unless(error == NULL, NULL);

	OSyncFormatConverterPath *path = osync_format_env_find_path_formats_with_detectors(env, data1, targets, NULL, &error);
	fail_unless(path != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_converter_path_num_edges(path) == 1, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 0) == converter3, NULL);
	
	osync_converter_path_unref(path);
	
	osync_format_env_free(env);
	
	osync_data_unref(data1);
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	osync_objformat_unref(format3);
	osync_objformat_unref(format4);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_find_multi_path_multi_target)
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
	
	OSyncObjFormat *format4 = osync_objformat_new("format4", "objtype", &error);
	fail_unless(format4 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format4);

	OSyncObjFormat *format5 = osync_objformat_new("format5", "objtype", &error);
	fail_unless(format5 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format5);
	
	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format4, convert_func, &error);
	fail_unless(converter2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);
	
	OSyncFormatConverter *converter3 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format3, convert_func, &error);
	fail_unless(converter3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter3);
	osync_converter_unref(converter3);
	
	OSyncFormatConverter *converter4 = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format4, convert_func, &error);
	fail_unless(converter4 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter4);
	osync_converter_unref(converter4);

	OSyncFormatConverter *converter5 = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format5, convert_func, &error);
	fail_unless(converter5 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter5);
	osync_converter_unref(converter5);

	OSyncList *targets = NULL; 
	targets = osync_list_prepend(targets, format4);
	targets = osync_list_prepend(targets, format5);

	OSyncData *data1 = osync_data_new("data", 5, format1, &error);
	fail_unless(data1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
 
	OSyncFormatConverterPath *path = osync_format_env_find_path_formats_with_detectors(env, data1, targets, NULL, &error);
	fail_unless(path != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_converter_path_num_edges(path) == 2, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 0) == converter1, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 1) == converter2, NULL);
	
	osync_converter_path_unref(path);
	
	osync_format_env_free(env);
	
	osync_data_unref(data1);
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	osync_objformat_unref(format3);
	osync_objformat_unref(format4);
	osync_objformat_unref(format5);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (conv_find_multi_path_multi_target_with_preferred)
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
	
	OSyncObjFormat *format4 = osync_objformat_new("format4", "objtype", &error);
	fail_unless(format4 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format4);

	OSyncObjFormat *format5 = osync_objformat_new("format5", "objtype", &error);
	fail_unless(format5 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format5);
	
	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_func, &error);
	fail_unless(converter1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format4, convert_func, &error);
	fail_unless(converter2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);
	
	OSyncFormatConverter *converter3 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format3, convert_func, &error);
	fail_unless(converter3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter3);
	osync_converter_unref(converter3);
	
	OSyncFormatConverter *converter4 = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format4, convert_func, &error);
	fail_unless(converter4 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter4);
	osync_converter_unref(converter4);

	OSyncFormatConverter *converter5 = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format5, convert_func, &error);
	fail_unless(converter5 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter5);
	osync_converter_unref(converter5);

	OSyncList *targets = NULL; 
	targets = osync_list_prepend(targets, format4);
	targets = osync_list_prepend(targets, format5);

	OSyncData *data1 = osync_data_new("data", 5, format1, &error);
	fail_unless(data1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
 
	OSyncFormatConverterPath *path = osync_format_env_find_path_formats_with_detectors(env, data1, targets, "format5", &error);
	fail_unless(path != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_converter_path_num_edges(path) == 2, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 0) == converter3, NULL);
	fail_unless(osync_converter_path_nth_edge(path, 1) == converter5, NULL);
	
	osync_converter_path_unref(path);
	
	osync_format_env_free(env);
	
	osync_data_unref(data1);
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	osync_objformat_unref(format3);
	osync_objformat_unref(format4);
	osync_objformat_unref(format5);
	
	destroy_testbed(testbed);
}
END_TEST

static osync_bool convert_addtest(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	*free_input = TRUE;
	*output = g_strdup_printf("%stest", input);
	*outpsize = inpsize + 4;
	return TRUE;
}

static osync_bool convert_remtest(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	*free_input = TRUE;
	*output = strdup(input);
	char *test = g_strrstr(*output, "test");
	*outpsize = 0;
	if (test) {
		test[0] = 0;
		*outpsize = inpsize - 4;
		return TRUE;
	} else {
		output = NULL;
		return FALSE;
	}
}

static osync_bool convert_addtest2(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	*output = g_strdup_printf("%stest2", input);
	*outpsize = inpsize + 5;
	*free_input = TRUE;
	return TRUE;
}

static osync_bool convert_remtest2(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	*free_input = TRUE;
	*output = strdup(input);
	char *test = g_strrstr(*output, "test2");
	*outpsize = 0;
	if (test) {
		test[0] = 0;
		*outpsize = inpsize - 4;
		return TRUE;
	} else {
		output = NULL;
		return FALSE;
	}
}

START_TEST (conv_env_convert1)
{
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("F1", "O1", &error);
	fail_unless(format1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format1);
	
	OSyncObjFormat *format2 = osync_objformat_new("F2", "O1", &error);
	fail_unless(format2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format2);
	
	OSyncObjFormat *format3 = osync_objformat_new("F3", "O1", &error);
	fail_unless(format3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format3);

	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_addtest, &error);
	fail_unless(converter1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format3, convert_addtest2, &error);
	fail_unless(converter2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	mark_point();

	OSyncData *data = osync_data_new("data", 5, format1, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);
    
	mark_point();

	OSyncFormatConverterPath *path = NULL;
	path = osync_format_env_find_path_with_detectors(env, data, format3, NULL, &error);
	osync_format_env_convert(env, path, data, &error); 
	fail_unless(error == NULL, NULL);

	char *buf;
	unsigned int size;
	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "datatesttest2"), NULL);
	OSyncObjFormat *format = osync_data_get_objformat(data);
	fail_unless(format == format3, NULL);
}
END_TEST

START_TEST (conv_env_convert_back)
{
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("F1", "O1", &error);
	fail_unless(format1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format1);
	
	OSyncObjFormat *format2 = osync_objformat_new("F2", "O1", &error);
	fail_unless(format2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format2);
	
	OSyncObjFormat *format3 = osync_objformat_new("F3", "O1", &error);
	fail_unless(format3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format3);

	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, convert_addtest, &error);
	fail_unless(converter1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);

	OSyncFormatConverter *converter1_back = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format1, convert_remtest, &error);
	fail_unless(converter1_back != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1_back);
	osync_converter_unref(converter1_back);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format3, convert_addtest2, &error);
	fail_unless(converter2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	OSyncFormatConverter *converter2_back = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format2, convert_remtest2, &error);
	fail_unless(converter2_back != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2_back);
	osync_converter_unref(converter2_back);

	mark_point();

	OSyncData *data = osync_data_new("data", 5, format1, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);
  
	mark_point();

	OSyncFormatConverterPath *path = NULL;
	path = osync_format_env_find_path_with_detectors(env, data, format3, NULL, &error);
	osync_format_env_convert(env, path, data, &error); 
	fail_unless(error == NULL, NULL);

	char *buf;
	unsigned int size;
	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "datatesttest2"), NULL);
	OSyncObjFormat *format = osync_data_get_objformat(data);
	fail_unless(format == format3, NULL);
	fail_unless(path != NULL, NULL);

	mark_point();

	path = NULL;
	path = osync_format_env_find_path_with_detectors(env, data, format1, NULL, &error);
	osync_format_env_convert(env, path, data, &error); 
	fail_unless(error == NULL, NULL);
	fail_unless(path != NULL, NULL);

	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "data"), NULL);
	format = osync_data_get_objformat(data);
	fail_unless(format == format1, NULL);

}
END_TEST

START_TEST (conv_env_convert_desenc)
{
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("F1", "O1", &error);
	fail_unless(format1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format1);
	
	OSyncObjFormat *format2 = osync_objformat_new("F2", "O1", &error);
	fail_unless(format2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format2);
	
	OSyncObjFormat *format3 = osync_objformat_new("F3", "O1", &error);
	fail_unless(format3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_objformat(env, format3);

	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_DECAP, format1, format2, convert_addtest, &error);
	fail_unless(converter1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);

	OSyncFormatConverter *converter1_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format2, format1, convert_remtest, &error);
	fail_unless(converter1_back != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter1_back);
	osync_converter_unref(converter1_back);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_DECAP, format2, format3, convert_addtest2, &error);
	fail_unless(converter2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	OSyncFormatConverter *converter2_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format3, format2, convert_remtest2, &error);
	fail_unless(converter2_back != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, converter2_back);
	osync_converter_unref(converter2_back);

	mark_point();

	OSyncData *data = osync_data_new("data", 5, format1, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);
  
	mark_point();

	OSyncFormatConverterPath *path = NULL;
	path = osync_format_env_find_path_with_detectors(env, data, format3, NULL, &error);
	osync_format_env_convert(env, path, data, &error); 
	fail_unless(error == NULL, NULL);

	char *buf;
	unsigned int size;
	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "datatesttest2"), NULL);
	OSyncObjFormat *format = osync_data_get_objformat(data);
	fail_unless(format == format3, NULL);
	fail_unless(path != NULL, NULL);

	mark_point();

	path = NULL;
	path = osync_format_env_find_path_with_detectors(env, data, format1, NULL, &error);
	osync_format_env_convert(env, path, data, &error); 
	fail_unless(error == NULL, NULL);
	fail_unless(path != NULL, NULL);

	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "data"), NULL);
	format = osync_data_get_objformat(data);
	fail_unless(format == format1, NULL);
}
END_TEST

static osync_bool detect_true(const char *data, int size, void *userdata)
{
	return TRUE;
}

static osync_bool detect_false(const char *data, int size, void *userdata)
{
	return FALSE;
}

START_TEST (conv_env_convert_desenc_complex)
{
  /* Test if the converter is going on the righ path, when the data detector
   * for the format reports a specific lower format
   */

	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("F1", "O1", &error);
	osync_format_env_register_objformat(env, format1);

	OSyncObjFormat *format2 = osync_objformat_new("F2", "O1", &error);
	osync_format_env_register_objformat(env, format2);
	
	OSyncObjFormat *format3 = osync_objformat_new("F3", "O1", &error);
	osync_format_env_register_objformat(env, format3);

	OSyncObjFormat *format4 = osync_objformat_new("F4", "O1", &error);
	osync_format_env_register_objformat(env, format4);

	OSyncObjFormat *format5 = osync_objformat_new("F5", "O1", &error);
	osync_format_env_register_objformat(env, format5);

	OSyncObjFormat *format6 = osync_objformat_new("F6", "O1", &error);
	osync_format_env_register_objformat(env, format6);

	/*** Detectors ***/

	OSyncFormatConverter *detector1 = osync_converter_new_detector(format2, format4, detect_true, &error);
	fail_unless(detector1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, detector1);
//	osync_converter_unref(detector1);

	OSyncFormatConverter *detector2 = osync_converter_new_detector(format2, format3, detect_false, &error);
	fail_unless(detector2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, detector2);
//	osync_converter_unref(detector2);

	/*** Converter ***/

	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_DECAP, format1, format2, convert_addtest, &error);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);

	OSyncFormatConverter *converter1_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format2, format1, convert_remtest, &error);
	osync_format_env_register_converter(env, converter1_back);
	osync_converter_unref(converter1_back);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_DECAP, format2, format3, convert_addtest2, &error);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	OSyncFormatConverter *converter2_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format3, format2, convert_remtest2, &error);
	osync_format_env_register_converter(env, converter2_back);
	osync_converter_unref(converter2_back);

	OSyncFormatConverter *converter3 = osync_converter_new(OSYNC_CONVERTER_DECAP, format2, format4, convert_addtest2, &error);
	osync_format_env_register_converter(env, converter3);
	osync_converter_unref(converter3);

	OSyncFormatConverter *converter3_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format4, format2, convert_remtest2, &error);
	osync_format_env_register_converter(env, converter3_back);
	osync_converter_unref(converter3_back);

	OSyncFormatConverter *converter4 = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format6, convert_addtest2, &error);
	osync_format_env_register_converter(env, converter4);
	osync_converter_unref(converter4);

	OSyncFormatConverter *converter5 = osync_converter_new(OSYNC_CONVERTER_CONV, format4, format5, convert_addtest2, &error);
	osync_format_env_register_converter(env, converter5);
	osync_converter_unref(converter5);

	OSyncFormatConverter *converter6 = osync_converter_new(OSYNC_CONVERTER_CONV, format5, format4, convert_remtest2, &error);
	osync_format_env_register_converter(env, converter6);
	osync_converter_unref(converter6);

	OSyncFormatConverter *converter7 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format5, format6, convert_addtest2, &error);
	osync_format_env_register_converter(env, converter7);
	osync_converter_unref(converter7);

	OSyncFormatConverter *converter7_back = osync_converter_new(OSYNC_CONVERTER_DECAP, format6, format5, convert_remtest2, &error);
	osync_format_env_register_converter(env, converter7_back);
	osync_converter_unref(converter7_back);

	mark_point();

	OSyncData *data = osync_data_new("data", 5, format1, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);
 
	mark_point();


	OSyncFormatConverterPath *path = NULL;
	path = osync_format_env_find_path_with_detectors(env, data, format6, NULL, &error);
	fail_unless(path != NULL, NULL);

	osync_format_env_convert(env, path, data, &error); 
	fail_unless(error == NULL, NULL);

	char *buf;
	unsigned int size;
	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "datatesttest2test2test2"), NULL);
	OSyncObjFormat *format = osync_data_get_objformat(data);
	fail_unless(format == format6, NULL);

	mark_point();
	path = NULL;
	path = osync_format_env_find_path_with_detectors(env, data, format1, NULL, &error);
	fail_unless(path != NULL, NULL);

	osync_format_env_convert(env, path, data, &error); 
	fail_unless(error == NULL, NULL);

	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "data"), NULL);
	format = osync_data_get_objformat(data);
	fail_unless(format == format1, NULL);
}
END_TEST

START_TEST (conv_env_detect_and_convert)
{
  /* The data will be detected as F3, so the shortest path should
   * not be taken because the path searching function should see that
   * the encapsulated data * is a F3 object, not a F4 object
   */
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("F1", "O1", &error);
	osync_format_env_register_objformat(env, format1);

	OSyncObjFormat *format2 = osync_objformat_new("F2", "O1", &error);
	osync_format_env_register_objformat(env, format2);
	
	OSyncObjFormat *format3 = osync_objformat_new("F3", "O1", &error);
	osync_format_env_register_objformat(env, format3);

	OSyncObjFormat *format4 = osync_objformat_new("F4", "O1", &error);
	osync_format_env_register_objformat(env, format4);

	/*** Detectors ***/

	OSyncFormatConverter *detector1 = osync_converter_new_detector(format1, format2, detect_true, &error);
	fail_unless(detector1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, detector1);
//	osync_converter_unref(detector1);

	/* Detect F3, not F4 */

	OSyncFormatConverter *detector2 = osync_converter_new_detector(format2, format3, detect_true, &error);
	fail_unless(detector2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, detector2);
//	osync_converter_unref(detector2);

	OSyncFormatConverter *detector3 = osync_converter_new_detector(format2, format4, detect_false, &error);
	fail_unless(detector3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, detector3);
//	osync_converter_unref(detector3);

	/*** Converter ***/

	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_DECAP, format1, format2, convert_addtest, &error);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);

	OSyncFormatConverter *converter1_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format2, format1, convert_remtest, &error);
	osync_format_env_register_converter(env, converter1_back);
	osync_converter_unref(converter1_back);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_DECAP, format2, format3, convert_addtest2, &error);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	OSyncFormatConverter *converter2_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format3, format2, convert_remtest2, &error);
	osync_format_env_register_converter(env, converter2_back);
	osync_converter_unref(converter2_back);

	OSyncFormatConverter *converter3 = osync_converter_new(OSYNC_CONVERTER_DECAP, format2, format4, convert_addtest2, &error);
	osync_format_env_register_converter(env, converter3);
	osync_converter_unref(converter3);

	OSyncFormatConverter *converter3_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format4, format2, convert_remtest2, &error);
	osync_format_env_register_converter(env, converter3_back);
	osync_converter_unref(converter3_back);

	OSyncFormatConverter *converter4 = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format4, convert_addtest2, &error);
	osync_format_env_register_converter(env, converter4);
	osync_converter_unref(converter4);

	OSyncFormatConverter *converter5 = osync_converter_new(OSYNC_CONVERTER_CONV, format4, format3, convert_remtest2, &error);
	osync_format_env_register_converter(env, converter5);
	osync_converter_unref(converter5);


	mark_point();

	OSyncData *data = osync_data_new("data", 5, format1, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);

	mark_point();

	OSyncFormatConverterPath *path = NULL;
	path = osync_format_env_find_path_with_detectors(env, data, format4, NULL, &error);
	fail_unless(path != NULL, NULL);

	osync_format_env_convert(env, path, data, &error);
	fail_unless(error == NULL, NULL);

	char *buf;
	unsigned int size;
	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "datatesttest2test2"), NULL);
	OSyncObjFormat *format = osync_data_get_objformat(data);
	fail_unless(format == format4, NULL);

	mark_point();

	path = osync_format_env_find_path_with_detectors(env, data, format1, NULL, &error);
	fail_unless(path != NULL, NULL);

	osync_format_env_convert(env, path, data, &error);
	fail_unless(error == NULL, NULL);

	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "datatest"), NULL);
	format = osync_data_get_objformat(data);
	fail_unless(format == format1, NULL);
}
END_TEST

START_TEST(conv_prefer_not_desencap)
{
  /* Test if the converter is getting the path that have no
   * lossy detectors
   *
   * F1 -- F2 -- F3 -- F5
   *   \              /
   *     --- F4 -----
   *
   * All converters are not lossy, except F1->F4.
   * The result path should be: F1 -> F2 -> F3 -> F5
   */
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("F1", "O1", &error);
	osync_format_env_register_objformat(env, format1);

	OSyncObjFormat *format2 = osync_objformat_new("F2", "O1", &error);
	osync_format_env_register_objformat(env, format2);
	
	OSyncObjFormat *format3 = osync_objformat_new("F3", "O1", &error);
	osync_format_env_register_objformat(env, format3);

	OSyncObjFormat *format4 = osync_objformat_new("F4", "O1", &error);
	osync_format_env_register_objformat(env, format4);

	OSyncObjFormat *format5 = osync_objformat_new("F5", "O1", &error);
	osync_format_env_register_objformat(env, format5);

	/*** Detectors ***/

	OSyncFormatConverter *detector1 = osync_converter_new_detector(format1, format2, detect_true, &error);
	fail_unless(detector1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, detector1);
//	osync_converter_unref(detector1);

	/* Detect F3, not F4 */

	OSyncFormatConverter *detector2 = osync_converter_new_detector(format2, format3, detect_true, &error);
	fail_unless(detector2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, detector2);
//	osync_converter_unref(detector2);

	OSyncFormatConverter *detector3 = osync_converter_new_detector(format2, format4, detect_false, &error);
	fail_unless(detector3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, detector3);
//	osync_converter_unref(detector3);

	/*** Converter ***/

	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format1, format2, convert_addtest, &error);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);

	OSyncFormatConverter *converter1_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format2, format3, convert_addtest, &error);
	osync_format_env_register_converter(env, converter1_back);
	osync_converter_unref(converter1_back);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format3, format5, convert_addtest, &error);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	OSyncFormatConverter *converter2_back = osync_converter_new(OSYNC_CONVERTER_DECAP, format1, format4, convert_addtest2, &error);
	osync_format_env_register_converter(env, converter2_back);
	osync_converter_unref(converter2_back);

	OSyncFormatConverter *converter3 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format4, format5, convert_addtest2, &error);
	osync_format_env_register_converter(env, converter3);
	osync_converter_unref(converter3);

	mark_point();

	OSyncData *data = osync_data_new("data", 5, format1, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);

	mark_point();

	OSyncFormatConverterPath *path = NULL;
	path = osync_format_env_find_path_with_detectors(env, data, format5, NULL, &error);
	fail_unless(path != NULL, NULL);

	osync_format_env_convert(env, path, data, &error);
	fail_unless(error == NULL, NULL);

	mark_point();

	char *buf;
	unsigned int size;
	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "datatesttesttest"), NULL);
	OSyncObjFormat *format = osync_data_get_objformat(data);
	fail_unless(format == format5, NULL);
}
END_TEST

START_TEST(conv_prefer_same_objtype)
{
  /* Test if the converter is getting the path
   * that doesn't change the objtype, even
   * if it is longer.
   *
   * Objtypes: F and G
   *
   * F1 -- F2 -- F3 -- F5 -- F6
   *   \
   *     --- G1
   *
   * The target list will be [ F6, G1 ].
   *
   * The result should be F6.
   */
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("F1", "F", &error);
	osync_format_env_register_objformat(env, format1);

	OSyncObjFormat *format2 = osync_objformat_new("F2", "F", &error);
	osync_format_env_register_objformat(env, format2);
	
	OSyncObjFormat *format3 = osync_objformat_new("F3", "F", &error);
	osync_format_env_register_objformat(env, format3);

	OSyncObjFormat *format4 = osync_objformat_new("F4", "F", &error);
	osync_format_env_register_objformat(env, format4);

	OSyncObjFormat *format5 = osync_objformat_new("F5", "F", &error);
	osync_format_env_register_objformat(env, format5);

	OSyncObjFormat *format6 = osync_objformat_new("F6", "F", &error);
	osync_format_env_register_objformat(env, format6);

	OSyncObjFormat *format_g1 = osync_objformat_new("G1", "G", &error);
	osync_format_env_register_objformat(env, format_g1);

	/*** Converter ***/

	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format1, format2, convert_addtest, &error);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);

	OSyncFormatConverter *converter1_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format2, format3, convert_addtest, &error);
	osync_format_env_register_converter(env, converter1_back);
	osync_converter_unref(converter1_back);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format3, format4, convert_addtest, &error);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	OSyncFormatConverter *converter3 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format4, format5, convert_addtest, &error);
	osync_format_env_register_converter(env, converter3);
	osync_converter_unref(converter3);

	OSyncFormatConverter *converter4 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format5, format6, convert_addtest, &error);
	osync_format_env_register_converter(env, converter4);
	osync_converter_unref(converter4);

	OSyncFormatConverter *converter5 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format1, format_g1, convert_addtest2, &error);
	osync_format_env_register_converter(env, converter5);
	osync_converter_unref(converter5);

	mark_point();

	OSyncData *data = osync_data_new("data", 5, format1, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);

	mark_point();

	OSyncList* fmtlist = NULL;
	fmtlist = osync_list_prepend(fmtlist, format_g1);
	fmtlist = osync_list_prepend(fmtlist, format6);

	OSyncFormatConverterPath *path = NULL;
	path = osync_format_env_find_path_formats_with_detectors(env, data, fmtlist, NULL, &error);
	fail_unless(path != NULL, NULL);

	osync_format_env_convert(env, path, data, &error);
	fail_unless(error == NULL, NULL);

	mark_point();

	char *buf;
	unsigned int size;
	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "datatesttesttesttesttest"), NULL);
	OSyncObjFormat *format = osync_data_get_objformat(data);
	fail_unless(format == format6, NULL);
}
END_TEST

START_TEST(conv_prefer_not_lossy_objtype_change)
{
  /* Test if the converter is getting the path
   * that have no lossy converters, even if
   * the objtype is being changed.
   *
   * Objtypes: F and G
   *
   * F1 -- F2 -- F3 -- F5 -- F6
   *   \
   *     --- G1
   *
   * The target list will be [ F6, G1 ].
   *
   * The converter F2 -> F3 is lossy.
   *
   * The result should be G1.
   */
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("F1", "F", &error);
	osync_format_env_register_objformat(env, format1);

	OSyncObjFormat *format2 = osync_objformat_new("F2", "F", &error);
	osync_format_env_register_objformat(env, format2);
	
	OSyncObjFormat *format3 = osync_objformat_new("F3", "F", &error);
	osync_format_env_register_objformat(env, format3);

	OSyncObjFormat *format4 = osync_objformat_new("F4", "F", &error);
	osync_format_env_register_objformat(env, format4);

	OSyncObjFormat *format5 = osync_objformat_new("F5", "F", &error);
	osync_format_env_register_objformat(env, format5);

	OSyncObjFormat *format6 = osync_objformat_new("F6", "F", &error);
	osync_format_env_register_objformat(env, format6);

	OSyncObjFormat *format_g1 = osync_objformat_new("G1", "G", &error);
	osync_format_env_register_objformat(env, format_g1);

	/*** Converter ***/

	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format1, format2, convert_addtest, &error);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);

	OSyncFormatConverter *converter1_back = osync_converter_new(OSYNC_CONVERTER_DECAP, format2, format3, convert_addtest, &error); /* Lossy */
	osync_format_env_register_converter(env, converter1_back);
	osync_converter_unref(converter1_back);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format3, format4, convert_addtest, &error);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	OSyncFormatConverter *converter3 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format4, format5, convert_addtest, &error);
	osync_format_env_register_converter(env, converter3);
	osync_converter_unref(converter3);

	OSyncFormatConverter *converter4 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format5, format6, convert_addtest, &error);
	osync_format_env_register_converter(env, converter4);
	osync_converter_unref(converter4);

	OSyncFormatConverter *converter5 = osync_converter_new(OSYNC_CONVERTER_ENCAP, format1, format_g1, convert_addtest2, &error);
	osync_format_env_register_converter(env, converter5);
	osync_converter_unref(converter5);

	mark_point();

	OSyncData *data = osync_data_new("data", 5, format1, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);

	mark_point();

	OSyncList* fmtlist = NULL;
	fmtlist = osync_list_prepend(fmtlist, format_g1);
	fmtlist = osync_list_prepend(fmtlist, format6);

	OSyncFormatConverterPath *path = NULL;
	path = osync_format_env_find_path_formats_with_detectors(env, data, fmtlist, NULL, &error);
	fail_unless(path != NULL, NULL);

	osync_format_env_convert(env, path, data, &error);
	fail_unless(error == NULL, NULL);

	mark_point();

	char *buf;
	unsigned int size;
	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "datatest2"), NULL);
	OSyncObjFormat *format = osync_data_get_objformat(data);
	fail_unless(format == format_g1, NULL);
}
END_TEST

START_TEST (conv_env_detect_false)
{

	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("F1", "O1", &error);
	osync_format_env_register_objformat(env, format1);

	OSyncObjFormat *format2 = osync_objformat_new("F2", "O1", &error);
	osync_format_env_register_objformat(env, format2);
	
	OSyncObjFormat *format3 = osync_objformat_new("F3", "O1", &error);
	osync_format_env_register_objformat(env, format3);

	/*** Detectors ***/

	OSyncFormatConverter *detector1 = osync_converter_new_detector(format1, format2, detect_true, &error);
	fail_unless(detector1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, detector1);
//	osync_converter_unref(detector1);

	OSyncFormatConverter *detector2 = osync_converter_new_detector(format2, format3, detect_false, &error);
	fail_unless(detector2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, detector2);
//	osync_converter_unref(detector2);

	/*** Converter ***/

	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_DECAP, format1, format2, convert_addtest, &error);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);

	OSyncFormatConverter *converter1_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format2, format1, convert_remtest, &error);
	osync_format_env_register_converter(env, converter1_back);
	osync_converter_unref(converter1_back);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_DECAP, format2, format3, convert_addtest2, &error);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	OSyncFormatConverter *converter2_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format3, format2, convert_remtest2, &error);
	osync_format_env_register_converter(env, converter2_back);
	osync_converter_unref(converter2_back);

	mark_point();

	OSyncData *data = osync_data_new("data", 5, format1, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);
 
	mark_point();

	OSyncFormatConverterPath *path = NULL;
	path = osync_format_env_find_path_with_detectors(env, data, format3, NULL, &error);
	fail_unless(path == NULL, NULL);

	//fail_unless(!osync_format_env_convert(env, path, data, &error), NULL); // path is supposed to be null and this function has an assert on path
}
END_TEST

static osync_bool detect_plain_as_f2(const char *data, int size, void *userdata)
{
	return TRUE;
}

START_TEST (conv_env_decap_and_detect)
{
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("F1", "O1", &error);
	osync_format_env_register_objformat(env, format1);

	OSyncObjFormat *format2 = osync_objformat_new("F2", "O1", &error);
	osync_format_env_register_objformat(env, format2);
	
	OSyncObjFormat *format3 = osync_objformat_new("F3", "O1", &error);
	osync_format_env_register_objformat(env, format3);
	
	OSyncObjFormat *format_plain = osync_objformat_new("plain", "O1", &error);
	osync_format_env_register_objformat(env, format_plain);

	/*** Detectors ***/

	OSyncFormatConverter *detector1 = osync_converter_new_detector(format_plain, format2, detect_plain_as_f2, &error);
	fail_unless(detector1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, detector1);
//	osync_converter_unref(detector1);

	/*** Converter ***/

	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_DECAP, format1, format_plain, convert_addtest, &error);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);

	OSyncFormatConverter *converter1_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format_plain, format1, convert_remtest, &error);
	osync_format_env_register_converter(env, converter1_back);
	osync_converter_unref(converter1_back);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format3, convert_addtest, &error);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	OSyncFormatConverter *converter2_back = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format2, convert_remtest, &error);
	osync_format_env_register_converter(env, converter2_back);
	osync_converter_unref(converter2_back);

	mark_point();

	OSyncData *data = osync_data_new("data", 5, format1, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);
 
	mark_point();

	OSyncFormatConverterPath *path = NULL;
	path = osync_format_env_find_path_with_detectors(env, data, format3, NULL, &error);
	fail_unless(path != NULL, NULL);

	char *buf;
	unsigned int size;

	mark_point();

	osync_format_env_convert(env, path, data, &error);
	fail_unless(error == NULL, NULL);

	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "datatesttest"), NULL);
	OSyncObjFormat *format = osync_data_get_objformat(data);
	fail_unless(format == format3, NULL);

	mark_point();

	path = osync_format_env_find_path_with_detectors(env, data, format1, NULL, &error);
	fail_unless(path != NULL, NULL);

	osync_format_env_convert(env, path, data, &error);
	fail_unless(error == NULL, NULL);

	mark_point();

	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "data"), NULL);
	format = osync_data_get_objformat(data);
	fail_unless(format == format1, NULL);
}
END_TEST

static osync_bool detect_f2(const char *data, int size, void *userdata)
{
	if (!strcmp(data, "F2"))
		return TRUE;
	return FALSE;
}

static osync_bool convert_f1_to_f2(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	fail_unless(!strcmp(input, "F1"), NULL);
	
	*free_input = TRUE;
	*output = g_strdup("F2");
	*outpsize = 3;
	return TRUE;
}

static osync_bool convert_f2_to_f1(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	fail_unless(!strcmp(input, "F2"), NULL);
	
	*free_input = TRUE;
	*output = g_strdup("F1");
	*outpsize = 3;
	return TRUE;
}

START_TEST (conv_env_decap_and_detect2)
{
	/*This is a more complicated version. Here we specify some data
	 * for the change that needs to be converted and which only gets detected
	 * if it really got converted by the decap */
	OSyncError *error = NULL;
	OSyncFormatEnv *env = osync_format_env_new(&error);
	fail_unless(env != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncObjFormat *format1 = osync_objformat_new("F1", "O1", &error);
	osync_format_env_register_objformat(env, format1);

	OSyncObjFormat *format2 = osync_objformat_new("F2", "O1", &error);
	osync_format_env_register_objformat(env, format2);
	
	OSyncObjFormat *format3 = osync_objformat_new("F3", "O1", &error);
	osync_format_env_register_objformat(env, format3);
	
	OSyncObjFormat *format_plain = osync_objformat_new("plain", "O1", &error);
	osync_format_env_register_objformat(env, format_plain);

	/*** Detectors ***/

 	OSyncFormatConverter *detector1 = osync_converter_new_detector(format_plain, format2, detect_f2, &error);
	fail_unless(detector1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_format_env_register_converter(env, detector1);
//	osync_converter_unref(detector1);

	/*** Converter ***/

	OSyncFormatConverter *converter1 = osync_converter_new(OSYNC_CONVERTER_DECAP, format1, format_plain, convert_f1_to_f2, &error);
	osync_format_env_register_converter(env, converter1);
	osync_converter_unref(converter1);

	OSyncFormatConverter *converter1_back = osync_converter_new(OSYNC_CONVERTER_ENCAP, format_plain, format1, convert_f2_to_f1, &error);
	osync_format_env_register_converter(env, converter1_back);
	osync_converter_unref(converter1_back);
	
	OSyncFormatConverter *converter2 = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format3, convert_addtest, &error);
	osync_format_env_register_converter(env, converter2);
	osync_converter_unref(converter2);

	OSyncFormatConverter *converter2_back = osync_converter_new(OSYNC_CONVERTER_CONV, format3, format2, convert_remtest, &error);
	osync_format_env_register_converter(env, converter2_back);
	osync_converter_unref(converter2_back);

	mark_point();

	OSyncData *data = osync_data_new("F1", 3, format1, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);
 
	mark_point();

	OSyncFormatConverterPath *path = NULL;
	path = osync_format_env_find_path_with_detectors(env, data, format3, NULL, &error);
	fail_unless(path != NULL, NULL);

	char *buf;
	unsigned int size;

	mark_point();

	osync_format_env_convert(env, path, data, &error);
	fail_unless(error == NULL, NULL);

	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "F2test"), NULL);
	OSyncObjFormat *format = osync_data_get_objformat(data);
	fail_unless(format == format3, NULL);

	mark_point();

	path = osync_format_env_find_path_with_detectors(env, data, format1, NULL, &error);
	fail_unless(path != NULL, NULL);

	osync_format_env_convert(env, path, data, &error);
	fail_unless(error == NULL, NULL);

	mark_point();

	osync_data_get_data(data, &buf, &size);

	fail_unless(buf != NULL, NULL);
	fail_unless(!strcmp(buf, "F1"), NULL);
	format = osync_data_get_objformat(data);
	fail_unless(format == format1, NULL);
}
END_TEST


Suite *format_env_suite(void)
{
	Suite *s = suite_create("Format-Env");
	//Suite *s2 = suite_create("Format-Env");

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
	
	create_case(s, "conv_find_path", conv_find_path);
	create_case(s, "conv_find_path2", conv_find_path2);
	create_case(s, "conv_find_path_false", conv_find_path_false);
	create_case(s, "conv_find_multi_path", conv_find_multi_path);
	create_case(s, "conv_find_multi_path_with_preferred", conv_find_multi_path_with_preferred);
	create_case(s, "conv_find_circular_false", conv_find_circular_false);
	create_case(s, "conv_find_complex", conv_find_complex);
	
	create_case(s, "conv_find_multi_target", conv_find_multi_target);
	create_case(s, "conv_find_multi_target2", conv_find_multi_target2);
	create_case(s, "conv_find_multi_path_multi_target", conv_find_multi_path_multi_target);
	create_case(s, "conv_find_multi_path_multi_target_with_preferred", conv_find_multi_path_multi_target_with_preferred);

	// Gone?
	//create_case(s, "conv_env_osp_circular_false", conv_env_osp_circular_false);
	//create_case(s, "conv_env_osp_complex", conv_env_osp_complex);

	create_case(s, "conv_env_convert1", conv_env_convert1);
	create_case(s, "conv_env_convert_back", conv_env_convert_back);
	create_case(s, "conv_env_convert_desenc", conv_env_convert_desenc);

	create_case(s, "conv_env_convert_desenc_complex", conv_env_convert_desenc_complex); 
	create_case(s, "conv_env_detect_and_convert", conv_env_detect_and_convert);
	create_case(s, "conv_prefer_not_desencap", conv_prefer_not_desencap);
	create_case(s, "conv_prefer_same_objtype", conv_prefer_same_objtype);
	create_case(s, "conv_prefer_not_lossy_objtype_change", conv_prefer_not_lossy_objtype_change);
	create_case(s, "conv_env_detect_false", conv_env_detect_false);
	create_case(s, "conv_env_decap_and_detect", conv_env_decap_and_detect);
	create_case(s, "conv_env_decap_and_detect2", conv_env_decap_and_detect2);

	/*
	 * osync_bool osync_format_env_load_plugins(OSyncFormatEnv *env, const char *path, OSyncError **error);

OSyncObjFormat *osync_format_env_detect_objformat(OSyncFormatEnv *env, OSyncData *data);
OSyncObjFormat *osync_format_env_detect_objformat_full(OSyncFormatEnv *env, OSyncData *input, OSyncError **error);

osync_bool osync_format_env_convert(OSyncFormatEnv *env, OSyncFormatConverterPath *path, OSyncData *data, OSyncError **error);

OSyncFormatConverterPath *osync_format_env_find_path(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat, OSyncError **error);
OSyncFormatConverterPath *osync_format_env_find_path_formats(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncObjFormat **targets, OSyncError **error);
	 * 
	 */
	
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
