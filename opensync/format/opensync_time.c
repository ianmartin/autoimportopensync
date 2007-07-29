/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2006 Daniel Gollub <dgollub@suse.de>
 * Copyright (C) 2007 Chris Frey <cdfrey@netdirect.ca>
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
inline struct tm* localtime_r (const time_t *clock, struct tm *result)
{
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

inline struct tm* gmtime_r (const time_t *clock, struct tm *result)
{
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


//////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup OSyncTimeAPI OpenSync Time
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncTimeAPI
 * 
 */
/*@{*/

/* Floating Timestamps...... (handle tzid!) */


//////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup OSyncTimeFormatting Time formatting helpers
 * @ingroup OSyncTimeAPI
 * @brief Helper functions for formatting time strings, stripping out
 *	invalid characters, etc.
 * 
 */
/*@{*/

/* 
 * Time formatting helper
 */

/*! @brief Function remove dashes from datestamp and colon
 * 
 * @param timestamp The timestamp which gets cleaned
 * @returns valid vtime stamp in YYYYMMDD[THHMMDD[Z]] (the caller is responsible for freeing)
 */
static char *osync_time_timestamp_remove_dash(const char *timestamp)
{
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
char *osync_time_timestamp(const char *vtime)
{
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
char *osync_time_datestamp(const char *vtime)
{
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
osync_bool osync_time_isdate(const char *vtime)
{
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
osync_bool osync_time_isutc(const char *vtime)
{
	if (!strstr(vtime, "Z"))
		return FALSE;

	return TRUE;
}

#if 0
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
char *osync_time_set_vtime(const char *vtime, const char *time, osync_bool is_utc)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %s)", __func__, vtime, time);

	char *tmp = NULL;
	
	// TODO

	osync_trace(TRACE_EXIT, "%s: %s", __func__, tmp);
	return tmp;
}
#endif
/*@}*/





//////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup OSyncTimeStringToStruct String to struct converters.
 * @ingroup OSyncTimeAPI
 * @brief Helper functions for converting time between vtime formatted
 *	strings and struct tm time structures.  These functions are not
 *	smart in any way, with regards to timezones or daylight saving
 *	time, nor do they need to be.
 * 
 */
/*@{*/

/*
 * String <-> struct converters, no smarts
 */

/*! @brief Function converts vtime to tm struct
 * 
 * @param vtime The formatted timestamp (YYYYMMDDTHHMMSS)
 * @returns struct tm (caller is responsible for freeing)
 */
struct tm *osync_time_vtime2tm(const char *vtime)
{
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

	/* ask C library to clean up any anomalies */
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
char *osync_time_tm2vtime(const struct tm *time, osync_bool is_utc)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, time, is_utc);
	GString *vtime = g_string_new("");
	struct tm my_time = *time;

	/* ask C library to clean up any anomalies */
	mktime(&my_time);

	g_string_printf(vtime, "%04d%02d%02dT%02d%02d%02d",
				my_time.tm_year + 1900, my_time.tm_mon + 1, my_time.tm_mday,
				my_time.tm_hour, my_time.tm_min, my_time.tm_sec);

	if (is_utc)
		vtime = g_string_append(vtime, "Z");

	osync_trace(TRACE_EXIT, "%s: %s", __func__, vtime->str);
	return g_string_free(vtime, FALSE);
}
/*@}*/




//////////////////////////////////////////////////////////////////////////////

/*
 * Unix converters
 */

/**
 * @defgroup OSyncTimeUnixTimeConverters Unix time converters
 * @ingroup OSyncTimeAPI
 * @brief Helper functions for converting to and from unix time_t
 *	timestamps.  Includes functions supporting vtime strings,
 *	and struct tm's in local and UTC time.
 * 
 */
/*@{*/

/*! @brief Function converts vtime to unix time
 * 
 * @param offset Seconds of UTC offset
 * @param vtime The osync formmatted timestamp
 * @returns Unix timestamp in time_t (UTC)
 */ 
time_t osync_time_vtime2unix(const char *vtime, int offset)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %i)", __func__, vtime, offset);
	struct tm *utime = NULL; 
	time_t timestamp;
	char *utc = NULL;

	utc = osync_time_vtime2utc(vtime, offset);
	utime = osync_time_vtime2tm(utc);
	timestamp = osync_time_utctm2unix(utime);

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
char *osync_time_unix2vtime(const time_t *timestamp)
{
	osync_trace(TRACE_ENTRY, "%s(%lu)", __func__, *timestamp);
	char *vtime;
	struct tm utc;

	gmtime_r(timestamp, &utc);
	vtime = osync_time_tm2vtime(&utc, TRUE);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, vtime);
	return vtime;
}


/*
 * Unix time_t converters
 */

/*! @brief Function converts struct tm, in localtime, to unix timestamp.
 *		This is the same as calling mktime(), except that
 *		tmtime is not modified, and tm_isdst is always
 *		forced to -1.
 *
 * @param tmtime The struct tm, in localtime, which gets converted
 * @returns time_t (in UTC of course)
 */ 
time_t osync_time_localtm2unix(const struct tm *tmtime)
{
	time_t timestamp;
	struct tm *tmp = g_malloc0(sizeof(struct tm));

	memcpy(tmp, tmtime, sizeof(struct tm));

	tmp->tm_isdst = -1;
	timestamp = mktime(tmp);

	g_free(tmp);

	return timestamp; 
}

/*! @brief Function converts struct tm, in utc, to unix timestamp.
 *
 * @param tmtime The struct tm, in utc, which gets converted
 * @returns time_t (in UTC of course)
 *
 * This algorithm abuses the POSIX time functions, only because
 * there seems to be no standard API to do this more simply.
 * We could use the tm2utc() and tm2localtime() functions defined
 * farther on, but those are just simple math, and assume completely
 * that the offset provided is perfectly accurate.  This is not
 * always the case, when all you have is a random date in a tm struct.
 *
 * If there is a better way, I'd love to know!  - cdfrey
 */ 
time_t osync_time_utctm2unix(const struct tm *utctime)
{
	time_t timestamp;

#if 1
	struct tm *tmp = g_malloc0(sizeof(struct tm));
	struct tm localnow;
	struct tm check;

	// calculate local timezone difference... this is only used
	// to reduce the number of loops to find the correct match
	time(&timestamp);
	localtime_r(&timestamp, &localnow);
	int tzdiff = osync_time_timezone_diff(&localnow);

	// now loop, converting "local time" to time_t to utctm,
	// and adjusting until there are no differences... this
	// automatically takes care of DST issues.

	// do first conversion
	memcpy(tmp, utctime, sizeof(struct tm));
	tmp->tm_sec += tzdiff; // mktime will normalize the seconds for us
	tmp->tm_isdst = -1;
	timestamp = mktime(tmp);
	gmtime_r(&timestamp, &check);

	// loop until match
	while (check.tm_hour != utctime->tm_hour ||
		check.tm_min != utctime->tm_min)
	{
		tmp->tm_min += utctime->tm_min - check.tm_min;
		tmp->tm_hour += utctime->tm_hour - check.tm_hour;
		tmp->tm_mday += utctime->tm_mday - check.tm_mday;
		tmp->tm_year += utctime->tm_year - check.tm_year;
		tmp->tm_isdst = -1;

		timestamp = mktime(tmp);
		gmtime_r(&timestamp, &check);
	}

	g_free(tmp);
#endif

#if 0

	This method is broken with regard to DST.

	struct tm *local = NULL;

	// calculate local timezone difference...
	// note.. this does NOT take DST into account
	time(&timestamp);
	localtime_r(&timestamp, &localnow);
	int tzdiff = osync_time_timezone_diff(&localnow);

	local = osync_time_tm2localtime(utctime, tzdiff);
	if (local == NULL)
		return -1;

	local->tm_isdst = -1;
	timestamp = mktime(local);

	g_free(local);
#endif

	return timestamp; 
}

/*! @brief Function converts unix timestamp to struct tm in localtime.
 *		This is the same as calling localtime_r(), except you
 *		have to free the returned value.
 * 
 * @param timestamp The unixtimestamp which gets converted
 * @returns: struct tm (in localtime) (Caller is responsible for freeing!)
 */ 
struct tm *osync_time_unix2localtm(const time_t *timestamp)
{
	struct tm *ptr_tm = g_malloc0(sizeof(struct tm));

	localtime_r(timestamp, ptr_tm);

	return ptr_tm;
}

/*! @brief Function converts unix timestamp to struct tm in utc.
 *		This is the same as calling gmtime_r(), except you
 *		have to free the returned value.
 * 
 * @param timestamp The unixtimestamp which gets converted
 * @returns: struct tm (in UTC) (Caller is responsible for freeing)
 */ 
struct tm *osync_time_unix2utctm(const time_t *timestamp)
{
	struct tm *ptr_tm = g_malloc0(sizeof(struct tm));

	gmtime_r(timestamp, ptr_tm);

	return ptr_tm;
}
/*@}*/




//////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup OSyncTimeTimezoneHelpers Timezone helpers
 * @ingroup OSyncTimeAPI
 * @brief Helper functions for working with timezones, and doing
 *	conversions with user-specified timezone offsets.
 *	Note that in all functions below that require a timezone offset,
 *	they *require* the caller to be completely accurate with it,
 *	including in the DST case.  The result is only as accurate
 *	as the offset you provide.
 * 
 */
/*@{*/

/*
 * Timezone helper
 */

/*! @brief Function gets offset of parameter time between UTC and
 *	localtime in seconds east of UTC.  (i.e. east is positive,
 *	west is negative)
 * 
 * @param time is the point in time when the offset have to be calculated,
 *	specified in localtime (need for CEST/CET)
 * @returns Seconds of timezone offset
 */  
int osync_time_timezone_diff(const struct tm *local)
{
	osync_trace(TRACE_ENTRY, "%s()", __func__);

	struct tm utime;
	unsigned int lsecs, usecs;
	long zonediff, daydiff = 0;
	time_t timestamp;

	/* convert local time to UTC */
	timestamp = osync_time_localtm2unix(local);

	/* convert UTC to split tm struct in UTC */
	gmtime_r(&timestamp, &utime);

        /* calculate seconds of difference between local and UTC */
        lsecs = 3600 * local->tm_hour + 60 * local->tm_min + local->tm_sec;
	usecs = 3600 * utime.tm_hour + 60 * utime.tm_min + utime.tm_sec;
	zonediff = lsecs - usecs;

	/* check for different day */
	if (utime.tm_mday != local->tm_mday) {
		/* if uday < lday, then local time
		 * is ahead of UTC, and the above difference
		 * will straddle midnight and be 24 hours behind.
		 * Example: U: 23h on Jan 01
		 *          L: 01h on Jan 02
		 *          Real timezone offset is +0200
		 *          Calculated = 01 - 23 = -22
		 *          Corrected = -22 + 24 = +2
		 *
		 * Opposite case needs different correction.
		 */
		if (utime.tm_mday < local->tm_mday)
			daydiff = 24 * 3600; 
		else
			daydiff = -24 * 3600;
 
		/* if months are not the same, then we are
		 * straddling a month, and the above day
		 * comparison is upside down... correct again.
		 *
		 * Example:
		 *    U: 23h on Jan 31
		 *    L: 01h on Feb 01
		 *
		 * Above logic would subtract instead of add.
		 */
		if (utime.tm_mon != local->tm_mon) {
			daydiff = -daydiff;
		}
 
		/* perform the correction */
		zonediff += daydiff;
	}

	osync_trace(TRACE_EXIT, "%s: %i", __func__, zonediff);
	return zonediff;
}

/*! @brief Function converts (struct tm) ltime from localtime to UTC.
 *         Paramter offset is used as UTC offset.  Note that _only_ the
 *         following fields can be relied upon in the result:
 *         tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year.
 * 
 * @param ltime The struct tm which gets converted to UTC timezone
 * @param offset Seconds of UTC offset, in seconds east of UTC.
 * @returns struct tm in UTC (caller is responsible for freeing)
 */ 
struct tm *osync_time_tm2utc(const struct tm *ltime, int offset)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, ltime, offset);
	struct tm *tmtime = g_malloc0(sizeof(struct tm));

	tmtime->tm_year = ltime->tm_year;
	tmtime->tm_mon = ltime->tm_mon;
	tmtime->tm_mday = ltime->tm_mday;
	tmtime->tm_hour = ltime->tm_hour;
	tmtime->tm_min = ltime->tm_min;
	tmtime->tm_sec = ltime->tm_sec;

	// in seconds - to have a exactly timezone diff like -13.5h
	tmtime->tm_sec -= offset;
	tmtime->tm_isdst = -1;

	// normalize the struct to get rid of any negatives...
	// we throw away the time_t result here, and only use
	// mktime() for its normalizing abilities
	mktime(tmtime);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, tmtime);
	return tmtime;
}

