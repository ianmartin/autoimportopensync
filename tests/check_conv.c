#include <check.h>
#include <glib.h>
#include <gmodule.h>
#include <opensync.h>
#include <opensync_internals.h>
#include <stdlib.h>
#include <string.h>

START_TEST (conv_env_create)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  fail_unless(env != NULL, "env == NULL on creation");
}
END_TEST

START_TEST (conv_env_add_type)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  OSyncObjType *type = osync_conv_register_objtype(env, "test");
  fail_unless(type != NULL, "type == NULL on creation");
  //fail_unless(osynctype->mergeable == FALSE, "mergable set wrong");
  fail_unless(!strcmp(osync_conv_objtype_get_name(type), "test"), "string not test");
}
END_TEST

START_TEST (conv_env_add_type_find)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  osync_conv_register_objtype(env, "test");
  OSyncObjType *type = osync_conv_find_objtype(env, "test");
  fail_unless(type != NULL, "type == NULL on creation");
  //fail_unless(osynctype->mergeable == FALSE, "mergable set wrong");
  fail_unless(!strcmp(osync_conv_objtype_get_name(type), "test"), "string not test2");
}
END_TEST

START_TEST (conv_env_add_type_find_false)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  osync_conv_register_objtype(env, "test");
  OSyncObjType *type = osync_conv_find_objtype(env, "test2");
  fail_unless(type == NULL, "type != NULL by false find");
}
END_TEST

START_TEST (conv_env_type_register2)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  OSyncObjType *type1 = osync_conv_register_objtype(env, "test");
  OSyncObjType *type2 = osync_conv_register_objtype(env, "test");
  fail_unless(type1 == type2, "type1 != type2");
}
END_TEST

START_TEST (conv_env_add_format)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  osync_conv_register_objtype(env, "test");
  OSyncObjFormat *format = osync_conv_register_objformat(env, "test", "fmt_test");
  fail_unless(format != NULL, "format == NULL");
  OSyncObjFormat *format1 = osync_conv_find_objformat(env, "fmt_test");
  fail_unless(format == format1, "format != format1 by find");
}
END_TEST

//FIXME Move this to the change testcase
START_TEST (conv_env_set_format)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  osync_conv_register_objtype(env, "test");
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "test", "fmt_test");

  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);

  fail_unless(g_list_length(change->objformats) == 1, NULL);
  fail_unless(g_list_nth_data(change->objformats, 0) == format1, NULL);
}
END_TEST

START_TEST (conv_env_set_format_string)
{
  OSyncEnv *osenv = osync_env_new();
  OSyncGroup *group = osync_group_new(osenv);
  mark_point();
  osync_conv_register_objtype(group->conv_env, "test");
  mark_point();
  OSyncObjFormat *format1 = osync_conv_register_objformat(group->conv_env, "test", "fmt_test");
  
  mark_point();
  OSyncMember *member = osync_member_new(group);

  mark_point();
  OSyncChange *change = osync_change_new();
  osync_change_set_member(change, member);
  osync_change_set_objformat_string(change, "fmt_test");

  fail_unless(g_list_length(change->objformats) == 1, NULL);
  fail_unless(g_list_nth_data(change->objformats, 0) == format1, NULL);
}
END_TEST

START_TEST (conv_env_append_format)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  osync_conv_register_objtype(env, "test");
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "test", "fmt_test");
  OSyncObjFormat *format2 = osync_conv_register_objformat(env, "test", "fmt_test2");

  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_append_objformat(change, format2);

  fail_unless(g_list_length(change->objformats) == 2, NULL);
  fail_unless(g_list_nth_data(change->objformats, 0) == format1, NULL);
  fail_unless(g_list_nth_data(change->objformats, 1) == format2, NULL);
}
END_TEST

static osync_bool dummyconvert(const char *input, int inpsize, char **output, int *outpsize)
{
	*output = g_strdup("test");
	*outpsize = 5;
	return TRUE;
}

