#include <check.h>
#include <glib.h>
#include <gmodule.h>
#include <opensync.h>
#include <stdlib.h>

START_TEST (env_create)
{
  OSyncEnv *os_env = osync_env_new();
  fail_unless(os_env != NULL, "env == NULL on creation");
}
END_TEST

START_TEST (env_configdir)
{
  OSyncEnv *os_env = osync_env_new();
  char *configpath = osync_env_get_configdir(os_env);
  fail_unless(configpath != NULL, "configpath == NULL on creation");
  osync_env_set_configdir(os_env, "test");
  configpath = osync_env_get_configdir(os_env);
  if (g_ascii_strcasecmp (configpath, "test") != 0)
  	fail("configpath == \"test\"");
}
END_TEST

START_TEST (env_null_configdir)
{
  char *configpath = osync_env_get_configdir(NULL);
  fail_unless(configpath == NULL, "configpath != NULL on creation");
}
END_TEST

START_TEST (env_null_configdir2)
{
  osync_env_set_configdir(NULL, NULL);
}
END_TEST

Suite *env_suite(void)
{
  Suite *s = suite_create("Env");
  TCase *tc_core = tcase_create("Core");

  suite_add_tcase (s, tc_core);
  tcase_add_test(tc_core, env_create);
  tcase_add_test(tc_core, env_configdir);
  tcase_add_test(tc_core, env_null_configdir);
  tcase_add_test(tc_core, env_null_configdir2);
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
