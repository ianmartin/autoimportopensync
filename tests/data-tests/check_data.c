#include "support.h"

#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>

START_TEST (data_new)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("test", "test", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncData *data = osync_data_new(NULL, 0, format, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_data_ref(data);
	osync_data_unref(data);
	
	osync_data_unref(data);
	osync_objformat_unref(format);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (data_new_with_data)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("test", "test", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncData *data = osync_data_new("test", 4, format, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_objformat_unref(format);
	
	char *buffer = NULL;
	unsigned int size = 0;
	osync_data_get_data(data, &buffer, &size);
	fail_unless(!strncmp(buffer, "test", 4), NULL);
	fail_unless(size == 4, NULL);
	
	osync_data_unref(data);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (data_set_data)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("test", "test", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncData *data = osync_data_new(NULL, 0, format, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_objformat_unref(format);
	
	osync_data_set_data(data, "test", 4);
	
	char *buffer = NULL;
	unsigned int size = 0;
	osync_data_get_data(data, &buffer, &size);
	fail_unless(!strncmp(buffer, "test", 4), NULL);
	fail_unless(size == 4, NULL);
	
	osync_data_unref(data);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (data_set_data2)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("test", "test", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncData *data = osync_data_new(NULL, 0, format, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_objformat_unref(format);
	
	fail_unless(osync_data_has_data(data) == FALSE, NULL);
	
	osync_data_set_data(data, "test", 4);
	
	fail_unless(osync_data_has_data(data) == TRUE, NULL);
	
	char *buffer = NULL;
	unsigned int size = 0;
	osync_data_get_data(data, &buffer, &size);
	fail_unless(!strncmp(buffer, "test", 4), NULL);
	fail_unless(size == 4, NULL);
	
	osync_data_set_data(data, "test2", 5);
	
	osync_data_get_data(data, &buffer, &size);
	fail_unless(!strncmp(buffer, "test2", 5), NULL);
	fail_unless(size == 5, NULL);
	
	osync_data_unref(data);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (data_objformat)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("test", "test", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncData *data = osync_data_new(NULL, 0, format, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_data_get_objformat(data) == format, NULL);
	
	osync_data_set_objformat(data, format);
	osync_data_set_objformat(data, format);
	osync_data_set_objformat(data, format);
	
	osync_data_unref(data);
	osync_objformat_unref(format);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (data_objtype)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("test", "objtype", &error);
	fail_unless(format != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncData *data = osync_data_new(NULL, 0, format, &error);
	fail_unless(data != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_data_get_objtype(data) == NULL, NULL);
	
	osync_data_set_objtype(data, "objtype");
	fail_unless(!strcmp(osync_data_get_objtype(data), "objtype"), NULL);

	osync_data_set_objtype(data, "objtype");
	osync_data_set_objtype(data, "objtype2");
	fail_unless(!strcmp(osync_data_get_objtype(data), "objtype2"), NULL);
	
	osync_data_unref(data);
	osync_objformat_unref(format);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *data_suite(void)
{
	Suite *s = suite_create("Data");
	Suite *s2 = suite_create("Data");
	
	create_case(s, "data_new", data_new);
	create_case(s, "data_new_with_data", data_new_with_data);
	create_case(s, "data_set_data", data_set_data);
	create_case(s2, "data_set_data2", data_set_data2);
	create_case(s, "data_objformat", data_objformat);
	create_case(s, "data_objtype", data_objtype);
	
	/* OSyncData *osync_data_clone(OSyncData *data, OSyncError **error); */
	
	return s2;
}

int main(void)
{
	int nf;

	Suite *s = data_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
