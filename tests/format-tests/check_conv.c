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

osync_bool convert_func(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
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

#ifdef asasdasd

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

START_TEST (conv_env_osp_simple)
{
  OSyncEnv *osync = init_env_none();
  osync_env_register_objtype(osync, "test");
  osync_env_register_objformat(osync, "test", "fmt_test1");
  osync_env_register_objformat(osync, "test", "fmt_test2");
  osync_env_register_converter(osync, CONVERTER_CONV, "fmt_test1", "fmt_test2", dummyconvert);
  OSyncFormatEnv *env = osync_conv_env_new(osync);
  
  OSyncFormatConverter *converter1 = osync_conv_find_converter(env, "fmt_test1", "fmt_test2");
  fail_unless(converter1 != NULL, NULL);
  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "fmt_test1");
  OSyncObjFormat *format2 = osync_conv_find_objformat(env, "fmt_test2");
  mark_point();
  GList *converters;
  OSyncChange *chg = create_change(format1, dummy_data, 1);
  fail_unless(osync_conv_find_path_fmtlist(env, chg, g_list_append(NULL, format2), &converters), NULL);
  fail_unless(g_list_length(converters) == 1, NULL);
  fail_unless(g_list_nth_data(converters, 0) == converter1, NULL);
}
END_TEST

START_TEST (conv_env_osp_simple2)
{
  OSyncEnv *osync = init_env_none();
  osync_env_register_objtype(osync, "test");
  osync_env_register_objformat(osync, "test", "fmt_test1");
  osync_env_register_objformat(osync, "test", "fmt_test2");
  osync_env_register_objformat(osync, "test", "fmt_test3");
  osync_env_register_converter(osync, CONVERTER_CONV, "fmt_test1", "fmt_test2", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "fmt_test2", "fmt_test3", dummyconvert);
  OSyncFormatEnv *env = osync_conv_env_new(osync);
  
  OSyncFormatConverter *converter1 = osync_conv_find_converter(env, "fmt_test1", "fmt_test2");
  OSyncFormatConverter *converter2 = osync_conv_find_converter(env, "fmt_test2", "fmt_test3");
  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "fmt_test1");
  OSyncObjFormat *format3 = osync_conv_find_objformat(env, "fmt_test3");
  fail_unless(converter1 != NULL, NULL);
  fail_unless(converter2 != NULL, NULL);

  mark_point();
  GList *converters;
  OSyncChange *chg = create_change(format1, dummy_data, 1);
  fail_unless(osync_conv_find_path_fmtlist(env, chg, g_list_append(NULL, format3), &converters), NULL);
  fail_unless(g_list_length(converters) == 2, NULL);
  fail_unless(g_list_nth_data(converters, 0) == converter1, NULL);
  fail_unless(g_list_nth_data(converters, 1) == converter2, NULL);
}
END_TEST

START_TEST (conv_env_osp_false)
{
  OSyncEnv *osync = init_env_none();
  
  osync_env_register_objtype(osync, "test");
  osync_env_register_objformat(osync, "test", "fmt_test1");
  osync_env_register_objformat(osync, "test", "fmt_test2");
  osync_env_register_objformat(osync, "test", "fmt_test3");
  osync_env_register_converter(osync, CONVERTER_CONV, "fmt_test1", "fmt_test2", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "fmt_test3", "fmt_test2", dummyconvert);
  OSyncFormatEnv *env = osync_conv_env_new(osync);
  
  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "fmt_test1");
  OSyncObjFormat *format3 = osync_conv_find_objformat(env, "fmt_test3");
  
  mark_point();
  GList *converters;
  OSyncChange *chg = create_change(format1, dummy_data, 1);
  fail_unless(!osync_conv_find_path_fmtlist(env, chg, g_list_append(NULL, format3), &converters), NULL);
  fail_unless(converters == FALSE, NULL);
}
END_TEST

