#include "support.h"

#include <opensync/opensync-module.h>

START_TEST (module_create)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncModule *module = osync_module_new(&error);
	fail_unless(module != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_module_free(module);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (module_load)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncModule *module = osync_module_new(&error);
	fail_unless(module != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	char *curdir = g_get_current_dir();
	char *path = g_strdup_printf("%s/mockformat.so", curdir);
	fail_unless(osync_module_load(module, path, &error), NULL);
	fail_unless(error == NULL, NULL);
	g_free(path);
	g_free(curdir);
	
	osync_module_unload(module);
	
	osync_module_free(module);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (module_load_false)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncModule *module = osync_module_new(&error);
	fail_unless(module != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	char *curdir = g_get_current_dir();
	char *path = g_strdup_printf("%s/does-not-exist.so", curdir);
	fail_unless(!osync_module_load(module, path, &error), NULL);
	fail_unless(error != NULL, NULL);
	g_free(path);
	g_free(curdir);
	osync_error_unref(&error);
	
	osync_module_free(module);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (module_function)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncModule *module = osync_module_new(&error);
	fail_unless(module != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	char *curdir = g_get_current_dir();
	char *path = g_strdup_printf("%s/mockformat.so", curdir);
	fail_unless(osync_module_load(module, path, &error), NULL);
	fail_unless(error == NULL, NULL);
	g_free(path);
	g_free(curdir);
	
	void *func = osync_module_get_function(module, "get_version", &error);
	fail_unless(func != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_module_unload(module);
	
	osync_module_free(module);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (module_function_false)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncModule *module = osync_module_new(&error);
	fail_unless(module != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	char *curdir = g_get_current_dir();
	char *path = g_strdup_printf("%s/mockformat.so", curdir);
	fail_unless(osync_module_load(module, path, &error), NULL);
	fail_unless(error == NULL, NULL);
	g_free(path);
	g_free(curdir);
	
	void *func = osync_module_get_function(module, "get_version1", &error);
	fail_unless(func == NULL, NULL);
	fail_unless(error != NULL, NULL);
	osync_error_unref(&error);
	
	osync_module_unload(module);
	
	osync_module_free(module);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (module_version)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncModule *module = osync_module_new(&error);
	fail_unless(module != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	char *curdir = g_get_current_dir();
	char *path = g_strdup_printf("%s/mockformat.so", curdir);
	fail_unless(osync_module_load(module, path, &error), NULL);
	fail_unless(error == NULL, NULL);
	g_free(path);
	g_free(curdir);
	
	int version = osync_module_get_version(module);
	fail_unless(version == 1, NULL);
	
	osync_module_unload(module);
	
	osync_module_free(module);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (module_check)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncModule *module = osync_module_new(&error);
	fail_unless(module != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	char *curdir = g_get_current_dir();
	char *path = g_strdup_printf("%s/mockformat.so", curdir);
	fail_unless(osync_module_load(module, path, &error), NULL);
	fail_unless(error == NULL, NULL);
	g_free(path);
	g_free(curdir);
	
	fail_unless(osync_module_check(module, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_module_unload(module);
	
	osync_module_free(module);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *module_suite(void)
{
	Suite *s = suite_create("Module");
	//Suite *s2 = suite_create("Module");
	
	create_case(s, "module_create", module_create);
	create_case(s, "module_load", module_load);
	create_case(s, "module_load_false", module_load_false);
	create_case(s, "module_function", module_function);
	create_case(s, "module_function_false", module_function_false);
	create_case(s, "module_version", module_version);
	create_case(s, "module_check", module_check);
	
	return s;
}

int main(void)
{
	int nf;
	
	Suite *s = module_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