START_TEST (conv_env_add_converter_resolve_later)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  osync_conv_register_objtype(env, "test");
  osync_conv_register_objformat(env, "test", "fmt_test1");
  osync_conv_register_objformat(env, "test", "fmt_test2");
  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test1", "fmt_test2", dummyconvert, 0);  
  osync_conv_register_converter(env, CONVERTER_ENCAP, "fmt_test2", "fmt_test3", dummyconvert, 0);  
  osync_conv_register_converter(env, CONVERTER_DESENCAP, "fmt_test3", "fmt_test2", dummyconvert, 0);  

  /* The first converter will resolve */
  OSyncFormatConverter *converter = osync_conv_find_converter(env, "fmt_test1", "fmt_test2");
  fail_unless(converter != NULL, NULL);
  fail_unless(converter->type == CONVERTER_CONV, NULL);
  fail_unless(!strcmp(converter->source_format->name,"fmt_test1") , NULL);
  fail_unless(!strcmp(converter->target_format->name,"fmt_test2") , NULL);
  fail_unless(converter->convert_func == dummyconvert, NULL);

  /* The other two converters will resolve only after registering fmt_test3 */
  OSyncFormatConverter *unresolved_converter;
  unresolved_converter = osync_conv_find_converter(env, "fmt_test2", "fmt_test3");
  fail_unless(unresolved_converter == NULL, NULL);
  unresolved_converter = osync_conv_find_converter(env, "fmt_test3", "fmt_test2");
  fail_unless(unresolved_converter == NULL, NULL);

  /* Check the unresolved list */
  fail_unless(g_list_length(env->unresolved_converters) == 2, NULL);

  osync_conv_register_objformat(env, "test", "fmt_test3");

  /* All converters should be resolved, now */
  fail_unless(g_list_length(env->unresolved_converters) == 0, NULL);

  /* Now all converters should be found */
  OSyncFormatConverter *converter1 = osync_conv_find_converter(env, "fmt_test1", "fmt_test2");
  fail_unless(converter1 == converter, NULL); /* Should be the same converter found above */

  OSyncFormatConverter *converter2 = osync_conv_find_converter(env, "fmt_test2", "fmt_test3");
  fail_unless(converter2 != NULL, NULL);
  fail_unless(converter2->type == CONVERTER_ENCAP, NULL);
  fail_unless(!strcmp(converter2->source_format->name,"fmt_test2") , NULL);
  fail_unless(!strcmp(converter2->target_format->name,"fmt_test3") , NULL);
  fail_unless(converter2->convert_func == dummyconvert, NULL);

  OSyncFormatConverter *converter3 = osync_conv_find_converter(env, "fmt_test3", "fmt_test2");
  fail_unless(converter3 != NULL, NULL);
  fail_unless(converter3->type == CONVERTER_DESENCAP, NULL);
  fail_unless(!strcmp(converter3->source_format->name,"fmt_test3") , NULL);
  fail_unless(!strcmp(converter3->target_format->name,"fmt_test2") , NULL);
  fail_unless(converter3->convert_func == dummyconvert, NULL);
}
END_TEST

START_TEST (conv_env_add_converter_unresolved)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  osync_conv_register_objtype(env, "test");
  osync_conv_register_objformat(env, "test", "fmt_test1");
  osync_conv_register_objformat(env, "test", "fmt_test3");
  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test1", "fmt_test2", dummyconvert, 0);
  OSyncFormatConverter *conv = osync_conv_find_converter(env, "fmt_test1", "fmt_test2");
  fail_unless(conv == NULL, "Converter should not be registered");
}
END_TEST

START_TEST (conv_env_osp_simple)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  OSyncObjType *type = osync_conv_register_objtype(env, "test");
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "test", "fmt_test1");
  OSyncObjFormat *format2 = osync_conv_register_objformat(env, "test", "fmt_test2");
  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test1", "fmt_test2", dummyconvert, 0);
  OSyncFormatConverter *converter1 = osync_conv_find_converter(env, "fmt_test1", "fmt_test2");
  fail_unless(converter1 != NULL, NULL);
  mark_point();
  GList *converters;
  fail_unless(osync_conv_find_shortest_path(type->converters, format1, format2, &converters), NULL);
  fail_unless(g_list_length(converters) == 1, NULL);
  fail_unless(g_list_nth_data(converters, 0) == converter1, NULL);
}
END_TEST

