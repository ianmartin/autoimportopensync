#include <check.h>
#include <opensync/opensync.h>
#include <opensync/opensync-group.h>
#include <opensync/opensync_internals.h>
#include <stdlib.h>

#include <glib.h>
#include <gmodule.h>

START_TEST (test_create)
{
  OSyncMember *member = NULL;
  member = osync_member_new(NULL);
  fail_unless(member != NULL, "Member == NULL on creation");
  osync_member_unref(member);
}
END_TEST

Suite *member_suite(void)
{
  Suite *s = suite_create("Member");
  TCase *tc_core = tcase_create("Core");

  suite_add_tcase (s, tc_core);
  tcase_add_test(tc_core, test_create);

  return s;
}

int main(void)
{
	int nf;
	
	Suite *s = member_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
