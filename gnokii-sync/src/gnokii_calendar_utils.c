/***************************************************************************
 *   Copyright (C) 2006 by Daniel Gollub                                   *
 *                            <dgollub@suse.de>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "gnokii_sync.h"

/* Reads a iCal duration string and convert it to seconds.
 * TODO: It only handles "Before Start" Events" - change this :)
 *
 * iCal duration string looks like this:
 * PTOs		(0 seconds after event start)
 * -P2DT12H30M	(2 days, 12hours and 30 minuets before event start)
 *
 * Returns: seconds before event
 */
int gnokii_util_alarmevent2secs(const char *alarm) {

	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, alarm);

        int i, secs, digits;
        int is_digit = 0;
	int sign = -1;	// when ical stamp doesn't start with '-' => seconds after event
        int days = 0, weeks = 0, hours = 0, minutes = 0, seconds = 0;

	        for (i=0; i < (int) strlen(alarm); i++) {

                switch (alarm[i]) {
                        case '-':
				sign = 1; // seconds before event - so we haven't to change the sign
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

	secs = secs * sign;	// change sign if the alarm is in seconds after event (no leading '-')

	osync_trace(TRACE_EXIT, "%s: %i", __func__, secs);
        return secs;
}

/* Converts seconds into to iCal duration string.
 * When parameter secs_before_event is 0 then "PT0S" will be returned.
 * This will remind on event start.
 * 
 * Returns: iCal duration string
 * ReturnVal: (for example) "-PT2DT4H30M"
 */
char *gnokii_util_secs2alarmevent(int secs_before_event) {

	osync_trace(TRACE_ENTRY, "%s(%i)", __func__, secs_before_event);

        char *tmp = NULL;
	char *prefix = NULL;

	if (!secs_before_event) { 
		tmp = g_strdup("PT0S");
		goto end;
	}

	if (secs_before_event > 0) { 
		prefix = g_strdup("-P");
	} else {
		prefix = g_strdup("P");
		secs_before_event *= -1;
	}

	// Days 
        if (!(secs_before_event % (3600 * 24))) {
               tmp = g_strdup_printf("%s%iD", prefix, secs_before_event / (3600 * 24));
	       goto end;
	}

	// Hours
        if (!(secs_before_event % 3600)) {
                tmp = g_strdup_printf("%sT%iH", prefix, secs_before_event / 3600);
		goto end;
	}

	// Minutes
        if (!(secs_before_event % 60) && secs_before_event < 3600) {
                tmp = g_strdup_printf("%sT%iM", prefix, secs_before_event / 60);
		goto end;
	}

        if (secs_before_event > 60)
                tmp = g_strdup_printf("%sT%iM", prefix, secs_before_event / 60);

        if (secs_before_event > 3600)
                tmp = g_strdup_printf("%sT%iH%iM", prefix, secs_before_event / 3600,
                                (secs_before_event % 3600) / 60);

        if (secs_before_event > (3600 * 24))
               tmp = g_strdup_printf("%s%iDT%iH%iM", prefix, secs_before_event / (3600 * 24),
                               secs_before_event % (3600 * 24) / 3600,
                               ((secs_before_event % (3600 * 24) % 3600)) / 60);

end:
	g_free(prefix);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, tmp);
        return tmp;
}

/*! @brief Function converts struct tm to gn_timestamp
 *
 * @param timetm The struct tm which gets converted
 * @returns gn_timestamp
 */ 
gn_timestamp gnokii_util_tm2timestamp(const struct tm *timetm) {
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, timetm);

	gn_timestamp timestamp;

	timestamp.year = timetm->tm_year + 1900;
	timestamp.month = timetm->tm_mon + 1;
	timestamp.day = timetm->tm_mday;
	timestamp.hour = timetm->tm_hour;
	timestamp.minute = timetm->tm_min;
	timestamp.second = 0;
	timestamp.timezone = 0;

	osync_trace(TRACE_EXIT, "%s()", __func__);
	return timestamp;
}

/*! @brief Function converts struct tm to gn_timestamp
 *
 * @param timetm The struct tm which gets converted
 * @returns gn_timestamp (the caller is responsible for freeing)
 */ 
struct tm *gnokii_util_timestamp2tm(const gn_timestamp *timestamp) {
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, timestamp);

	struct tm *timetm = g_malloc0(sizeof(struct tm));

	tzset();
	timetm->tm_year = timestamp->year - 1900;
	timetm->tm_mon = timestamp->month - 1;
	timetm->tm_mday = timestamp->day;
	timetm->tm_hour = timestamp->hour;
	timetm->tm_min = timestamp->minute;
	timetm->tm_sec = 0;
	timetm->tm_wday = 0;
	timetm->tm_yday = 0;
	timetm->tm_isdst = -1;

	osync_trace(TRACE_EXIT, "%s()", __func__);
	return timetm;
}

