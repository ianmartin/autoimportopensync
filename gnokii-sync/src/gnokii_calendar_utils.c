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
int gnokii_util_alarmevent2secs(char *alarm) {

	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, alarm);

        int i, secs, digits;
        int is_digit = 0;
        int days = 0, weeks = 0, hours = 0, minutes = 0, seconds = 0;

	        for (i=0; i < (int) strlen(alarm); i++) {

                switch (alarm[i]) {
                        case '-':
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

	if (!secs_before_event)
		return g_strdup("PT0S");

        if (!(secs_before_event % (3600 * 24))) 
               return g_strdup_printf("-P%iD", secs_before_event / (3600 * 24));

        if (!(secs_before_event % 3600))
                return g_strdup_printf("-PT%iH", secs_before_event / 3600);

        if (!(secs_before_event % 60) && secs_before_event < 3600)
                return g_strdup_printf("-PT%iM", secs_before_event / 60);


        if (secs_before_event > 60)
                tmp = g_strdup_printf("-PT%iM", secs_before_event / 60);

        if (secs_before_event > 3600)
                tmp = g_strdup_printf("-PT%iH%iM", secs_before_event / 3600,
                                (secs_before_event % 3600) / 60);

        if (secs_before_event > (3600 * 24))
               tmp = g_strdup_printf("-P%iDT%iH%iM", secs_before_event / (3600 * 24),
                               secs_before_event % (3600 * 24) / 3600,
                               ((secs_before_event % (3600 * 24) % 3600)) / 60);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, tmp);
        return tmp;
}

/* Convert a time_t struct to a gnokii timestamp (gn_timestamp).
 * Seconds will be ignored - cellphone cannot handle seconds.
 * 
 * Returns: gn_timestamp
 */
gn_timestamp gnokii_util_ttm2timestamp(time_t time) {

	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, time);

	struct tm *date = (struct tm *) malloc(sizeof(struct tm)); 
	gn_timestamp timestamp;

	memset(&timestamp, 0, sizeof(gn_timestamp));

	tzset();
	date = localtime(&time);	

	timestamp.second = 0; 		// cellphone calendar doesn't support seconds
	timestamp.minute = date->tm_min;
	timestamp.hour = date->tm_hour;
	timestamp.day = date->tm_mday;
	timestamp.month = date->tm_mon + 1;
	timestamp.year = date->tm_year + 1900;

	osync_trace(TRACE_EXIT, "%s: %p", __func__, timestamp);
	return timestamp;
}

/* Converts gnokii timestamp (gn_timestamp) to time_t struct.
 * 
 * Returns: time_t of event 
 */
time_t gnokii_util_timestamp2ttm(gn_timestamp timestamp) {

	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, timestamp);
	struct tm *date = (struct tm *) malloc(sizeof(struct tm));

	tzset();
	date->tm_sec = timestamp.second;
	date->tm_min = timestamp.minute;
	date->tm_hour = timestamp.hour;
	date->tm_mday = timestamp.day;
	date->tm_mon = timestamp.month;
	date->tm_mon -= 1;
	date->tm_year = timestamp.year;
	date->tm_year -= 1900;
	date->tm_wday = 0;
	date->tm_yday = 0;
	date->tm_isdst = -1;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return mktime(date);
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
char *gnokii_util_ttm2wday(const time_t *date) {

	struct tm *tmp_date = localtime(date);
	char *day_string = NULL;

	switch (tmp_date->tm_wday) {
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


	return day_string;
}

