#include <check.h>
#include <glib.h>
#include <gmodule.h>
#include <opensync/opensync.h>
#include <opensync/opensync-time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// this test assumes Eastern time, sorry.
START_TEST (time_timezone_diff)
{
       struct tm local;
       int zonediff_normal, zonediff_day, zonediff_month;

       // discover localtime zone
//     time_t t;
//     localtime_r(&now, &local);
//     gmtime_r


       // test normal case
       local.tm_sec = 0;
       local.tm_min = 0;
       local.tm_hour = 0;
       local.tm_mday = 15;
       local.tm_mon = 0;
       local.tm_year = 107;
       local.tm_wday = 0;
       local.tm_yday = 0;
       local.tm_isdst = 0;

//     t = mktime(&local);
//     gmtime_r(&t, &utc);

       zonediff_normal = osync_time_timezone_diff(&local);


       // test day straddle
       local.tm_hour = 23;
       zonediff_day = osync_time_timezone_diff(&local);


       // test month straddle
       local.tm_mday = 31;
       zonediff_month = osync_time_timezone_diff(&local);

       printf("normal = %d\nday = %d\nmonth = %d\n",
               zonediff_normal, zonediff_day, zonediff_month);

       fail_unless(zonediff_normal == zonediff_day && zonediff_day == zonediff_month, NULL);
}
END_TEST

static int test_relative2tm(const char *byday, int month, int year,
	int expected_day, int expected_wday)
{
	printf("Test parameters: %s, month: %d, year: %d\n",
		byday, month, year);

	struct tm *test = osync_time_relative2tm(byday, month, year);
	if( !test ) {
		printf("   Error in osync_time_relative2tm()\n");
		return expected_day == -1;
	}
	else {
		int ret = expected_day == test->tm_mday &&
			expected_wday == test->tm_wday &&
			(month-1) == test->tm_mon &&
			(year-1900) == test->tm_year;

		printf("   Result: %s", asctime(test));
		g_free(test);
		return ret;
	}
}

START_TEST (time_relative2tm)
{
	/*
	     June 2007      
	Su Mo Tu We Th Fr Sa
			1  2
	 3  4  5  6  7  8  9
	10 11 12 13 14 15 16
	17 18 19 20 21 22 23
	24 25 26 27 28 29 30
	*/
                    
	fail_unless( test_relative2tm("+1MO,SU", 6, 2007, 4, 1), NULL);
	fail_unless( test_relative2tm("+2TU,SU", 6, 2007, 12, 2), NULL);
	fail_unless( test_relative2tm("+3WE,SU", 6, 2007, 20, 3), NULL);
	fail_unless( test_relative2tm("+4TH,SU", 6, 2007, 28, 4), NULL);
	fail_unless( test_relative2tm("+5SU,FR", 6, 2007, -1, -1), NULL);

	fail_unless( test_relative2tm("-1MO,SU", 6, 2007, 25, 1), NULL);
	fail_unless( test_relative2tm("-2TU,SU", 6, 2007, 19, 2), NULL);
	fail_unless( test_relative2tm("-3WE,SU", 6, 2007, 13, 3), NULL);
	fail_unless( test_relative2tm("-4TH,SU", 6, 2007, 7, 4), NULL);
	fail_unless( test_relative2tm("-5FR,SU", 6, 2007, 1, 5), NULL);
	fail_unless( test_relative2tm("-6FR,SU", 6, 2007, -1, -1), NULL);
}
END_TEST

// Returns nonzero if tm structs are substantially equal.
// Ignores wday and yday.
int tm_equal(struct tm *a, struct tm *b)
{
	return	a->tm_sec == b->tm_sec &&
		a->tm_min == b->tm_min &&
		a->tm_hour == b->tm_hour &&
		a->tm_mday == b->tm_mday &&
		a->tm_mon == b->tm_mon &&
		a->tm_year == b->tm_year &&
		a->tm_isdst == b->tm_isdst;
}

void test_unix_converter(const struct tm *base, const char *vresult)
{
	struct tm tm_first, tm_second, *tm_ptr = NULL;
	time_t first, second;
	char *vtime = NULL;

	// test that osync_time_localtm2unix() behaves like mktime()
	memcpy(&tm_first, base, sizeof(struct tm));
	first = osync_time_localtm2unix(&tm_first);
	tm_first.tm_isdst = -1;
	second = mktime(&tm_first);
	fail_unless( first == second, NULL );

	// test that osync_time_unix2localtm() behaves like localtime()
	tm_ptr = osync_time_unix2localtm(&first);
	localtime_r(&first, &tm_second);
	fail_unless( tm_equal(&tm_first, &tm_second), NULL );
	fail_unless( tm_equal(tm_ptr, &tm_second), NULL );
	g_free(tm_ptr);
	tm_ptr = NULL;

	// test that osync_time_unix2utctm() behaves like gmtime_r()
	tm_ptr = osync_time_unix2utctm(&first);
	gmtime_r(&first, &tm_second);
	fail_unless( tm_equal(tm_ptr, &tm_second), NULL );
	g_free(tm_ptr);
	tm_ptr = NULL;

	// test that osync_time_utctm2unix() works correctly
	tm_second.tm_isdst = 0;		// make sure incorrect value is handled
	second = osync_time_utctm2unix(&tm_second);
	fail_unless( first == second, NULL );

	// test vtime string converters, in both directions
	vtime = osync_time_unix2vtime(&first);
	fail_unless( vtime != NULL, NULL );
	fail_unless( strcmp(vtime, vresult) == 0, NULL );
	printf("osync_time_unix2vtime() returned: %s\n", vtime);
	second = osync_time_vtime2unix(vtime, 0);
	fail_unless( first == second );
	g_free(vtime);
}

START_TEST (time_unix_converters)
{
	struct tm base;

	// simple test, no DST
	base.tm_sec = 0;
	base.tm_min = 0;
	base.tm_hour = 0;
	base.tm_mday = 1;
	base.tm_mon = 0;
	base.tm_year = 2007 - 1900;
	base.tm_isdst = 0;
	test_unix_converter(&base, "20070101T050000Z");

	// test dates that may straddle DST
	base.tm_hour = 1;
	base.tm_mday = 11;
	base.tm_mon = 2;
	test_unix_converter(&base, "20070311T060000Z");

	base.tm_hour = 4;
	base.tm_mday = 11;
	base.tm_mon = 2;
	test_unix_converter(&base, "20070311T080000Z");

	// simple test, pure DST
	base.tm_hour = 1;
	base.tm_mday = 11;
	base.tm_mon = 5;
	test_unix_converter(&base, "20070611T050000Z");
}
END_TEST

Suite *env_suite(void)
{
	Suite *s = suite_create("Time");

	TCase *tc_timezones = tcase_create("timezones");
	TCase *tc_relative  = tcase_create("relative");
	TCase *tc_unix      = tcase_create("unix_converters");

	suite_add_tcase (s, tc_timezones);
	tcase_add_test(tc_timezones, time_timezone_diff);

	suite_add_tcase (s, tc_relative);
	tcase_add_test(tc_relative, time_relative2tm);

	suite_add_tcase (s, tc_unix);
	tcase_add_test(tc_unix, time_unix_converters);

	return s;
}

int main(void)
{
       int nf;

       Suite *s = env_suite();

       SRunner *sr;
       sr = srunner_create(s);

       srunner_run_all(sr, CK_NORMAL);
       nf = srunner_ntests_failed(sr);
       srunner_free(sr);
       return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