/*! @brief Function converts (struct tm) utime from UTC to localtime 
 *         Paramter offset is used as UTC offset.  Note that _only_ the
 *         following fields can be relied upon in the result:
 *         tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year.
 * 
 * @param utime The struct tm which gets converted to localtime
 * @param offset Seconds of UTC offset, in seconds east of UTC.
 * @returns struct tm in localtime (caller is responsible for freeing)
 */ 
struct tm *osync_time_tm2localtime(const struct tm *utime, int offset)
{
	struct tm *tmtime = g_malloc0(sizeof(struct tm));
	
	tmtime->tm_year = utime->tm_year;
	tmtime->tm_mon = utime->tm_mon;
	tmtime->tm_mday = utime->tm_mday;
	tmtime->tm_hour = utime->tm_hour;
	tmtime->tm_min = utime->tm_min;
	tmtime->tm_sec = utime->tm_sec;
	tmtime->tm_isdst = -1;

	// in seconds - to have a exactly timezone diff like -13.5h
	tmtime->tm_sec += offset;

	// normalize the struct to get rid of any negatives...
	// we throw away the time_t result here, and only use
	// mktime() for its normalizing abilities
	mktime(tmtime);

	return tmtime; 
}

/*! @brief Functions converts a localtime vtime stamp to a UTC vtime stamp
 *
 * @param localtime The local timestamp in vtime format
 * @param offset Seconds of UTC offset, in seconds east of UTC.
 * @returns vtime in UTC timezone (caller is responsible for freeing)
 */ 
