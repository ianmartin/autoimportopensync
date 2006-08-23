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

/* Floating Timestamps...... (handle tzid!) */

/* 
 * Time formatting helper
 */

/*! @brief Function remove dashes from datestamp and colon
 * 
 * @param timestamp The timestamp which gets cleaned
 * @returns valid vtime stamp in YYYYMMDD[THHMMDD[Z]] (the caller is responsible for freeing)
 */
static char *osync_time_timestamp_remove_dash(const char *timestamp) {
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

/*! @brief Function returns a date-timestamp in OSyncTime Spec format
 * 
 * @param vtime The timestamp which gets converted to a valid osync date-timestamp
 * @returns vtime date-timestring (the caller is responsible for freeing)
 */ 
char *osync_time_timestamp(const char *vtime) {
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, vtime);

	char *timestamp;

	timestamp = osync_time_timestamp_remove_dash(vtime);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, timestamp);
	return timestamp;
}

/*! @brief Function returns a date without timestamp in OSyncTime Spec format
 * 
 * @param vtime The timestamp which gets converted to a single datestamp
 * @returns valid single datestamp YYYYMMDD (the caller is responsible for freeing) 
 */ 
char *osync_time_datestamp(const char *vtime) {
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, vtime);

	char *tmp;
	const char *p;
	GString *str = g_string_new ("");

	tmp = osync_time_timestamp_remove_dash(vtime); 

	for (p=tmp; *p && *p != 'T'; p++)
		str = g_string_append_c (str, *p);

	free(tmp);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, str->str);
	return (char*) g_string_free(str, FALSE);
}

/*! @brief Function returns TRUE if vtime is a valid datestamp (YYYYMMDD)
 * 
 * @returns FALSE if vtime includes a timestamp, TRUE on a single datestamp
 */
osync_bool osync_time_isdate(const char *vtime) {

	int year, month, day;

	if (strstr(vtime, "T"))
		return FALSE;

	// YYYYMMDD
	if (sscanf(vtime, "%04d%02d%02d", &year, &month, &day) != 3)
		return FALSE;

	return TRUE;
}

/*! @brief Function returns TRUE if vtime is in UTC (YYYYMMDDTHH:MM:SSZ)
 * 
 * @returns FALSE if vtime includes no Zulu, TRUE if the timestamp is UTC
 */
osync_bool osync_time_isutc(const char *vtime) {

	if (!strstr(vtime, "Z"))
		return FALSE;

	return TRUE;
}

/*! @brief Function sets the time of vtime timestamp to the given time parameter
 * 
 * If vtime only stores date (without THHMMSS[Z]) parameter time will
 * appended. The is_utc append a Z (Zulu) for UTC if not present. 
 *
 * Mainly used for workarounds.
 *
 * @param vtime The original data-timestamp which gets modified
 * @param time The time which should be set
 * @param is_utc If the given time is UTC is_utc have to be TRUE
 * @returns data-timestamp in UTC if is_utc TRUE
 */
/*
char *osync_time_set_vtime(const char *vtime, const char *time, osync_bool is_utc) {
	osync_trace(TRACE_ENTRY, "%s(%s, %s)", __func__, vtime, time);

	char *tmp = NULL;
	
	// TODO

	osync_trace(TRACE_EXIT, "%s: %s", __func__, tmp);
	return tmp;
}
*/

/*
 * Timetype helper  
 */

/*! @brief Function converts vtime to tm struct
 * 
 * @param vtime The formatted timestamp (YYYYMMDDTHHMMSS)
 * @returns struct tm (caller is responsible for freeing)
 */
struct tm *osync_time_vtime2tm(const char *vtime) {

	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, vtime);

	struct tm *utime = g_malloc0(sizeof(struct tm));

	utime->tm_year = 0;
	utime->tm_mon = 0;
	utime->tm_mday = 0;
	utime->tm_hour = 0;
	utime->tm_min = 0;
	utime->tm_sec = 0;

	sscanf(vtime, "%04d%02d%02dT%02d%02d%02d%*01c",
			&(utime->tm_year), &(utime->tm_mon), &(utime->tm_mday),
			&(utime->tm_hour), &(utime->tm_min), &(utime->tm_sec));


	utime->tm_year -= 1900;
	utime->tm_mon -= 1;

	// isdst is handled by tz offset calcualtion
	utime->tm_isdst = -1;

	mktime(utime);

	osync_trace(TRACE_EXIT, "%s", __func__);

	return utime;
}

