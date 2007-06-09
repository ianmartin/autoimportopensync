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

#include <glib.h>
#include <opensync/opensync.h>
#include <opensync/opensync_xml.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-merger.h>
#include <opensync/opensync-time.h>

#include "gnokii_calendar_utils.h"

/*
 * Converts the gnokii event object type (gn_calnote) into XML.
 */
static osync_bool conv_gnokii_event_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %s, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);

	OSyncXMLField *xmlfield = NULL;

	time_t timet;
	time_t start_timet = 0;
	char *tmp = NULL, *vtime = NULL, *wday = NULL;
	int secs_before_event = 0;
	int offset = 0;

	gn_calnote *cal = (gn_calnote *) input;

	if (inpsize != sizeof(gn_calnote)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	OSyncXMLFormat *xmlformat = osync_xmlformat_new("event", error);


	// Type
	xmlfield = osync_xmlfield_new(xmlformat, "Categories", error);
	switch (cal->type) {
		case GN_CALNOTE_MEETING:
			osync_xmlfield_set_key_value(xmlfield, "Category", "Meeting");
			 break;
		case GN_CALNOTE_CALL:
			osync_xmlfield_set_key_value(xmlfield, "Category", "Calling");
			 break;
		case GN_CALNOTE_BIRTHDAY:
			 osync_xmlfield_set_key_value(xmlfield, "Category", "Birthday");
			 break;
		case GN_CALNOTE_REMINDER:
			 osync_xmlfield_set_key_value(xmlfield, "Category", "Reminder");
			 break;
		case GN_CALNOTE_MEMO: 
			 osync_xmlfield_set_key_value(xmlfield, "Category", "Memo");
			 break;
	}

	// StartTime
	if (cal->time.year) {

		vtime = g_strdup_printf("%04d%02d%02dT%02d%02d%02d", 
				cal->time.year,
				cal->time.month,
				cal->time.day,
				cal->time.hour,
				cal->time.minute,
				cal->time.second);

		// Day Events have no Time only Date:
		if (cal->type == GN_CALNOTE_REMINDER
				|| cal->type == GN_CALNOTE_MEMO
				|| cal->type == GN_CALNOTE_BIRTHDAY) {
			tmp = osync_time_datestamp(vtime);
			vtime = strdup(tmp);
			g_free(tmp);

		// Date/Time Events:
		} else {
			tmp = osync_time_timestamp(vtime);
			g_free(vtime);

			// determine system UTC offset
			struct tm *ttm = osync_time_vtime2tm(tmp);
			offset = osync_time_timezone_diff(ttm);
			g_free(ttm);

			vtime = osync_time_vtime2utc(tmp, offset);
			g_free(tmp);
		}

		osync_trace(TRACE_SENSITIVE, "start time: %s (ical - UTC)\n", vtime);
		xmlfield = osync_xmlfield_new(xmlformat, "DateStarted", error);
		osync_xmlfield_set_key_value(xmlfield, "Content", vtime);
		g_free(vtime);

		// Also used for alarm timestamp calculation
		start_timet = gnokii_util_timestamp2unix(&(cal->time)); 	
	}

	// EndTime
	if (cal->end_time.year) {

		// Day Events have no Time only Date:
		if (cal->type == GN_CALNOTE_MEMO) {
			tmp = g_strdup_printf("%04u%02u%02u",
					cal->end_time.year,
					cal->end_time.month,
					cal->end_time.day + 1);	// PIMs need this.
				// Phones counts started days. PIMs counts full days.

			vtime = osync_time_datestamp(tmp);

		// Date/Time Events:
		} else {
			vtime = g_strdup_printf("%04d%02d%02dT%02d%02d%02d", 
					cal->end_time.year,
					cal->end_time.month,
					cal->end_time.day,
					cal->end_time.hour,
					cal->end_time.minute,
					cal->end_time.second);

			tmp = osync_time_timestamp(vtime);
			g_free(vtime);
			vtime = osync_time_vtime2utc(tmp, offset);
		}

		xmlfield = osync_xmlfield_new(xmlformat, "DateEnd", error);
		osync_xmlfield_set_key_value(xmlfield, "Content", vtime);

		g_free(tmp);	
		g_free(vtime);
	}


	// Alarm (Time)
	if (cal->alarm.enabled) {

		timet = gnokii_util_timestamp2unix(&(cal->alarm.timestamp)); 

		// Set start time on calnote type Reminder to 00:00:00
		if (cal->type == GN_CALNOTE_REMINDER
		//		|| cal->type == GN_CALNOTE_MEMO) {
		) {
			gn_timestamp daystamp;
			
			daystamp.year = cal->alarm.timestamp.year;
			daystamp.month  = cal->alarm.timestamp.month;
			daystamp.day = cal->alarm.timestamp.day;
			daystamp.hour = 0; 
			daystamp.minute = 0; 
			daystamp.second = 0; 

			start_timet = gnokii_util_timestamp2unix(&daystamp);
		}
		/*
		} else if (cal->type == GN_CALNOTE_BIRTHDAY) {
			gn_timestamp daystamp;

			daystamp.year = cal->alarm.timestamp.year;
			daystamp.month = cal->alarm.timestamp.month;
			daystamp.day = cal->alarm.timestamp.day;
			daystamp.hour = cal->alarm.timestamp.hour;
			daystamp.minute = cal->alarm.timestamp.minute;
			daystamp.second = cal->alarm.timestamp.second;
		}
		*/

		secs_before_event = start_timet - timet; 

		// convert seconds into ical duration string - example: -P1DT22H30M 
		tmp = gnokii_util_secs2alarmevent(secs_before_event);	
		
		xmlfield = osync_xmlfield_new(xmlformat, "Alarm", error);

		if (cal->alarm.tone)
			osync_xmlfield_set_key_value(xmlfield, "AlarmAction", "DISPLAY");


		osync_xmlfield_set_key_value(xmlfield, "AlarmTrigger", tmp);
		osync_xmlfield_set_attr(xmlfield, "Value", "DURATION");

		g_free(tmp);
	}

	// Summary
	if (cal->text) {
		xmlfield = osync_xmlfield_new(xmlformat, "Summary", error);
		osync_xmlfield_set_key_value(xmlfield, "Content", cal->text);
	}

	// Phone Number
	if (cal->phone_number && cal->type == GN_CALNOTE_CALL) {
		xmlfield = osync_xmlfield_new(xmlformat, "Description", error);
		osync_xmlfield_set_key_value(xmlfield, "Content", cal->phone_number);
	}

	// mlocation
	if (cal->mlocation && cal->type == GN_CALNOTE_MEETING) {
		xmlfield = osync_xmlfield_new(xmlformat, "Location", error);
		osync_xmlfield_set_key_value(xmlfield, "Content", cal->mlocation);
	}

	// Recurrence
	if (cal->recurrence) {

		xmlfield = osync_xmlfield_new(xmlformat, "RecurrenceRule", error);

		switch (cal->recurrence) {
			case GN_CALNOTE_DAILY:
				osync_xmlfield_set_key_value(xmlfield, "Frequency", "DAILY");
				break;
			case GN_CALNOTE_WEEKLY:
			case GN_CALNOTE_2WEEKLY:
				osync_xmlfield_set_key_value(xmlfield, "Frequency", "WEEKLY");
				break;
			case GN_CALNOTE_MONTHLY:
				osync_xmlfield_set_key_value(xmlfield, "Frequency", "MONTHLY");
				break;
			case GN_CALNOTE_YEARLY:
				osync_xmlfield_set_key_value(xmlfield, "Frequency", "YEARLY");
				break;
			case GN_CALNOTE_NEVER:
				break;
		}
		
		// prepare "by day/day of month/..." string
		// TODO: BYMONTHDAY
		switch (cal->recurrence) {
			case GN_CALNOTE_DAILY:
				break;
			case GN_CALNOTE_2WEEKLY:
				// tmp = g_strdup("INTERVAL=4"); - was 4 wrong?!?!? - Bug?!
				osync_xmlfield_set_key_value(xmlfield, "Interval", "2");
			case GN_CALNOTE_WEEKLY:
				// XXX: Always BYDAY?
				wday = gnokii_util_unix2wday(&start_timet);
				osync_xmlfield_set_key_value(xmlfield, "ByDay", wday);
				g_free(wday);
				break;
			case GN_CALNOTE_MONTHLY:
			case GN_CALNOTE_YEARLY:
			case GN_CALNOTE_NEVER:
				break;
		}

	}

	*free_input = TRUE;
	*output = (char *)xmlformat;
	*outpsize = sizeof(xmlformat);

        // XXX: remove this later?
        osync_xmlformat_sort(xmlformat);
        
        unsigned int size;
        char *str;
        osync_xmlformat_assemble(xmlformat, &str, &size);
        osync_trace(TRACE_INTERNAL, "Output XMLFormat is:\n%s", str);
        g_free(str);

        if (osync_xmlformat_validate(xmlformat) == FALSE)
                osync_trace(TRACE_INTERNAL, "XMLFORMAT EVENT: Not valid!");
        else
                osync_trace(TRACE_INTERNAL, "XMLFORMAT EVENT: VAILD");


	osync_trace(TRACE_EXIT, "%s", __func__);	
	return TRUE;
}