char *osync_time_vtime2utc(const char* localtime, int offset)
{
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, localtime);

	char *utc = NULL; 
	struct tm *tm_local = NULL, *tm_utc = NULL;

	if (strstr(localtime, "Z")) {
		utc = g_strdup(localtime);
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
char *osync_time_vtime2localtime(const char* utc, int offset)
{
	char *localtime = NULL; 
	struct tm *tm_local = NULL, *tm_utc = NULL;

	if (!strstr(utc, "Z")) {
		localtime = g_strdup(utc);
		return localtime;
	}
		
	tm_utc = osync_time_vtime2tm(utc);
	tm_local = osync_time_tm2localtime(tm_utc, offset);
	localtime = osync_time_tm2vtime(tm_local, FALSE);

	g_free(tm_local);
	g_free(tm_utc);
	
	return localtime;
}

/*! @brief Function converts UTC offset string in offset in seconds
 *
 * @param offset The offset string of the form a timezone field (Example +0200) 
 * @returns seconds of UTC offset 
 */ 
int osync_time_utcoffset2sec(const char *offset)
{
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

/*@}*/




//////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup OSyncTimeBackwardCompatibility Backward compatibility functions.
 * @ingroup OSyncTimeAPI
 * @brief These functions should only be used as workaround for plugins
 *	which only support localtime without any timezone information.
 * 
 */
/*@{*/

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

/*! @brief Function converts a UTC vtime stamp to a localtime vtime stamp
 * 
 * @param entry The whole vcal entry as GString which gets modified. 
 * @param field The field name which should be modified. 
 * @param toUTC The toggle in which direction we convert. TRUE = convert to UTC
 */ 
static void _convert_time_field(GString *entry, const char *field, osync_bool toUTC)
{
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
char *_convert_entry(const char *vcal, osync_bool toUTC)
{
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
char *osync_time_vcal2localtime(const char *vcal)
{
	return _convert_entry(vcal, FALSE);
}

/*! @brief Functions converts timestamps of vcal to UTC
 * 
 * @param vcal The vcalendar which has to be converted.
 * @return modified vcalendar with UTC timestamps (related to system time) 
 */ 
char *osync_time_vcal2utc(const char *vcal)
{
	return _convert_entry(vcal, TRUE);
}
/*@}*/



//////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup OSyncTimeAlarmDurationFormat Alarm duration format helpers.
 * @ingroup OSyncTimeAPI
 * @brief Functions for converting alarm durations to and from string formats.
 * 
 */
/*@{*/

/*! @brief Functions converts seconds in duration before or after alarm event 
 * 
 * @param seconds 
 * @returns ical alarm duration string (caller is preponsible for freeing) 
 */ 
char *osync_time_sec2alarmdu(int seconds)
{
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
int osync_time_alarmdu2sec(const char *alarm)
{
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
/*@}*/



//////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup OSyncTimeRecurrence Recurring time calculators.
 * @ingroup OSyncTimeAPI
 * @brief Functions related to calculating recurring dates.
 * 
 */
/*@{*/

/*! @brief Function converts a week day string to the struct tm wday integer. 
 *
 * @param weekday string of the week day
 * @returns integer of the weekday (Sunday = 0), or -1 on error
 */ 
int osync_time_str2wday(const char *swday)
{
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

/*! @brief Function determines the exact date of relative information.
 *         It is used for example to determine the last sunday of a month (-1SU) 
 *         in a specific year. 
 *         Note that the RFC2445 spec states that a weekday without
 *         a prefixed numeric value means *every* weekday in the month.
 *         This function will fail in such a case and return NULL.
 *         Also note that if byday contains multiple weekdays in its
 *         string (e.g. -1SU,MO) only the first weekday will be used (SU).
 *
 * @param byday string of the relative day of month modifier
 * @param bymonth calendar number of the month (January = 1)
 * @param year calendar year (e.g. 1970, 2007, etc)
 * @returns struct tm of the relative information date with 00:00:00 timestamp
 *          or NULL on error.
 *	    (Caller is responsible for freeing)
 */ 
struct tm *osync_time_relative2tm(const char *byday, const int bymonth, const int year)
{
	struct tm *datestamp = g_malloc0(sizeof(struct tm));
	struct tm search;
	char weekday[3];
	int first_wday = 0, last_wday = 0;
	int daymod, mday, searched_wday;

	if (sscanf(byday, "%d%2s", &daymod, weekday) != 2) {
		g_free(datestamp);
		return NULL;
	}
	weekday[2] = '\0';

	searched_wday = osync_time_str2wday(weekday);

	datestamp->tm_year = year - 1900; 
	datestamp->tm_mon = bymonth - 1;
	datestamp->tm_mday = 0; 
	datestamp->tm_hour = 0; 
	datestamp->tm_min = 0; 
	datestamp->tm_sec = 0;
	datestamp->tm_isdst = -1;

	for (mday = 1; mday <= 31; mday++) {
		memcpy(&search, datestamp, sizeof(struct tm));
		search.tm_mday = mday; 
		if( mktime(&search) == -1 || search.tm_mday != mday )
			break;	// we've cycled past a month end

		if (search.tm_wday == searched_wday) { 
			if (!first_wday)
				first_wday = mday;

			last_wday = mday;
		}
	}

	if (daymod >= 0)
		datestamp->tm_mday = first_wday + (7 * (daymod - 1));
	else
		datestamp->tm_mday = last_wday + (7 * (daymod + 1));

	// save for later check
	search.tm_mon = datestamp->tm_mon;;

	// normalize the tm struct and make sure the result
	// is in the same month as we started with
	datestamp->tm_isdst = -1;
	if (mktime(datestamp) == -1 || search.tm_mon != datestamp->tm_mon) {
		g_free(datestamp);
		return NULL;
	}

	return datestamp;
}
/*@}*/



//////////////////////////////////////////////////////////////////////////////
// undocumented XML related functions.

/*! @brief Function determines the change timestamp of daylight saving of the given
 * 	   XML Timezone from dstNode. 
 * 
 * @param dstNode daylight saving or standard XML information of a timezone.
 * @returns struct tm of exact date-timestamp of the change from/to daylight saving time, or NULL on error.
 *          (Caller is responsible for freeing!)
 */ 
struct tm *osync_time_dstchange(xmlNode *dstNode)
{
	int month;
	struct tm *dst_change = NULL, *tm_started = NULL;
	char *started = NULL, *rule = NULL, *byday = NULL;

	xmlNode *current = osxml_get_node(dstNode, "DateStarted");
	started = (char*) xmlNodeGetContent(current);
	tm_started = osync_time_vtime2tm(started);
	
	xmlFree(started);

	current = osxml_get_node(dstNode, "RecurrenceRule");
	current = current->children;

	while (current) {
		rule = (char *) xmlNodeGetContent(current);

		if (strstr(rule, "BYDAY="))
			byday = g_strdup(rule + 6);
		else if (strstr(rule, "BYMONTH="))
			sscanf(rule, "BYMONTH=%d", &month);
		
		xmlFree(rule);

		current = current->next;
	}

	dst_change = osync_time_relative2tm(byday, month, tm_started->tm_year + 1900);

	g_free(byday);

	if (dst_change != NULL) {
		dst_change->tm_hour = tm_started->tm_hour;
		dst_change->tm_min = tm_started->tm_min;
	}

	g_free(tm_started);

	return dst_change;
}

/*! @brief Functions determines if parameter vtime is Daylight Saving time in given Timezone ID (tzid) 
 * 
 * @param vtime Timestamp of time which should be determined 
 * @param tzid Timezone ID of timestamp 
 * @returns TRUE if vtime is daylight saving time of tzid
 */ 
/*
osync_bool osync_time_isdst(const char *vtime, xmlNode *tzid)
{
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

	// Handle XML Timezone field
	current = osxml_get_node(tzid, "Standard");
	std_changetime = osync_time_dstchange(current);

	current = osxml_get_node(tzid, "DaylightSavings");
	dst_changetime = osync_time_dstchange(current);

	// determine in which timezone is vtime
	dstStamp = osync_time_tm2unix(dst_changetime); 
	stdStamp = osync_time_tm2unix(std_changetime);

	if (timestamp > stdStamp && timestamp < dstStamp) {
		osync_trace(TRACE_EXIT, "%s: FALSE (Standard Timezone)", __func__);
		return FALSE;
	}	
	
	osync_trace(TRACE_EXIT, "%s: TRUE (Daylight Saving Timezone)", __func__);
	return TRUE; 
}
*/

/*! @brief Function returns the current UTC offset of the given vtime and interprets
 * 	   the Timezone XML information tz.
 * 
 * @param vtime Timestamp of given Timezone information
 * @param tz Timezone information in XML 
 * @returns seconds seconds of current DST state and timezone 
 */ 
/*
int osync_time_tzoffset(const char *vtime, xmlNode *tz)
{
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
*/

/*! @brief Function returns the Timezone id of the Timezone information XML.
 * 
 * @param tz Timezone information in XML 
 * @returns Timezone ID (Caller is responsible for freeing!) 
 */ 
char *osync_time_tzid(xmlNode *tz)
{
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
/*
No longer exists in schema
char *osync_time_tzlocation(xmlNode *tz)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, tz);

	char *location = NULL; 

	location = osxml_find_node(tz, "TimezoneLocation");

	osync_trace(TRACE_EXIT, "%s: %s", __func__, location);
	return location;
}
*/

/*! @brief Function search for the matching Timezode node of tzid. 
 * 
 * @param root XML of entry 
 * @param tzid The TimezoneID which should match 
 * @returns *xmlNode Node of the matching Timezone 
 */ 
xmlNode *osync_time_tzinfo(xmlNode *root, const char *tzid)
{
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
			tz = NULL;
			continue;
		}

		if (!strcmp(tzinfo_tzid, tzid)) {
			g_free(tzinfo_tzid);
			break;
		}
		g_free(tzinfo_tzid);
	}

	if (!tz)
		goto noresult;
		

	osync_trace(TRACE_EXIT, "%s: %p", __func__, tz);
	return tz;

noresult:	
	osync_trace(TRACE_EXIT, "%s: No matching Timezone node found. Seems to be a be a floating timestamp.", __func__);
	return NULL;
}

/*! @brief Function converts a field with localtime with timezone information to UTC timestamp.  
 * 
 * @param root XML of entry 
 * @param field Name of field node with timestamp and timezone information 
 * @returns UTC timestamp, or NULL when TZ information is missing (floating time) or field is not found 
 */ 
#if 0
char *osync_time_tzlocal2utc(xmlNode *root, const char *field)
{
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
	ttm->tm_isdst = -1;
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
#endif



#if 0
struct _RecurringSet
{
	// can be NULL if not existing in data
	OSyncXMLField *xml_rdate;
	OSyncXMLField *xml_rrule;
	OSyncXMLField *xml_exdate;
	OSyncXMLField *xml_exrule;
};
typedef struct _RecurringSet RecurringSet;


/*! @brief Calculates the next date based on a recurring ruleset.
 * 
 * @param tm_base	Date to start with.
 * @param rset		Filled recurring ruleset.
 *
 * @returns New tm struct with the calculated next date.  Caller is
 *		responsible for freeing!
 */ 
struct tm* osync_time_next_recurring(struct tm *tm_base,
				     RecurringSet *rset)
{
	osync_xmlfield_get_next
}

struct TimezoneSetNode
{
	uint64_t localTimestamp;	// 20070311020000
	int secondsFrom;
	int secondsTo;
	char *name;

	struct TimezoneSetNode *next;
};
typedef struct TimezoneSetNode TimezoneSet;

osync_bool _osync_zone_add_point(struct TimezoneSet **head, const struct tm *local, int secondsFrom, int secondsTo, const char *name);

TimezoneSet* osync_zone_load_tzset(OSyncXMLFormat *event, const char *tzid);
void osync_zone_free(struct TimezoneSet *head);
int osync_zone_get_tzdiff(struct TimezoneSet *head, const struct tm *local);

TimezoneSet* osync_zone_load_tzset(OSyncXMLFormat *event, const char *tzid)
{
	struct TimezoneSet *head = NULL;

xpathset;
	osync_xmlfield_get_key_value

	while( search_for_component ) {
		osync_zone_add_point();
	}

	return head;

ozbp_error:
	osync_zone_free_points(head);
	return NULL;
}
#endif


#if 0
/*! @brief Searches event for the timezone contained in dateTimeContent and
 *		sets tzoffset to the calculated timezone offset,
 *		in seconds east of UTC.
 *
 * @param event Pointer to XMLFormat of event data.
 * @param dateTimeContent OSyncXMLField pointer to field to be converted.
 * @param tzoffset Pointer to int, which will be filled with the offset
 *		value on success.
 * @returns TRUE on success, FALSE on error
 */
OSyncXMLField* osync_time_tzcomponent(OSyncXMLFormat *event, OSyncXMLField *dateTimeContent)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, event, dateTiemContent, tzoffset);

	// get tzid
	char *tzid = osync_xmlfield_get_attr(dateTimeContent, "TimezoneID");
	if (tzid == NULL) {
		osync_trace(TRACE_EXIT, "%s: could not find TimezoneID", __func__);
		return FALSE;
	}

	// fin
	xmlNode *current = NULL;

	if (osync_time_isdst(vtime, tz))
		current = osxml_get_node(tz, "DaylightSavings");
	else
		current = osxml_get_node(tz, "Standard");

	offset = osxml_find_node(current, "TZOffsetFrom");
	seconds = osync_time_utcoffset2sec(offset);

	osync_trace(TRACE_EXIT, "%s: %i", __func__, *tzoffset);
	return TRUE;
}
#endif


#if 0
/*! @brief Converts an XML DateTimeContent field (such as DateStarted or
 *	DateEnd) to a unix UTC time_t.
 * 
 * @param event Pointer to XMLFormat of event data.
 * @param dateTimeContent OSyncXMLField pointer to field to be converted.
 * @returns UTC time_t, or ((time_t)-1) on error.
 */ 
time_t osync_time_xml2unix(OSyncXMLFormat *event, OSyncXMLField *dateTimeContent)
{
	osync_trace(TRACE_ENTRY, "%s()", __func__);

	time_t ret = -1;

	// retrieve Content of xmlfield, assuming it is a DateTimeContent type
	const char *content = osync_xmlfield_get_key_value("Content");
	if (content == NULL)
		return -1;

	// retrieve Value attribute
	const char *value = osync_xmlfield_get_attr_value("Value");
	if (value == NULL) {
		// nothing specified, default to DATE-TIME
		// (for example, DTSTART in RFC2445, 4.8.2.4)
		value = "DATE-TIME";
	}

	// if timestamp is in UTC already, or if Value is DATE,
	// then we don't have a timezone to worry about, and we're done
	if (osync_time_isutc(content) || strcmp(value, "DATE") == 0) {
		// convert vtime to unix and return
		ret = osync_time_vtime2unix(content, 0);
		return ret;
	}

	// retrieve TimezoneID
	const char *timezoneid = osync_xmlfield_get_attr_value("TimezoneID");
	if (timezoneid == NULL) {
		// if we get NULL here, and we've already passed the
		// UTC check, then we are to assume the timezone is
		// the same timezone as the attendee is in at any
		// given moment (RFC2445, 4.3.5).  This floating time
		// changes with the location of the attendee.
		// Since the ATTENDEE field doesn't seem to have any
		// timezone info, assume system localtime.
		struct tm *local = osync_time_vtime2tm(content);
		ret = osync_time_localtm2unix(local);
		g_free(local);
		return ret;
	}

	// load timezone data
	TimezoneSet *tzset = osync_zone_load_tzset(event, timezoneid);
	if (set == NULL) {
		// no timezone with that ID is available
		return -1;
	}

	// retrieve tzoffset
	int tzoffset = osync_zone_tzoffset(tzset, content);
	if (tzoffset == /*FIXME tzoffset can be negative... how do we detect error?*/) {
		osync_zone_free(tzset);
		return -1;
	}
	osync_zone_free(tzset);

	// return unix time
	ret = osync_time_vtime2unix(content, tzoffset);

	osync_trace(TRACE_EXIT, "%s()", __func__);
	return ret;
}
#endif

/*@}*/