/*! @brief Function converts struct tm in vtime string
 * 
 * YYYYMMDDTHHMMSS[Z]
 * Returned timezone is equal to the timezone of struct tm.  
 *
 * @param time The tm struct which gets converted
 * @param is_utc If struct tm is UTC time is_utc have to be TRUE
 * @returns vtime formatted as YYYYMMDDTHHMMSS[Z] (caller is responsible for freeing)
 */
char *osync_time_tm2vtime(const struct tm *time, osync_bool is_utc) {

	osync_trace(TRACE_ENTRY, "%s(%p, %b)", __func__, time, is_utc);
	GString *vtime = g_string_new("");

	g_string_printf(vtime, "%04d%02d%02dT%02d%02d%02d",
				time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
				time->tm_hour, time->tm_min, time->tm_sec);

	if (is_utc)
		vtime = g_string_append(vtime, "Z");

	osync_trace(TRACE_EXIT, "%s: %s", __func__, vtime->str);
	return g_string_free(vtime, FALSE);
}

/*! @brief Function converts vtime to unix time
 * 
 * @param vtime The osync formmatted timestamp
 * @returns Unix timestamp in time_t (UTC)
 */ 
time_t osync_time_vtime2unix(const char *vtime) {

	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, vtime);
	struct tm *utime = NULL; 
	time_t timestamp;
	char *utc = NULL;

	/* convert the time to utc before doing format conversion */
	utc = osync_time_vtime2utc(vtime);
	utime = osync_time_vtime2tm(utc);
	timestamp = mktime(utime);

	g_free(utc);
	g_free(utime);

	osync_trace(TRACE_EXIT, "%s: %ld", __func__, timestamp);
	return timestamp;
}

/*! @brief Function converts unix timestamp to vtime in UTC 
 *
 * @param timestamp The unix timestamp which gets converted 
 * @returns vtime formatted as YYYYMMDDTHHMMSSZ (caller is responsible for freeing)
 */
char *osync_time_unix2vtime(const time_t *timestamp) {

	osync_trace(TRACE_ENTRY, "%s(%ld)", __func__, timestamp);
	char *vtime;
	struct tm *utc;

	utc = gmtime(timestamp);
	vtime = osync_time_tm2vtime(utc, TRUE);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, vtime);
	return vtime;
}

/*! @brief Function converts struct tm to unix timestamp
 *
 * @param tmtime The struct tm which gets converted
 * @returns time_t (in UTC XXX is this correct ?)
 */ 
time_t osync_time_tm2unix(const struct tm *tmtime) {

	time_t timestamp;
	struct tm *tmp = g_malloc0(sizeof(struct tm));

	memcpy(tmp, tmtime, sizeof(struct tm));

	timestamp = mktime(tmp);

	g_free(tmp);

	return timestamp; 
}

/*! @brief Function converts unix timestamp to struct tm
 * 
 * @param timestamp The unixtimestamp which gets converted
 * @returns: struct tm (in UTC)
 */ 
struct tm *osync_time_unix2tm(const time_t *timestamp) {

	struct tm *tmtime = NULL;
	
	tmtime = gmtime(timestamp);

	return tmtime; 
}

/*
 * Timezone helper
 * TODO: function which handles icalendar tzid fields
 */

/*! @brief Function gets offset of parameter time between UTC and localtime in seconds. 
 * 
 * @param time is the point in time when the offset have to be calculated (need for CEST/CET)
 * @returns Seconds of timezone offset
 */  
int osync_time_timezone_diff(const struct tm *time) {	
	osync_trace(TRACE_ENTRY, "%s()", __func__);

	struct tm ltime, utime;
	unsigned int lsecs, usecs;
	long zonediff;
	time_t timestamp;

	timestamp = osync_time_tm2unix(time);

	tzset();

	ltime = *localtime( &timestamp );
	utime = *gmtime( &timestamp );

	lsecs = 3600 * ltime.tm_hour + 60 * ltime.tm_min + ltime.tm_sec;
	usecs = 3600 * utime.tm_hour + 60 * utime.tm_min + utime.tm_sec;

	zonediff = lsecs - usecs;

	/* check for different day */
	if (utime.tm_mday != ltime.tm_mday) {
		if (utime.tm_mday < ltime.tm_mday)
			zonediff += 24 * 3600; 
		else
			zonediff -= 24 * 3600;
	}

	osync_trace(TRACE_EXIT, "%s: %i", __func__, zonediff);
	return zonediff;
}