static void _xmlfield_category(gn_calnote *calnote, OSyncXMLField *xmlfield)
{
	int i;
	int numnodes = osync_xmlfield_get_key_count(xmlfield);
	for (i=0; i < numnodes; i++) {
		const char *category = osync_xmlfield_get_nth_key_value(xmlfield, i);

		if (!strcasecmp(category, "Meeting"))
			calnote->type = GN_CALNOTE_MEETING;
		else if (!strcasecmp(category, "Calling"))
			calnote->type = GN_CALNOTE_CALL;
		else if (!strcasecmp(category, "Birthday"))
			calnote->type = GN_CALNOTE_BIRTHDAY;
		else if (!strcasecmp(category, "Reminder"))
			calnote->type = GN_CALNOTE_REMINDER;
		else if (!strcasecmp(category, "Memo"))
			calnote->type = GN_CALNOTE_MEMO;

		else
			// When no known type was found it will check later
			// for a valid type. 
			calnote->type = 0;

	}
}

static void _xmlfield_datestarted(gn_calnote *calnote, OSyncXMLField *xmlfield, int *alldayevent)
{

	const char *dtstart = osync_xmlfield_get_key_value(xmlfield, "Content");

	struct tm *starttm = osync_time_vtime2tm(dtstart);
	struct tm *tmptm = NULL;

	if (!osync_time_isdate(dtstart) && osync_time_isutc(dtstart)) {
		tmptm = starttm;
		int offset = osync_time_timezone_diff(tmptm);
		starttm = osync_time_tm2localtime(tmptm, offset);
		g_free(tmptm);
	}

	calnote->time = gnokii_util_tm2timestamp(starttm);

	g_free(starttm);

	// Only 3 matches (=date) means all day event 
	if (osync_time_isdate(dtstart))
		*alldayevent = 1;

	// Nokia cellphones cannot handle seconds in calendar - so set it to ZERO
	calnote->time.second = 0;

}

