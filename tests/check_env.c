#include "support.h"

START_TEST (env_create)
{
  OSyncEnv *os_env = osync_env_new();
  fail_unless(os_env != NULL, NULL);
}
END_TEST

START_TEST (env_free)
{
  OSyncEnv *os_env = osync_env_new();
  fail_unless(os_env != NULL, NULL);
  osync_env_free(os_env);
}
END_TEST

START_TEST (env_init)
{
  char *testbed = setup_testbed("env_init");
  OSyncEnv *env = osync_env_new();
  fail_unless(env != NULL, NULL);
  
  osync_env_set_option(env, "GROUPS_DIRECTORY", "configs");
  
  fail_unless(osync_env_initialize(env, NULL), NULL);
  
  fail_unless(osync_env_finalize(env, NULL), NULL);
  osync_env_free(env);
  destroy_testbed(testbed);
}
END_TEST

START_TEST (env_double_init)
{
  char *testbed = setup_testbed("env_init");
  OSyncEnv *env = osync_env_new();
  fail_unless(env != NULL, NULL);
  
  osync_env_set_option(env, "GROUPS_DIRECTORY", "configs");
  fail_unless(osync_env_initialize(env, NULL), NULL);
  fail_unless(!osync_env_initialize(env, NULL), NULL);
  
  fail_unless(osync_env_finalize(env, NULL), NULL);
  osync_env_free(env);
  destroy_testbed(testbed);
}
END_TEST

START_TEST (env_pre_fin)
{
  char *testbed = setup_testbed("env_init");
  OSyncEnv *env = osync_env_new();
  fail_unless(env != NULL, NULL);
  
  osync_env_set_option(env, "GROUPS_DIRECTORY", "configs");
  
  fail_unless(!osync_env_finalize(env, NULL), NULL);
  osync_env_free(env);
  destroy_testbed(testbed);
}
END_TEST


START_TEST (env_init_false)
{
  char *testbed = setup_testbed("sync_setup_false");
  OSyncEnv *osync = osync_env_new();
  osync_env_set_option(osync, "GROUPS_DIRECTORY", "configs");
  OSyncError *error = NULL;
  osync_env_initialize(osync, &error);
  fail_unless(osync_env_num_groups(osync) == 1, NULL);
  destroy_testbed(testbed);
}
END_TEST

START_TEST (env_init_false2)
{
  char *testbed = setup_testbed("sync_setup_false");
  OSyncEnv *osync = init_env();
  OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
  fail_unless(group != NULL, NULL);
  fail_unless(osync_env_num_groups(osync) == 1, NULL);
  destroy_testbed(testbed);
}
END_TEST

START_TEST (env_sync_false)
{
	char *testbed = setup_testbed("sync_setup_false");
	OSyncEnv *env = init_env();
	OSyncGroup *group = osync_group_load(env, "configs/group", NULL);
	
	OSyncEngine *engine = osync_engine_new(group, NULL);
	
	OSyncError *error = NULL;
	fail_unless(!osync_engine_init(engine, &error), NULL);
	fail_unless(!synchronize_once(engine, NULL), NULL);
	osync_engine_finalize(engine);
	osync_engine_free(engine);
	
	osync_env_finalize(env, NULL);
	osync_env_free(env);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (env_check_plugin_true1)
{
	OSyncEnv *env = init_env();
	
	OSyncError *error = NULL;
	fail_unless(osync_env_plugin_is_usable(env, "file-sync", &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	osync_env_free(env);
}
END_TEST

START_TEST (env_check_plugin_true2)
{
	g_setenv("IS_AVAILABLE", "1", TRUE);
	
	OSyncEnv *env = init_env();
	
	OSyncError *error = NULL;
	
	fail_unless(osync_env_plugin_is_usable(env, "file-sync", &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	osync_env_free(env);
}
END_TEST

START_TEST (env_check_plugin_false)
{
	OSyncEnv *env = init_env();
	
	OSyncError *error = NULL;
	fail_unless(!osync_env_plugin_is_usable(env, "file-syncc", &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_env_free(env);
}
END_TEST

START_TEST (env_check_plugin_false2)
{
	g_setenv("IS_AVAILABLE", "1", TRUE);
	g_setenv("IS_NOT_AVAILABLE", "1", TRUE);
	
	OSyncEnv *env = init_env();
	
	OSyncError *error = NULL;

	fail_unless(!osync_env_plugin_is_usable(env, "file-sync", &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_env_free(env);
}
END_TEST

Suite *env_suite(void)
{
	Suite *s = suite_create("Env");
	//Suite *s2 = suite_create("Env");
	create_case(s, "env_create", env_create);
	create_case(s, "env_free", env_free);
	create_case(s, "env_init", env_init);
	create_case(s, "env_double_init", env_double_init);
	create_case(s, "env_pre_fin", env_pre_fin);
	create_case(s, "env_init_false", env_init_false);
	create_case(s, "env_init_false2", env_init_false2);
	create_case(s, "env_sync_false", env_sync_false);
	create_case(s, "env_check_plugin_true1", env_check_plugin_true1);
	create_case(s, "env_check_plugin_true2", env_check_plugin_true2);
	create_case(s, "env_check_plugin_false", env_check_plugin_false);
	create_case(s, "env_check_plugin_false2", env_check_plugin_false2);

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