/*! @brief Function converts (struct tm) ltime from localtime to UTC 
 * 
 * @param ltime The struct tm which gets converted to UTC timezone
 * @returns struct tm in UTC (caller is responsible for freeing)
 */ 
struct tm *osync_time_tm2utc(const struct tm *ltime) {

	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ltime);
	int tzdiff = 0;
	struct tm *tmtime = g_malloc0(sizeof(struct tm));

	/* set timezone difference */
	tzdiff = osync_time_timezone_diff(ltime);

	tmtime->tm_year = ltime->tm_year;
	tmtime->tm_mon = ltime->tm_mon;
	tmtime->tm_mday = ltime->tm_mday;
	tmtime->tm_hour = ltime->tm_hour;
	tmtime->tm_min = ltime->tm_min;
	tmtime->tm_sec = ltime->tm_sec;

	/* in seconds to have a exactly timezone diff like -13.5h */
	tmtime->tm_hour -= tzdiff / 3600;

	if (tmtime->tm_hour > 23 || tmtime->tm_hour < 0) {
	
		if (tmtime->tm_hour < 0) {
			tmtime->tm_hour += 24;	
			tmtime->tm_mday -= 1;
		} else {
			tmtime->tm_hour -= 24;
			tmtime->tm_mday += 1;
		}
	
	}

	osync_trace(TRACE_EXIT, "%s: %p", __func__, tmtime);
	return tmtime; 
}

/*! @brief Function converts (struct tm) utime from UTC to localtime 
 * 
 * @param utime The struct tm which gets converted to localtime
 * @returns struct tm in localtime (caller is responsible for freeing)
 */ 
struct tm *osync_time_tm2localtime(const struct tm *utime) {

	int tzdiff = 0;
	struct tm *tmtime = g_malloc0(sizeof(struct tm));
	
	tzdiff = osync_time_timezone_diff(utime);

	tmtime->tm_year = utime->tm_year;
	tmtime->tm_mon = utime->tm_mon;
	tmtime->tm_mday = utime->tm_mday;
	tmtime->tm_hour = utime->tm_hour;
	tmtime->tm_min = utime->tm_min;
	tmtime->tm_sec = utime->tm_sec;

	tmtime->tm_hour += tzdiff / 3600;

	if (tmtime->tm_hour > 23 || tmtime->tm_hour < 0) {

		if (tmtime->tm_hour < 0) {
			tmtime->tm_mday -= 1;
			tmtime->tm_hour += 24;
		} else {
			tmtime->tm_mday += 1;
			tmtime->tm_hour -= 24;
		}
	}

	return tmtime; 
}

/*! @brief Functions converts a localtime vtime stamp to a UTC vtime stamp
 *
 * @param localtime The local timestamp in vtime format
 * @returns vtime in UTC timezone (caller is responsible for freeing)
 */ 
char *osync_time_vtime2utc(const char* localtime) {
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, localtime);
	
	char *utc = NULL; 
	struct tm *tm_local = NULL, *tm_utc = NULL;

	if (strstr(localtime, "Z")) {
		utc = strdup(localtime);
		goto end;
	}
		
	tm_local = osync_time_vtime2tm(localtime);
	tm_utc = osync_time_tm2utc(tm_local);
	utc = osync_time_tm2vtime(tm_utc, TRUE);

	g_free(tm_local);
// FIXME memory leak	
//	g_free(tm_utc);
	
end:	
	osync_trace(TRACE_EXIT, "%s: %s", __func__, utc);
	return utc;
}

/*! @brief Functions converts a UTC vtime stamp to a localtime vtime stamp
 * 
 * @param utc The timestap in UTC timezone whic gets converted to localtime 
 * @returns vtime in local  timezon (caller is preponsible for freeing) 
 */ 
char *osync_time_vtime2localtime(const char* utc) {
	
	char *localtime = NULL; 
	struct tm *tm_local = NULL, *tm_utc = NULL;

	if (!strstr(utc, "Z")) {
		localtime = strdup(utc);
		return localtime;
	}
		
	tm_utc = osync_time_vtime2tm(utc);
	tm_local = osync_time_tm2localtime(tm_utc);
	localtime = osync_time_tm2vtime(tm_local, FALSE);

	g_free(tm_local);
	g_free(tm_utc);
	
	return localtime;
}

