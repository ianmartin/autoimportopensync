/*
 * xmlformat-vcalendar - common code for xmlformat-vevent*, -vnote*, -vtodo*
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007  Daniel Gollub <dgollub@suse.de>
 * Copyright (C) 2007  Christopher Stender <cstender@suse.de>
 * Copyright (C) 2007  Jerry Yu <jijun.yu@sun.com>
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

#include "xmlformat-vcalendar.h"

/* Briefing:
 *
 * use handle_vcal* for vCalendar handlers
 * use handle_* for vCalendar and iCalendar handlers
 *
 */



/*
 * Basic Recurrence Rules
 *
 * The functions below are necessary for converting a vCalendar Recurrence Rule
 * to xmlformat-event.
 *
 * Description:
 * convert_vcal_rrule_frequency      get frequency value
 * convert_vcal_rrule_freqmod        get frequency modifier
 * convert_vcal_rrule_countuntil     get count or until value
 * convert_vcal_rrule_to_xml         get interval and call functions above
 *
 */

static int convert_vcal_rrule_frequency(OSyncXMLField *xmlfield, const char *rule)
{
        int frequency_state = 0;
	char next = *(rule + 1);
	char *frequency;

	/* get frequency: only D(1), W(2), MP(3), MD(4), YD(5) and YM(6) are allowed */
	if (*rule == 'D') {
        	frequency_state = 1;
                frequency = "DAILY";
	} else if (*rule == 'W') {
                frequency_state = 2;
                frequency = "WEEKLY";
	} else if (*rule == 'M' && next == 'P') {
                frequency_state = 3;
		frequency = "MONTHLY";
	} else if (*rule == 'M' && next == 'D') {
	        frequency_state = 4;
		frequency = "MONTHLY";
	} else if (*rule == 'Y' && next == 'D') {
		frequency_state = 5;
		frequency = "YEARLY";
	} else if (*rule == 'Y' && next == 'M') {
		frequency_state = 6;
		frequency = "YEARLY";
	} else {
		osync_trace(TRACE_INTERNAL, "invalid or missing frequency");
		return -1;
	}

	/* set Frequency */
	osync_xmlfield_set_key_value(xmlfield, "Frequency", frequency);

	return frequency_state;
}

static char *convert_vcal_rrule_freqmod(OSyncXMLField *xmlfield, gchar **rule, int size, int freqstate)
{
	int i;
	GString *fm_buffer = g_string_new("");

	/* for each modifier do... */
	for(i=1; i < size-1; i++) {

		int count;
		char sign;

		if(fm_buffer->len > 0)
			g_string_append(fm_buffer, ",");

		/* check frequency modifier */
		if (sscanf(rule[i], "%d%c" , &count, &sign) == 2) {

			/* we need to convert $COUNT- to -$COUNT -> RFC2445 */
			if (sign == '-')
				count = -count;

			g_string_append_printf(fm_buffer, "%d", count);

			/* if first freqmod is "(-)2" and second one is "TU" we
			 * have to convert it to 2TU */
			if (i < size-2 && !sscanf(rule[i+1], "%d", &count)) {
				g_string_append_printf(fm_buffer, "%s", rule[i+1]);
				i++;
			}

		} else {
			/* e.g. Day or 'LD' (Last day) */
			g_string_append(fm_buffer, rule[i]);
		}
	}

	return g_string_free(fm_buffer, FALSE);
}

static void convert_vcal_rrule_countuntil(OSyncXMLField *xmlfield, const char *duration_block)
{
	int count;
	int offset = 0; 
	char *until = NULL;

	/* COUNT: #20 */
	if (sscanf(duration_block, "#%d", &count) == 1) {
		osync_xmlfield_set_key_value(xmlfield, "Count", duration_block+1);
		return;
	}

	/* UNTIL: 20070515T120000Z */
	if (!osync_time_isdate(duration_block)) {

		/* Check if this duration_block is a localtime timestamp.
		 * If it is not UTC change the offset from 0 to the system UTC offset.·
		 * vcal doesn't store any TZ information. This means the device have to be
		 * in the same Timezone as the host.
		 */

		if (!osync_time_isutc(duration_block)) {
			struct tm *ttm = osync_time_vtime2tm(duration_block);
			offset = osync_time_timezone_diff(ttm);
			g_free(ttm);
		}

		until = osync_time_vtime2utc(duration_block, offset);
	} else {
		until = g_strdup(duration_block);
	}

	osync_xmlfield_set_key_value(xmlfield, "Until", until);

	g_free(until);
}

