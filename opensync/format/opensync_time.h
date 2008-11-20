/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2006-2008 Daniel Gollub <dgollub@suse.de>
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

#ifndef _OPENSYNC_TIME_H_
#define _OPENSYNC_TIME_H_

/**
 * @defgroup OSyncTimeAPI OpenSync Time
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncTimeAPI
 * 
 */

/**
 * @defgroup OSyncTimeFormatting Time formatting helpers
 * @ingroup OSyncTimeAPI
 * @brief Helper functions for formatting time strings, stripping out
 *	invalid characters, etc.
 */
/*@{*/

/* 
 * Time formatting helper
 */

/*! @brief Function returns a date-timestamp in OSyncTime Spec format
 * 
 * @param vtime The timestamp which gets converted to a valid osync date-timestamp
 * @returns vtime date-timestring (the caller is responsible for freeing)
 */
OSYNC_EXPORT char *osync_time_timestamp(const char *vtime);

/*! @brief Function returns a date without timestamp in OSyncTime Spec format
 * 
 * @param vtime The timestamp which gets converted to a single datestamp
 * @returns valid single datestamp YYYYMMDD (the caller is responsible for freeing) 
 */
OSYNC_EXPORT char *osync_time_datestamp(const char *vtime); 

/*! @brief Function returns TRUE if vtime is a valid datestamp (YYYYMMDD)
 * 
 * @returns FALSE if vtime includes a timestamp, TRUE on a single datestamp
 */
OSYNC_EXPORT osync_bool osync_time_isdate(const char *vformat);

/*! @brief Function returns TRUE if vtime is in UTC (YYYYMMDDTHH:MM:SSZ)
 * 
 * @returns FALSE if vtime includes no Zulu, TRUE if the timestamp is UTC
 */
OSYNC_EXPORT osync_bool osync_time_isutc(const char *vformat);

/*@}*/

/*****************************************************************************/

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

/*! @brief Function converts vtime to tm struct
 * 
 * @param vtime The formatted timestamp (YYYYMMDDTHHMMSS)
 * @returns struct tm (caller is responsible for freeing)
 */
OSYNC_EXPORT struct tm *osync_time_vtime2tm(const char *vtime);

/*! @brief Function converts struct tm in vtime string
 * 
 * YYYYMMDDTHHMMSS[Z]
 * Returned timezone is equal to the timezone of struct tm.  
 *
 * @param time The tm struct which gets converted
 * @param is_utc If struct tm is UTC time is_utc have to be TRUE
 * @returns vtime formatted as YYYYMMDDTHHMMSS[Z] (caller is responsible for freeing)
 */
OSYNC_EXPORT char *osync_time_tm2vtime(const struct tm *time, osync_bool is_utc);

/*@}*/

/*****************************************************************************/

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
OSYNC_EXPORT time_t osync_time_vtime2unix(const char *vtime, int offset);

/*! @brief Function converts unix timestamp to vtime in UTC
 *
 * @param timestamp The unix timestamp which gets converted 
 * @returns vtime formatted as YYYYMMDDTHHMMSSZ (caller is responsible for freeing)
 */
OSYNC_EXPORT char *osync_time_unix2vtime(const time_t *timestamp);

/* Unix time_t converters */

/*! @brief Function converts struct tm, in localtime, to unix timestamp.
 *		This is the same as calling mktime(), except that
 *		localtime is not modified, and tm_isdst is always
 *		forced to -1. Aka, mktime().
 *
 * @param localtime The struct tm, in localtime, which gets converted
 * @returns time_t (in UTC of course)
 */ 
OSYNC_EXPORT time_t osync_time_localtm2unix(const struct tm *localtime);

/*! @brief Function converts struct tm, in utc, to unix timestamp.
 *
 * @param utctime The struct tm, in utc, which gets converted
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
OSYNC_EXPORT time_t osync_time_utctm2unix(const struct tm *utctime);

/*! @brief Function converts unix timestamp to struct tm in localtime.
 *		This is the same as calling localtime_r(), except you
 *		have to free the returned value. Aka, localtime().
 * 
 * @param timestamp The unixtimestamp which gets converted
 * @returns: struct tm (in localtime) (Caller is responsible for freeing!)
 */ 
OSYNC_EXPORT struct tm *osync_time_unix2localtm(const time_t *timestamp);

/*! @brief Function converts unix timestamp to struct tm in utc.
 *		This is the same as calling gmtime_r(), except you
 *		have to free the returned value. Aka, gmtime().
 * 
 * @param timestamp The unixtimestamp which gets converted
 * @returns: struct tm (in UTC) (Caller is responsible for freeing)
 */ 
OSYNC_EXPORT struct tm *osync_time_unix2utctm(const time_t *timestamp);

/*@}*/

/*****************************************************************************/

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