START_TEST (conv_env_osp_2way)
{
  OSyncEnv *osync = init_env_none();
  osync_env_register_objtype(osync, "test");
  osync_env_register_objformat(osync, "test", "fmt_test1");
  osync_env_register_objformat(osync, "test", "fmt_test2");
  osync_env_register_objformat(osync, "test", "fmt_test3");
  osync_env_register_objformat(osync, "test", "fmt_test4");

  osync_env_register_converter(osync, CONVERTER_CONV, "fmt_test1", "fmt_test2", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "fmt_test2", "fmt_test4", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "fmt_test1", "fmt_test3", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "fmt_test3", "fmt_test4", dummyconvert);

  OSyncFormatEnv *env = osync_conv_env_new(osync);

  OSyncFormatConverter *converter1 = osync_conv_find_converter(env, "fmt_test1", "fmt_test2");
  OSyncFormatConverter *converter2 = osync_conv_find_converter(env, "fmt_test2", "fmt_test4");

  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "fmt_test1");
  OSyncObjFormat *format4 = osync_conv_find_objformat(env, "fmt_test4");

  mark_point();
  GList *converters;
  OSyncChange *chg = create_change(format1, dummy_data, 1);
  fail_unless(osync_conv_find_path_fmtlist(env, chg, g_list_append(NULL, format4), &converters), NULL);
  fail_unless(g_list_length(converters) == 2, NULL);
  fail_unless(g_list_nth_data(converters, 0) == converter1, NULL);
  fail_unless(g_list_nth_data(converters, 1) == converter2, NULL);
}
END_TEST

START_TEST (conv_env_osp_circular_false)
{
  OSyncEnv *osync = init_env_none();
  
  osync_env_register_objtype(osync, "test");
  
  osync_env_register_objformat(osync, "test", "fmt_test1");
  osync_env_register_objformat(osync, "test", "fmt_test2");
  osync_env_register_objformat(osync, "test", "fmt_test3");
  osync_env_register_objformat(osync, "test", "fmt_test4");

  osync_env_register_converter(osync, CONVERTER_CONV, "fmt_test1", "fmt_test2", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "fmt_test2", "fmt_test3", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "fmt_test3", "fmt_test1", dummyconvert);
  OSyncFormatEnv *env = osync_conv_env_new(osync);

  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "fmt_test1");
  OSyncObjFormat *format4 = osync_conv_find_objformat(env, "fmt_test4");

  mark_point();
  GList *converters;
  OSyncChange *chg = create_change(format1, dummy_data, 1);
  fail_unless(!osync_conv_find_path_fmtlist(env, chg, g_list_append(NULL, format4), &converters), NULL);
  fail_unless(converters == NULL, NULL);
}
END_TEST

START_TEST (conv_env_osp_complex)
{
  OSyncEnv *osync = init_env_none();
  
  osync_env_register_objtype(osync, "test");
  
  osync_env_register_objformat(osync, "test", "A");
  osync_env_register_objformat(osync, "test", "B");
  osync_env_register_objformat(osync, "test", "C");
  osync_env_register_objformat(osync, "test", "D");
  osync_env_register_objformat(osync, "test", "E");
  osync_env_register_objformat(osync, "test", "F");
  osync_env_register_objformat(osync, "test", "G");
  osync_env_register_objformat(osync, "test", "H");

  osync_env_register_converter(osync, CONVERTER_CONV, "A", "B", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "A", "C", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "A", "D", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "C", "E", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "D", "G", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "E", "G", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "G", "H", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "H", "F", dummyconvert);
  osync_env_register_converter(osync, CONVERTER_CONV, "E", "F", dummyconvert);

  OSyncFormatEnv *env = osync_conv_env_new(osync);

  OSyncFormatConverter *converter1 = osync_conv_find_converter(env, "A", "C");
  OSyncFormatConverter *converter2 = osync_conv_find_converter(env, "C", "E");
  OSyncFormatConverter *converter3 = osync_conv_find_converter(env, "E", "F");

  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "A");
  OSyncObjFormat *format2 = osync_conv_find_objformat(env, "F");

  mark_point();
  GList *converters;
  OSyncChange *chg = create_change(format1, dummy_data, 1);
  fail_unless(osync_conv_find_path_fmtlist(env, chg, g_list_append(NULL, format2), &converters), NULL);
  fail_unless(g_list_length(converters) == 3, NULL);
  fail_unless(g_list_nth_data(converters, 0) == converter1, NULL);
  fail_unless(g_list_nth_data(converters, 1) == converter2, NULL);
  fail_unless(g_list_nth_data(converters, 2) == converter3, NULL);
}
END_TEST