static void convert_vcal_rrule_to_xml(OSyncXMLField *xmlfield, const char *rule)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, xmlfield, rule);

	int frequency_state = 0, counter = 0;
	char *frequency_block = NULL, *freq_mod = NULL, *duration_block = NULL;

	gchar** blocks = g_strsplit(rule, " ", 256);

	/* count blocks, e.g. (W2 TU TH #5) -> 4 blocks */
	for(; blocks[counter]; counter++);

	frequency_block = blocks[0];
	duration_block = blocks[counter-1];

	/* get frequency */
	frequency_state = convert_vcal_rrule_frequency(xmlfield, frequency_block);

	/* get count or until value */
	convert_vcal_rrule_countuntil(xmlfield, duration_block);

	/* the interval value is at the end of the frequency_block */
	frequency_block++;

	if (frequency_state > 2)
		frequency_block++;

	/* set Interval */
	osync_xmlfield_set_key_value(xmlfield, "Interval", frequency_block);

	/* get Frequency modifier (ByDay, ByMonthDay, etc. */
	if (counter > 2)
		freq_mod = convert_vcal_rrule_freqmod(xmlfield, blocks, counter, frequency_state);

	// TODO enum
	/* W(2), MP(3), MD(4), YD(5) and YM(6) */
	switch(frequency_state) {
		case 2:
		case 3:
			osync_xmlfield_set_key_value(xmlfield, "ByDay", freq_mod); 
			break;
		case 4:
			osync_xmlfield_set_key_value(xmlfield, "ByMonthDay", freq_mod);
			break;
		case 5:
			osync_xmlfield_set_key_value(xmlfield, "ByYearDay", freq_mod);
			break;
		case 6:
			osync_xmlfield_set_key_value(xmlfield, "ByMonth", freq_mod);
			break;
		default:
			break;
	}

	g_strfreev(blocks);
} 
// End of Basic Recurrence Rule


/* vCal only attributes */
OSyncXMLField *handle_vcal_aalarm_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	osync_trace(TRACE_INTERNAL, "Handling aalarm attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Alarm", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	} 

	osync_xmlfield_set_key_value(xmlfield, "AlarmAction", "AUDIO");
	osync_xmlfield_set_key_value(xmlfield, "AlarmTrigger", vformat_attribute_get_nth_value(attr, 0)); 
	return xmlfield; 
}

OSyncXMLField *handle_vcal_dalarm_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	osync_trace(TRACE_INTERNAL, "Handling dalarm attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Alarm", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	} 

	osync_xmlfield_set_key_value(xmlfield, "AlarmAction", "DISPLAY");
	osync_xmlfield_set_key_value(xmlfield, "AlarmTrigger", vformat_attribute_get_nth_value(attr, 0)); 
	return xmlfield; 
}

OSyncXMLField *handle_vcal_rrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling RecurrenceRule attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "RecurrenceRule", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}

	const char *rule = vformat_attribute_get_nth_value(attr, 0);

        convert_vcal_rrule_to_xml(xmlfield, rule);

	return xmlfield;
}

/* vCal and iCal attributes */
OSyncXMLField *handle_arepeat_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmRepeat", error);
}

OSyncXMLField *handle_atrigger_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmTrigger", error);
}

OSyncXMLField *handle_attach_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Attach", error);
}

OSyncXMLField *handle_attendee_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Attendee", error);
}

OSyncXMLField *handle_created_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Created", error);
}

OSyncXMLField *handle_dtstamp_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "DateCalendarCreated", error);
}

OSyncXMLField *handle_dtstart_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "DateStarted", error);
}

OSyncXMLField *handle_description_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Description", error);
}

