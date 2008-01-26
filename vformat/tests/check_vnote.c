#include "support.h"
#include "conversion.h"
#include <time.h>

static void conv_vnote(const char *filename)
{
	conv("note", filename, NULL);
}

static void compare_vnote(const char *lfilename, const char *rfilename, OSyncConvCmpResult result)
{
	compare("note", lfilename, rfilename, result);
}

static time_t vnote_get_revision(const char *filename)
{
	return get_revision("note", filename, NULL);
}

START_TEST (conv_vnote1)
{
	conv_vnote("/vnotes/vnote1.vnt");
}
END_TEST

START_TEST (conv_vnote2)
{
	conv_vnote("/vnotes/vnote2.vnt");
}
END_TEST

START_TEST (conv_vnote3)
{
	conv_vnote("/vnotes/vnote3.vnt");
}
END_TEST

START_TEST (conv_vnote_minimal)
{
	conv_vnote("/vnotes/vnote-minimal.vnt");
}
END_TEST

START_TEST (get_revision1)
{
	struct tm testtm = {0, 0, 0, 6, 4 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vnote_get_revision("/vnotes/vnote1.vnt") == (mktime(&testtm) - timezone), NULL);
}
END_TEST

START_TEST (get_revision2)
{
	struct tm testtm = {1, 1, 1, 6, 4 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vnote_get_revision("/vnotes/vnote2.vnt") == (mktime(&testtm) - timezone), NULL);
}
END_TEST

START_TEST (get_revision3)
{
	struct tm testtm = {0, 0, 0, 6, 4 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vnote_get_revision("/vnotes/vnote3.vnt") == (mktime(&testtm) - timezone), NULL);
}
END_TEST

START_TEST (get_revision4)
{
	fail_unless(vnote_get_revision("/vnotes/vnote-minimal.vnt") == -1, NULL);
}
END_TEST

START_TEST (compare_vnote_same1)
{
	compare_vnote("/vnotes/vnote1.vnt", "/vnotes/vnote1.vnt", OSYNC_CONV_DATA_SAME);
}
END_TEST

START_TEST (compare_vnote_same2)
{
	compare_vnote("/vnotes/vnote1.vnt", "/vnotes/vnote1-same.vnt", OSYNC_CONV_DATA_SAME);
}
END_TEST

START_TEST (compare_vnote_similar1)
{
	compare_vnote("/vnotes/vnote1.vnt", "/vnotes/vnote1-similar.vnt", OSYNC_CONV_DATA_SIMILAR);
}
END_TEST

START_TEST (compare_vnote_mismatch1)
{
	compare_vnote("/vnotes/vnote1.vnt", "/vnotes/vnote2.vnt", OSYNC_CONV_DATA_MISMATCH);
}
END_TEST

START_TEST (compare_vnote_mismatch2)
{
	compare_vnote("/vnotes/vnote1.vnt", "/vnotes/vnote-minimal.vnt", OSYNC_CONV_DATA_MISMATCH);
}
END_TEST

Suite *vnote_suite(void)
{
	Suite *s = suite_create("VNote");
	//Suite *s2 = suite_create("VNote");
	
	create_case(s, "conv_vnote1", conv_vnote1);
	create_case(s, "conv_vnote2", conv_vnote2);
	create_case(s, "conv_vnote3", conv_vnote3);
	create_case(s, "conv_vnote_minimal", conv_vnote_minimal);
	
	create_case(s, "get_revision1", get_revision1);
	create_case(s, "get_revision2", get_revision2);
	create_case(s, "get_revision3", get_revision3);
	create_case(s, "get_revision4", get_revision4);
	
	create_case(s, "compare_vnote_same1", compare_vnote_same1);
	create_case(s, "compare_vnote_same2", compare_vnote_same2);
	create_case(s, "compare_vnote_similar1", compare_vnote_similar1);
	create_case(s, "compare_vnote_mismatch1", compare_vnote_mismatch1);
	create_case(s, "compare_vnote_mismatch2", compare_vnote_mismatch2);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = vnote_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
