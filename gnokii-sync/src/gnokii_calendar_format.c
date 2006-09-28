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
#include "gnokii_calendar_utils.h"
#include "gnokii_calendar_format.h"
#include <opensync/opensync-xml.h>

/*
 * Converts the gnokii event object type (gn_calnote) into XML.
 */
static osync_bool conv_gnokii_event_to_xml(void *conv_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, conv_data, input, inpsize, output, outpsize, free_input, error);

	xmlNode *current = NULL;
	time_t timet;
	time_t start_timet = 0;
	char *tmp = NULL, *vtime = NULL, *wday = NULL;
	int secs_before_event = 0;

	gn_calnote *cal = (gn_calnote *) input;

	if (inpsize != sizeof(gn_calnote)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	xmlDoc *doc = xmlNewDoc((xmlChar*) "1.0");
	xmlNode *root = osxml_node_add_root(doc, "vcal");
	root = xmlNewTextChild(root, NULL, (xmlChar*) "Event", NULL);

	// Type
	current = xmlNewTextChild(root, NULL, (xmlChar *) "Categories", NULL);
	switch (cal->type) {
		case GN_CALNOTE_MEETING:
			 osxml_node_add(current, "Category", "Meeting");
			 break;
		case GN_CALNOTE_CALL:
			 osxml_node_add(current, "Category", "Calling");
			 break;
		case GN_CALNOTE_BIRTHDAY:
			 osxml_node_add(current, "Category", "Birthday");
			 break;
		case GN_CALNOTE_REMINDER:
			 osxml_node_add(current, "Category", "Reminder");
			 break;
		case GN_CALNOTE_MEMO: 
			 osxml_node_add(current, "Category", "Memo");
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
			vtime = osync_time_vtime2utc(tmp);
			g_free(tmp);
		}

		osync_trace(TRACE_SENSITIVE, "start time: %s (ical - UTC)\n", vtime);
		current = xmlNewTextChild(root, NULL, (xmlChar *) "DateStarted", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*) "Content", (xmlChar*) vtime);

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
			vtime = osync_time_vtime2utc(tmp);
		}

		current = xmlNewTextChild(root, NULL, (xmlChar *) "DateEnd", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*) "Content", (xmlChar*) vtime);

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
		
		current = xmlNewTextChild(root, NULL, (xmlChar *) "Alarm", NULL);

		if (cal->alarm.tone)
			xmlNewTextChild(current, NULL, (xmlChar*) "AlarmAction", (xmlChar*) "DISPLAY");

		xmlNode *sub = xmlNewTextChild(current, NULL, (xmlChar*) "AlarmTrigger", NULL);
		xmlNewTextChild(sub, NULL, (xmlChar*) "Content", (xmlChar*) tmp); 
		xmlNewTextChild(sub, NULL, (xmlChar*) "Value", (xmlChar*) "DURATION");

		g_free(tmp);
	}

	// Summary
	if (cal->text) {
		current = xmlNewTextChild(root, NULL, (xmlChar*) "Summary", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*) "Content", (xmlChar *) cal->text);
	}

	// Phone Number
	if (cal->phone_number && cal->type == GN_CALNOTE_CALL) {
		current = xmlNewTextChild(root, NULL, (xmlChar*) "Description", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*) "Content", (xmlChar *) cal->phone_number);
	}

	// mlocation
	if (cal->mlocation && cal->type == GN_CALNOTE_MEETING) {
		current = xmlNewTextChild(root, NULL, (xmlChar*) "Location", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*) "Content", (xmlChar*) cal->mlocation);
	}

	// Recurrence
	if (cal->recurrence) {

		switch (cal->recurrence) {
			case GN_CALNOTE_DAILY:
				tmp = g_strdup("FREQ=DAILY");
				break;
			case GN_CALNOTE_WEEKLY:
			case GN_CALNOTE_2WEEKLY:
				tmp = g_strdup("FREQ=WEEKLY");
				break;
			case GN_CALNOTE_MONTHLY:
				tmp = g_strdup("FREQ=MONTHLY");
				break;
			case GN_CALNOTE_YEARLY:
				tmp = g_strdup("FREQ=YEARLY");
				break;
			default:
				tmp = g_strdup("");				
				break;
		}
		
		current = xmlNewTextChild(root, NULL, (xmlChar*) "RecurrenceRule", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*) "Rule", (xmlChar*) tmp);
		g_free(tmp);

		// prepare "by day/day of month/..." string
		// TODO: BYMONTHDAY
		switch (cal->recurrence) {
			case GN_CALNOTE_DAILY:
				break;
			case GN_CALNOTE_2WEEKLY:
				tmp = g_strdup("INTERVAL=4");
				xmlNewTextChild(current, NULL, (xmlChar*) "Rule", (xmlChar*) tmp);
			case GN_CALNOTE_WEEKLY:
				wday = gnokii_util_unix2wday(&start_timet);
				tmp = g_strdup_printf("BYDAY=%s", wday);
				xmlNewTextChild(current, NULL, (xmlChar*) "Rule", (xmlChar*) tmp);
				g_free(wday);
				break;
			case GN_CALNOTE_MONTHLY:
				break;
			case GN_CALNOTE_YEARLY:
				break;
			default:
				break;	
		}