START_TEST (conv_env_osp_simple2)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  OSyncObjType *type = osync_conv_register_objtype(env, "test");
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "test", "fmt_test1");
  osync_conv_register_objformat(env, "test", "fmt_test2");
  OSyncObjFormat *format3 = osync_conv_register_objformat(env, "test", "fmt_test3");
  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test1", "fmt_test2", dummyconvert, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test2", "fmt_test3", dummyconvert, 0);
  OSyncFormatConverter *converter1 = osync_conv_find_converter(env, "fmt_test1", "fmt_test2");
  OSyncFormatConverter *converter2 = osync_conv_find_converter(env, "fmt_test2", "fmt_test3");
  fail_unless(converter1 != NULL, NULL);
  fail_unless(converter2 != NULL, NULL);

  mark_point();
  GList *converters;
  fail_unless(osync_conv_find_shortest_path(type->converters, format1, format3, &converters), NULL);
  fail_unless(g_list_length(converters) == 2, NULL);
  fail_unless(g_list_nth_data(converters, 0) == converter1, NULL);
  fail_unless(g_list_nth_data(converters, 1) == converter2, NULL);
}
END_TEST

START_TEST (conv_env_osp_false)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  OSyncObjType *type = osync_conv_register_objtype(env, "test");
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "test", "fmt_test1");
  osync_conv_register_objformat(env, "test", "fmt_test2");
  OSyncObjFormat *format3 = osync_conv_register_objformat(env, "test", "fmt_test3");
  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test1", "fmt_test2", dummyconvert, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test3", "fmt_test2", dummyconvert, 0);
  mark_point();
  GList *converters;
  fail_unless(!osync_conv_find_shortest_path(type->converters, format1, format3, &converters), NULL);
  fail_unless(converters == FALSE, NULL);
}
END_TEST

START_TEST (conv_env_osp_2way)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  OSyncObjType *type = osync_conv_register_objtype(env, "test");
  
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "test", "fmt_test1");
  osync_conv_register_objformat(env, "test", "fmt_test2");
  osync_conv_register_objformat(env, "test", "fmt_test3");
  OSyncObjFormat *format4 = osync_conv_register_objformat(env, "test", "fmt_test4");

  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test1", "fmt_test2", dummyconvert, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test2", "fmt_test4", dummyconvert, 0);
  OSyncFormatConverter *converter1 = osync_conv_find_converter(env, "fmt_test1", "fmt_test2");
  OSyncFormatConverter *converter2 = osync_conv_find_converter(env, "fmt_test2", "fmt_test4");

  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test1", "fmt_test3", dummyconvert, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test3", "fmt_test4", dummyconvert, 0);

  mark_point();
  GList *converters;
  fail_unless(osync_conv_find_shortest_path(type->converters, format1, format4, &converters), NULL);
  fail_unless(g_list_length(converters) == 2, NULL);
  fail_unless(g_list_nth_data(converters, 0) == converter1, NULL);
  fail_unless(g_list_nth_data(converters, 1) == converter2, NULL);
}
END_TEST

START_TEST (conv_env_osp_circular_false)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  OSyncObjType *type = osync_conv_register_objtype(env, "test");
  
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "test", "fmt_test1");
  osync_conv_register_objformat(env, "test", "fmt_test2");
  osync_conv_register_objformat(env, "test", "fmt_test3");
  OSyncObjFormat *format4 = osync_conv_register_objformat(env, "test", "fmt_test4");

  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test1", "fmt_test2", dummyconvert, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test2", "fmt_test3", dummyconvert, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "fmt_test3", "fmt_test1", dummyconvert, 0);

  mark_point();
  GList *converters;
  fail_unless(!osync_conv_find_shortest_path(type->converters, format1, format4, &converters), NULL);
  fail_unless(converters == NULL, NULL);
}
END_TEST

