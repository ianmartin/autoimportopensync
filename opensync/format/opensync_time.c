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

#include <time.h>

#include "opensync.h"
#include "opensync_time.h"
#include "opensync_xml.h"
#include "opensync_internals.h"

#ifdef _WIN32
inline struct tm* localtime_r (const time_t *clock, struct tm *result) { 
	static int protector=0;
 	protector++;
 	while(protector>1){
 		protector--;
 		g_usleep(10);// sleep for milsec
 		protector++;
 	}
	if (!clock || !result){
		result = NULL;
	} else {
		memcpy(result,localtime(clock),sizeof(*result));
	}
	protector--;
	return result; 
}

inline struct tm* gmtime_r (const time_t *clock, struct tm *result) { 
	static int protector=0;
 	protector++;
 	while(protector>1){
 		protector--;
 		g_usleep(10);// sleep for milsec
 		protector++;
 	}
	if (!clock || !result){
		result = NULL;
	} else {
		memcpy(result,gmtime(clock),sizeof(*result));
	}
	protector--;
	return result; 
}
#endif

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

	/* YYYYMMDD */
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

	/* isdst is handled by tz offset calcualtion */
	utime->tm_isdst = -1;

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

	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, time, is_utc);
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
 * @param offset Seconds of UTC offset
 * @param vtime The osync formmatted timestamp
 * @returns Unix timestamp in time_t (UTC)
 */ 
time_t osync_time_vtime2unix(const char *vtime, int offset) {

	osync_trace(TRACE_ENTRY, "%s(%s, %i)", __func__, vtime, offset);
	struct tm *utime = NULL; 
	time_t timestamp;
	char *utc = NULL;

	utc = osync_time_vtime2utc(vtime, offset);
	utime = osync_time_vtime2tm(utc);

	timestamp = osync_time_tm2unix(utime);

	g_free(utc);
	g_free(utime);

	osync_trace(TRACE_EXIT, "%s: %lu", __func__, timestamp);
	return timestamp;
}

/*! @brief Function converts unix timestamp to vtime in UTC
 *
 * @param timestamp The unix timestamp which gets converted 
 * @returns vtime formatted as YYYYMMDDTHHMMSSZ (caller is responsible for freeing)
 */
char *osync_time_unix2vtime(const time_t *timestamp) {

	osync_trace(TRACE_ENTRY, "%s(%lu)", __func__, *timestamp);
	char *vtime;
	struct tm utc;

	/* FIXME Find conversion problem from vtime to unixtime and change then to gmtime_r().
	   (There is a probelm when converting from vtime to unixstamp and back. The result will be a wrong offset) */
//	gmtime_r(timestamp, &utc);
	localtime_r(timestamp, &utc);
	vtime = osync_time_tm2vtime(&utc, TRUE);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, vtime);
	return vtime;
}

/*! @brief Function converts struct tm to unix timestamp
 *
 * @param tmtime The struct tm which gets converted
 * @returns time_t (in UTC)
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

	struct tm *ptr_tm;
	struct tm tmtime;
	
	gmtime_r(timestamp, &tmtime);

	ptr_tm = &tmtime;

	return ptr_tm;
}

/*
 * Timezone helper
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

	localtime_r(&timestamp, &ltime);
	gmtime_r(&timestamp, &utime);

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

/*! @brief Function converts (struct tm) ltime from localtime to UTC.
 *         Paramter offset is used as UTC offset. 
 * 
 * @param ltime The struct tm which gets converted to UTC timezone
 * @param offset Seconds of UTC offset
 * @returns struct tm in UTC (caller is responsible for freeing)
 */ 
struct tm *osync_time_tm2utc(const struct tm *ltime, int offset) {

	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, ltime, offset);
	struct tm *tmtime = g_malloc0(sizeof(struct tm));

	tmtime->tm_year = ltime->tm_year;
	tmtime->tm_mon = ltime->tm_mon;
	tmtime->tm_mday = ltime->tm_mday;
	tmtime->tm_hour = ltime->tm_hour;
	tmtime->tm_min = ltime->tm_min;
	tmtime->tm_sec = ltime->tm_sec;

	/* in seconds - to have a exactly timezone diff like -13.5h */
	tmtime->tm_hour -= offset / 3600;
	tmtime->tm_min -= (offset % 3600) / 60;

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
 *         Parameter is used as UTC offset.          
 * 
 * @param utime The struct tm which gets converted to localtime
 * @param offset Seconds of UTC offset
 * @returns struct tm in localtime (caller is responsible for freeing)
 */ 
