/*
 * libopensync - A synchronization framework
 * Copyright (C) 2007-2008  Daniel Gollub <dgollub@suse.de> 
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

/*
 * Note: in all functions below that require a timezone offset,
 *       they *require* the caller to be completely accurate with it,
 *       including in the DST case.  The result is only as accurate
 *       as the offset you provide.
 */

/* Timeformat helper */
OSYNC_EXPORT char *osync_time_timestamp(const char *vtime);
OSYNC_EXPORT char *osync_time_datestamp(const char *vtime); 
OSYNC_EXPORT osync_bool osync_time_isdate(const char *vformat);
OSYNC_EXPORT osync_bool osync_time_isutc(const char *vformat);
//char *osync_time_set_vtime(const char *vtime, const char *time, osync_bool is_utc);

/* String <-> struct converters, no smarts */
OSYNC_EXPORT struct tm *osync_time_vtime2tm(const char *vtime);
OSYNC_EXPORT char *osync_time_tm2vtime(const struct tm *time, osync_bool is_utc);

/* Timetype helper */
OSYNC_EXPORT time_t osync_time_vtime2unix(const char *vtime, int offset);
OSYNC_EXPORT char *osync_time_unix2vtime(const time_t *timestamp);

/* Unix time_t converters */
OSYNC_EXPORT time_t osync_time_localtm2unix(const struct tm *tmtime); // aka, mktime()
OSYNC_EXPORT time_t osync_time_utctm2unix(const struct tm *tmtime);     // actually useful!
OSYNC_EXPORT struct tm *osync_time_unix2localtm(const time_t *timestamp);// aka, localtime()
OSYNC_EXPORT struct tm *osync_time_unix2utctm(const time_t *timestamp);  // aka, gmtime()

/* Timezone helper */
/* System Timezone-Reliable Helpers */
OSYNC_EXPORT int osync_time_timezone_diff(const struct tm *local);
OSYNC_EXPORT struct tm *osync_time_tm2utc(const struct tm *ltime, int offset);
OSYNC_EXPORT struct tm *osync_time_tm2localtime(const struct tm *utime, int offset);
OSYNC_EXPORT char *osync_time_vtime2utc(const char* localtime, int offset);
OSYNC_EXPORT char *osync_time_vtime2localtime(const char* utc, int offset);

/* XXX This functions should only be used as workaround for plugins which
   only supports localtime without any timezone information. */
OSYNC_EXPORT char *osync_time_vcal2localtime(const char *vcal);
OSYNC_EXPORT char *osync_time_vcal2utc(const char *vcal);

/* Alarm Duration Timeformat helper  */
OSYNC_EXPORT char *osync_time_sec2alarmdu(int seconds);
OSYNC_EXPORT int osync_time_alarmdu2sec(const char *alarm);

/* Timezone ID helper */
// FIXME: how do we handle iCal weekday strings with multiple days?
// something like the following?
//int osync_time_str2wday(const char *weekday, int *wdaymap);
OSYNC_EXPORT int osync_time_str2wday(const char *weekday);
OSYNC_EXPORT struct tm *osync_time_relative2tm(const char *byday, const int bymonth, const int year);
OSYNC_EXPORT int osync_time_utcoffset2sec(const char *offset);

/* Recurrence API */
//struct OSyncRecur *osync_recur_parse_rules(OSyncXMLFormat *event);
	// hmmm, recurrence rules only pertain to a certain
	// field.... how do we get that data?  can we get a sub
	// field of a field?
//struct tm *osync_recur_get_next(OSyncRecur *recur, struct tm *current);

/* Smart Timezone Helpers */
/*
struct tm *osync_time_dstchange(xmlNode *dstNode);
osync_bool osync_time_isdst(const char *vtime, xmlNode *tzid);
int osync_time_tzoffset(const char *vtime, xmlNode *tz);
char *osync_time_tzid(xmlNode *tz);
char *osync_time_tzlocation(xmlNode *tz);
xmlNode *osync_time_tzinfo(xmlNode *root, const char *tzid);
char *osync_time_tzlocal2utc(xmlNode *root, const char *field);
*/

//struct tm *osync_time_dstchange(OSyncXMLFormat *event, OSyncXMLField *dateTimeContent);
//osync_bool osync_time_isdst(OSyncXMLFormat *event, OSyncXMLField *dateTimeContent);
//int osync_time_tzoffset(const char *vtime, xmlNode *tz);
//char *osync_time_tzid(xmlNode *tz);
//char *osync_time_tzlocation(xmlNode *tz);
//xmlNode *osync_time_tzinfo(xmlNode *root, const char *tzid);
//char *osync_time_tzlocal2utc(xmlNode *root, const char *field);

OSYNC_EXPORT time_t osync_time_xml2unix(OSyncXMLFormat *event, OSyncXMLField *dateTimeContent);

#endif /*_OPENSYNC_TIME_H_*/
