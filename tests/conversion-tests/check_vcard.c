#include "conversion.h"

static void conv_vcard(const char *filename, const char *extension)
{
	conv("contact", filename, extension);
}

static void compare_vcard(const char *lfilename, const char *rfilename, OSyncConvCmpResult result)
{
	compare("contact", lfilename, rfilename, result);
}

static time_t vcard_get_revision(const char *filename, const char *extension)
{
	return get_revision("contact", filename, extension);
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
	fail_unless(vcard_get_revision("/vcards/evolution2/evo2-full1.vcf", "VCARD_EXTENSION=Evolution") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (get_revision2)
{
	struct tm testtm = {0, 0, 0, 26, 2 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcard_get_revision("/vcards/evolution2/evo2-full2.vcf", "VCARD_EXTENSION=Evolution") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (get_revision3)
{
	struct tm testtm = {0, 0, 0, 26, 2 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcard_get_revision("/vcards/evolution2/evo2-multiline.vcf", "VCARD_EXTENSION=Evolution") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (get_revision4)
{
	struct tm testtm = {24, 41, 10, 26, 2 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcard_get_revision("/vcards/evolution2/evo2-photo.vcf", "VCARD_EXTENSION=Evolution") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (get_no_revision)
{
	fail_unless(vcard_get_revision("/vcards/evolution2/compare/1-same.vcf", "VCARD_EXTENSION=Evolution") == -1, NULL);
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
