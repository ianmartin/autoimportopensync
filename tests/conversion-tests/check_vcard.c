#include "support.h"

#include <opensync/opensync-format.h>
#include <opensync/opensync-support.h>
#include <opensync/opensync-data.h>
#include <time.h>

static void conv_vcard(const char *filename, const char *extension)
{
	char *command = g_strdup_printf("cp "OPENSYNC_TESTDATA"%s .", filename);
	char *testbed = setup_testbed(NULL);
	system(command);
	g_free(command);
	
	char *buffer;
	unsigned int size;
	
	OSyncData *data = NULL;
	OSyncError *error = NULL;

	OSyncFormatEnv *format_env = osync_format_env_new(&error);
	fail_unless(format_env != NULL, NULL);

	fail_unless(osync_format_env_load_plugins(format_env, NULL, &error), NULL);

	char *file = g_path_get_basename(filename);
	fail_unless(osync_file_read(file, &buffer, &size, &error), NULL);
	
	OSyncChange *change = osync_change_new(&error);
	osync_change_set_uid(change, file);
	g_free(file);

	OSyncObjFormat *sourceformat = osync_objformat_new("plain", "data", &error);

	data = osync_data_new(buffer, size, sourceformat, &error);
	fail_unless(data != NULL, NULL);

	osync_change_set_data(change, data);

	sourceformat = osync_format_env_detect_objformat_full(format_env, data, &error);
	fail_unless(sourceformat != NULL, NULL);

	OSyncObjFormat *targetformat = NULL;
	if (!strcmp(osync_objformat_get_name(sourceformat), "vcard21"))
		targetformat = osync_format_env_find_objformat(format_env, "vcard30");

	if (!strcmp(osync_objformat_get_name(sourceformat), "vcard30"))
		targetformat = osync_format_env_find_objformat(format_env, "vcard21");

	fail_unless(targetformat != NULL, NULL);
	
	// Create new change .. duplicate and give new uid
	OSyncChange *newchange = osync_change_new(&error);
	OSyncData *newdata = osync_data_clone(data, &error);
	osync_data_set_objformat(newdata, osync_format_env_detect_objformat_full(format_env, newdata, &error));

	osync_change_set_data(newchange, newdata);
	char *newuid = g_strdup_printf("%s_original", osync_change_get_uid(change)); 
	osync_change_set_uid(newchange, newuid);
	g_free(newuid);

	fail_unless(newchange != NULL, NULL);

	OSyncFormatConverterPath *path = osync_format_env_find_path(format_env, sourceformat, targetformat, &error);
	fail_unless(path != NULL, NULL);
	osync_converter_path_set_config(path, extension);

	//Convert to
	fail_unless(osync_format_env_convert(format_env, path, data, &error), NULL);

	//Detect the output
	fail_unless(osync_data_get_objformat(data) == targetformat, NULL);
	
	//Compare old to new
//	fail_unless(osync_change_compare(newchange, change) == OSYNC_CONV_DATA_SAME, NULL);
	
	//Convert back
	path = osync_format_env_find_path(format_env, targetformat, sourceformat, &error);

	fail_unless(path != NULL, NULL);
	osync_converter_path_set_config(path, extension);

	fail_unless(osync_format_env_convert(format_env, path, data, &error), NULL);

	
	//Detect the output again
	fail_unless(osync_data_get_objformat(data) == sourceformat, NULL);

	// converter old and new to XMLFormat-contact
	targetformat = osync_format_env_find_objformat(format_env, "xmlformat-contact");

	path = osync_format_env_find_path(format_env, sourceformat, targetformat, &error);
	fail_unless(path != NULL, NULL);
	osync_converter_path_set_config(path, extension);

	fail_unless(osync_format_env_convert(format_env, path, data, &error), NULL);
	fail_unless(osync_format_env_convert(format_env, path, newdata, &error), NULL);

	char *xml1 = osync_data_get_printable(data);
	char *xml2 = osync_data_get_printable(newdata);
	osync_trace(TRACE_INTERNAL, "ConvertedXML:\n%s\nOriginal:\n%s\n", xml1, xml2);
	g_free(xml1);
	g_free(xml2);

	//Compare again
	fail_unless(osync_change_compare(newchange, change) == OSYNC_CONV_DATA_SAME, NULL);

	osync_format_env_free(format_env);
	
	destroy_testbed(testbed);
}

