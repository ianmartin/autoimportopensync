#include <check.h>
#include "opensync.h"
#include "opensync_internals.h"

char *olddir = NULL;

char *setup_testbed(char *fkt_name)
{
	setuid(65534);
	char *testbed = g_strdup_printf("%s/testbed.XXXXXX", g_get_tmp_dir());
	mkdtemp(testbed);
	char *command = g_strdup_printf("cp -a data/%s/* %s", fkt_name, testbed);
	if (system(command))
		abort();
	olddir = g_get_current_dir();
	if (chdir(testbed))
		abort();
	g_free(command);
	printf("Seting up %s at %s\n", fkt_name, testbed);
	return testbed;
}

void destroy_testbed(char *path)
{
	char *command = g_strdup_printf("rm -rf %s", path);
	if (olddir)
		chdir(olddir);
	system(command);
	g_free(command);
	printf("Tearing down %s\n", path);
	g_free(path);
}

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

START_TEST (env_configdir)
{
  OSyncEnv *os_env = osync_env_new();
  fail_unless(osync_env_get_configdir(os_env) != NULL, "configpath == NULL on creation");
  osync_env_set_configdir(os_env, "test");
  if (g_ascii_strcasecmp (osync_env_get_configdir(os_env), "test") != 0)
  	fail("configpath == \"test\"");
}
END_TEST

START_TEST (env_init)
{
  char *testbed = setup_testbed("env_init");
  OSyncEnv *env = osync_env_new();
  fail_unless(env != NULL, NULL);
  
  osync_env_set_configdir(env, "configs/group");
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
  
  osync_env_set_configdir(env, "configs/group");
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
  
  osync_env_set_configdir(env, "configs/group");
  
  fail_unless(!osync_env_finalize(env, NULL), NULL);
  osync_env_free(env);
  destroy_testbed(testbed);
}
END_TEST

Suite *env_suite(void)
{
  Suite *s = suite_create("Env");
  TCase *tc_core = tcase_create("Core");

  suite_add_tcase (s, tc_core);
  tcase_add_test(tc_core, env_create);
  tcase_add_test(tc_core, env_free);
  tcase_add_test(tc_core, env_configdir);
  tcase_add_test(tc_core, env_init);
  tcase_add_test(tc_core, env_double_init);
  tcase_add_test(tc_core, env_pre_fin);
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