static osync_bool convert_addtest(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	*free_input = TRUE;
	*output = g_strdup_printf("%stest", input);
	*outpsize = inpsize + 4;
	return TRUE;
}

static osync_bool convert_remtest(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
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

static osync_bool convert_addtest2(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	*output = g_strdup_printf("%stest2", input);
	*outpsize = inpsize + 5;
	*free_input = TRUE;
	return TRUE;
}

static osync_bool convert_remtest2(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
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
  OSyncEnv *osync = init_env_none();
  
  osync_env_register_objtype(osync, "O1");
  
  osync_env_register_objformat(osync, "O1", "F1");
  osync_env_register_objformat(osync, "O1", "F2");
  osync_env_register_objformat(osync, "O1", "F3");
  osync_env_register_converter(osync, CONVERTER_CONV, "F1", "F2", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_CONV, "F2", "F3", convert_addtest2);
  mark_point();
  OSyncFormatEnv *env = osync_conv_env_new(osync);
  
  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "F1");
  OSyncObjFormat *format3 = osync_conv_find_objformat(env, "F3");
  OSyncObjType *type = osync_conv_find_objtype(env, "O1");
  
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_set_objtype(change, type);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();
  osync_change_convert(env, change, format3, NULL);
  
  char *data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatesttest2"), NULL);
  OSyncObjFormat *format = change->format;
  fail_unless(format == format3, NULL);
}
END_TEST

START_TEST (conv_env_convert_back)
{
  OSyncEnv *osync = init_env_none();
  osync_env_register_objtype(osync, "O1");
  
  osync_env_register_objformat(osync, "O1", "F1");
  osync_env_register_objformat(osync, "O1", "F2");
  osync_env_register_objformat(osync, "O1", "F3");
  osync_env_register_converter(osync, CONVERTER_CONV, "F1", "F2", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_CONV, "F2", "F1", convert_remtest);
  osync_env_register_converter(osync, CONVERTER_CONV, "F2", "F3", convert_addtest2);
  osync_env_register_converter(osync, CONVERTER_CONV, "F3", "F2", convert_remtest2);
  
  OSyncFormatEnv *env = osync_conv_env_new(osync);
  
  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "F1");
  OSyncObjFormat *format3 = osync_conv_find_objformat(env, "F3");
  OSyncObjType *type = osync_conv_find_objtype(env, "01");
  
  mark_point();
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_set_objtype(change, type);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();
  osync_change_convert(env, change, format3, NULL);
  
  char *data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatesttest2"), NULL);
  OSyncObjFormat *format = change->format;
  fail_unless(format == format3, NULL);
  
  mark_point();
  osync_change_convert(env, change, format1, NULL);
  
  data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "data"), NULL);
  format = change->format;
  fail_unless(format == format1, NULL);
}
END_TEST

START_TEST (conv_env_convert_desenc)
{
  OSyncEnv *osync = init_env_none();
  
  osync_env_register_objtype(osync, "O1");
  
  osync_env_register_objformat(osync, "O1", "F1");
  osync_env_register_objformat(osync, "O1", "F2");
  osync_env_register_objformat(osync, "O1", "F3");
  osync_env_register_converter(osync, CONVERTER_DECAP, "F1", "F2", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F2", "F1", convert_remtest);
  osync_env_register_converter(osync, CONVERTER_DECAP, "F2", "F3", convert_addtest2);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F3", "F2", convert_remtest2);
  mark_point();
  
  OSyncFormatEnv *env = osync_conv_env_new(osync);
  
  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "F1");
  OSyncObjFormat *format3 = osync_conv_find_objformat(env, "F3");
  OSyncObjType *type = osync_conv_find_objtype(env, "O1");
  
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_set_objtype(change, type);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();
  osync_change_convert(env, change, format3, NULL);
  
  char *data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatesttest2"), NULL);
  OSyncObjFormat *format = change->format;
  fail_unless(format == format3, NULL);
  
  mark_point();
  osync_change_convert(env, change, format1, NULL);
  
  data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "data"), NULL);
  format = change->format;
  fail_unless(format == format1, NULL);
}
END_TEST