static void compare_vcard(const char *lfilename, const char *rfilename, OSyncConvCmpResult result)
{
	char *command1 = g_strdup_printf("cp "OPENSYNC_TESTDATA"%s lfile", lfilename);
	char *command2 = g_strdup_printf("cp "OPENSYNC_TESTDATA"%s rfile", rfilename);
	char *testbed = setup_testbed(NULL);
	system(command1);
	g_free(command1);
	system(command2);
	g_free(command2);
	
	OSyncError *error = NULL;
	
	OSyncFormatEnv *format_env = osync_format_env_new(&error);
	fail_unless(format_env != NULL, NULL);

	char *buffer;
	unsigned int size;
	
	// left data
	fail_unless(osync_file_read("lfile", &buffer, &size, &error), NULL);
	
	OSyncChange *lchange = osync_change_new(&error);
	osync_change_set_uid(lchange, "lfile");

	OSyncObjFormat *sourceformat = osync_objformat_new("plain", "data", &error);

	OSyncData *ldata = osync_data_new(buffer, size, sourceformat, &error);
	fail_unless(ldata != NULL, NULL);

	osync_change_set_data(lchange, ldata);


	sourceformat = osync_format_env_detect_objformat_full(format_env, ldata, &error);
	fail_unless(sourceformat != NULL, NULL);

	// right data
	fail_unless(osync_file_read("rfile", &buffer, &size, &error), NULL);
	
	OSyncChange *rchange = osync_change_new(&error);
	osync_change_set_uid(rchange, "rfile");

	OSyncData *rdata = osync_data_new(buffer, size, sourceformat, &error);
	fail_unless(rdata != NULL, NULL);

	osync_change_set_data(rchange, rdata);


	sourceformat = osync_format_env_detect_objformat_full(format_env, rdata, &error);
	fail_unless(sourceformat != NULL, NULL);

	
	// compare
	fail_unless(osync_change_compare(lchange, rchange) == result, NULL);
	
	osync_format_env_free(format_env);
	destroy_testbed(testbed);
}

static time_t vcard_get_revision(const char *filename)
{
	char *command = g_strdup_printf("cp "OPENSYNC_TESTDATA"%s .", filename);
	char *testbed = setup_testbed(NULL);
	system(command);
	g_free(command);
	
	
	OSyncError *error = NULL;
	
	OSyncFormatEnv *format_env = osync_format_env_new(&error);
	fail_unless(format_env != NULL, NULL);

	fail_unless(osync_format_env_load_plugins(format_env, NULL, &error), NULL);

	char *buffer;
	unsigned int size;
	
	char *file = g_path_get_basename(filename);
	fail_unless(osync_file_read(file, &buffer, &size, &error), NULL);
	
	OSyncChange *change = osync_change_new(&error);
	osync_change_set_uid(change, file);
	g_free(file);


	// sourceformat
	OSyncObjFormat *sourceformat = osync_objformat_new("plain", "data", &error);

	OSyncData *data = osync_data_new(buffer, size, sourceformat, &error);
	fail_unless(data != NULL, NULL);

	osync_change_set_data(change, data);

	sourceformat = osync_format_env_detect_objformat_full(format_env, data, &error);
	fail_unless(sourceformat != NULL, NULL);

	// targetformat
	OSyncObjFormat *targetformat = osync_format_env_find_objformat(format_env, "xmlformat-contact");

	fail_unless(targetformat != NULL, NULL);
	

	// Find converter
	OSyncFormatConverter *conv_env = osync_format_env_find_converter(format_env, targetformat, sourceformat);
	fail_unless(conv_env != NULL, NULL);

	// convert
	fail_unless (osync_converter_invoke(conv_env, data, "VCARD_EXTENSION=Evolution", &error), NULL);

	
	time_t time = osync_data_get_revision(data, &error);
	
	osync_format_env_free(format_env);
	
	destroy_testbed(testbed);
	return time;
}

