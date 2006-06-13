#include "support.h"

START_TEST (plugin_new)
{
	char *testbed = setup_testbed(NULL);
	OSyncError *error = NULL;
	
	OSyncPlugin *plugin = osync_plugin_new(&error);
	fail_unless(plugin != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_plugin_ref(plugin);
	osync_plugin_unref(plugin);
	osync_plugin_unref(plugin);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST(plugin_set)
{
	char *testbed = setup_testbed(NULL);
	OSyncError *error = NULL;
	
	OSyncPlugin *plugin = osync_plugin_new(&error);
	fail_unless(plugin != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_plugin_set_name(plugin, "name");
	osync_plugin_set_longname(plugin, "longname");
	osync_plugin_set_description(plugin, "description");
	osync_plugin_set_data(plugin, GINT_TO_POINTER(1));
	osync_plugin_set_initialize(plugin, GINT_TO_POINTER(1));
	osync_plugin_set_finalize(plugin, GINT_TO_POINTER(1));
	
	fail_unless(!strcmp(osync_plugin_get_name(plugin), "name"), NULL);
	fail_unless(!strcmp(osync_plugin_get_longname(plugin), "longname"), NULL);
	fail_unless(!strcmp(osync_plugin_get_description(plugin), "description"), NULL);
	fail_unless(GPOINTER_TO_INT(osync_plugin_get_data(plugin)) == 1, NULL);
	fail_unless(GPOINTER_TO_INT(osync_plugin_get_initialize(plugin)) == 1, NULL);
	fail_unless(GPOINTER_TO_INT(osync_plugin_get_finalize(plugin)) == 1, NULL);
	
	osync_plugin_unref(plugin);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *plugin_suite(void)
{
	Suite *s = suite_create("Plugin");
	//Suite *s2 = suite_create("Plugin");
	
	create_case(s, "plugin_new", plugin_new);
	create_case(s, "plugin_set", plugin_set);

	return s;
}

int main(void)
{
	int nf;

	Suite *s = plugin_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