struct tm *osync_time_tm2localtime(const struct tm *utime, int offset) {

	struct tm *tmtime = g_malloc0(sizeof(struct tm));
	
	tmtime->tm_year = utime->tm_year;
	tmtime->tm_mon = utime->tm_mon;
	tmtime->tm_mday = utime->tm_mday;
	tmtime->tm_hour = utime->tm_hour;
	tmtime->tm_min = utime->tm_min;
	tmtime->tm_sec = utime->tm_sec;

	tmtime->tm_hour += offset / 3600;
	tmtime->tm_min += (offset % 3600) / 60;

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
 * @param offset Seconds of UTC offset
 * @returns vtime in UTC timezone (caller is responsible for freeing)
 */ 
char *osync_time_vtime2utc(const char* localtime, int offset) {
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, localtime);
	
	char *utc = NULL; 
	struct tm *tm_local = NULL, *tm_utc = NULL;

	if (strstr(localtime, "Z")) {
		utc = strdup(localtime);
		goto end;
	}
		
	tm_local = osync_time_vtime2tm(localtime);
	tm_utc = osync_time_tm2utc(tm_local, offset);
	utc = osync_time_tm2vtime(tm_utc, TRUE);

	g_free(tm_local);
	g_free(tm_utc);
	
end:	
	osync_trace(TRACE_EXIT, "%s: %s", __func__, utc);
	return utc;
}

/*! @brief Functions converts a UTC vtime stamp to a localtime vtime stamp
 * 
 * @param utc The timestap in UTC timezone whic gets converted to localtime 
 * @returns vtime in local  timezon (caller is preponsible for freeing) 
 */ 
char *osync_time_vtime2localtime(const char* utc, int offset) {
	
	char *localtime = NULL; 
	struct tm *tm_local = NULL, *tm_utc = NULL;

	if (!strstr(utc, "Z")) {
		localtime = strdup(utc);
		return localtime;
	}
		
	tm_utc = osync_time_vtime2tm(utc);
	tm_local = osync_time_tm2localtime(tm_utc, offset);
	localtime = osync_time_tm2vtime(tm_local, FALSE);

	g_free(tm_local);
	g_free(tm_utc);
	
	return localtime;
}



/* XXX This functions should only be used as workaround for plugins which
   only supports localtime without any timezone information. */

/*! List of vcal fields which have should be converted by following
 *  workaround functions. 
 */
const char *_time_attr[] = {
	"DTSTART:",
	"DTEND:",
	"DTSTAMP:",
	"AALARM:",
	"DALARM:",
	"DUE:",
	NULL
};

/*! @brief Functions converts a UTC vtime stamp to a localtime vtime stamp
 * 
 * @param entry The whole vcal entry as GString which gets modified. 
 * @param field The field name which should be modified. 
 * @param toUTC The toggle in which direction we convert. TRUE = convert to UTC
 */ 
static void _convert_time_field(GString *entry, const char *field, osync_bool toUTC) {

	int i, tzdiff;
	char *res = NULL;
	char *new_stamp = NULL;

	GString *stamp = g_string_new("");

	if ((res = strstr(entry->str, field))) {
		res += strlen(field);

		for (i=0; res[i] != '\n' && res[i] != '\r'; i++)
			stamp = g_string_append_c(stamp, res[i]);

		gssize pos = res - entry->str; 
		entry = g_string_erase(entry, pos, i);

		// Get System offset to UTC
		struct tm *tm_stamp = osync_time_vtime2tm(stamp->str);
		tzdiff = osync_time_timezone_diff(tm_stamp);
		g_free(tm_stamp);

		if (toUTC)
			new_stamp = osync_time_vtime2utc(stamp->str, tzdiff);
		else
			new_stamp = osync_time_vtime2localtime(stamp->str, tzdiff); 

		entry = g_string_insert(entry, pos, new_stamp);
		g_free(new_stamp);
	}
}

