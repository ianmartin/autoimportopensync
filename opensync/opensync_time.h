#ifndef _OPENSYNC_TIME_H_
#define _OPENSYNC_TIME_H_

/* Timeformat helper */
char *osync_time_timestamp(const char *vtime);
char *osync_time_datestamp(const char *vtime); 
osync_bool osync_time_isdate(const char *vformat);
osync_bool osync_time_isutc(const char *vformat);
//char *osync_time_set_vtime(const char *vtime, const char *time, osync_bool is_utc);

/* Timetype helper */
struct tm *osync_time_vtime2tm(const char *vtime);
char *osync_time_tm2vtime(const struct tm *time, osync_bool is_utc);
time_t osync_time_vtime2unix(const char *vtime, int offset);
char *osync_time_unix2vtime(const time_t *timestamp);
time_t osync_time_tm2unix(const struct tm *tmtime);
struct tm *osync_time_unix2tm(const time_t *timestamp);

/* Timezone helper */
/* System Timezone-Reliable Helpers */
int osync_time_timezone_diff(const struct tm *time);
struct tm *osync_time_tm2utc(const struct tm *ltime, int offset);
struct tm *osync_time_tm2localtime(const struct tm *utime, int offset);
char *osync_time_vtime2utc(const char* localtime, int offset);
char *osync_time_vtime2localtime(const char* utc, int offset);
/* Smart Timezone Helpers */
// TODO

/* Alarm Duration Timeformat helper  */
char *osync_time_sec2alarmdu(int seconds);
int osync_time_alarmdu2sec(const char *alarm);

/* Timezone ID helper */
int osync_time_str2wday(const char *weekday);
struct tm *osync_time_relative2tm(const char *byday, const int bymonth, const int year);
int osync_time_utcoffset2sec(const char *offset);

#endif // _OPENSYNC_TIME_H_