static osync_bool detect_true(OSyncFormatEnv *env, const char *data, int size)
{
	return TRUE;
}

static osync_bool detect_false(OSyncFormatEnv *env, const char *data, int size)
{
	return FALSE;
}

START_TEST (conv_env_convert_desenc_complex)
{
  /* Test if the converter is going on the righ path, when the data detector
   * for the format reports a specific lower format
   */
  OSyncEnv *osync = init_env_none();
  
  osync_env_register_objtype(osync, "O1");
  
  osync_env_register_objformat(osync, "O1", "F1");
  osync_env_register_objformat(osync, "O1", "F2");
  osync_env_register_detector(osync, "F2", "F4", detect_true);
  osync_env_register_detector(osync, "F2", "F3", detect_false);
  osync_env_register_objformat(osync, "O1", "F3");
  osync_env_register_objformat(osync, "O1", "F4");
  osync_env_register_objformat(osync, "O1", "F5");
  osync_env_register_objformat(osync, "O1", "F6");
  osync_env_register_converter(osync, CONVERTER_DECAP, "F1", "F2", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F2", "F1", convert_remtest);
  osync_env_register_converter(osync, CONVERTER_DECAP, "F2", "F3", convert_addtest2);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F3", "F2", convert_remtest2);
  osync_env_register_converter(osync, CONVERTER_DECAP, "F2", "F4", convert_addtest2);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F4", "F2", convert_remtest2);
  osync_env_register_converter(osync, CONVERTER_CONV, "F3", "F6", convert_addtest2);
  osync_env_register_converter(osync, CONVERTER_CONV, "F4", "F5", convert_addtest2);
  osync_env_register_converter(osync, CONVERTER_CONV, "F5", "F4", convert_remtest2);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F5", "F6", convert_addtest2);
  osync_env_register_converter(osync, CONVERTER_DECAP, "F6", "F5", convert_remtest2);
  
  OSyncFormatEnv *env = osync_conv_env_new(osync);

  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "F1");
  OSyncObjFormat *format6 = osync_conv_find_objformat(env, "F6");
  OSyncObjType *type = osync_conv_find_objtype(env, "O1");

  mark_point();
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_set_objtype(change, type);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();
  osync_change_convert(env, change, format6, NULL);
  
  char *data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatesttest2test2test2"), NULL);
  fail_unless(change->format == format6, NULL);
  
  mark_point();
  osync_change_convert(env, change, format1, NULL);
  
  data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "data"), NULL);
  fail_unless(change->format == format1, NULL);
}
END_TEST