/*! @brief Functions converts timestamps of vcal in localtime or UTC. 
 * 
 * @param vcal The vcalendar which has to be converted.
 * @param toUTC If TRUE conversion from localtime to UTC.
 * @return timestamp modified vcalendar 
 */ 
char *_convert_entry(const char *vcal, osync_bool toUTC) {

	int i = 0;
	GString *new_entry = g_string_new(vcal);

	for (i=0; _time_attr[i] != NULL; i++) 
		_convert_time_field(new_entry, _time_attr[i], toUTC);

	return g_string_free(new_entry, FALSE);
}

/*! @brief Functions converts timestamps of vcal to localtime
 * 
 * @param vcal The vcalendar which has to be converted.
 * @return modified vcalendar with local timestamps (related to system time) 
 */ 
char *osync_time_vcal2localtime(const char *vcal) {

	return _convert_entry(vcal, FALSE);
}

/*! @brief Functions converts timestamps of vcal to UTC
 * 
 * @param vcal The vcalendar which has to be converted.
 * @return modified vcalendar with UTC timestamps (related to system time) 
 */ 
char *osync_time_vcal2utc(const char *vcal) {

	return _convert_entry(vcal, TRUE);
}

/*! @brief Functions converts seconds in duration before or after alarm event 
 * 
 * @param seconds 
 * @returns ical alarm duration string (caller is preponsible for freeing) 
 */ 

char *osync_time_sec2alarmdu(int seconds) {

	osync_trace(TRACE_ENTRY, "%s(%i)", __func__, seconds);

        char *tmp = NULL;
	char *prefix = NULL;

	if (!seconds) { 
		tmp = g_strdup("PT0S");
		goto end;
	}

	if (seconds > 0) { 
		prefix = g_strdup("P");
	} else {
		prefix = g_strdup("-P");
		seconds *= -1;
	}

	// Days 
        if (!(seconds % (3600 * 24))) {
               tmp = g_strdup_printf("%s%iD", prefix, seconds / (3600 * 24));
	       goto end;
	}

	// Hours
        if (!(seconds % 3600)) {
                tmp = g_strdup_printf("%sT%iH", prefix, seconds / 3600);
		goto end;
	}

	// Minutes
        if (!(seconds % 60) && seconds < 3600) {
                tmp = g_strdup_printf("%sT%iM", prefix, seconds / 60);
		goto end;
	}

	// Seconds
	if (seconds < 60) { 
		tmp = g_strdup_printf("%sT%iS", prefix, seconds);
		goto end;
	}

        if (seconds > 60)
                tmp = g_strdup_printf("%sT%iM", prefix, seconds / 60);

        if (seconds > 3600)
                tmp = g_strdup_printf("%sT%iH%iM", prefix, seconds / 3600,
                                (seconds % 3600) / 60);

        if (seconds > (3600 * 24))
               tmp = g_strdup_printf("%s%iDT%iH%iM", prefix, seconds / (3600 * 24),
                               seconds % (3600 * 24) / 3600,
                               ((seconds % (3600 * 24) % 3600)) / 60);

end:
	g_free(prefix);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, tmp);
        return tmp;
}

/*! @brief Functions converts alarm duration event to seconds needed for reminder of vcal/ical
 * 
 * TODO: Test support for ALARM after/before end and after start
 *
 * @param alarm 
 * @returns seconds of alarm and duration
 */ 