OSyncXMLField *handle_summary_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Summary", error);
}

OSyncXMLField *handle_due_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "DateDue", error);
}

OSyncXMLField *handle_priority_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Priority", error);
}

OSyncXMLField *handle_sequence_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Sequence", error);
}

OSyncXMLField *handle_last_modified_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "LastModified", error);
}

OSyncXMLField *handle_geo_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	osync_trace(TRACE_INTERNAL, "Handling Geo attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Geo", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Latitude", vformat_attribute_get_nth_value(attr, 0));
	osync_xmlfield_set_key_value(xmlfield, "Longitude", vformat_attribute_get_nth_value(attr, 1));
	return xmlfield;
}

OSyncXMLField *handle_prodid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "ProductID", error);
}

OSyncXMLField *handle_rdate_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "RecurrenceDate", error);
}

OSyncXMLField *handle_location_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Location", error);
}

// todo only?
OSyncXMLField *handle_completed_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Completed", error);
}

OSyncXMLField *handle_status_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Status", error);
}

OSyncXMLField *handle_exdate_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "ExclusionDate", error);
}

OSyncXMLField *handle_exrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "ExclusionRule", error);
}

// ical, vtodo?
OSyncXMLField *handle_rstatus_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "RStatus", error);
}

OSyncXMLField *handle_related_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Related", error);
}

OSyncXMLField *handle_resources_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Resources", error);
}

OSyncXMLField *handle_dtend_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "DateEnd", error);
}

OSyncXMLField *handle_transp_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "TimeTransparency", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	} 

	const char *transp = vformat_attribute_get_nth_value(attr, 0);
        if (!strcmp(transp, "0") || !strcmp(transp, "OPAQUE")) {
		osync_xmlfield_set_key_value(xmlfield, "Content", "OPAQUE"); 
        } else {
		osync_xmlfield_set_key_value(xmlfield, "Content", "TRANSPARENT"); 
        }

	return xmlfield; 

}

OSyncXMLField *handle_method_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Method", error);
}

// todo only?
OSyncXMLField *handle_percent_complete_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "PercentComplete", error);
}

OSyncXMLField *handle_rrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Recurrence Rule attribute");
	
	OSyncXMLField *xmlfield = NULL;
	GString *frequency = NULL;
	GString *end = NULL;
	GString *interval = NULL;
	GString *modifier = NULL;
	char *modname = NULL;
	osync_bool set_count = FALSE;
	osync_bool set_until = FALSE;
	osync_bool set_extended = FALSE;

	// parse values
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = values->data;
		osync_assert(retstr);

		if (strstr(retstr->str, "FREQ=")) {
			frequency = values->data;
		} else if (strstr(retstr->str, "COUNT=")) {
			end = values->data;
			set_count = TRUE;
		} else if (strstr(retstr->str, "UNTIL=")) {	
			end = values->data;
			set_until = TRUE;
		} else if (strstr(retstr->str, "INTERVAL=")) {
			interval = values->data;
		} else if (strstr(retstr->str, "BYSECOND=")) {
			modifier = values->data;
			modname = "BySecond";
			set_extended = TRUE;
		} else if (strstr(retstr->str, "BYMINUTE=")) {
			modifier = values->data;
			modname = "ByMinute";
			set_extended = TRUE; 
		} else if (strstr(retstr->str, "BYHOUR=")) { 
			modifier = values->data;
			modname = "ByHour";
			set_extended = TRUE; 
		} else if (strstr(retstr->str, "BYDAY=")) { 
			modifier = values->data;
			modname = "ByDay";
		} else if (strstr(retstr->str, "BYMONTHDAY=")) { 
			modifier = values->data;
			modname = "ByMonthDay";
		} else if (strstr(retstr->str, "BYYEARDAY=")) { 
			modifier = values->data;
			modname = "ByYearDay";
		} else if (strstr(retstr->str, "BYWEEKNO=")) { 
			modifier = values->data;
			modname = "ByWeekNo";
			set_extended = TRUE; 
		} else if (strstr(retstr->str, "BYMONTH=")) { 
			modifier = values->data;
			modname = "ByMonth";
		} else if (strstr(retstr->str, "BYSETPOS=")) { 
			modifier = values->data;
			modname = "BySetPos";
			set_extended = TRUE; 
		} else if (strstr(retstr->str, "WKST=")) { 
			modifier = values->data;
			modname = "WKST";
			set_extended = TRUE; 
		}
	}

	// create new xmlfield
	if (set_extended) {	
		xmlfield = osync_xmlfield_new(xmlformat, "RecurrenceRuleExtended", error);
	} else {
		xmlfield = osync_xmlfield_new(xmlformat, "RecurrenceRule", error);
	}
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}

	// set frequency, count/until, interval and modifier
	osync_xmlfield_add_key_value(xmlfield, "Frequency", frequency->str + strlen("FREQ="));
	if (set_count) {
		osync_xmlfield_add_key_value(xmlfield, "Count", end->str + strlen("COUNT="));
	} else if (set_until) {
		osync_xmlfield_add_key_value(xmlfield, "Until", end->str + strlen("UNTIL="));
	} else {
		// if neither COUNT nor UNTIL is set the event should be appear forever -> count = 0
		osync_xmlfield_add_key_value(xmlfield, "Count", "0");
	}
	if (interval != NULL) {
		osync_xmlfield_add_key_value(xmlfield, "Interval", interval->str + strlen("INTERVAL="));
	} else {
		// use "1" if no interval was set
		osync_xmlfield_add_key_value(xmlfield, "Interval", "1");
	}
	if (modname != NULL) {
		osync_xmlfield_add_key_value(xmlfield, modname, modifier->str + (strlen(modname)+1));
	}

	return xmlfield;
}