/*! @brief Function gets offset of parameter time between UTC and
 *	localtime in seconds east of UTC.  (i.e. east is positive,
 *	west is negative)
 * 
 * @param local The point in time when the offset have to be calculated,
 *	specified in localtime (need for CEST/CET)
 * @returns Seconds of timezone offset
 */
OSYNC_EXPORT int osync_time_timezone_diff(const struct tm *local);

/*! @brief Function converts (struct tm) ltime from localtime to UTC.
 *         Paramter offset is used as UTC offset.  Note that _only_ the
 *         following fields can be relied upon in the result:
 *         tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year.
 * 
 * @param ltime The struct tm which gets converted to UTC timezone
 * @param offset Seconds of UTC offset, in seconds east of UTC.
 * @returns struct tm in UTC (caller is responsible for freeing)
 */
OSYNC_EXPORT struct tm *osync_time_tm2utc(const struct tm *ltime, int offset);

/*! @brief Function converts (struct tm) utime from UTC to localtime 
 *         Paramter offset is used as UTC offset.  Note that _only_ the
 *         following fields can be relied upon in the result:
 *         tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year.
 * 
 * @param utime The struct tm which gets converted to localtime
 * @param offset Seconds of UTC offset, in seconds east of UTC.
 * @returns struct tm in localtime (caller is responsible for freeing)
 */
OSYNC_EXPORT struct tm *osync_time_tm2localtime(const struct tm *utime, int offset);

/*! @brief Functions converts a localtime vtime stamp to a UTC vtime stamp
 *
 * @param localtime The local timestamp in vtime format
 * @param offset Seconds of UTC offset, in seconds east of UTC.
 * @returns vtime in UTC timezone (caller is responsible for freeing)
 */
OSYNC_EXPORT char *osync_time_vtime2utc(const char* localtime, int offset);

/*! @brief Functions converts a UTC vtime stamp to a localtime vtime stamp
 * 
 * @param utc The timestap in UTC timezone which gets converted to localtime 
 * @param offset The offset in seconds between UTC and localtime
 * @returns vtime in local  timezon (caller is preponsible for freeing) 
 */
OSYNC_EXPORT char *osync_time_vtime2localtime(const char* utc, int offset);

/*! @brief Function converts UTC offset string in offset in seconds
 *
 * @param offset The offset string of the form a timezone field (Example +0200) 
 * @returns seconds of UTC offset 
 */ 
OSYNC_EXPORT int osync_time_utcoffset2sec(const char *offset);

/*@}*/

/*****************************************************************************/

/**
 * @defgroup OSyncTimeBackwardCompatibility Backward compatibility functions.
 * @ingroup OSyncTimeAPI
 * @brief These functions should only be used as workaround for plugins
 *	which only support localtime without any timezone information.
 * 
 *      XXX This functions should only be used as workaround for plugins which
 *      only supports localtime without any timezone information.
 */
/*@{*/


/*! @brief Functions converts timestamps of vcal to localtime
 * 
 * @param vcal The vcalendar which has to be converted.
 * @return modified vcalendar with local timestamps (related to system time) 
 */ 
OSYNC_EXPORT char *osync_time_vcal2localtime(const char *vcal);

/*! @brief Functions converts timestamps of vcal to UTC
 * 
 * @param vcal The vcalendar which has to be converted.
 * @return modified vcalendar with UTC timestamps (related to system time) 
 */ 
OSYNC_EXPORT char *osync_time_vcal2utc(const char *vcal);


/*@}*/

/*****************************************************************************/

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
OSYNC_EXPORT char *osync_time_sec2alarmdu(int seconds);

/*! @brief Functions converts alarm duration event to seconds needed for reminder of vcal/ical
 * 
 * TODO: Test support for ALARM after/before end and after start
 *
 * @param alarm 
 * @returns seconds of alarm and duration
 */ 

OSYNC_EXPORT int osync_time_alarmdu2sec(const char *alarm);

/*@}*/

/*****************************************************************************/

/**
 * @defgroup OSyncTimeRecurrence Recurring time calculators.
 * @ingroup OSyncTimeAPI
 * @brief Functions related to calculating recurring dates.
 * 
 */
/*@{*/

/*! @brief Function converts a week day string to the struct tm wday integer. 
 *
 * FIXME: how do we handle iCal weekday strings with multiple days?
 * something like the following?
 * int osync_time_str2wday(const char *weekday, int *wdaymap);
 *
 * @param swday string of the week day
 * @returns integer of the weekday (Sunday = 0), or -1 on error
 */ 
OSYNC_EXPORT int osync_time_str2wday(const char *swday);

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
OSYNC_EXPORT struct tm *osync_time_relative2tm(const char *byday, const int bymonth, const int year);

/*@}*/

#endif /*_OPENSYNC_TIME_H_*/