int osync_time_alarmdu2sec(const char *alarm) {

	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, alarm);

        int i, secs, digits;
        int is_digit = 0;
	int sign = 1;	// when ical stamp doesn't start with '-' => seconds after event
        int days = 0, weeks = 0, hours = 0, minutes = 0, seconds = 0;

	        for (i=0; i < (int) strlen(alarm); i++) {

                switch (alarm[i]) {
                        case '-':
				sign = -1; // seconds before event - change the sign 
                        case 'P':
                        case 'T':
                                is_digit = 0;
                                break;
                        case 'W':
                                is_digit = 0;
                                weeks = digits;
                                break;
                        case 'D':
                                is_digit = 0;
                                days = digits;
                                break;
                        case 'H':
                                is_digit = 0;
                                hours = digits;
                                break;
                        case 'M':
                                is_digit = 0;
                                minutes = digits;
                                break;
                        case 'S':
                                is_digit = 0;
                                seconds = digits;
                                break;
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                                if (is_digit)
                                        break;

                                sscanf((char*)(alarm+i),"%d",&digits);
                                is_digit = 1;
                                break;
                }
	}

        secs = (weeks * 7 * 24 * 3600) + (days * 24 * 3600) + (hours * 3600) + (minutes * 60) + seconds;

	secs = secs * sign;	// change sign if the alarm is in seconds before event (leading '-')

	osync_trace(TRACE_EXIT, "%s: %i", __func__, secs);
        return secs;
}

/*
 * Timezone ID helper
 */

/*! @brief Function converts a week day string to the struct tm wday integer. 
 *
 * @param weekday string of the week day
 * @returns integer of the weekday (Sunday = 0) 
 */ 
int osync_time_str2wday(const char *swday) {

	int weekday = -1;

	if (!strcmp(swday, "SU"))
		weekday = 0;
	else if (!strcmp(swday, "MO"))
		weekday = 1;
	else if (!strcmp(swday, "TU"))
		weekday = 2;
	else if (!strcmp(swday, "WE"))
		weekday = 3;
	else if (!strcmp(swday, "TH"))
		weekday = 4;
	else if (!strcmp(swday, "FR"))
		weekday = 5;
	else if (!strcmp(swday, "SA"))
		weekday = 6;

	return weekday; 
}

/*! @brief Function determines the exactly date of relative information 
 *         It is used for example to determine the last sunday of a month (-1SU) 
 *         in a specific year. 
 *
 * @param byday string of the relative day of month modifier
 * @param bymonth calendar number of the monath (January = 1)
 * @param year calendar year (year = 1970) 
 * @returns struct tm of the relative information date with 00:00:00 timestamp.
 *	    (Caller is responsible for freeing)
 */ 
struct tm *osync_time_relative2tm(const char *byday, const int bymonth, const int year) {

		struct tm *datestamp = g_malloc0(sizeof(struct tm));
		char weekday[3];
		int first_wday = 0, last_wday = 0;
		int daymod, mday, searched_wday;

		sscanf(byday, "%d%s", &daymod, weekday); 
		weekday[2] = '\0';

		searched_wday = osync_time_str2wday(weekday);

		datestamp->tm_year = year - 1900; 
		datestamp->tm_mon = bymonth - 1;
		datestamp->tm_mday = 0; 
		datestamp->tm_hour = 0; 
		datestamp->tm_min = 0; 
		datestamp->tm_sec = 0;
		datestamp->tm_isdst = -1;

		for (mday = 0; mday <= 31; mday++) {
			datestamp->tm_mday = mday; 
			mktime(datestamp);

			if (datestamp->tm_wday == searched_wday) { 
				if (!first_wday)
					first_wday = searched_wday;

				last_wday = searched_wday;
			}
		}

		if (daymod > 0)
			datestamp->tm_mday = first_wday + (7 * (daymod - 1));
		else
			datestamp->tm_mday = last_wday - (7 * (daymod - 1));

		mktime(datestamp);

		return datestamp;
}

/*! @brief Function converts UTC offset string in offset in seconds
 *
 * @param offset The offset string of the form a timezone field (Example +0200) 
 * @returns seconds of UTC offset 
 */ 
int osync_time_utcoffset2sec(const char *offset) {
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, offset);

	char csign = 0;
	int seconds = 0, sign = 1;
	int hours = 0, minutes = 0;

	sscanf(offset, "%c%2d%2d", &csign, &hours, &minutes);

	if (csign == '-')
		sign = -1;

	seconds = (hours * 3600 + minutes * 60) * sign; 

	osync_trace(TRACE_EXIT, "%s: %i", __func__, seconds);
	return seconds;
}

