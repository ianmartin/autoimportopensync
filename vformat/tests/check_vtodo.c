#include "conversion.h"

static void conv_vtodo(const char *filename, const char *extension)
{
	conv("todo", filename, extension);
}

/* unused
static void compare_vtodo(const char *lfilename, const char *rfilename, OSyncConvCmpResult result)
{
	compare("todo", lfilename, rfilename, result);
}
*/

static time_t vtodo_get_revision(const char *filename, const char *extension)
{
	return get_revision("todo", filename, extension);
}

START_TEST (conv_vtodo_evolution2_simple)
{
	conv_vtodo("/vtodos/evolution2/todo-simple.vcf", "VTODO_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full1)
{
	conv_vtodo("/vtodos/evolution2/todo-full1.vcf", "VTODO_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full2)
{
	conv_vtodo("/vtodos/evolution2/todo-full2.vcf", "VTODO_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full3)
{
	conv_vtodo("/vtodos/evolution2/todo-full3.vcf", "VTODO_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full4)
{
	conv_vtodo("/vtodos/evolution2/todo-full4.vcf", "VTODO_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full5)
{
	conv_vtodo("/vtodos/evolution2/todo-full5.vcf", "VTODO_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full6)
{
	conv_vtodo("/vtodos/evolution2/todo-full6.vcf", "VTODO_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full7)
{
	conv_vtodo("/vtodos/evolution2/todo-full7.vcf", "VTODO_EXTENSION=Evolution");
}
END_TEST

START_TEST (todo_get_revision1)
{
	struct tm testtm = {50, 56, 0, 6, 3 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vtodo_get_revision("/vtodos/evolution2/todo-full1.vcf", "VTODO_EXTENSION=Evolution") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (todo_get_revision2)
{
	struct tm testtm = {50, 56, 0, 6, 3 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vtodo_get_revision("/vtodos/evolution2/todo-full2.vcf", "VTODO_EXTENSION=Evolution") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (todo_get_revision3)
{
	struct tm testtm = {0, 0, 0, 6, 3 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vtodo_get_revision("/vtodos/evolution2/todo-full3.vcf", "VTODO_EXTENSION=Evolution") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (todo_no_revision)
{
	fail_unless(vtodo_get_revision("/vtodos/kdepim/todo-full1.vcs", "VTODO_EXTENSION=Evolution") == -1, NULL);
}
END_TEST

Suite *vtodo_suite(void)
{
	Suite *s = suite_create("VTodo");

	create_case(s, "conv_vtodo_evolution2_simple", conv_vtodo_evolution2_simple);
	create_case(s, "conv_vtodo_evolution2_full1", conv_vtodo_evolution2_full1);
	create_case(s, "conv_vtodo_evolution2_full2", conv_vtodo_evolution2_full2);
	create_case(s, "conv_vtodo_evolution2_full3", conv_vtodo_evolution2_full3);
	create_case(s, "conv_vtodo_evolution2_full4", conv_vtodo_evolution2_full4);
	create_case(s, "conv_vtodo_evolution2_full5", conv_vtodo_evolution2_full5);
	create_case(s, "conv_vtodo_evolution2_full6", conv_vtodo_evolution2_full6);
	create_case(s, "conv_vtodo_evolution2_full7", conv_vtodo_evolution2_full7);
	
	create_case(s, "todo_get_revision1", todo_get_revision1);
	create_case(s, "todo_get_revision2", todo_get_revision2);
	create_case(s, "todo_get_revision3", todo_get_revision3);
	create_case(s, "todo_no_revision", todo_no_revision);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = vtodo_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
