#include <check.h>
#include "opensync.h"
#include "opensync_internals.h"

START_TEST (user_create)
{
  OSyncUserInfo *osuser = _osync_user_new();
  fail_unless(osuser != NULL, "osuser == NULL on creation");
  fail_unless(_osync_user_get_confdir(osuser) != NULL, "confdir == NULL on creation");
}
END_TEST

START_TEST (user_confdir)
{
  OSyncUserInfo *osuser = _osync_user_new();
  fail_unless(_osync_user_get_confdir(osuser) != NULL, "configdir == NULL on creation");
  _osync_user_set_confdir(osuser, "test");
  if (g_ascii_strcasecmp (_osync_user_get_confdir(osuser), "test") != 0)
  	fail("configpath == \"test\"");
}
END_TEST

Suite *env_suite(void)
{
  Suite *s = suite_create("User");
  TCase *tc_core = tcase_create("Core");

  suite_add_tcase (s, tc_core);
  tcase_add_test(tc_core, user_create);
  tcase_add_test(tc_core, user_confdir);
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