/*! @brief Functions determines the change timestamp of daylight saving of the given
 * 	   XML Timezone from dstNode. 
 * 
 * @param dstNode daylight saving or standard XML information of a timezone.
 * @returns struct tm of exactly date-timestamp of the change from/to daylight saving time.
 *          (Caller is responsible for freeing!)
 */ 
struct tm *osync_time_dstchange(xmlNode *dstNode) {

	int month;
	struct tm *dst_change = NULL, *tm_started = NULL;
	char *started = NULL, *rule = NULL, *byday = NULL;

	xmlNode *current = osxml_get_node(dstNode, "DateStarted");
	started = (char*) xmlNodeGetContent(current);
	tm_started = osync_time_vtime2tm(started);
	
	g_free(started);

	current = osxml_get_node(dstNode, "RecurrenceRule");
	current = current->children;

	while (current) {
		rule = (char *) xmlNodeGetContent(current);

		if (strstr(rule, "BYDAY="))
			byday = g_strdup(rule + 6);
		else if (strstr(rule, "BYMONTH="))
			sscanf(rule, "BYMONTH=%d", &month);
		
		g_free(rule);

		current = current->next;
	}

	dst_change = osync_time_relative2tm(byday, month, tm_started->tm_year + 1900);

	g_free(byday);

	dst_change->tm_hour = tm_started->tm_hour;
	dst_change->tm_min = tm_started->tm_min;

	g_free(tm_started);

	return dst_change;
}

/*! @brief Functions determines if parameter vtime is Daylight Saving time in given Timezone ID (tzid) 
 * 
 * @param vtime Timestamp of time which should be determined 
 * @param tzid Timezone ID of timestamp 
 * @returns TRUE if vtime is daylight saving time of tzid
 */ 
osync_bool osync_time_isdst(const char *vtime, xmlNode *tzid) {

	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, vtime, tzid);

	int year;
	char *newyear = NULL;
	time_t newyear_t, timestamp;
	struct tm *std_changetime, *dst_changetime;
	time_t dstStamp, stdStamp;
	xmlNode *current = NULL;

	sscanf(vtime, "%4d%*2d%*2dT%*2d%*d%*2d%*c", &year);

	newyear = g_strdup_printf("%4d0101T000000", year);
	newyear_t = osync_time_vtime2unix(newyear, 0);
	timestamp = osync_time_vtime2unix(vtime, 0);

	/* Handle XML Timezone field */
	current = osxml_get_node(tzid, "Standard");
	std_changetime = osync_time_dstchange(current);

	current = osxml_get_node(tzid, "DaylightSavings");
	dst_changetime = osync_time_dstchange(current);

	/* determine in which timezone is vtime */
	dstStamp = osync_time_tm2unix(dst_changetime); 
	stdStamp = osync_time_tm2unix(std_changetime);

	if (timestamp > stdStamp && timestamp < dstStamp) {
		osync_trace(TRACE_EXIT, "%s: FALSE (Standard Timezone)", __func__);
		return FALSE;
	}	
	
	osync_trace(TRACE_EXIT, "%s: TRUE (Daylight Saving Timezone)", __func__);
	return TRUE; 
}

/*! @brief Functions returns the current UTC offset of the given vtime and interprets
 * 	   the Timezone XML information tz.
 * 
 * @param vtime Timestamp of given Timezone information
 * @param tz Timezone information in XML 
 * @returns seconds seconds of current DST state and timezone 
 */ 
int osync_time_tzoffset(const char *vtime, xmlNode *tz) {

	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, vtime, tz);

	int seconds;
        char *offset = NULL; 
	xmlNode *current = NULL;

	if (osync_time_isdst(vtime, tz))
		current = osxml_get_node(tz, "DaylightSavings");
	else
		current = osxml_get_node(tz, "Standard");

	offset = osxml_find_node(current, "TZOffsetFrom");
	seconds = osync_time_utcoffset2sec(offset);

	osync_trace(TRACE_EXIT, "%s: %i", __func__, seconds);
	return seconds;
}