START_TEST (conv_env_osp_complex)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  OSyncObjType *type = osync_conv_register_objtype(env, "test");
  
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "test", "A");
  osync_conv_register_objformat(env, "test", "B");
  osync_conv_register_objformat(env, "test", "C");
  osync_conv_register_objformat(env, "test", "D");
  osync_conv_register_objformat(env, "test", "E");
  OSyncObjFormat *format2 = osync_conv_register_objformat(env, "test", "F");
  osync_conv_register_objformat(env, "test", "G");
  osync_conv_register_objformat(env, "test", "H");

  osync_conv_register_converter(env, CONVERTER_CONV, "A", "B", dummyconvert, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "A", "C", dummyconvert, 0);
  OSyncFormatConverter *converter1 = osync_conv_find_converter(env, "A", "C");
  osync_conv_register_converter(env, CONVERTER_CONV, "A", "D", dummyconvert, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "C", "E", dummyconvert, 0);
  OSyncFormatConverter *converter2 = osync_conv_find_converter(env, "C", "E");
  osync_conv_register_converter(env, CONVERTER_CONV, "D", "G", dummyconvert, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "E", "G", dummyconvert, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "G", "H", dummyconvert, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "H", "F", dummyconvert, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "E", "F", dummyconvert, 0);
  OSyncFormatConverter *converter3 = osync_conv_find_converter(env, "E", "F");

  mark_point();
  GList *converters;
  fail_unless(osync_conv_find_shortest_path(type->converters, format1, format2, &converters), NULL);
  fail_unless(g_list_length(converters) == 3, NULL);
  fail_unless(g_list_nth_data(converters, 0) == converter1, NULL);
  fail_unless(g_list_nth_data(converters, 1) == converter2, NULL);
  fail_unless(g_list_nth_data(converters, 2) == converter3, NULL);
}
END_TEST

static osync_bool convert_addtest(const char *input, int inpsize, char **output, int *outpsize)
{
	*output = g_strdup_printf("%stest", input);
	*outpsize = inpsize + 4;
	return TRUE;
}

static osync_bool convert_remtest(const char *input, int inpsize, char **output, int *outpsize)
{
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

static osync_bool convert_addtest2(const char *input, int inpsize, char **output, int *outpsize)
{
	*output = g_strdup_printf("%stest2", input);
	*outpsize = inpsize + 5;
	return TRUE;
}

static osync_bool convert_remtest2(const char *input, int inpsize, char **output, int *outpsize)
{
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
  OSyncFormatEnv *env = osync_conv_env_new();
  OSyncObjType *type = osync_conv_register_objtype(env, "O1");
  
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "O1", "F1");
  osync_conv_register_objformat(env, "O1", "F2");
  OSyncObjFormat *format3 = osync_conv_register_objformat(env, "O1", "F3");
  osync_conv_register_converter(env, CONVERTER_CONV, "F1", "F2", convert_addtest, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "F2", "F3", convert_addtest2, 0);
  mark_point();
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_set_objtype(change, type);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();
  osync_conv_convert(env, change, format3);
  
  char *data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatesttest2"), NULL);
  OSyncObjFormat *format = g_list_last(change->objformats)->data;
  fail_unless(format == format3, NULL);
}
END_TEST