/*
static OSyncXMLField *handle_organizer_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Organizer", error);
}

static OSyncXMLField *handle_recurid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "RecurrenceID", error);
}
*/
/*
static OSyncXMLField *handle_duration_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Duration", error);
}
*/

/*
static OSyncXMLField *handle_contact_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Contact", error);
}
*/

/*
static OSyncXMLField *handle_calscale_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "CalendarScale", error);
}
*/


/* FIXME... Duration wrong placed? in XSD */
/*
static OSyncXMLField *handle_aduration_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Duration", error);
}

static OSyncXMLField *handle_aaction_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmAction", error);
}
*/
/* TODO: Add alarm attach to XSD */ 
/*
static OSyncXMLField *handle_aattach_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmAttach", error);
}

static OSyncXMLField *handle_adescription_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmDescription", error);
}
*/
/* TODO: Add alarm attende to XSD */
/*
static OSyncXMLField *handle_aattendee_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmAttendee", error);
}
*/
/* TODO: Add alarm summary to XSD */
/*
static OSyncXMLField *handle_asummary_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmSummary", error);
}
*/

// Timezone: iCal only
OSyncXMLField *handle_tzid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	return handle_attribute_simple_content(xmlformat, attr, "TimezoneID", error);
}

OSyncXMLField *handle_tz_last_modified_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	return handle_attribute_simple_content(xmlformat, attr, "LastModified", error);
}

OSyncXMLField *handle_tzurl_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	return handle_attribute_simple_content(xmlformat, attr, "TimezoneUrl", error);
}

OSyncXMLField *handle_tzdtstart_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	return handle_attribute_simple_content(xmlformat, attr, "DateStarted", error);
}

OSyncXMLField *handle_tzoffsetto_location_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	return handle_attribute_simple_content(xmlformat, attr, "TZOffsetTo", error);
}

OSyncXMLField *handle_tzoffsetfrom_location_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	return handle_attribute_simple_content(xmlformat, attr, "TZOffsetFrom", error);
}

OSyncXMLField *handle_tzrdate_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	return handle_attribute_simple_content(xmlformat, attr, "TimezoneDate", error);
}

OSyncXMLField *handle_tzrrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling tzrrule attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "RecurrenceRule", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osync_xmlfield_add_key_value(xmlfield, "Rule", retstr->str);
	}
	
	return xmlfield;
}

