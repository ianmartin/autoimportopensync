#ifndef _OPENSYNC_FORMAT_H_
#define _OPENSYNC_FORMAT_H_

struct tm _tmbuf;

char *osync_time_timestamp_remove_dash(const char *timestamp);
char *osync_time_datestamp(const char *stamp); 
char *osync_time_timestamp(const char *vtime);
char *osync_time_bdaystamp(const char *bday);
osync_bool osync_time_isdate(const char *vformat);
char *osync_time_set_vtime(const char *vtime, const char *time, osync_bool is_utc);
int osync_time_timezone_diff(void);
struct tm *osync_time_vtime2tm(const char *vtime);
char *osync_time_tm2vtime(const struct tm *utime, osync_bool is_utc);
time_t osync_time_vtime2unix(const char *vtime);
char *osync_time_unix2vtime_utc(const time_t *timestamp);
char *osync_time_unix2vtime_localtime(const time_t *timestamp);
struct tm *osync_time_utc2localtime(const struct tm *utime, int tzdiff);
struct tm *osync_time_localtime2utc(const struct tm *ltime, int tzdiff); 

#endif // _OPENSYNC_FORMAT_H_

