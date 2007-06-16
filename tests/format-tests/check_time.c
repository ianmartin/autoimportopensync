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

Suite *env_suite(void)
{
       Suite *s = suite_create("Time");

       TCase *tc_timezones = tcase_create("timezones");

       suite_add_tcase (s, tc_timezones);

       tcase_add_test(tc_timezones, time_timezone_diff);
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

