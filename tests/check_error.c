#include <check.h>
#include "opensync.h"
#include "opensync_internals.h"

START_TEST (error_create)
{
  OSyncError *error = NULL;
  osync_error_set(&error, OSYNC_ERROR_GENERIC, "test%i", 1);
  fail_unless(error != NULL, NULL);
  fail_unless(error->type == OSYNC_ERROR_GENERIC, NULL);
  fail_unless(!strcmp(error->message, "test1"), NULL);
  fail_unless(osync_error_is_set(&error), NULL);
  
  osync_error_free(&error);
  fail_unless(error == NULL, NULL);
}
END_TEST

START_TEST (error_create_null)
{
  osync_error_set(NULL, OSYNC_ERROR_GENERIC, "test%i", 1);
}
END_TEST

Suite *error_suite(void)
{
  Suite *s = suite_create("Error");
  TCase *tc_error = tcase_create("Core");

  suite_add_tcase (s, tc_error);
  tcase_add_test(tc_error, error_create);
  tcase_add_test(tc_error, error_create_null);
  return s;
}

int main(void)
{
	int nf;
	
	Suite *s = error_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
