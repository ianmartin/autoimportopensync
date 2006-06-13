#include "support.h"
#include <opensync/opensync-error.h>

START_TEST (error_create)
{
	OSyncError *error = NULL;
	osync_error_set(&error, OSYNC_ERROR_GENERIC, "test%i", 1);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_get_type(&error) == OSYNC_ERROR_GENERIC, NULL);
	fail_unless(!strcmp(osync_error_print(&error), "test1"), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
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
	osync_error_unref(&error);
}
END_TEST

START_TEST (error_free_null)
{
	osync_error_unref(NULL);

}
END_TEST

START_TEST (error_free_null2)
{
	OSyncError *error = NULL;
	osync_error_unref(&error);

}
END_TEST

START_TEST (error_free)
{
	OSyncError *error = NULL;
	osync_error_set(&error, OSYNC_ERROR_GENERIC, "test");
	fail_unless(error != NULL, NULL);
	osync_error_unref(&error);
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
	osync_error_unref(&error);
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
	fail_unless(!strcmp(osync_error_print(&error), "test21"), NULL);
	osync_error_unref(&error);
}
END_TEST

START_TEST (error_update2)
{
	OSyncError *error = NULL;
	osync_error_set(&error, OSYNC_ERROR_GENERIC, "test");
	osync_error_update(&error, "test2%s", osync_error_print(&error));
	fail_unless(!strcmp(osync_error_print(&error), "test2test"), NULL);
	osync_error_unref(&error);
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
	osync_error_unref(&error);
}
END_TEST

START_TEST (error_duplicate_null)
{
	OSyncError *error = NULL;
	osync_error_set(&error, OSYNC_ERROR_GENERIC, "asd");
	osync_error_duplicate(NULL, &error);
	osync_error_unref(&error);
}
END_TEST

Suite *error_suite(void)
{
	Suite *s = suite_create("Error");
	//Suite *s2 = suite_create("Error");

	create_case(s, "error_create", error_create);
	create_case(s, "error_create_null", error_create_null);
	create_case(s, "error_get_name_null", error_get_name_null);
	create_case(s, "error_get_name_null2", error_get_name_null2);
	create_case(s, "error_get_name", error_get_name);
	create_case(s, "error_free_null", error_free_null);
	create_case(s, "error_free_null2", error_free_null2);
	create_case(s, "error_free", error_free);
	create_case(s, "error_check_null", error_check_null);
	create_case(s, "error_check_null2", error_check_null2);
	create_case(s, "error_check", error_check);
	create_case(s, "error_check2", error_check2);
	create_case(s, "error_update_null", error_update_null);
	create_case(s, "error_update_null2", error_update_null2);
	create_case(s, "error_update", error_update);
	create_case(s, "error_update2", error_update2);
	create_case(s, "error_set_null", error_set_null);
	create_case(s, "error_set_null2", error_set_null2);
	create_case(s, "error_duplicate_null", error_duplicate_null);
	
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
