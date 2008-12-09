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

#include <time.h>

#include "opensync.h"
#include "opensync_time.h"
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

/*****************************************************************************/

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

 
char *osync_time_timestamp(const char *vtime)
{
	char *timestamp;
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, vtime);

	timestamp = osync_time_timestamp_remove_dash(vtime);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, timestamp);
	return timestamp;
}

 
char *osync_time_datestamp(const char *vtime)
{
	char *tmp;
	const char *p;
	GString *str = g_string_new ("");

	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, vtime);

	tmp = osync_time_timestamp_remove_dash(vtime); 

	for (p=tmp; *p && *p != 'T'; p++)
		str = g_string_append_c (str, *p);

	free(tmp);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, str->str);
	return (char*) g_string_free(str, FALSE);
}


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
	char *tmp = NULL;
	osync_trace(TRACE_ENTRY, "%s(%s, %s)", __func__, vtime, time);
	
	// TODO

	osync_trace(TRACE_EXIT, "%s: %s", __func__, tmp);
	return tmp;
}
#endif

/*****************************************************************************/

struct tm *osync_time_vtime2tm(const char *vtime)
{
	struct tm *utime = g_malloc0(sizeof(struct tm));
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, vtime);

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

char *osync_time_tm2vtime(const struct tm *time, osync_bool is_utc)
{
	GString *vtime = g_string_new("");
	struct tm my_time = *time;
	const char *tz = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, time, is_utc);

	/* ask C library to clean up any anomalies */
	if (is_utc) {
		tz = g_getenv("TZ");
		putenv("TZ=Etc/UTC");
	}
	mktime(&my_time);
	if(is_utc) {
		if (tz) {
			g_setenv("TZ", tz, TRUE);
		} else {
			g_unsetenv("TZ");
		}
	}

	g_string_printf(vtime, "%04d%02d%02dT%02d%02d%02d",
				my_time.tm_year + 1900, my_time.tm_mon + 1, my_time.tm_mday,
				my_time.tm_hour, my_time.tm_min, my_time.tm_sec);

	if (is_utc)
		vtime = g_string_append(vtime, "Z");

	osync_trace(TRACE_EXIT, "%s: %s", __func__, vtime->str);
	return g_string_free(vtime, FALSE);
}

/*****************************************************************************/

time_t osync_time_vtime2unix(const char *vtime, int offset)
{
	struct tm *utime = NULL; 
	time_t timestamp;
	char *utc = NULL;
	osync_trace(TRACE_ENTRY, "%s(%s, %i)", __func__, vtime, offset);

	utc = osync_time_vtime2utc(vtime, offset);
	utime = osync_time_vtime2tm(utc);
	timestamp = osync_time_utctm2unix(utime);

	g_free(utc);
	g_free(utime);

	osync_trace(TRACE_EXIT, "%s: %lu", __func__, timestamp);
	return timestamp;
}


char *osync_time_unix2vtime(const time_t *timestamp)
{
	char *vtime;
	struct tm utc;
	osync_trace(TRACE_ENTRY, "%s(%lu)", __func__, *timestamp);

	gmtime_r(timestamp, &utc);
	vtime = osync_time_tm2vtime(&utc, TRUE);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, vtime);
	return vtime;
}


/*
 * Unix time_t converters
 */

time_t osync_time_localtm2unix(const struct tm *localtime)
{
	time_t timestamp;
	struct tm *tmp = g_malloc0(sizeof(struct tm));

	memcpy(tmp, localtime, sizeof(struct tm));

	tmp->tm_isdst = -1;
	timestamp = mktime(tmp);

	g_free(tmp);

	return timestamp; 
}
 
time_t osync_time_utctm2unix(const struct tm *utctime)
{
	time_t timestamp;

#if 1
	struct tm *tmp = g_malloc0(sizeof(struct tm));
	struct tm localnow;
	struct tm check;
	int tzdiff;

	// calculate local timezone difference... this is only used
	// to reduce the number of loops to find the correct match
	time(&timestamp);
	localtime_r(&timestamp, &localnow);
	tzdiff = osync_time_timezone_diff(&localnow);

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

struct tm *osync_time_unix2localtm(const time_t *timestamp)
{
	struct tm *ptr_tm = g_malloc0(sizeof(struct tm));

	localtime_r(timestamp, ptr_tm);

	return ptr_tm;
}

struct tm *osync_time_unix2utctm(const time_t *timestamp)
{
	struct tm *ptr_tm = g_malloc0(sizeof(struct tm));

	gmtime_r(timestamp, ptr_tm);

	return ptr_tm;
}

/*****************************************************************************/
  
int osync_time_timezone_diff(const struct tm *local)
{
	struct tm utime;
	unsigned int lsecs, usecs;
	long zonediff, daydiff = 0;
	time_t timestamp;
	osync_trace(TRACE_ENTRY, "%s()", __func__);

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
 
struct tm *osync_time_tm2utc(const struct tm *ltime, int offset)
{
	struct tm *tmtime = g_malloc0(sizeof(struct tm));
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, ltime, offset);

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
 
char *osync_time_vtime2utc(const char* localtime, int offset)
{
	char *utc = NULL; 
	struct tm *tm_local = NULL, *tm_utc = NULL;
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, localtime);

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

int osync_time_utcoffset2sec(const char *offset)
{
	char csign = 0;
	int seconds = 0, sign = 1;
	int hours = 0, minutes = 0;
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, offset);

	sscanf(offset, "%c%2d%2d", &csign, &hours, &minutes);

	if (csign == '-')
		sign = -1;

	seconds = (hours * 3600 + minutes * 60) * sign; 

	osync_trace(TRACE_EXIT, "%s: %i", __func__, seconds);
	return seconds;
}

/*****************************************************************************/

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
		gssize pos = 0; 
                struct tm *tm_stamp = NULL;
		res += strlen(field);

		for (i=0; res[i] != '\n' && res[i] != '\r'; i++)
			stamp = g_string_append_c(stamp, res[i]);

		pos = res - entry->str; 
		entry = g_string_erase(entry, pos, i);

		// Get System offset to UTC
		tm_stamp = osync_time_vtime2tm(stamp->str);
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


char *osync_time_vcal2localtime(const char *vcal)
{
	return _convert_entry(vcal, FALSE);
}


char *osync_time_vcal2utc(const char *vcal)
{
	return _convert_entry(vcal, TRUE);
}

/*****************************************************************************/

char *osync_time_sec2alarmdu(int seconds)
{
        char *tmp = NULL;
	char *prefix = NULL;
	osync_trace(TRACE_ENTRY, "%s(%i)", __func__, seconds);

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

int osync_time_alarmdu2sec(const char *alarm)
{
        int i, secs, digits = 0;
        int is_digit = 0;
	int sign = 1;	// when ical stamp doesn't start with '-' => seconds after event
        int days = 0, weeks = 0, hours = 0, minutes = 0, seconds = 0;
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, alarm);

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

                                if (sscanf((char*)(alarm+i),"%d",&digits) == EOF)
					return -1;

                                is_digit = 1;
                                break;
                }
	}

        secs = (weeks * 7 * 24 * 3600) + (days * 24 * 3600) + (hours * 3600) + (minutes * 60) + seconds;

	secs = secs * sign;	// change sign if the alarm is in seconds before event (leading '-')

	osync_trace(TRACE_EXIT, "%s: %i", __func__, secs);
        return secs;
}

/*****************************************************************************/

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