START_TEST (conv_env_convert_back)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  OSyncObjType *type = osync_conv_register_objtype(env, "O1");
  
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "O1", "F1");
  osync_conv_register_objformat(env, "O1", "F2");
  OSyncObjFormat *format3 = osync_conv_register_objformat(env, "O1", "F3");
  osync_conv_register_converter(env, CONVERTER_CONV, "F1", "F2", convert_addtest, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "F2", "F1", convert_remtest, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "F2", "F3", convert_addtest2, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "F3", "F2", convert_remtest2, 0);
  mark_point();
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_set_objtype(change, type);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();
  osync_conv_convert(env, change, format3);
  
  char *data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatesttest2"), NULL);
  OSyncObjFormat *format = g_list_last(change->objformats)->data;
  fail_unless(format == format3, NULL);
  
  mark_point();
  osync_conv_convert(env, change, format1);
  
  data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "data"), NULL);
  format = g_list_last(change->objformats)->data;
  fail_unless(format == format1, NULL);
}
END_TEST

START_TEST (conv_env_convert_desenc)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  OSyncObjType *type = osync_conv_register_objtype(env, "O1");
  
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "O1", "F1");
  OSyncObjFormat *format2 = osync_conv_register_objformat(env, "O1", "F2");
  OSyncObjFormat *format3 = osync_conv_register_objformat(env, "O1", "F3");
  osync_conv_register_converter(env, CONVERTER_DESENCAP, "F1", "F2", convert_addtest, 0);
  osync_conv_register_converter(env, CONVERTER_ENCAP, "F2", "F1", convert_remtest, 0);
  osync_conv_register_converter(env, CONVERTER_DESENCAP, "F2", "F3", convert_addtest2, 0);
  osync_conv_register_converter(env, CONVERTER_ENCAP, "F3", "F2", convert_remtest2, 0);
  mark_point();
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format3);
  osync_change_append_objformat(change, format2);
  osync_change_append_objformat(change, format1);
  osync_change_set_objtype(change, type);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();
  osync_conv_convert(env, change, format3);
  
  char *data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatesttest2"), NULL);
  OSyncObjFormat *format = g_list_last(change->objformats)->data;
  fail_unless(format == format3, NULL);
  fail_unless(g_list_length(change->objformats) == 1, NULL);
  
  mark_point();
  osync_conv_convert(env, change, format1);
  
  data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "data"), NULL);
  format = g_list_last(change->objformats)->data;
  fail_unless(format == format1, NULL);
  fail_unless(g_list_length(change->objformats) == 3, NULL);
  fail_unless(g_list_nth_data(change->objformats, 0) == format3, NULL);
  fail_unless(g_list_nth_data(change->objformats, 1) == format2, NULL);
  fail_unless(g_list_nth_data(change->objformats, 2) == format1, NULL);
}
END_TEST

START_TEST (conv_env_convert_desenc_complex)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  OSyncObjType *type = osync_conv_register_objtype(env, "O1");
  
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "O1", "F1");
  OSyncObjFormat *format2 = osync_conv_register_objformat(env, "O1", "F2");
  osync_conv_register_objformat(env, "O1", "F3");
  OSyncObjFormat *format4 = osync_conv_register_objformat(env, "O1", "F4");
  OSyncObjFormat *format5 = osync_conv_register_objformat(env, "O1", "F5");
  OSyncObjFormat *format6 = osync_conv_register_objformat(env, "O1", "F6");
  osync_conv_register_converter(env, CONVERTER_DESENCAP, "F1", "F2", convert_addtest, 0);
  osync_conv_register_converter(env, CONVERTER_ENCAP, "F2", "F1", convert_remtest, 0);
  osync_conv_register_converter(env, CONVERTER_DESENCAP, "F2", "F3", convert_addtest2, 0);
  osync_conv_register_converter(env, CONVERTER_ENCAP, "F3", "F2", convert_remtest2, 0);
  osync_conv_register_converter(env, CONVERTER_DESENCAP, "F2", "F4", convert_addtest2, 0);
  osync_conv_register_converter(env, CONVERTER_ENCAP, "F4", "F2", convert_remtest2, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "F3", "F6", convert_addtest2, 0);
  osync_conv_register_converter(env, CONVERTER_DESENCAP, "F2", "F4", convert_addtest2, 0);
  osync_conv_register_converter(env, CONVERTER_ENCAP, "F4", "F2", convert_remtest2, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "F4", "F5", convert_addtest2, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "F5", "F4", convert_remtest2, 0);
  osync_conv_register_converter(env, CONVERTER_ENCAP, "F5", "F6", convert_addtest2, 0);
  osync_conv_register_converter(env, CONVERTER_DESENCAP, "F6", "F5", convert_remtest2, 0);

  mark_point();
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format4);
  osync_change_append_objformat(change, format2);
  osync_change_append_objformat(change, format1);
  osync_change_set_objtype(change, type);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();
  osync_conv_convert(env, change, format6);
  
  char *data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatesttest2test2test2"), NULL);
  fail_unless(g_list_length(change->objformats) == 2, NULL);
  fail_unless(g_list_nth_data(change->objformats, 0) == format5, NULL);
  fail_unless(g_list_nth_data(change->objformats, 1) == format6, NULL);
  
  mark_point();
  osync_conv_convert(env, change, format1);
  
  data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "data"), NULL);
  fail_unless(g_list_length(change->objformats) == 3, NULL);
  fail_unless(g_list_nth_data(change->objformats, 0) == format4, NULL);
  fail_unless(g_list_nth_data(change->objformats, 1) == format2, NULL);
  fail_unless(g_list_nth_data(change->objformats, 2) == format1, NULL);
}
END_TEST

