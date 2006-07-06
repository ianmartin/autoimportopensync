/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2006 Daniel Gollub <dgollub@suse.de>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */

#include "opensync.h"
#include "opensync_internals.h"

struct tm _tmbuf;

/* 
 * Time formatting helper
 */

/* Function remove dashes from datestamp and colon. 
 * Returns: YYYYMMDD[THHMMDD[Z]]
 */
char *osync_time_timestamp_remove_dash(const char *timestamp) {
        int i, len;
        GString *str = g_string_new("");

        len = strlen(timestamp);

	for (i=0; i < len; i++) {
		if (timestamp[i] == '-')
			continue;

		if (timestamp[i] == ':')
			continue;

		str = g_string_append_c(str, timestamp[i]);
	}

        return (char*) g_string_free(str, FALSE);
}

char *osync_time_timestamp(const char *vformat) {
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, vformat);

	char *timestamp;

	timestamp = osync_time_timestamp_remove_dash(vformat);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, timestamp);
	return timestamp;
}

/* Function returns a date without timestamp
 * Returns: new allocated date string
 */ 
char *osync_time_datestamp(const char *vformat) {
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, vformat);

	char *tmp;
	const char *p;
	GString *str = g_string_new ("");

	tmp = osync_time_timestamp_remove_dash(vformat); 

	for (p=tmp; *p && *p != 'T'; p++)
		str = g_string_append_c (str, *p);

	free(tmp);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, str->str);
	return (char*) g_string_free(str, FALSE);
}

/* Function returns TRUE if vformat is a valid datestamp (YYYYMMDD).
 * FALSE is returned if vformat includes a timestamp.
 */
osync_bool osync_time_isdate(const char *vformat) {

	int year, month, day;

	if (strstr(vformat, "T"))
		return FALSE;

	// YYYYMMDD
	if (sscanf(vformat, "%04d%02d%02d", &year, &month, &day) != 3)
		return FALSE;

	return TRUE;
}

/* Function sets the time of vtime timestamp to the given time parameter.
 * If vtime only stores date (without THHMMSS[Z]) parameter time will
 * appended. The is_utc append a Z (Zulu) for UTC if not present. 
 */
char *osync_time_set_vtime(const char *vtime, const char *time, osync_bool is_utc) {
	osync_trace(TRACE_ENTRY, "%s(%s, %s)", __func__, vtime, time);

	char *tmp = NULL;
	
	// TODO

	osync_trace(TRACE_EXIT, "%s: %s", __func__, tmp);
	return tmp;
}


/*
 * Timezone helper
 */

/* Function gets current offset between UTC and localtime in seconds.
 * Returns: Seconds of timezone offset
 */  
int osync_time_timezone_diff(void) {	
	osync_trace(TRACE_ENTRY, "%s()", __func__);

	struct tm ltime, utime;
	long zonediff;
	time_t timestamp;

	time( &timestamp );

	tzset();

	ltime = *localtime( &timestamp );
	utime = *gmtime( &timestamp );

	zonediff = utime.tm_hour - ltime.tm_hour; 
	zonediff *= 3600;

	osync_trace(TRACE_EXIT, "%s: %i", __func__, zonediff);
	return zonediff;
}

/* Function converts vtime to tm struct and adjusting year (-1900)
 * and month (-1).
 * Returns: struct tm
 */
struct tm *osync_time_vtime2tm(const char *vtime) {

	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, vtime);

	struct tm *utime = &_tmbuf;

	utime->tm_year = 0;
	utime->tm_mon = 0;
	utime->tm_mday = 0;
	utime->tm_hour = 0;
	utime->tm_min = 0;
	utime->tm_sec = 0;

	// TODO handle zulu?
	sscanf(vtime, "%04d%02d%02dT%02d%02d%02d%*01c",
			&(utime->tm_year), &(utime->tm_mon), &(utime->tm_mday),
			&(utime->tm_hour), &(utime->tm_min), &(utime->tm_sec));

	osync_trace(TRACE_INTERNAL, "date: %04d-%02d-%02d T %02d:%02d:%02d\n",
			utime->tm_year, utime->tm_mon, utime->tm_mday,
			utime->tm_hour, utime->tm_min, utime->tm_sec);

	utime->tm_year -= 1900;
	utime->tm_mon -= 1;

	osync_trace(TRACE_EXIT, "%s", __func__);

	return utime;
}