START_TEST (conv_vcard_evolution2_full1)
{
	conv_vcard("/vcards/evolution2/evo2-full1.vcf", "VCARD_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vcard_evolution2_full2)
{
	conv_vcard("/vcards/evolution2/evo2-full2.vcf", "VCARD_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vcard_evolution2_photo)
{
	conv_vcard("/vcards/evolution2/evo2-photo.vcf", "VCARD_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vcard_evolution2_multiline)
{
	conv_vcard("/vcards/evolution2/evo2-multiline.vcf", "VCARD_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vcard_evolution2_umlaute)
{
	conv_vcard("/vcards/evolution2/evo2-umlaute.vcf", "VCARD_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vcard_evolution2_special)
{
	conv_vcard("/vcards/evolution2/evo2-special.vcf", "VCARD_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vcard_kde_21_full1)
{
	conv_vcard("/vcards/kdepim/kdepim-full1-2.1.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_30_full1)
{
	conv_vcard("/vcards/kdepim/kdepim-full1-3.0.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_21_nonuid)
{
	conv_vcard("/vcards/kdepim/kdepim-nonuid-2.1.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_21_full2)
{
	conv_vcard("/vcards/kdepim/kdepim-full2-2.1.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_30_full2)
{
	conv_vcard("/vcards/kdepim/kdepim-full2-3.0.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_21_multiline)
{
	conv_vcard("/vcards/kdepim/kdepim-multiline-2.1.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_30_multiline)
{
	conv_vcard("/vcards/kdepim/kdepim-multiline-3.0.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_21_photo1)
{
	conv_vcard("/vcards/kdepim/kdepim-photo1-2.1.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_30_photo1)
{
	conv_vcard("/vcards/kdepim/kdepim-photo1-3.0.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_21_photo2)
{
	conv_vcard("/vcards/kdepim/kdepim-photo2-2.1.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_30_photo2)
{
	conv_vcard("/vcards/kdepim/kdepim-photo2-3.0.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_21_sound1)
{
	conv_vcard("/vcards/kdepim/kdepim-sound1-2.1.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_30_sound1)
{
	conv_vcard("/vcards/kdepim/kdepim-sound1-3.0.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_21_sound2)
{
	conv_vcard("/vcards/kdepim/kdepim-sound2-2.1.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_30_sound2)
{
	conv_vcard("/vcards/kdepim/kdepim-sound2-3.0.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_21_special)
{
	conv_vcard("/vcards/kdepim/kdepim-special-2.1.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_30_special)
{
	conv_vcard("/vcards/kdepim/kdepim-special-3.0.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_21_umlaute)
{
	conv_vcard("/vcards/kdepim/kdepim-umlaute-2.1.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vcard_kde_30_umlaute)
{
	conv_vcard("/vcards/kdepim/kdepim-umlaute-3.0.vcf", "VCARD_EXTENSION=KDE");
}
END_TEST

START_TEST (compare_vformat_mismatch1)
{
	compare_vcard("/vcards/evolution2/compare/1-different.vcf", "/vcards/kdepim/compare/1-different.vcf", OSYNC_CONV_DATA_MISMATCH);
}
END_TEST

START_TEST (compare_vformat_similar1)
{
	compare_vcard("/vcards/evolution2/compare/1-conflict.vcf", "/vcards/kdepim/compare/1-conflict.vcf", OSYNC_CONV_DATA_SIMILAR);
}
END_TEST

START_TEST (compare_vformat_mismatch2)
{
	compare_vcard("/vcards/evolution2/compare/2-conflict.vcf", "/vcards/kdepim/compare/2-conflict.vcf", OSYNC_CONV_DATA_MISMATCH);
}
END_TEST

START_TEST (compare_vformat_similar2)
{
	compare_vcard("/vcards/evolution2/compare/2-different.vcf", "/vcards/kdepim/compare/2-different.vcf", OSYNC_CONV_DATA_SIMILAR);
}
END_TEST

START_TEST (compare_vformat_same1)
{
	compare_vcard("/vcards/evolution2/compare/1-same.vcf", "/vcards/kdepim/compare/1-same.vcf", OSYNC_CONV_DATA_SAME);
}
END_TEST

START_TEST (compare_vformat_same2)
{
	compare_vcard("/vcards/evolution2/compare/2-same.vcf", "/vcards/kdepim/compare/2-same.vcf", OSYNC_CONV_DATA_SAME);
}
END_TEST

START_TEST (get_revision1)
{
	struct tm testtm = {24, 41, 10, 26, 2 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcard_get_revision("/vcards/evolution2/evo2-full1.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (get_revision2)
{
	struct tm testtm = {0, 0, 0, 26, 2 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcard_get_revision("/vcards/evolution2/evo2-full2.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (get_revision3)
{
	struct tm testtm = {0, 0, 0, 26, 2 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcard_get_revision("/vcards/evolution2/evo2-multiline.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (get_revision4)
{
	struct tm testtm = {24, 41, 10, 26, 2 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcard_get_revision("/vcards/evolution2/evo2-photo.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (get_no_revision)
{
	fail_unless(vcard_get_revision("/vcards/evolution2/compare/1-same.vcf") == -1, NULL);
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
	

	create_case(s, "conv_vcard_kde_21_nonuid", conv_vcard_kde_21_nonuid);
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
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