osync_bool detect_format1_no(OSyncFormatEnv *env, OSyncChange *change)
{
	return FALSE;
}

osync_bool detect_format1(OSyncFormatEnv *env, OSyncChange *change)
{	
	OSyncObjFormat *format2 = osync_conv_find_objformat(env, "F2");
	osync_change_prepend_objformat(change, format2);
	return TRUE;
}

osync_bool detect_format2(OSyncFormatEnv *env, OSyncChange *change)
{
	change->objtype = osync_conv_find_objtype(env, "O1");
	OSyncObjFormat *format3 = osync_conv_find_objformat(env, "F3");
	osync_change_prepend_objformat(change, format3);
	return TRUE;
}

START_TEST (conv_env_detect_and_convert)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  OSyncObjType *type = osync_conv_register_objtype(env, "O1");
  
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "O1", "F1");
  OSyncObjFormat *format2 = osync_conv_register_objformat(env, "O1", "F2");
  OSyncObjFormat *format3 = osync_conv_register_objformat(env, "O1", "F3");
  OSyncObjFormat *format4 = osync_conv_register_objformat(env, "O1", "F4");
  osync_conv_format_set_detect_func(format1, detect_format1);
  osync_conv_format_set_detect_func(format2, detect_format2);
  
  osync_conv_register_converter(env, CONVERTER_DESENCAP, "F1", "F2", convert_addtest, 0);
  osync_conv_register_converter(env, CONVERTER_ENCAP, "F2", "F1", convert_remtest, 0);
  osync_conv_register_converter(env, CONVERTER_DESENCAP, "F2", "F3", convert_addtest2, 0);
  osync_conv_register_converter(env, CONVERTER_ENCAP, "F3", "F2", convert_remtest2, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "F3", "F4", convert_addtest2, 0);
  osync_conv_register_converter(env, CONVERTER_CONV, "F4", "F3", convert_remtest2, 0);
  mark_point();
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();

  fail_unless(osync_conv_convert(env, change, format4), NULL);
  mark_point();
  char *data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "datatesttest2test2"), NULL);
  fail_unless(change->objtype == type, NULL);
  OSyncObjFormat *format = g_list_last(change->objformats)->data;
  fail_unless(format == format4, NULL);
  fail_unless(g_list_length(change->objformats) == 1, NULL);
  
  mark_point();
  osync_conv_convert(env, change, format1);
  mark_point();
  data = osync_change_get_data(change);
  fail_unless(!strcmp(data, "data"), NULL);
  format = g_list_last(change->objformats)->data;
  fail_unless(format == format1, NULL);
  fail_unless(g_list_length(change->objformats) == 3, NULL);
  fail_unless(g_list_nth_data(change->objformats, 0) == format3, NULL);
  fail_unless(g_list_nth_data(change->objformats, 1) == format2, NULL);
  fail_unless(g_list_nth_data(change->objformats, 2) == format1, NULL);
}
END_TEST