START_TEST (conv_env_detect_and_convert)
{
  /* The data will be detected as F3, so the shortest path should
   * not be taken because the path searching function should see that
   * the encapsulated data * is a F3 object, not a F4 object
   */
  OSyncEnv *osync = init_env_none();
  
  osync_env_register_objtype(osync, "O1");
  
  osync_env_register_objformat(osync, "O1", "F1");
  osync_env_register_objformat(osync, "O1", "F2");
  osync_env_register_objformat(osync, "O1", "F3");
  osync_env_register_objformat(osync, "O1", "F4");

  osync_env_register_detector(osync, "F1", "F2", detect_true);

  /* Detect F3, not F4 */
  osync_env_register_detector(osync, "F2", "F3", detect_true);
  osync_env_register_detector(osync, "F2", "F4", detect_false);
  
  osync_env_register_converter(osync, CONVERTER_DECAP, "F1", "F2", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F2", "F1", convert_remtest);
  osync_env_register_converter(osync, CONVERTER_DECAP, "F2", "F3", convert_addtest2);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F3", "F2", convert_remtest2);
  osync_env_register_converter(osync, CONVERTER_DECAP, "F2", "F4", convert_addtest2);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F4", "F2", convert_remtest2);
  osync_env_register_converter(osync, CONVERTER_CONV, "F3", "F4", convert_addtest2);
  osync_env_register_converter(osync, CONVERTER_CONV, "F4", "F3", convert_remtest2);
  
  OSyncFormatEnv *env = osync_conv_env_new(osync);
  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "F1");
  OSyncObjFormat *format4 = osync_conv_find_objformat(env, "F4");
  OSyncObjType *type = osync_conv_find_objtype(env, "O1");
  
  mark_point();
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();

  fail_unless(osync_change_convert(env, change, format4, NULL), NULL);
  mark_point();
  char *data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatesttest2test2"), NULL);
  fail_unless(change->objtype == type, NULL);
  OSyncObjFormat *format = change->format;
  fail_unless(format == format4, NULL);
  
  mark_point();
  osync_change_convert(env, change, format1, NULL);
  mark_point();
  data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatest"), NULL);
  format = change->format;
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
  OSyncEnv *osync = init_env_none();
  osync_env_register_objtype(osync, "O1");
  
  osync_env_register_objformat(osync, "O1", "F1");
  osync_env_register_objformat(osync, "O1", "F2");
  osync_env_register_objformat(osync, "O1", "F3");
  osync_env_register_objformat(osync, "O1", "F4");
  osync_env_register_objformat(osync, "O1", "F5");

  osync_env_register_converter(osync, CONVERTER_ENCAP, "F1", "F2", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F2", "F3", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F3", "F5", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_DECAP, "F1", "F4", convert_addtest2);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F4", "F5", convert_addtest2);
  mark_point();

  OSyncFormatEnv *env = osync_conv_env_new(osync);
  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "F1");
  OSyncObjFormat *format5 = osync_conv_find_objformat(env, "F5");
  OSyncObjType *type = osync_conv_find_objtype(env, "O1");
  
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();

  fail_unless(osync_change_convert(env, change, format5, NULL), NULL);
  mark_point();
  char *data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatesttesttest"), NULL);

  fail_unless(change->objtype == type, NULL);
  fail_unless(change->format == format5, NULL);
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
  OSyncEnv *osync = init_env_none();
  osync_env_register_objtype(osync, "F");
  osync_env_register_objtype(osync, "G");
  
  osync_env_register_objformat(osync, "F", "F1");
  osync_env_register_objformat(osync, "F", "F2");
  osync_env_register_objformat(osync, "F", "F3");
  osync_env_register_objformat(osync, "F", "F4");
  osync_env_register_objformat(osync, "F", "F5");
  osync_env_register_objformat(osync, "F", "F6");

  osync_env_register_objformat(osync, "G", "G1");

  osync_env_register_converter(osync, CONVERTER_ENCAP, "F1", "F2", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F2", "F3", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F3", "F4", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F4", "F5", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F5", "F6", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F1", "G1", convert_addtest2);
  mark_point();

  OSyncFormatEnv *env = osync_conv_env_new(osync);
  OSyncObjFormat *f1 = osync_conv_find_objformat(env, "F1");
  OSyncObjFormat *f6 = osync_conv_find_objformat(env, "F6");
  OSyncObjFormat *g1 = osync_conv_find_objformat(env, "G1");
  OSyncObjType *typef = osync_conv_find_objtype(env, "F");
  
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, f1);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();

  GList *targets = g_list_append(NULL, g1);
  targets = g_list_append(targets, f6);
  fail_unless(osync_conv_convert_fmtlist(env, change, targets), NULL);
  mark_point();
  char *data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatesttesttesttesttest"), NULL);

  fail_unless(change->objtype == typef, NULL);
  fail_unless(change->format == f6, NULL);
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
  OSyncEnv *osync = init_env_none();
  osync_env_register_objtype(osync, "F");
  osync_env_register_objtype(osync, "G");
  
  osync_env_register_objformat(osync, "F", "F1");
  osync_env_register_objformat(osync, "F", "F2");
  osync_env_register_objformat(osync, "F", "F3");
  osync_env_register_objformat(osync, "F", "F4");
  osync_env_register_objformat(osync, "F", "F5");
  osync_env_register_objformat(osync, "F", "F6");

  osync_env_register_objformat(osync, "G", "G1");

  osync_env_register_converter(osync, CONVERTER_ENCAP, "F1", "F2", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_DECAP, "F2", "F3", convert_addtest); /* Lossy */
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F3", "F4", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F4", "F5", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F5", "F6", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F1", "G1", convert_addtest2);
  mark_point();

  OSyncFormatEnv *env = osync_conv_env_new(osync);
  OSyncObjFormat *f1 = osync_conv_find_objformat(env, "F1");
  OSyncObjFormat *f6 = osync_conv_find_objformat(env, "F6");
  OSyncObjFormat *g1 = osync_conv_find_objformat(env, "G1");
  OSyncObjType *typeg = osync_conv_find_objtype(env, "G");
  
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, f1);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();

  GList *targets = g_list_append(NULL, g1);
  targets = g_list_append(targets, f6);
  fail_unless(osync_conv_convert_fmtlist(env, change, targets), NULL);
  mark_point();
  char *data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatest2"), NULL);

  fail_unless(change->objtype == typeg, NULL);
  fail_unless(change->format == g1, NULL);
}
END_TEST