/* Function converts struct tm in vtime string: YYYYMMDDTHHMMSS[Z]
 * Parameter bool utc appends Z (Zulu) for UTC timezone.  
 * Returns: vtime in UTC
 */
char *osync_time_tm2vtime(const struct tm *time, osync_bool is_utc) {

	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, time, is_utc);
	struct tm *utime = &_tmbuf;
	char *vtime = NULL;
	int zonediff = 0;

	if (!is_utc) {
		zonediff = osync_time_timezone_diff();
		utime = osync_time_localtime2utc(time, zonediff);

		vtime = g_strdup_printf("%04d%02d%02dT%02d%02d%02dZ",
				utime->tm_year + 1900, utime->tm_mon + 1, utime->tm_mday,
				utime->tm_hour, utime->tm_min, utime->tm_sec);
	} else {
		vtime = g_strdup_printf("%04d%02d%02dT%02d%02d%02dZ",
				time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
				time->tm_hour, time->tm_min, time->tm_sec);
	}


	osync_trace(TRACE_EXIT, "%s: %s", __func__, vtime);
	return vtime;
}

/* Function converts vtime to unix time.
 * Returns: time_t (in UTC)
 */ 
time_t osync_time_vtime2unix(const char *vtime) {

	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, vtime);
	struct tm *utime = &_tmbuf;
	time_t timestamp;

	utime = osync_time_vtime2tm(vtime);
	timestamp = mktime(utime);

	osync_trace(TRACE_EXIT, "%s: %ld", __func__, timestamp);
	return timestamp;
}

/* Function converts unix timestamp to vtime in UTC
 * Returns: vtime (in UTC)   
 */
char *osync_time_unix2vtime_utc(const time_t *timestamp) {

	osync_trace(TRACE_ENTRY, "%s(%ld)", __func__, timestamp);
	char *vtime;
	struct tm utime;

	utime = *gmtime(timestamp);
	vtime = osync_time_tm2vtime(&utime, TRUE);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, vtime);
	return vtime;
}

/* Function converts unix timestamp to vtime in localtime
 * Returns: vtime (in localtime)   
 */
char *osync_time_unix2vtime_localtime(const time_t *timestamp) {

	osync_trace(TRACE_ENTRY, "%s(%ld)", __func__, timestamp);
	char *vtime;
	struct tm ltime;

	ltime = *localtime(timestamp);
	vtime = osync_time_tm2vtime(&ltime, FALSE);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, vtime);
	return vtime;
}

/* Function converts (struct tm) utime from UTC to localtime 
 * Returns: struct tm in localtime / system timezone
 */ 
struct tm *osync_time_utc2localtime(const struct tm *utime, int tzdiff) {

	int hour;
	time_t timestamp;
	struct tm *tmtime = &_tmbuf;

	tmtime->tm_year = utime->tm_year;
	tmtime->tm_mon = utime->tm_mon;
	tmtime->tm_mday = utime->tm_mday;
	tmtime->tm_hour = utime->tm_hour;
	tmtime->tm_min = utime->tm_min;
	tmtime->tm_sec = utime->tm_sec;

	tmtime->tm_hour -= tzdiff / 3600;

	if (tmtime->tm_hour > 23) {

		hour = tmtime->tm_hour - 24;

		timestamp = mktime(tmtime);
		timestamp += 24 * 3600;

		utime = localtime(&timestamp);

		tmtime->tm_hour = hour;
	}

	return tmtime; 
}

/* Function converts (struct tm) ltime from localtime to UTC 
 * Returns: struct tm in UTC 
 */ 
struct tm *osync_time_localtime2utc(const struct tm *ltime, int tzdiff) {

	int hour;
	time_t timestamp;
	struct tm *tmtime = &_tmbuf;

	tmtime->tm_year = ltime->tm_year;
	tmtime->tm_mon = ltime->tm_mon;
	tmtime->tm_mday = ltime->tm_mday;
	tmtime->tm_hour = ltime->tm_hour;
	tmtime->tm_min = ltime->tm_min;
	tmtime->tm_sec = ltime->tm_sec;

	tmtime->tm_hour += tzdiff / 3600;

	if (tmtime->tm_hour > 23) {

		hour = tmtime->tm_hour - 24;

		timestamp = mktime(tmtime); 
		timestamp += 24 * 3600;

		tmtime = localtime(&timestamp);

		tmtime->tm_hour = hour;
	}

	return tmtime; 
}

