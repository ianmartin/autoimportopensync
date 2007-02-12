#include <check.h>
#include <glib.h>
#include <gmodule.h>
#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>
#include <stdlib.h>

START_TEST (user_create)
{
  OSyncUserInfo *osuser = _osync_user_new();
  fail_unless(osuser != NULL, "osuser == NULL on creation");
  char *confdir = _osync_user_get_confdir(osuser);
  fail_unless(confdir != NULL, "confdir == NULL on creation");
}
END_TEST

START_TEST (user_confdir)
{
  OSyncUserInfo *osuser = _osync_user_new();
  char *configdir = _osync_user_get_confdir(osuser);
  fail_unless(configdir != NULL, "configdir == NULL on creation");
  _osync_user_set_confdir(osuser, "test");
  configdir = _osync_user_get_confdir(osuser);
  if (g_ascii_strcasecmp (configdir, "test") != 0)
  	fail("configpath == \"test\"");
}
END_TEST

START_TEST (user_null_configdir)
{
  char *configpath = _osync_user_get_confdir(NULL);
  fail_unless(configpath == NULL, "configpath != NULL on creation");
}
END_TEST

START_TEST (user_null_configdir2)
{
  _osync_user_set_confdir(NULL, NULL);
}
END_TEST

START_TEST (user_null_configdir3)
{
  OSyncUserInfo *osuser = _osync_user_new();
  _osync_user_set_confdir(osuser, NULL);
  _osync_user_set_confdir(osuser, "test");
}
END_TEST

Suite *env_suite(void)
{
  Suite *s = suite_create("User");
  TCase *tc_core = tcase_create("Core");

  suite_add_tcase (s, tc_core);
  tcase_add_test(tc_core, user_create);
  tcase_add_test(tc_core, user_confdir);
  tcase_add_test(tc_core, user_null_configdir);
  tcase_add_test(tc_core, user_null_configdir2);
  tcase_add_test(tc_core, user_null_configdir3);
  return s;
}

int main(void)
{
	int nf;
	
	Suite *s = env_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

//	srunner_set_fork_status (sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