START_TEST (conv_env_detect_false)
{
  OSyncEnv *osync = init_env_none();
  osync_env_register_objtype(osync, "O1");
  
  osync_env_register_objformat(osync, "O1", "F1");
  osync_env_register_objformat(osync, "O1", "F2");
  osync_env_register_objformat(osync, "O1", "F3");
  osync_env_register_detector(osync, "F1", "F2", detect_true);
  osync_env_register_detector(osync, "F2", "F3", detect_false);
  
  osync_env_register_converter(osync, CONVERTER_DECAP, "F1", "F2", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F2", "F1", convert_remtest);
  osync_env_register_converter(osync, CONVERTER_DECAP, "F2", "F3", convert_addtest2);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "F3", "F2", convert_remtest2);
  
  OSyncFormatEnv *env = osync_conv_env_new(osync);
  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "F1");
  OSyncObjFormat *format3 = osync_conv_find_objformat(env, "F3");
  
  mark_point();
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();
  fail_unless(!osync_change_convert(env, change, format3, NULL), NULL);
}
END_TEST

static osync_bool detect_plain_as_f2(OSyncFormatEnv *env, const char *data, int size)
{
	return TRUE;
}

START_TEST (conv_env_decap_and_detect)
{
  OSyncEnv *osync = init_env_none();
  osync_env_register_objtype(osync, "O1");
  
  osync_env_register_objformat(osync, "O1", "F1");
  osync_env_register_objformat(osync, "O1", "F2");
  osync_env_register_objformat(osync, "O1", "plain");
  osync_env_register_objformat(osync, "O1", "F3");
  
  osync_env_register_detector(osync, "plain", "F2", detect_plain_as_f2);
  
  osync_env_register_converter(osync, CONVERTER_DECAP, "F1", "plain", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "plain", "F1", convert_remtest);
  osync_env_register_converter(osync, CONVERTER_CONV, "F2", "F3", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_CONV, "F3", "F2", convert_remtest);
  
  OSyncFormatEnv *env = osync_conv_env_new(osync);
  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "F1");
  OSyncObjFormat *format3 = osync_conv_find_objformat(env, "F3");
  
  mark_point();
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();
  OSyncError *error = NULL;
  fail_unless(osync_change_convert(env, change, format3, &error), NULL);
  fail_unless(!strcmp(osync_change_get_data(change), "datatesttest"), NULL);
  fail_unless(osync_change_get_objformat(change) == format3, NULL);
  
  fail_unless(osync_change_convert(env, change, format1, &error), NULL);
  fail_unless(!strcmp(osync_change_get_data(change), "data"), NULL);
  fail_unless(osync_change_get_objformat(change) == format1, NULL);
}
END_TEST

static osync_bool detect_f2(OSyncFormatEnv *env, const char *data, int size)
{
	if (!strcmp(data, "F2"))
		return TRUE;
	return FALSE;
}

static osync_bool convert_f1_to_f2(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	fail_unless(!strcmp(input, "F1"), NULL);
	
	*free_input = TRUE;
	*output = g_strdup("F2");
	*outpsize = 3;
	return TRUE;
}