START_TEST (conv_env_detect_false)
{
  OSyncFormatEnv *env = osync_conv_env_new();
  osync_conv_register_objtype(env, "O1");
  
  OSyncObjFormat *format1 = osync_conv_register_objformat(env, "O1", "F1");
  OSyncObjFormat *format2 = osync_conv_register_objformat(env, "O1", "F2");
  OSyncObjFormat *format3 = osync_conv_register_objformat(env, "O1", "F3");
  osync_conv_format_set_detect_func(format1, detect_format1);
  osync_conv_format_set_detect_func(format2, detect_format1_no);
  
  osync_conv_register_converter(env, CONVERTER_DESENCAP, "F1", "F2", convert_addtest, 0);
  osync_conv_register_converter(env, CONVERTER_ENCAP, "F2", "F1", convert_remtest, 0);
  osync_conv_register_converter(env, CONVERTER_DESENCAP, "F2", "F3", convert_addtest2, 0);
  osync_conv_register_converter(env, CONVERTER_ENCAP, "F3", "F2", convert_remtest2, 0);
  mark_point();
  OSyncChange *change = osync_change_new();
  osync_change_set_objformat(change, format1);
  osync_change_set_data(change, "data", 5, TRUE);
  
  mark_point();
  fail_unless(!osync_conv_convert(env, change, format3), NULL);
}
END_TEST

Suite *env_suite(void)
{
	Suite *s = suite_create("Conv");
	TCase *tc_env = tcase_create("env");
	TCase *tc_type = tcase_create("type");
	TCase *tc_format = tcase_create("format");
	TCase *tc_conv = tcase_create("conv");
	TCase *tc_osp = tcase_create("osp");
	TCase *tc_convert = tcase_create("convert");
	TCase *tc_detect = tcase_create("detect");
	suite_add_tcase (s, tc_env);
	suite_add_tcase (s, tc_type);
	suite_add_tcase (s, tc_format);
	suite_add_tcase (s, tc_conv);
	suite_add_tcase (s, tc_osp);
	suite_add_tcase (s, tc_convert);
	suite_add_tcase (s, tc_detect);
	tcase_add_test(tc_env, conv_env_create);
	tcase_add_test(tc_type, conv_env_add_type);
	tcase_add_test(tc_type, conv_env_add_type_find);
	tcase_add_test(tc_type, conv_env_add_type_find_false);
	tcase_add_test(tc_type, conv_env_type_register2);
	tcase_add_test(tc_format, conv_env_add_format);
	tcase_add_test(tc_format, conv_env_set_format);
	tcase_add_test(tc_format, conv_env_set_format_string);
	tcase_add_test(tc_format, conv_env_append_format);
	tcase_add_test(tc_conv, conv_env_add_converter_resolve_later);
	tcase_add_test(tc_conv, conv_env_add_converter_unresolved);
	tcase_add_test(tc_osp, conv_env_osp_simple);
	tcase_add_test(tc_osp, conv_env_osp_simple2);
	tcase_add_test(tc_osp, conv_env_osp_false);
	tcase_add_test(tc_osp, conv_env_osp_2way);
	tcase_add_test(tc_osp, conv_env_osp_circular_false);
	tcase_add_test(tc_osp, conv_env_osp_complex);
	tcase_add_test(tc_convert, conv_env_convert1);
	tcase_add_test(tc_convert, conv_env_convert_back);
	tcase_add_test(tc_convert, conv_env_convert_desenc);
	tcase_add_test(tc_convert, conv_env_convert_desenc_complex);
	tcase_add_test(tc_detect, conv_env_detect_and_convert);
	tcase_add_test(tc_detect, conv_env_detect_false);
	return s;
}

int main(void)
{
	int nf;
	
	Suite *s = env_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