/*! @brief Functions returns the Timezone id of the Timezone information XML.
 * 
 * @param tz Timezone information in XML 
 * @returns Timezone ID (Caller is responsible for freeing!) 
 */ 
char *osync_time_tzid(xmlNode *tz) {
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, tz);

	char *id = NULL; 

	id = osxml_find_node(tz, "TimezoneID");

	osync_trace(TRACE_EXIT, "%s: %s", __func__, id);
	return id;
}

/*! @brief Functions returns the Timezone location of the Timezone information XML.
 * 
 * @param tz Timezone information in XML 
 * @returns Timezone location (Caller is responsible for freeing!) 
 */ 
char *osync_time_tzlocation(xmlNode *tz) {
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, tz);

	char *location = NULL; 

	location = osxml_find_node(tz, "TimezoneLocation");

	osync_trace(TRACE_EXIT, "%s: %s", __func__, location);
	return location;
}

/*! @brief Function search for the matching Timezode node of tzid. 
 * 
 * @param root XML of entry 
 * @param tzid The TimezoneID which should match 
 * @returns *xmlNode Node of the matching Timezone 
 */ 
xmlNode *osync_time_tzinfo(xmlNode *root, const char *tzid) {
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, root, tzid);

	int numnodes, i;
	char *tzinfo_tzid = NULL;

	xmlNode *tz = NULL;
	xmlNodeSet *nodes = NULL;
	xmlXPathObject *xobj = NULL;

	/* search matching Timezone information */
	xobj = osxml_get_nodeset(root->doc, "/vcal/Timezone");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;

	osync_trace(TRACE_INTERNAL, "Found %i Timezone field(s)", numnodes);

	if (!numnodes)
		goto noresult;


	for (i=0; i < numnodes; i++) {
		tz = nodes->nodeTab[i];
		tzinfo_tzid = osync_time_tzid(tz);

		if (!tzinfo_tzid) {
			g_free(tzinfo_tzid);
			tz = NULL;
			continue;
		}

		if (!strcmp(tzinfo_tzid, tzid))
			break;
	}

	g_free(tzinfo_tzid);

	if (!tz)
		goto noresult;
		

	osync_trace(TRACE_EXIT, "%s: %p", __func__, tz);
	return tz;

noresult:	
	osync_trace(TRACE_EXIT, "%s: No matching Timezone node found. Seems to be a be a floating timestamp.", __func__);
	return NULL;
}

/*! @brief Functions converts a field with localtime with timezone information to UTC timestamp.  
 * 
 * @param root XML of entry 
 * @param field Name of field node with timestamp and timezone information 
 * @returns UTC timestamp, or NULL when TZ information is missing (floating time) or field is not found 
 */ 
char *osync_time_tzlocal2utc(xmlNode *root, const char *field) {
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, root, field);

	int offset = 0;
	char *utc = NULL, *field_tzid = NULL, *vtime = NULL;
	xmlNode *tz = NULL;

	/*
	node = osxml_get_node(root, field);
	if (!node) {
		osync_trace(TRACE_EXIT, "%s: field \"%s\" not found", __func__, field);
		return NULL;
	}
	*/

	field_tzid = osync_time_tzid(root);
	if (!field_tzid) {
		g_free(field_tzid);
		goto noresult;
	}

	tz = osync_time_tzinfo(root, field_tzid);
	g_free(field_tzid);

	if (!tz)
		goto noresult;

	vtime = osxml_find_node(root, "Content");

	/* Handle UTC offset like 13.5h */
	offset = osync_time_tzoffset(vtime, tz);
	struct tm *ttm = osync_time_vtime2tm(vtime);
	ttm->tm_hour -= offset / 3600;
	ttm->tm_min -= (offset % 3600) / 60;
	mktime(ttm);
	utc = osync_time_tm2vtime(ttm, TRUE);

	g_free(vtime);
	g_free(ttm);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, utc);
	return utc;

noresult:
	osync_trace(TRACE_EXIT, "%s: No matching Timezone node is found.", __func__);	
	return NULL;
}