/* Convert a time_t struct to a gnokii timestamp (gn_timestamp).
 * Seconds will be ignored - cellphone cannot handle seconds.
 * 
 * Returns: gn_timestamp
 */
gn_timestamp gnokii_util_unix2timestamp(time_t time) {

	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, time);

	struct tm *date = NULL;
	gn_timestamp timestamp;

	tzset();
	date = localtime(&time);	

	timestamp = gnokii_util_tm2timestamp(date);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return timestamp;
}



/* Converts gnokii timestamp (gn_timestamp) to time_t struct.
 * 
 * Returns: time_t of event 
 */
time_t gnokii_util_timestamp2unix(gn_timestamp *timestamp) {

	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, timestamp);
	struct tm *date = NULL; 
	time_t timet;

	date = gnokii_util_timestamp2tm(timestamp);
	timet = mktime(date);
	g_free(date);

	osync_trace(TRACE_EXIT, "%s: %ul", __func__, timet);
	return timet; 
}

/* Converts gnokii calendar note type into a string.
 * 
 * Returns: Event type string
 */ 
char *gnokii_util_caltype2string(gn_calnote_type type) {

	osync_trace(TRACE_ENTRY, "%s(%u)", __func__, type);

	char *tmp = NULL;

	switch (type) {
		case GN_CALNOTE_MEETING:
			tmp = g_strdup("Meeting");
			break;
		case GN_CALNOTE_CALL:
			tmp = g_strdup("Calling");
			break;
		case GN_CALNOTE_BIRTHDAY:
			tmp = g_strdup("Birthday");
			break;
		case GN_CALNOTE_REMINDER:
			tmp = g_strdup("Reminder");
			break;
		case GN_CALNOTE_MEMO:			
			tmp = g_strdup("Memo");
			break;
	}

	osync_trace(TRACE_EXIT, "%s: %s", __func__, tmp);
	return tmp;
}	

/* Gets day of week from  time_t struct as string.
 *
 * Returns: day string (SU, MO, TU, WE, ...) 
 */
char *gnokii_util_unix2wday(const time_t *date) {

	osync_trace(TRACE_ENTRY, "%s(%i)", __func__, *date);

	struct tm tmp_date = *localtime(date);
	char *day_string = NULL;

	switch (tmp_date.tm_wday) {
		case 0:
			day_string = strdup("SU");
			break;
		case 1:
			day_string = strdup("MO");
			break;
		case 2:
			day_string = strdup("TU");
			break;
		case 3:
			day_string = strdup("WE");
			break;
		case 4:
			day_string = strdup("TH");
			break;
		case 5:
			day_string = strdup("FR");
			break;
		case 6:		
			day_string = strdup("SA");
			break;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return day_string;
}

/* Check for valid phone number
 *
 * Returns: osync_bool
 * ReturnValue: true - valid number
 * ReturnValue: false - invalid number
 */ 
osync_bool gnokii_util_valid_number(const char *number) {

	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, number);

	int i;
	int len = (int) strlen(number);

	for (i=0; i < len; i++) {
		switch (number[i]) {
			// valid chars
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
			case '+':	
			case '*':
			case '#':
			case 'p':
			case 'w':	
				break;
			default:
				// invalid char...
				return FALSE;	
				break;
		}
	}

	osync_trace(TRACE_EXIT, "%s: valid number", __func__);
	return TRUE;
}


/* Find a calendar type which fits. 
 *
 * Returns: gn_calnote_type
 */
gn_calnote_type gnokii_util_calendar_type(gn_calnote *calnote, osync_bool alldayevent) {
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, calnote, alldayevent);

	gn_calnote_type type;

	// calnote type
	type = GN_CALNOTE_MEETING;

	// FIXME this breaks the hash calculation - modified after 2nd sync(?)
	if (!calnote->end_time.year && !alldayevent 
			&& gnokii_util_valid_number(calnote->phone_number))
		type = GN_CALNOTE_CALL;

	if (calnote->end_time.year && alldayevent)
		type = GN_CALNOTE_MEMO;

	if (!calnote->end_time.year && !alldayevent)
		type = GN_CALNOTE_REMINDER;

	if (strlen(calnote->mlocation) && !alldayevent)
		type = GN_CALNOTE_MEETING;

	osync_trace(TRACE_EXIT, "%s: %i", __func__, type);
	return type;
}