OSyncXMLField *handle_tzname_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	return handle_attribute_simple_content(xmlformat, attr, "TimezoneName", error);
}

OSyncXMLField *handle_tz_location_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	return handle_attribute_simple_content(xmlformat, attr, "Location", error);
}
// END TIMEZONE


/* ******* Paramter ****** */
/*
static void handle_tzid_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "TimezoneID");
}

static void handle_altrep_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "AlternateRep");
}

static void handle_cn_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "CommonName");
}

static void handle_delegated_from_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "DelegatedFrom");
}

static void handle_delegated_to_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "DelegatedTo");
}

static void handle_dir_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "Directory");
}

static void handle_format_type_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	// TODO handle FormatType in XSD //
	osync_xmlfield_set_attr(xmlfield, "Type", "FormatType");
}

static void handle_fb_type_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "FreeBusyType");
}

static void handle_member_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "Member");
}

static void handle_partstat_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "PartStat");
}

static void handle_range_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "Range");
}

static void handle_related_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "Related");
}

static void handle_reltype_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "RelationType");
}

static void handle_role_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "Role");
}

static void handle_rsvp_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "RSVP");
}

static void handle_sent_by_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "SentBy");
}

static void handle_status_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "Status");
}
*/


/*
void xml_parse_attribute(OSyncHookTables *hooks, GHashTable *table, OSyncXMLField **xmlfield, VFormat *vcal)
{
	osync_trace(TRACE_INTERNAL, "parsing xml attributes");
	OSyncXMLField *xmlfield = xmlfield;
	while (xmlfield) {
		if (!strcmp((char*)xmlfield->name, "Todo")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VTODO");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->comptable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VTODO");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)xmlfield->name, "Timezone")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VTIMEZONE");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VTIMEZONE");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)xmlfield->name, "Event")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VEVENT");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->comptable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VEVENT");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)xmlfield->name, "Journal")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VJOURNAL");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VJOURNAL");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)xmlfield->name, "DaylightSavings")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "DAYLIGHT");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "DAYLIGHT");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)xmlfield->name, "Standard")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "STANDARD");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "STANDARD");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)xmlfield->name, "Alarm")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VALARM");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->alarmtable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VALARM");
			vformat_add_attribute(vcal, attr);
		} else {
			xml_handle_attribute(table, vcal, xmlfield);
		}
		xmlfield = xmlfield->next;
	}
}
*/

void vcalendar_parse_attributes(OSyncHookTables *hooks, GHashTable *table, OSyncXMLFormat *xmlformat, GHashTable *paramtable, GList **attributes)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, attributes);
	
	GList *a = NULL;
	for (a = *attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;

		osync_trace(TRACE_INTERNAL, "attribute: \"%s\"", vformat_attribute_get_name(attr));

		if (!strcmp(vformat_attribute_get_name(attr), "BEGIN")) {
			//Handling supcomponent
	//		a = a->next;
			/*
			if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "VTIMEZONE")) {
				OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Timezone", NULL);
				vcal_parse_attributes(hooks, hooks->tztable, xmlformat, hooks->tztable, &a, xmlfield);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "DAYLIGHT")) {
				OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "DaylightSavings", NULL);
				vcal_parse_attributes(hooks, hooks->tztable, hooks->tztable, &a, xmlfield);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "STANDARD")) {
				OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Standard", NULL);
				vcal_parse_attributes(hooks, hooks->tztable, hooks->tztable, &a, xmlfield);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "VTODO")) {
				OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Todo", NULL);
				vcal_parse_attributes(hooks, hooks->comptable, hooks->compparamtable, &a, xmlfield);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "VEVENT")) {
				OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Event", NULL);
				vcal_parse_attributes(hooks, hooks->comptable, hooks->compparamtable, &a, xmlfield);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "VJOURNAL")) {
				OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Journal", NULL);
				vcal_parse_attributes(hooks, hooks->comptable, hooks->compparamtable, &a, xmlfield);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "VALARM")) {
				OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Alarm", NULL);
				vcal_parse_attributes(hooks, hooks->alarmtable, hooks->alarmtable, &a, xmlfield);
			}
			*/
		} else if (!strcmp(vformat_attribute_get_name(attr), "END")) {
			osync_trace(TRACE_EXIT, "%s: Found END", __func__);
			*attributes = a;
			return;
		} else
			handle_attribute(hooks, xmlformat, attr, NULL);

	}
	osync_trace(TRACE_EXIT, "%s: Done", __func__);
}