void static _xmlfield_dateend(gn_calnote *calnote, OSyncXMLField *xmlfield)
{

	const char *dtend = osync_xmlfield_get_key_value(xmlfield, "Content");


	struct tm *endtm = osync_time_vtime2tm(dtend);
	struct tm *tmptm = NULL;
	if (!osync_time_isdate(dtend) && osync_time_isutc(dtend)) {
		tmptm = endtm;
		int offset = osync_time_timezone_diff(tmptm);
		endtm = osync_time_tm2localtime(tmptm, offset);
		g_free(tmptm);
	}

	calnote->end_time = gnokii_util_tm2timestamp(endtm);
	g_free(endtm);

	// Nokia cellphones cannot handle seconds in calendar so set it to ZERO
	calnote->end_time.second = 0;

}

/* 
 * Converts from XMLFormat-event to the gnokii event object type (gn_calnote).
 */  
static osync_bool conv_xmlformat_to_gnokii_event(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %s, %p, %p)", __func__, input, inpsize, 
			output, outpsize, config, free_input, error);

	OSyncXMLFormat *xmlformat = (OSyncXMLFormat *)input;
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "Input XMLFormat is:\n%s", str);
	g_free(str);

//	struct tm *starttm = NULL, *endtm = NULL, *tmptm = NULL;
	osync_bool alldayevent = 0;

	if (strcmp("event", osync_xmlformat_get_objtype(xmlformat))) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xmlformat: %s",  osync_xmlformat_get_objtype(xmlformat));
		goto error;
	}

	// prepare calnote
	gn_calnote *calnote = osync_try_malloc0(sizeof(gn_calnote), error);

	OSyncXMLField *xmlfield = osync_xmlformat_get_first_field(xmlformat);
	for (; xmlfield; xmlfield = osync_xmlfield_get_next(xmlfield)) {
		osync_trace(TRACE_INTERNAL, "Field: %s", osync_xmlfield_get_name(xmlfield));

		if (!strcmp("Categories", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_category(calnote, xmlfield);
		else if (!strcmp("DateStarted", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_datestarted(calnote, xmlfield, &alldayevent);
		else if (!strcmp("DateEnd", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_dateend(calnote, xmlfield);


	}

		

	/* Alarm - TODO: is not fully supported
	 * 		Not supported:
	 *		^^^^^^^^^^^^^^
	 *		# alarm after event
	 *		# alarm before event end
	 *		# ...
	 */

	/* TODO porting to new XMLFormat-even layout */
	/*
	cur = osxml_get_node(root, "Alarm");
	if (cur && calnote->time.year) {

		int seconds_before = 0;
		time_t start_timet, alarm_timet;

		calnote->alarm.enabled = 1;
		
		tmp = osxml_find_node(cur, "AlarmAction");
		if (tmp && !strcasecmp(tmp, "DISPLAY"))
			calnote->alarm.tone = 1;

		g_free(tmp);
		
		// get AlarmTrigger root
		xmlNode *sub = osxml_get_node(cur, "AlarmTrigger");

		// get node with iCal duration
		tmp = osxml_find_node(sub, "Content");

		// convert iCal duration string into seconds (before event)
		seconds_before = gnokii_util_alarmevent2secs(tmp);

		g_free(tmp);
		
		// convert start event in to seconds
		start_timet = gnokii_util_timestamp2unix(&(calnote->time));

		// timestamp for alarm
		alarm_timet = start_timet - seconds_before;

		// convert timestamp of alarm to gnokii timestamp
		calnote->alarm.timestamp = gnokii_util_unix2timestamp(alarm_timet);
	}

	// Summary
	cur = osxml_get_node(root, "Summary");
	if (cur) {
		tmp = (char *) xmlNodeGetContent(cur);
		strncpy(calnote->text, tmp, sizeof(calnote->text)); 
		g_free(tmp);
	}

	// meeting location
	cur = osxml_get_node(root, "Location");
	if (cur) {
		tmp = (char *) xmlNodeGetContent(cur);

		if (calnote->type == GN_CALNOTE_MEETING && tmp)
			strncpy(calnote->mlocation, tmp, sizeof(calnote->mlocation));

		g_free(tmp);
	}

	// PhoneNumber
	cur = osxml_get_node(root, "Description");
	if (cur) {
		tmp = (char *) xmlNodeGetContent(cur);
		if (calnote->type == GN_CALNOTE_CALL || gnokii_util_valid_number(tmp))
			strncpy(calnote->phone_number, tmp, sizeof(calnote->phone_number));

		g_free(tmp);
	}

	// Reccurence
	cur = osxml_get_node(root, "RecurrenceRule");
	if (cur) {
		int interval = 0;

		for (cur = cur->children; cur; cur = cur->next) {

			tmp = (char *) xmlNodeGetContent(cur);

			if (strstr(tmp, "DAILY"))
				calnote->recurrence = GN_CALNOTE_DAILY;
			else if (strstr(tmp, "WEEKLY")) 
				calnote->recurrence = GN_CALNOTE_WEEKLY;
			else if (strstr(tmp, "MONTHLY"))
				calnote->recurrence = GN_CALNOTE_MONTHLY;
			else if (strstr(tmp, "YEARLY"))
				calnote->recurrence = GN_CALNOTE_YEARLY;

			else if (strstr(tmp, "INTERVAL"))
				sscanf(tmp, "INTERVAL=%u", &interval);

			g_free(tmp);
		}

		// Nokia phones only support a interval of 2 for weeks
		if (calnote->recurrence == GN_CALNOTE_WEEKLY && interval == 2)
			calnote->recurrence = GN_CALNOTE_2WEEKLY;
	}
	*/

	// check for type which fits for given data if no type was set
	if (!calnote->type)
		calnote->type = gnokii_util_calendar_type(calnote, alldayevent);


	*free_input = TRUE;
	*output = (void *)calnote;
	*outpsize = sizeof(gn_calnote);

	osync_trace(TRACE_EXIT, "%s", __func__);	
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


static void destroy_gnokii_event(char *input, size_t inpsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, input, inpsize);
	gn_calnote *calnote = (gn_calnote *) input;
	
	if (inpsize != sizeof(gn_calnote)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Wrong size!", __func__);
		return;
	}

	g_free(calnote);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*
 * Print the gnokii format in a human readable form.
 */ 
#if 0
static char *print_gnokii_event(OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, change);

	unsigned int size;
	GString *output = g_string_new(""); 
	char *buf = NULL; 

	OSyncData *data = osync_change_get_data(change);
	osync_data_get_data(data, &buf, &size);

	gn_calnote *calnote = (gn_calnote *) buf;

	// Event Type
	char *type = gnokii_util_caltype2string(calnote->type);
	char *tmp = g_strdup_printf("Type: %s\n", type);
	output = g_string_append(output, tmp);
	g_free(type);
	g_free(tmp);

	// Summary
	tmp = g_strdup_printf("Summary: %s\n", calnote->text); 
	output = g_string_append(output, tmp); 
	g_free(tmp);

	// Start
	tmp = g_strdup_printf("Begin: %04d-%02d-%02d", calnote->time.year,
			calnote->time.month, calnote->time.day);
	output = g_string_append(output, tmp); 
	g_free(tmp);

	if (calnote->type != GN_CALNOTE_BIRTHDAY
			&& calnote->type != GN_CALNOTE_MEMO
			&& calnote->type != GN_CALNOTE_REMINDER) {
		tmp = g_strdup_printf("%02d:%02d:%02d",
				calnote->time.hour, calnote->time.minute,
				calnote->time.second);
		output = g_string_append(output, tmp);
		g_free(tmp);
	}

	// End
	if (calnote->end_time.year) {
		tmp = g_strdup_printf("End: %04d-%02d-%02d",
				calnote->time.day, calnote->time.month,
				calnote->time.year);
		output = g_string_append(output, tmp);
		g_free(tmp);

		if (calnote->type != GN_CALNOTE_BIRTHDAY
			&& calnote->type != GN_CALNOTE_MEMO
			&& calnote->type != GN_CALNOTE_REMINDER) {
				tmp = g_strdup_printf("%02d:%02d:%02d",
					calnote->end_time.hour, calnote->end_time.minute,
					calnote->end_time.second);
				output = g_string_append(output, tmp);
				g_free(tmp);
		}

		output = g_string_append(output, "\n");
	}

	// Alarm
	if (calnote->alarm.enabled) {
		output = g_string_append(output, "Alarm enabled.\n");

		if (calnote->alarm.tone)
			output = g_string_append(output, "Alarm with Ring tone.\n");

		tmp = g_strdup_printf("Alarm: %04d-%02d-%02d %02d:%02d:%02d\n",
				calnote->alarm.timestamp.day, calnote->alarm.timestamp.month,
				calnote->alarm.timestamp.year, calnote->alarm.timestamp.hour,
				calnote->alarm.timestamp.minute, calnote->alarm.timestamp.second);

		output = g_string_append(output, tmp);
		g_free(tmp);

	}
	// Meeting Location
	if (calnote->mlocation) {
		tmp = g_strdup_printf("Location: %s\n", calnote->mlocation);
		output = g_string_append(output, tmp);
		g_free(tmp);
	}

	// Phone Number
	if (calnote->phone_number) {
		tmp = g_strdup_printf("Phone Number: %s\n", calnote->phone_number);
		output = g_string_append(output, tmp);
		g_free(tmp);
	}

	osync_trace(TRACE_EXIT, "%s: %s", __func__, output->str);
	return g_string_free(output, FALSE);
}
#endif

void get_format_info(OSyncFormatEnv *env, OSyncError **error)
{

        /* register gnokii-event format */
        OSyncObjFormat *format = osync_objformat_new("gnokii-event", "event", error);
        if (!format) {
                osync_trace(TRACE_ERROR, "Unable to register gnokii-event format: %s", osync_error_print(error));
                osync_error_unref(error);
                return;
        }
        
//      osync_objformat_set_compare_func(format, compare_event);
        osync_objformat_set_destroy_func(format, destroy_gnokii_event);
//      osync_objformat_set_duplicate_func(format, duplicate_xmlformat);
//        osync_objformat_set_print_func(format, print_gnokii_event);
//      osync_objformat_set_copy_func(format, copy_xmlformat);
//      osync_objformat_set_create_func(format, create_event);
        
//        osync_objformat_set_revision_func(format, get_revision);
        

//        osync_objformat_must_marshal(format);
//        osync_objformat_set_marshal_func(format, marshal_xmlformat);
//        osync_objformat_set_demarshal_func(format, demarshal_xmlformat);

        
        osync_format_env_register_objformat(env, format);
        osync_objformat_unref(format);

}

void get_conversion_info(OSyncFormatEnv *env)
{
	OSyncFormatConverter *conv;
	OSyncError *error = NULL;

	OSyncObjFormat *xmlformat = osync_format_env_find_objformat(env, "xmlformat-event");
	OSyncObjFormat *gnokii_event = osync_format_env_find_objformat(env, "gnokii-event");

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, gnokii_event, conv_xmlformat_to_gnokii_event, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, gnokii_event, xmlformat, conv_gnokii_event_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
}

int get_version(void)
{
	return 1;
}

