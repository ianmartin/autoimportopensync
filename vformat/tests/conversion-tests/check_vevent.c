#include "conversion.h"

static void conv_vevent(const char *filename, const char *extension)
{
	conv("event", filename, extension);
}

static void compare_vevent(const char *lfilename, const char *rfilename, OSyncConvCmpResult result)
{
	compare("event", lfilename, rfilename, result);
}

static time_t vevent_get_revision(const char *filename, const char *extension)
{
	return get_revision("event", filename, extension);
}

START_TEST (conv_vevent_evolution2_1hour)
{
	conv_vevent("/vevents/evolution2/1-hour.vcf", "VCAL_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vevent_evolution2_1hour_alarm)
{
	conv_vevent("/vevents/evolution2/1-hour-alarm.vcf", "VCAL_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vevent_evolution2_1hour_alarm2)
{
	conv_vevent("/vevents/evolution2/1-hour-alarm2.vcf", "VCAL_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vevent_evolution2_all_day)
{
	conv_vevent("/vevents/evolution2/all-day.vcf", "VCAL_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vevent_evolution2_all_day2)
{
	conv_vevent("/vevents/evolution2/all-day2.vcf", "VCAL_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vevent_evolution2_free_busy)
{
	conv_vevent("/vevents/evolution2/free-busy.vcf", "VCAL_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vevent_evolution2_full_special)
{
	conv_vevent("/vevents/evolution2/full-special.vcf", "VCAL_EXTENSION=Evolution");
}
END_TEST
START_TEST (conv_vevent_evolution2_rec_every_year)
{
	conv_vevent("/vevents/evolution2/rec-every-year.vcf", "VCAL_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vevent_evolution2_rec_except)
{
	conv_vevent("/vevents/evolution2/rec-except.vcf", "VCAL_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vevent_evolution2_rec_for)
{
	conv_vevent("/vevents/evolution2/rec-for.vcf", "VCAL_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vevent_evolution2_rec_forever)
{
	conv_vevent("/vevents/evolution2/rec-forever.vcf", "VCAL_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vevent_evolution2_rec_until)
{
	conv_vevent("/vevents/evolution2/rec-until.vcf", "VCAL_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vevent_evolution2_rec_until2)
{
	conv_vevent("/vevents/evolution2/evo2-recur-until.vcf", "VCAL_EXTENSION=Evolution");
}
END_TEST

START_TEST (conv_vevent_kdepim_1hour_10)
{
	conv_vevent("/vevents/kdepim/1-hour-1.0.vcs", "VCAL_EXTENSION=KDE");
}
END_TEST

START_TEST (conv_vevent_kdepim_1hour_20)
{
	conv_vevent("/vevents/kdepim/1-hour-2.0.ics", "VCAL_EXTENSION=KDE");
}
END_TEST

START_TEST (cmp_vevent_1hour_1)
{
	compare_vevent("/vevents/evolution2/1-hour.vcf", "/vevents/kdepim/1-hour-1.0.vcs", OSYNC_CONV_DATA_SAME);
}
END_TEST

START_TEST (cmp_vevent_1hour_2)
{
	compare_vevent("/vevents/evolution2/1-hour.vcf", "/vevents/kdepim/1-hour-2.0.ics", OSYNC_CONV_DATA_SAME);
}
END_TEST

START_TEST (event_get_revision1)
{
	struct tm testtm = {2, 6, 11, 29, 3 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vevent_get_revision("/vevents/evolution2/1-hour-alarm.vcf", "VCAL_EXTENSION=Evolution") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (event_get_revision2)
{
	struct tm testtm = {1, 8, 11, 29, 3 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vevent_get_revision("/vevents/evolution2/1-hour-alarm2.vcf", "VCAL_EXTENSION=Evolution") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (event_get_revision3)
{
	struct tm testtm = {13, 5, 11, 29, 3 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vevent_get_revision("/vevents/evolution2/1-hour.vcf", "VCAL_EXTENSION=Evolution") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (event_no_revision)
{
	fail_unless(vevent_get_revision("/vevents/evolution2/all-day.vcf", "VCAL_EXTENSION=Evolution") == -1, NULL);
}
END_TEST

Suite *vevent_suite(void)
{
	Suite *s = suite_create("VEVent");
	
	create_case(s, "conv_vevent_evolution2_1hour", conv_vevent_evolution2_1hour);
	create_case(s, "conv_vevent_evolution2_1hour_alarm", conv_vevent_evolution2_1hour_alarm);
	create_case(s, "conv_vevent_evolution2_1hour_alarm2", conv_vevent_evolution2_1hour_alarm2);
	create_case(s, "conv_vevent_evolution2_all_day", conv_vevent_evolution2_all_day);
	create_case(s, "conv_vevent_evolution2_all_day2", conv_vevent_evolution2_all_day2);
	create_case(s, "conv_vevent_evolution2_free_busy", conv_vevent_evolution2_free_busy);
	create_case(s, "conv_vevent_evolution2_full_special", conv_vevent_evolution2_full_special);
	create_case(s, "conv_vevent_evolution2_rec_every_year", conv_vevent_evolution2_rec_every_year);
	create_case(s, "conv_vevent_evolution2_rec_except", conv_vevent_evolution2_rec_except);
	create_case(s, "conv_vevent_evolution2_rec_for", conv_vevent_evolution2_rec_for);
	create_case(s, "conv_vevent_evolution2_rec_forever", conv_vevent_evolution2_rec_forever);
	create_case(s, "conv_vevent_evolution2_rec_until", conv_vevent_evolution2_rec_until);
	create_case(s, "conv_vevent_evolution2_rec_until2", conv_vevent_evolution2_rec_until2);
	
	create_case(s, "conv_vevent_kdepim_1hour_10", conv_vevent_kdepim_1hour_10);
	create_case(s, "conv_vevent_kdepim_1hour_20", conv_vevent_kdepim_1hour_20);
	
	create_case(s, "cmp_vevent_1hour_1", cmp_vevent_1hour_1);
	create_case(s, "cmp_vevent_1hour_2", cmp_vevent_1hour_2);
	
	create_case(s, "event_get_revision1", event_get_revision1);
	create_case(s, "event_get_revision2", event_get_revision2);
	create_case(s, "event_get_revision3", event_get_revision3);
	create_case(s, "event_no_revision", event_no_revision);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = vevent_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