/*
static void xml_handle_unknown_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown xml parameter");
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, (char*)xmlfield->name, content);
	g_free(content);
}
*/

/*
static void handle_xml_category_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_value(attr, content);
	g_free(content);
}
*/


/*
static void handle_xml_rule_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_value(attr, content);
	g_free(content);
}

static void handle_xml_value_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "VALUE", content);
	g_free(content);
}

static void handle_xml_altrep_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "ALTREP", content);
	g_free(content);
}

static void handle_xml_cn_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "CN", content);
	g_free(content);
}

static void handle_xml_delegated_from_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "DELEGATED-FROM", content);
	g_free(content);
}

static void handle_xml_delegated_to_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "DELEGATED-TO", content);
	g_free(content);
}

static void handle_xml_dir_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "DIR", content);
	g_free(content);
}

static void handle_xml_format_type_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "FMTTYPE", content);
	g_free(content);
}

static void handle_xml_fb_type_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "FBTYPE", content);
	g_free(content);
}

static void handle_xml_member_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "MEMBER", content);
	g_free(content);
}

static void handle_xml_partstat_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "PARTSTAT", content);
	g_free(content);
}

static void handle_xml_range_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "RANGE", content);
	g_free(content);
}

static void handle_xml_reltype_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "RELTYPE", content);
	g_free(content);
}

static void handle_xml_related_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "RELATED", content);
	g_free(content);
}

static void handle_xml_role_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "ROLE", content);
	g_free(content);
}

static void handle_xml_rsvp_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "RSVP", content);
	g_free(content);
}

static void handle_xml_sent_by_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "SENT-BY", content);
	g_free(content);
}
*/

/*
void xml_parse_attribute(OSyncHookTables *hooks, GHashTable *table, OSyncXMLField **xmlfield, VFormat *vcal)
{
	osync_trace(TRACE_INTERNAL, "parsing xml attributes");
	OSyncXMLField *xmlfield = xmlfield;
	while (xmlfield) {
		if (!strcmp((char*)xmlfield->name, "Todo")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VTODO");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->comptable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VTODO");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)xmlfield->name, "Timezone")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VTIMEZONE");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VTIMEZONE");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)xmlfield->name, "Event")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VEVENT");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->comptable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VEVENT");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)xmlfield->name, "Journal")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VJOURNAL");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VJOURNAL");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)xmlfield->name, "DaylightSavings")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "DAYLIGHT");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "DAYLIGHT");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)xmlfield->name, "Standard")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "STANDARD");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "STANDARD");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)xmlfield->name, "Alarm")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VALARM");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = xmlfield->children;
			xml_parse_attribute(hooks, hooks->alarmtable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VALARM");
			vformat_add_attribute(vcal, attr);
		} else {
			xml_handle_attribute(table, vcal, xmlfield);
		}
		xmlfield = xmlfield->next;
	}
}
*/

void insert_attr_handler(GHashTable *table, const char *attrname, void* handler)
{
	g_hash_table_insert(table, (gpointer)attrname, handler);
}


/*
static VFormatAttribute *xml_handle_unknown_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown xml attribute %s", xmlfield->name);
	char *name = osxml_find_node(xmlfield, "NodeName");
	VFormatAttribute *attr = vformat_attribute_new(NULL, name);
	g_free(name);
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}
*/

// TODO
VFormatAttribute *handle_xml_alarm_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
    return NULL;
}

VFormatAttribute *handle_xml_prodid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "PRODID", encoding);
}

VFormatAttribute *handle_xml_method_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "METHOD", encoding);
}

VFormatAttribute *handle_xml_geo_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling location xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "GEO");
	add_value(attr, xmlfield, "Latitude", encoding);
	add_value(attr, xmlfield, "Longitude", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_dtstamp_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "DTSTAMP", encoding);
}

