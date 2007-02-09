#include "support.h"

START_TEST (plugin_create)
{
  OSyncPlugin *plugin = osync_plugin_new(NULL);
  fail_unless(plugin != NULL, "plugin == NULL on creation");
}
END_TEST

START_TEST(plugin_no_config)
{
	char *testbed = setup_testbed("plugin_no_config");
	OSyncEnv *osync = init_env();
	fail_unless(osync != NULL, NULL);
	
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(osync) == 1, NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(!osengine_init(engine, &error), NULL);
	
	osengine_finalize(engine);
	osengine_free(engine);
	  
	fail_unless(osync_env_finalize(osync, NULL), NULL);
	osync_env_free(osync);
	destroy_testbed(testbed);
}
END_TEST

START_TEST(plugin_call_custom)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncMember *member = osync_group_nth_member(group, 0);
	
	OSyncError *error = NULL;
	int ret = GPOINTER_TO_INT(osync_member_call_plugin(member, "mock_custom_function", GINT_TO_POINTER(1), &error));
	fail_unless(ret == 2, NULL);
	
	osync_env_finalize(osync, &error);
	osync_env_free(osync);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *plugin_suite(void)
{
	Suite *s = suite_create("Plugins");
	//Suite *s2 = suite_create("Plugins");
	
	create_case(s, "plugin_create", plugin_create);
	create_case(s, "plugin_no_config", plugin_no_config);
	create_case(s, "plugin_call_custom", plugin_call_custom);

	return s;
}

int main(void)
{
	int nf;

	Suite *s = plugin_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_set_fork_status (sr, CK_NOFORK);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
