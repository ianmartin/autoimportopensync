#include <check.h>
#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>

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

START_TEST (error_get_name_null)
{
	fail_unless(osync_error_get_name(NULL) == NULL, NULL);

}
END_TEST

START_TEST (error_get_name_null2)
{
	OSyncError *error = NULL;
	fail_unless(osync_error_get_name(&error) != NULL, NULL);

}
END_TEST

START_TEST (error_get_name)
{
	OSyncError *error = NULL;
	osync_error_set(&error, OSYNC_ERROR_GENERIC, "test");
	fail_unless(osync_error_get_name(&error) != NULL, NULL);
}
END_TEST

START_TEST (error_free_null)
{
	osync_error_free(NULL);

}
END_TEST

START_TEST (error_free_null2)
{
	OSyncError *error = NULL;
	osync_error_free(&error);

}
END_TEST

START_TEST (error_free)
{
	OSyncError *error = NULL;
	osync_error_set(&error, OSYNC_ERROR_GENERIC, "test");
	fail_unless(error != NULL, NULL);
	osync_error_free(&error);
	fail_unless(error == NULL, NULL);
}
END_TEST

START_TEST (error_check_null)
{
	fail_unless(osync_error_is_set(NULL) == FALSE, NULL);

}
END_TEST

START_TEST (error_check_null2)
{
	OSyncError *error = NULL;
	fail_unless(osync_error_is_set(&error) == FALSE, NULL);

}
END_TEST

START_TEST (error_check)
{
	OSyncError *error = NULL;
	osync_error_set(&error, OSYNC_ERROR_GENERIC, "test");
	fail_unless(osync_error_is_set(&error) == TRUE, NULL);
}
END_TEST

START_TEST (error_check2)
{
	OSyncError *error = NULL;
	osync_error_set(&error, OSYNC_NO_ERROR, NULL);
	fail_unless(osync_error_is_set(&error) == FALSE, NULL);
}
END_TEST

START_TEST (error_update_null)
{
	osync_error_update(NULL, NULL);

}
END_TEST

START_TEST (error_update_null2)
{
	OSyncError *error = NULL;
	osync_error_update(&error, NULL);
}
END_TEST

START_TEST (error_update)
{
	OSyncError *error = NULL;
	osync_error_set(&error, OSYNC_ERROR_GENERIC, "test");
	osync_error_update(&error, "test2%i", 1);
	fail_unless(!strcmp(error->message, "test21"), NULL);
}
END_TEST

START_TEST (error_update2)
{
	OSyncError *error = NULL;
	osync_error_set(&error, OSYNC_ERROR_GENERIC, "test");
	osync_error_update(&error, "test2%s", error->message);
	fail_unless(!strcmp(error->message, "test2test"), NULL);
}
END_TEST

START_TEST (error_set_null)
{
	osync_error_set(NULL, OSYNC_NO_ERROR, NULL);

}
END_TEST

START_TEST (error_set_null2)
{
	OSyncError *error = NULL;
	osync_error_update(&error, OSYNC_NO_ERROR, NULL);
}
END_TEST

START_TEST (error_duplicate_null)
{
	OSyncError *error = NULL;
	osync_error_set(&error, OSYNC_ERROR_GENERIC, "asd");
	osync_error_duplicate(NULL, &error);
}
END_TEST

Suite *error_suite(void)
{
	Suite *s = suite_create("Error");
	TCase *tc_error = tcase_create("Core");

	suite_add_tcase (s, tc_error);
	tcase_add_test(tc_error, error_create);
	tcase_add_test(tc_error, error_create_null);
	tcase_add_test(tc_error, error_get_name_null);
	tcase_add_test(tc_error, error_get_name_null2);
	tcase_add_test(tc_error, error_get_name);
	tcase_add_test(tc_error, error_free_null);
	tcase_add_test(tc_error, error_free_null2);
	tcase_add_test(tc_error, error_free);
	tcase_add_test(tc_error, error_check_null);
	tcase_add_test(tc_error, error_check_null2);
	tcase_add_test(tc_error, error_check);
	tcase_add_test(tc_error, error_check2);
	tcase_add_test(tc_error, error_update_null);
	tcase_add_test(tc_error, error_update_null2);
	tcase_add_test(tc_error, error_update);
	tcase_add_test(tc_error, error_update2);
	tcase_add_test(tc_error, error_set_null);
	tcase_add_test(tc_error, error_set_null2);
	tcase_add_test(tc_error, error_duplicate_null);
	
	return s;
}

int main(void)
{
	int nf;
	
	Suite *s = error_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

//	srunner_set_fork_status (sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