VFormatAttribute *handle_xml_description_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "DESCRIPTION", encoding);
}

VFormatAttribute *handle_xml_summary_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "SUMMARY", encoding);
}

VFormatAttribute *handle_xml_due_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	/*TODO timezone*/
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DUE");
	add_value(attr, xmlfield, "Content", encoding);
//	char *tzid = osxml_find_node(xmlfield, "TimezoneID")
//	vformat_attribute_add_param_with_value(attr, "TZID", tzid);
//	g_free(tzid);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_dtstart_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	/* TODO timezone */
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTSTART");
	add_value(attr, xmlfield, "Content", encoding);
//	char *tzid = osxml_find_node(xmlfield, "TimezoneID")
//	vformat_attribute_add_param_with_value(attr, "TZID", tzid);
//	g_free(tzid);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_percent_complete_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "PERCENT-COMPLETE", encoding);
}

VFormatAttribute *handle_xml_priority_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "PRIORITY", encoding);
}

VFormatAttribute *handle_xml_sequence_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "SEQUENCE", encoding);
}

VFormatAttribute *handle_xml_last_modified_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "LAST-MODIFIED", encoding);
}

VFormatAttribute *handle_xml_created_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "CREATED", encoding);
}

VFormatAttribute *handle_xml_rrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	/* TODO */
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RRULE");
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_rdate_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RDATE", encoding);
}

VFormatAttribute *handle_xml_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "LOCATION", encoding);
}

VFormatAttribute *handle_xml_completed_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "COMPLETED", encoding);
}

VFormatAttribute *handle_xml_organizer_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "ORGANIZER", encoding);
}

VFormatAttribute *handle_xml_recurid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RECURRENCE-ID", encoding);
}

VFormatAttribute *handle_xml_status_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "STATUS", encoding);
}

VFormatAttribute *handle_xml_duration_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "DURATION", encoding);
}

VFormatAttribute *handle_xml_attach_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "ATTACH", encoding);
}

VFormatAttribute *handle_xml_attendee_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "ATTENDEE", encoding);
}

VFormatAttribute *handle_xml_event_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "CONTACT", encoding);
}

VFormatAttribute *handle_xml_exdate_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "EXDATE", encoding);
}

VFormatAttribute *handle_xml_exrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "EXRULE", encoding);
}

VFormatAttribute *handle_xml_rstatus_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RSTATUS", encoding);
}

VFormatAttribute *handle_xml_related_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RELATED-TO", encoding);
}

VFormatAttribute *handle_xml_resources_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RESOURCES", encoding);
}

VFormatAttribute *handle_xml_dtend_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{

	/* TODO timezone */
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTEND");
	add_value(attr, xmlfield, "Content", encoding);
//	char *tzid = osxml_find_node(xmlfield, "TimezoneID")
//	vformat_attribute_add_param_with_value(attr, "TZID", tzid);
//	g_free(tzid);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_transp_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "TRANSP", encoding);
}

VFormatAttribute *handle_xml_calscale_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "CALSCALE", encoding);
}

VFormatAttribute *handle_xml_tzid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	/* TODO implement XMLFormat TZID handler */
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZID");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tz_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-LIC-LOCATION");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tzoffsetfrom_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZOFFSETFROM");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tzoffsetto_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZOFFSETTO");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tzname_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZNAME");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tzdtstart_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTSTART");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tzrrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RRULE");
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tz_last_modified_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LAST-MODIFIED");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tzurl_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZURL");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tzrdate_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RDATE");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_atrigger_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "TRIGGER", encoding);
}

VFormatAttribute *handle_xml_arepeat_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "REPEAT");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_aduration_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DURATION");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_aaction_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ACTION");
	/* FIXME add_Value() #3 NULL is wrong */
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_aattach_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTACH");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_adescription_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DESCRIPTION");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_aattendee_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTENDEE");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_asummary_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "SUMMARY");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

void insert_xml_attr_handler(GHashTable *table, const char *name, void *handler)
{
	g_hash_table_insert(table, (gpointer)name, handler);
}