//		g_free(tmp);
	}

	*free_input = TRUE;
	*output = (char *)doc;
	*outpsize = sizeof(doc);

	osync_trace(TRACE_SENSITIVE, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)doc));
	
	osync_trace(TRACE_EXIT, "%s", __func__);	
	return TRUE;
}

/* 
 * Converts from XML to the gnokii event object type (gn_calnote).
 */  
static osync_bool conv_xml_event_to_gnokii(void *conv_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, conv_data, input, inpsize, 
			output, outpsize, free_input, error);

	osync_trace(TRACE_SENSITIVE, "Input XML is:\n%s", osxml_write_to_string((xmlDoc *)input));

	char *tmp;
	struct tm *starttm = NULL, *endtm = NULL, *tmptm = NULL;
	osync_bool alldayevent = 0;
	xmlNode *cur = NULL;
	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);

	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		goto error;
	}

	if (xmlStrcmp(root->name, (xmlChar *) "vcal")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong (event) xml root element");
		goto error;
	}

	// Event child
	root = osxml_get_node(root, "Event");

	// prepare calnote
	gn_calnote *calnote = NULL;
	calnote = (gn_calnote *) malloc(sizeof(gn_calnote));

	memset(calnote, 0, sizeof(gn_calnote));

	// Type
	// TODO: handle more then one category - not only the first
	cur = osxml_get_node(root, "Categories");
	if (cur) {

		tmp = (char *) xmlNodeGetContent(cur);

		if (!strcasecmp(tmp, "Meeting"))
			calnote->type = GN_CALNOTE_MEETING;
		else if (!strcasecmp(tmp, "Calling"))
			calnote->type = GN_CALNOTE_CALL;
		else if (!strcasecmp(tmp, "Birthday"))
			calnote->type = GN_CALNOTE_BIRTHDAY;
		else if (!strcasecmp(tmp, "Reminder"))
			calnote->type = GN_CALNOTE_REMINDER;
		else if (!strcasecmp(tmp, "Memo"))
			calnote->type = GN_CALNOTE_MEMO;

		else
			// When no known type was found it will check later
			// for a valid type. 
			calnote->type = 0;

		g_free(tmp);
	}

	// DateStarted 
	cur = osxml_get_node(root, "DateStarted");
	if (cur) {

		tmp = osxml_find_node(cur, "Content");

		/*
		ret = sscanf(tmp, "%04u%02u%02uT%02u%02u%02u",
				&(calnote->time.year),
				&(calnote->time.month),
				&(calnote->time.day),
				&(calnote->time.hour),
				&(calnote->time.minute),
				&(calnote->time.second));
		*/		

		starttm = osync_time_vtime2tm(tmp);

		if (!osync_time_isdate(tmp) && osync_time_isutc(tmp)) {
			tmptm = starttm;
			starttm = osync_time_tm2localtime(tmptm);
			g_free(tmptm);
		}

		calnote->time = gnokii_util_tm2timestamp(starttm);

		g_free(starttm);

		// Only 3 matches (=date) means all day event 
		if (osync_time_isdate(tmp))
			alldayevent = 1;

		g_free(tmp);
			

		// Nokia cellphones cannot handle seconds in calendar - so set it to ZERO
		calnote->time.second = 0;
	}
		

	// DateEnd
	cur = osxml_get_node(root, "DateEnd");
	if (cur) {

		tmp = osxml_find_node(cur, "Content");

		endtm = osync_time_vtime2tm(tmp);
		if (!osync_time_isdate(tmp) && osync_time_isutc(tmp)) {
			tmptm = endtm;
			endtm = osync_time_tm2localtime(tmptm);
			g_free(tmptm);
		}

		g_free(tmp);

		calnote->end_time = gnokii_util_tm2timestamp(endtm);

		g_free(endtm);

		// Nokia cellphones cannot handle seconds in calendar so set it to ZERO
		calnote->end_time.second = 0;
	}

	/* Alarm - TODO: is not fully supported
	 * 		Not supported:
	 *		^^^^^^^^^^^^^^
	 *		# alarm after event
	 *		# alarm before event end
	 *		# ...
	 */
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
static char *print_gnokii_event(OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, change);

	char *tmp = NULL, *type = NULL;
	GString *output = g_string_new(""); 
	gn_calnote *calnote = (gn_calnote *) osync_change_get_data(change);

	// Event Type
	type = gnokii_util_caltype2string(calnote->type);
	tmp = g_strdup_printf("Type: %s\n", type);
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



void gnokii_calendar_format_get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "event");
	
	//Tell OpenSync that we want to register a new format
	osync_env_register_objformat(env, "event", "gnokii-event");
	//Now we can set the function on your format we have created above
//	osync_env_format_set_compare_func(env, "gnokii-event", compare_format1);
//	osync_env_format_set_duplicate_func(env, "gnokii-event", duplicate_format1);
	osync_env_format_set_destroy_func(env, "gnokii-event", destroy_gnokii_event);
	osync_env_format_set_print_func(env, "gnokii-event", print_gnokii_event);
	
	osync_env_register_converter(env, CONVERTER_CONV, "gnokii-event", "xml-event", conv_gnokii_event_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-event", "gnokii-event", conv_xml_event_to_gnokii);

}

