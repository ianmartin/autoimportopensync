#include "support.h"

#include <opensync/opensync-format.h>
#include <opensync/opensync-serializer.h>

static OSyncConvCmpResult compare_format(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	if (rightsize == leftsize && !strcmp(leftdata, rightdata))
		return OSYNC_CONV_DATA_SAME;
	
	return OSYNC_CONV_DATA_MISMATCH;
}

void destroy_format(char *data, unsigned int size)
{
	g_free(data);
}

osync_bool copy_format(const char *indata, unsigned int insize, char **outdata, unsigned int *outsize, OSyncError **error)
{
	*outdata = strdup(indata);
	*outsize = insize;
	return TRUE;
}

static osync_bool duplicate_format(const char *uid, const char *input, unsigned int insize, char **newuid, char **output, unsigned int *outsize, osync_bool *dirty, OSyncError **error)
{
	fail_unless(!strcmp(uid, "uid"), NULL);
	*newuid = strdup("newuid");
	return TRUE;
}

void create_format(char **data, unsigned int *size)
{
	*data = strdup("data");
	*size = 5;
}

char *print_format(const char *data, unsigned int size)
{
	return strdup(data);
}

time_t revision_format(const char *data, unsigned int size, OSyncError **error)
{
	return atoi(data);
}

osync_bool marshal_format(const char *input, unsigned int inpsize, OSyncMessage *message, OSyncError **error)
{
	osync_message_write_buffer(message, input, inpsize);
	return TRUE;
}

osync_bool demarshal_format(OSyncMessage *message, char **output, unsigned int *outsize, OSyncError **error)
{
	osync_message_read_buffer(message, (void *)output, (int *)outsize);
	return TRUE;
}

START_TEST (objformat_new)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("format1", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_objformat_ref(format);
	osync_objformat_unref(format);
	osync_objformat_unref(format);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (objformat_get)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!strcmp(osync_objformat_get_name(format), "format"), NULL);
	fail_unless(!strcmp(osync_objformat_get_objtype(format), "objtype"), NULL);
	
	osync_objformat_unref(format);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (objformat_equal)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format1 = osync_objformat_new("format", "objtype", &error);
	fail_unless(format1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	OSyncObjFormat *format2 = osync_objformat_new("format", "objtype", &error);
	fail_unless(format2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	OSyncObjFormat *format3 = osync_objformat_new("format2", "objtype", &error);
	fail_unless(format3 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_objformat_is_equal(format1, format2), NULL);
	fail_unless(!osync_objformat_is_equal(format1, format3), NULL);
	
	osync_objformat_unref(format1);
	osync_objformat_unref(format2);
	osync_objformat_unref(format3);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (objformat_compare)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_objformat_set_compare_func(format, compare_format);
	
	fail_unless(osync_objformat_compare(format, "test", 5, "test", 5) == OSYNC_CONV_DATA_SAME, NULL);
	fail_unless(osync_objformat_compare(format, "test", 5, "tesd", 5) == OSYNC_CONV_DATA_MISMATCH, NULL);
	
	osync_objformat_unref(format);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (objformat_destroy)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_objformat_set_destroy_func(format, destroy_format);
	
	osync_objformat_destroy(format, strdup("test"), 5);
	
	osync_objformat_unref(format);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (objformat_copy)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_objformat_set_copy_func(format, copy_format);
	osync_objformat_set_destroy_func(format, destroy_format);
	
	char *outdata = NULL;
	unsigned int outsize = 0;
	fail_unless(osync_objformat_copy(format, "test", 5, &outdata, &outsize, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!strcmp(outdata, "test"), NULL);
	fail_unless(outsize == 5, NULL);
	
	osync_objformat_destroy(format, outdata, 5);
	
	osync_objformat_unref(format);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (objformat_duplicate)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_objformat_set_duplicate_func(format, duplicate_format);
	
	char *newuid = NULL;
	char *output = NULL;
	unsigned int outsize = 0;
	osync_bool dirty = FALSE;
	fail_unless(osync_objformat_duplicate(format, "uid", "test", 5, &newuid, &output, &outsize, &dirty, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!strcmp(newuid, "newuid"), NULL);
	g_free(newuid);
	
	osync_objformat_unref(format);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (objformat_create)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_objformat_set_create_func(format, create_format);
	osync_objformat_set_destroy_func(format, destroy_format);
	
	char *outdata = NULL;
	unsigned int outsize = 0;
	osync_objformat_create(format, &outdata, &outsize);
	
	fail_unless(!strcmp(outdata, "data"), NULL);
	fail_unless(outsize == 5, NULL);
	
	osync_objformat_destroy(format, outdata, 5);
	
	osync_objformat_unref(format);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (objformat_print)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_objformat_set_print_func(format, print_format);
	
	char *print = osync_objformat_print(format, "test", 5);
	
	fail_unless(!strcmp(print, "test"), NULL);
	g_free(print);
	
	osync_objformat_unref(format);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (objformat_revision)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_objformat_set_revision_func(format, revision_format);
	
	time_t curtime = osync_objformat_get_revision(format, "5", 2, &error);
	fail_unless(error == NULL, NULL);
	
	fail_unless(curtime == 5, NULL);
	
	osync_objformat_unref(format);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (objformat_marshal)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	fail_unless(osync_objformat_must_marshal(format) == FALSE, NULL);
	
	osync_objformat_set_marshal_func(format, marshal_format);
	osync_objformat_set_destroy_func(format, destroy_format);
	
	fail_unless(osync_objformat_must_marshal(format) == TRUE, NULL);

	OSyncMessage *message = osync_message_new(0, 0, &error);
	fail_unless(message != NULL, NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(osync_objformat_marshal(format, "test", 5, message, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_message_unref(message);
	
	osync_objformat_unref(format);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (objformat_demarshal)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("format", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_objformat_set_demarshal_func(format, demarshal_format);
	osync_objformat_set_marshal_func(format, marshal_format);
	osync_objformat_set_destroy_func(format, destroy_format);
	
	OSyncMessage *message = osync_message_new(0, 0, &error);
	fail_unless(message != NULL, NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(osync_objformat_marshal(format, "test", 5, message, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	char *outdata = NULL;
	unsigned int outsize = 0;
	fail_unless(osync_objformat_demarshal(format, message, &outdata, &outsize, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!strcmp(outdata, "test"), NULL);
	fail_unless(outsize == 5, NULL);
	g_free(outdata);
	
	osync_objformat_unref(format);
	osync_message_unref(message);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *objformat_suite(void)
{
	Suite *s = suite_create("Objformat");
//	Suite *s2 = suite_create("Objformat");
	
	create_case(s, "objformat_new", objformat_new);
	create_case(s, "objformat_get", objformat_get);
	create_case(s, "objformat_equal", objformat_equal);
	create_case(s, "objformat_compare", objformat_compare);
	create_case(s, "objformat_destroy", objformat_destroy);
	create_case(s, "objformat_copy", objformat_copy);
	create_case(s, "objformat_duplicate", objformat_duplicate);
	create_case(s, "objformat_create", objformat_create);
	create_case(s, "objformat_print", objformat_print);
	create_case(s, "objformat_revision", objformat_revision);
	create_case(s, "objformat_marshal", objformat_marshal);
	create_case(s, "objformat_demarshal", objformat_demarshal);
	
	return s;
}

int main(void)
{
	int nf;
	
	Suite *s = objformat_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
