#include "support.h"
#include <time.h>

static void conv_vcard(const char *filename, const char *extension)
{
	char *command = g_strdup_printf("cp %s/"OPENSYNC_TESTDATA"%s .", g_get_current_dir(), filename);
	char *testbed = setup_testbed(NULL);
	system(command);
	g_free(command);
	
	
	OSyncError *error = NULL;
	OSyncEnv *env = init_env();
	
	OSyncFormatEnv *conv_env = osync_conv_env_new(env);
	fail_unless(conv_env != NULL, NULL);

	char *buffer;
	int size;
	
	char *file = g_path_get_basename(filename);
	fail_unless(osync_file_read(file, &buffer, &size, &error), NULL);
	
	OSyncChange *change = osync_change_new();
	osync_change_set_uid(change, file);
	g_free(file);
	osync_change_set_data(change, buffer, size + 1, TRUE);
	osync_change_set_conv_env(change, conv_env);
	
	osync_change_set_objformat_string(change, "plain");

	OSyncObjFormat *sourceformat = osync_change_detect_objformat(conv_env, change, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_change_set_objformat(change, sourceformat);
	osync_change_set_objtype(change, osync_objformat_get_objtype(sourceformat));
	
	OSyncObjFormat *targetformat = NULL;
	if (!strcmp(osync_objformat_get_name(sourceformat), "vcard21"))
		targetformat = osync_conv_find_objformat(conv_env, "vcard30");
	
	if (!strcmp(osync_objformat_get_name(sourceformat), "vcard30"))
		targetformat = osync_conv_find_objformat(conv_env, "vcard21");

	fail_unless(targetformat != NULL, NULL);
	
	OSyncChange *newchange = osync_change_copy(change, &error);
	fail_unless(newchange != NULL, NULL);
	
	//Convert to
	fail_unless(osync_change_convert_extension(conv_env, change, targetformat, extension, &error), NULL);
	
	//Detect the output
	osync_change_set_objformat_string(change, "plain");
	fail_unless(osync_change_detect_objformat(conv_env, change, &error) == targetformat, NULL);
	
	//Compare old to new
	fail_unless(osync_change_compare(newchange, change) == CONV_DATA_SAME, NULL);
	
	//Convert back
	fail_unless(osync_change_convert_extension(conv_env, change, sourceformat, extension, &error), NULL);
	
	//Detect the output again
	osync_change_set_objformat_string(change, "plain");
	fail_unless(osync_change_detect_objformat(conv_env, change, &error) == sourceformat, NULL);
	
	//Compare again
	fail_unless(osync_change_compare(newchange, change) == CONV_DATA_SAME, NULL);

	osync_conv_env_free(conv_env);
	osync_env_finalize(env, NULL);
	osync_env_free(env);
	
	destroy_testbed(testbed);
}

static void compare_vcard(const char *lfilename, const char *rfilename, OSyncConvCmpResult result)
{
	char *command1 = g_strdup_printf("cp %s/"OPENSYNC_TESTDATA"%s lfile", g_get_current_dir(), lfilename);
	char *command2 = g_strdup_printf("cp %s/"OPENSYNC_TESTDATA"%s rfile", g_get_current_dir(), rfilename);
	char *testbed = setup_testbed(NULL);
	system(command1);
	g_free(command1);
	system(command2);
	g_free(command2);
	
	OSyncError *error = NULL;
	OSyncEnv *env = init_env();
	
	OSyncFormatEnv *conv_env = osync_conv_env_new(env);
	fail_unless(conv_env != NULL, NULL);

	char *buffer;
	int size;
	
	fail_unless(osync_file_read("lfile", &buffer, &size, &error), NULL);
	
	OSyncChange *lchange = osync_change_new();
	osync_change_set_uid(lchange, "lfile");
	osync_change_set_data(lchange, buffer, size + 1, TRUE);
	osync_change_set_conv_env(lchange, conv_env);
	osync_change_set_objformat_string(lchange, "plain");

	OSyncObjFormat *sourceformat = osync_change_detect_objformat(conv_env, lchange, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_change_set_objformat(lchange, sourceformat);
	osync_change_set_objtype(lchange, osync_objformat_get_objtype(sourceformat));
	
	fail_unless(osync_file_read("rfile", &buffer, &size, &error), NULL);
	
	OSyncChange *rchange = osync_change_new();
	osync_change_set_uid(rchange, "rfile");
	osync_change_set_data(rchange, buffer, size + 1, TRUE);
	osync_change_set_conv_env(rchange, conv_env);
	osync_change_set_objformat_string(rchange, "plain");

	sourceformat = osync_change_detect_objformat(conv_env, rchange, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_change_set_objformat(rchange, sourceformat);
	osync_change_set_objtype(rchange, osync_objformat_get_objtype(sourceformat));
	
	fail_unless(osync_change_compare(lchange, rchange) == result, NULL);
	
	osync_conv_env_free(conv_env);
	osync_env_finalize(env, NULL);
	osync_env_free(env);
	destroy_testbed(testbed);
}

static time_t vcard_get_revision(const char *filename)
{
	char *command = g_strdup_printf("cp %s/"OPENSYNC_TESTDATA"%s .", g_get_current_dir(), filename);
	char *testbed = setup_testbed(NULL);
	system(command);
	g_free(command);
	
	
	OSyncError *error = NULL;
	OSyncEnv *env = init_env();
	
	OSyncFormatEnv *conv_env = osync_conv_env_new(env);
	fail_unless(conv_env != NULL, NULL);

	char *buffer;
	int size;
	
	char *file = g_path_get_basename(filename);
	fail_unless(osync_file_read(file, &buffer, &size, &error), NULL);
	
	OSyncChange *change = osync_change_new();
	osync_change_set_uid(change, file);
	g_free(file);
	osync_change_set_data(change, buffer, size + 1, TRUE);
	osync_change_set_conv_env(change, conv_env);
	
	osync_change_set_objformat_string(change, "plain");

	OSyncObjFormat *sourceformat = osync_change_detect_objformat(conv_env, change, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_change_set_objformat(change, sourceformat);
	
	OSyncObjFormat *targetformat = osync_conv_find_objformat(conv_env, "xml-contact");
	fail_unless(targetformat != NULL, NULL);
	
	fail_unless(osync_change_convert_extension(conv_env, change, targetformat, "evolution", &error), NULL);
	
	time_t time = osync_change_get_revision(change, &error);
	
	osync_conv_env_free(conv_env);
	osync_env_finalize(env, NULL);
	osync_env_free(env);
	
	destroy_testbed(testbed);
	return time;
}

START_TEST (conv_vcard_evolution2_full1)
{
	conv_vcard("data/vcards/evolution2/evo2-full1.vcf", "evolution");
}
END_TEST

START_TEST (conv_vcard_evolution2_full2)
{
	conv_vcard("data/vcards/evolution2/evo2-full2.vcf", "evolution");
}
END_TEST

START_TEST (conv_vcard_evolution2_photo)
{
	conv_vcard("data/vcards/evolution2/evo2-photo.vcf", "evolution");
}
END_TEST

START_TEST (conv_vcard_evolution2_multiline)
{
	conv_vcard("data/vcards/evolution2/evo2-multiline.vcf", "evolution");
}
END_TEST

START_TEST (conv_vcard_evolution2_umlaute)
{
	conv_vcard("data/vcards/evolution2/evo2-umlaute.vcf", "evolution");
}
END_TEST

START_TEST (conv_vcard_evolution2_special)
{
	conv_vcard("data/vcards/evolution2/evo2-special.vcf", "evolution");
}
END_TEST

START_TEST (conv_vcard_kde_21_full1)
{
	conv_vcard("data/vcards/kdepim/kdepim-full1-2.1.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_30_full1)
{
	conv_vcard("data/vcards/kdepim/kdepim-full1-3.0.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_21_full2)
{
	conv_vcard("data/vcards/kdepim/kdepim-full2-2.1.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_30_full2)
{
	conv_vcard("data/vcards/kdepim/kdepim-full2-3.0.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_21_multiline)
{
	conv_vcard("data/vcards/kdepim/kdepim-multiline-2.1.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_30_multiline)
{
	conv_vcard("data/vcards/kdepim/kdepim-multiline-3.0.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_21_photo1)
{
	conv_vcard("data/vcards/kdepim/kdepim-photo1-2.1.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_30_photo1)
{
	conv_vcard("data/vcards/kdepim/kdepim-photo1-3.0.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_21_photo2)
{
	conv_vcard("data/vcards/kdepim/kdepim-photo2-2.1.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_30_photo2)
{
	conv_vcard("data/vcards/kdepim/kdepim-photo2-3.0.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_21_sound1)
{
	conv_vcard("data/vcards/kdepim/kdepim-sound1-2.1.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_30_sound1)
{
	conv_vcard("data/vcards/kdepim/kdepim-sound1-3.0.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_21_sound2)
{
	conv_vcard("data/vcards/kdepim/kdepim-sound2-2.1.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_30_sound2)
{
	conv_vcard("data/vcards/kdepim/kdepim-sound2-3.0.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_21_special)
{
	conv_vcard("data/vcards/kdepim/kdepim-special-2.1.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_30_special)
{
	conv_vcard("data/vcards/kdepim/kdepim-special-3.0.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_21_umlaute)
{
	conv_vcard("data/vcards/kdepim/kdepim-umlaute-2.1.vcf", "kde");
}
END_TEST

START_TEST (conv_vcard_kde_30_umlaute)
{
	conv_vcard("data/vcards/kdepim/kdepim-umlaute-3.0.vcf", "kde");
}
END_TEST

START_TEST (compare_vformat_mismatch1)
{
	compare_vcard("data/vcards/evolution2/compare/1-different.vcf", "data/vcards/kdepim/compare/1-different.vcf", CONV_DATA_MISMATCH);
}
END_TEST

START_TEST (compare_vformat_similar1)
{
	compare_vcard("data/vcards/evolution2/compare/1-conflict.vcf", "data/vcards/kdepim/compare/1-conflict.vcf", CONV_DATA_SIMILAR);
}
END_TEST

START_TEST (compare_vformat_mismatch2)
{
	compare_vcard("data/vcards/evolution2/compare/2-conflict.vcf", "data/vcards/kdepim/compare/2-conflict.vcf", CONV_DATA_MISMATCH);
}
END_TEST

START_TEST (compare_vformat_similar2)
{
	compare_vcard("data/vcards/evolution2/compare/2-different.vcf", "data/vcards/kdepim/compare/2-different.vcf", CONV_DATA_SIMILAR);
}
END_TEST

START_TEST (compare_vformat_same1)
{
	compare_vcard("data/vcards/evolution2/compare/1-same.vcf", "data/vcards/kdepim/compare/1-same.vcf", CONV_DATA_SAME);
}
END_TEST

START_TEST (compare_vformat_same2)
{
	compare_vcard("data/vcards/evolution2/compare/2-same.vcf", "data/vcards/kdepim/compare/2-same.vcf", CONV_DATA_SAME);
}
END_TEST

START_TEST (get_revision1)
{
	struct tm testtm = {24, 41, 10, 26, 2 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcard_get_revision("data/vcards/evolution2/evo2-full1.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (get_revision2)
{
	struct tm testtm = {0, 0, 0, 26, 2 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcard_get_revision("data/vcards/evolution2/evo2-full2.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (get_revision3)
{
	struct tm testtm = {0, 0, 0, 26, 2 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcard_get_revision("data/vcards/evolution2/evo2-multiline.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (get_revision4)
{
	struct tm testtm = {24, 41, 10, 26, 2 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcard_get_revision("data/vcards/evolution2/evo2-photo.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (get_no_revision)
{
	fail_unless(vcard_get_revision("data/vcards/evolution2/compare/1-same.vcf") == -1, NULL);
}
END_TEST

Suite *vcard_suite(void)
{
	Suite *s = suite_create("Vcard");
	//Suite *s2 = suite_create("Vcard");
	
	create_case(s, "conv_vcard_evolution2_full1", conv_vcard_evolution2_full1);
	create_case(s, "conv_vcard_evolution2_full2", conv_vcard_evolution2_full2);
	create_case(s, "conv_vcard_evolution2_photo", conv_vcard_evolution2_photo);
	create_case(s, "conv_vcard_evolution2_multiline", conv_vcard_evolution2_multiline);
	create_case(s, "conv_vcard_evolution2_umlaute", conv_vcard_evolution2_umlaute);
	create_case(s, "conv_vcard_evolution2_special", conv_vcard_evolution2_special);
	
	create_case(s, "conv_vcard_kde_21_full1", conv_vcard_kde_21_full1);
	create_case(s, "conv_vcard_kde_30_full1", conv_vcard_kde_30_full1);
	create_case(s, "conv_vcard_kde_21_full2", conv_vcard_kde_21_full2);
	create_case(s, "conv_vcard_kde_30_full2", conv_vcard_kde_30_full2);
	create_case(s, "conv_vcard_kde_21_multiline", conv_vcard_kde_21_multiline);
	create_case(s, "conv_vcard_kde_30_multiline", conv_vcard_kde_30_multiline);
	create_case(s, "conv_vcard_kde_21_photo1", conv_vcard_kde_21_photo1);
	create_case(s, "conv_vcard_kde_30_photo1", conv_vcard_kde_30_photo1);
	create_case(s, "conv_vcard_kde_21_photo2", conv_vcard_kde_21_photo2);
	create_case(s, "conv_vcard_kde_30_photo2", conv_vcard_kde_30_photo2);
	create_case(s, "conv_vcard_kde_21_sound1", conv_vcard_kde_21_sound1);
	create_case(s, "conv_vcard_kde_30_sound1", conv_vcard_kde_30_sound1);
	create_case(s, "conv_vcard_kde_21_sound2", conv_vcard_kde_21_sound2);
	create_case(s, "conv_vcard_kde_30_sound2", conv_vcard_kde_30_sound2);
	create_case(s, "conv_vcard_kde_21_special", conv_vcard_kde_21_special);
	create_case(s, "conv_vcard_kde_30_special", conv_vcard_kde_30_special);
	create_case(s, "conv_vcard_kde_21_umlaute", conv_vcard_kde_21_umlaute);
	create_case(s, "conv_vcard_kde_30_umlaute", conv_vcard_kde_30_umlaute);

	create_case(s, "compare_vformat_mismatch1", compare_vformat_mismatch1);
	create_case(s, "compare_vformat_mismatch2", compare_vformat_mismatch2);
	create_case(s, "compare_vformat_similar1", compare_vformat_similar1);
	create_case(s, "compare_vformat_similar2", compare_vformat_similar2);
	create_case(s, "compare_vformat_same1", compare_vformat_same1);
	create_case(s, "compare_vformat_same2", compare_vformat_same2);
	
	create_case(s, "get_revision1", get_revision1);
	create_case(s, "get_revision2", get_revision2);
	create_case(s, "get_revision3", get_revision3);
	create_case(s, "get_revision4", get_revision4);
	create_case(s, "get_no_revision", get_no_revision);

	return s;
}

int main(void)
{
	int nf;

	Suite *s = vcard_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_set_fork_status (sr, CK_NOFORK);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
