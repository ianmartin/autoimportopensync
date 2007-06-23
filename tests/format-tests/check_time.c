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

Suite *env_suite(void)
{
       Suite *s = suite_create("Time");

       TCase *tc_timezones = tcase_create("timezones");
       TCase *tc_relative  = tcase_create("relative");

       suite_add_tcase (s, tc_timezones);
       tcase_add_test(tc_timezones, time_timezone_diff);

       suite_add_tcase (s, tc_relative);
       tcase_add_test(tc_relative, time_relative2tm);

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