static osync_bool convert_f2_to_f1(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
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
  OSyncEnv *osync = init_env_none();
  osync_env_register_objtype(osync, "O1");
  
  osync_env_register_objformat(osync, "O1", "F1");
  osync_env_register_objformat(osync, "O1", "F2");
  osync_env_register_objformat(osync, "O1", "plain");
  osync_env_register_objformat(osync, "O1", "F3");
  
  osync_env_register_detector(osync, "plain", "F2", detect_f2);
  
  osync_env_register_converter(osync, CONVERTER_DECAP, "F1", "plain", convert_f1_to_f2);
  osync_env_register_converter(osync, CONVERTER_ENCAP, "plain", "F1", convert_f2_to_f1);
  osync_env_register_converter(osync, CONVERTER_CONV, "F2", "F3", convert_addtest);
  osync_env_register_converter(osync, CONVERTER_CONV, "F3", "F2", convert_remtest);
  
  OSyncFormatEnv *env = osync_conv_env_new(osync);
  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "F1");
  OSyncObjFormat *format3 = osync_conv_find_objformat(env, "F3");
  
  mark_point();
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_set_data(change, "F1", 3, TRUE);
  
  mark_point();
  OSyncError *error = NULL;
  fail_unless(osync_change_convert(env, change, format3, &error), NULL);
  fail_unless(!strcmp(osync_change_get_data(change), "F2test"), NULL);
  fail_unless(osync_change_get_objformat(change) == format3, NULL);
  
  fail_unless(osync_change_convert(env, change, format1, &error), NULL);
  fail_unless(!strcmp(osync_change_get_data(change), "F1"), NULL);
  fail_unless(osync_change_get_objformat(change) == format1, NULL);
}
END_TEST
#endif

Suite *format_env_suite(void)
{
	Suite *s = suite_create("Format-Env");
	Suite *s2 = suite_create("Format-Env");
	
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
	create_case(s2, "conv_env_register_filter_count", conv_env_register_filter_count);
	
	/*create_case(s, "conv_env_add_type", conv_env_add_type);
	create_case(s, "conv_env_add_type_find", conv_env_add_type_find);
	create_case(s, "conv_env_add_type_find_false", conv_env_add_type_find_false);
	create_case(s, "conv_env_type_register2", conv_env_type_register2);
	create_case(s, "conv_env_add_format", conv_env_add_format);
	create_case(s, "conv_env_set_format_string", conv_env_set_format_string);
	create_case(s, "conv_env_add_converters", conv_env_add_converters);
	create_case(s, "conv_env_add_converters_missing", conv_env_add_converters_missing);
	create_case(s, "conv_env_osp_simple", conv_env_osp_simple);
	create_case(s, "conv_env_osp_simple2", conv_env_osp_simple2);
	create_case(s, "conv_env_osp_false", conv_env_osp_false);
	create_case(s, "conv_env_osp_2way", conv_env_osp_2way);
	create_case(s, "conv_env_osp_circular_false", conv_env_osp_circular_false);
	create_case(s, "conv_env_osp_complex", conv_env_osp_complex);
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
	create_case(s, "conv_env_decap_and_detect2", conv_env_decap_and_detect2);*/
	
	/*
	 * osync_bool osync_format_env_load_plugins(OSyncFormatEnv *env, const char *path, OSyncError **error);

OSyncObjFormat *osync_format_env_detect_objformat(OSyncFormatEnv *env, OSyncData *data);
OSyncObjFormat *osync_format_env_detect_objformat_full(OSyncFormatEnv *env, OSyncData *input, OSyncError **error);

osync_bool osync_format_env_convert(OSyncFormatEnv *env, OSyncFormatConverterPath *path, OSyncData *data, OSyncError **error);

OSyncFormatConverterPath *osync_format_env_find_path(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat, OSyncError **error);
OSyncFormatConverterPath *osync_format_env_find_path_formats(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncObjFormat **targets, OSyncError **error);
	 * 
	 */
	
	return s2;
}

int main(void)
{
	int nf;
	
	Suite *s = format_env_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
