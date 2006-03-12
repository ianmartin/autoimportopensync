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
#include <opensync/opensync-xml.h>

/*
 * Converts the gnokii event object type (gn_calnote) into XML.
 */
static osync_bool conv_gnokii_event_to_xml(void *conv_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, conv_data, input, inpsize, output, outpsize, free_input, error);

	xmlNode *current = NULL;
	time_t ttm;
	time_t start_ttm = 0;
	char *tmp = NULL;
	int secs_before_event = 0;

	gn_calnote *cal = (gn_calnote *) input;

	if (inpsize != sizeof(gn_calnote)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	xmlDoc *doc = xmlNewDoc((xmlChar*) "1.0");
	xmlNode *root = osxml_node_add_root(doc, "vcal");
	root = xmlNewChild(root, NULL, (xmlChar*) "Event", NULL);

	// Type
	osync_trace(TRACE_INTERNAL, "Category: %i", cal->type);
	current = xmlNewChild(root, NULL, (xmlChar *) "Categories", NULL);
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

	// Time
	if (cal->time.year) {

		// Day Events have no Time only Date:
		if (cal->type == GN_CALNOTE_REMINDER
				|| cal->type == GN_CALNOTE_MEMO
				|| cal->type == GN_CALNOTE_BIRTHDAY) {

			tmp = g_strdup_printf("%04u%02u%02u",
					cal->time.year,
					cal->time.month,
					cal->time.day);

			cal->time.hour = 0;
			cal->time.minute = 0;
			cal->time.second = 0;
		
		// Date/Time Events:
		} else {
			tmp = g_strdup_printf("%04u%02u%02uT%02u%02u%02uZ", 
					cal->time.year,
					cal->time.month,
					cal->time.day,
					cal->time.hour,
					cal->time.minute,
					cal->time.second);
		}

		osync_trace(TRACE_INTERNAL, "start time: %s (ical)\n", tmp);
		current = xmlNewChild(root, NULL, (xmlChar *) "DateStarted", NULL);
		xmlNewChild(current, NULL, (xmlChar*) "Content", (xmlChar*) tmp);

		g_free(tmp);

		// Also used for alarm timestamp calculation
		start_ttm = gnokii_util_timestamp2ttm(cal->time); 	
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

		// Date/Time Events:
		} else {

			tmp = g_strdup_printf("%04u%02u%02uT%02u%02u%02uZ", 
					cal->end_time.year,
					cal->end_time.month,
					cal->end_time.day,
					cal->end_time.hour,
					cal->end_time.minute,
					cal->end_time.second);
		}

		current = xmlNewChild(root, NULL, (xmlChar *) "DateEnd", NULL);
		xmlNewChild(current, NULL, (xmlChar*) "Content", (xmlChar*) tmp);

		g_free(tmp);	
	}


	// Alarm (Time)
	if (cal->alarm.enabled) {

		ttm = gnokii_util_timestamp2ttm(cal->alarm.timestamp); 

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

			start_ttm = gnokii_util_timestamp2ttm(daystamp);
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

		secs_before_event = start_ttm - ttm;

		osync_trace(TRACE_INTERNAL, "start_ttm(%i) - ttm(%i) = %i",
				(int) start_ttm, (int) ttm, secs_before_event);
		
		// convert seconds into ical duration string - example: -P1DT22H30M 
		tmp = gnokii_util_secs2alarmevent(secs_before_event);	
		
		current = xmlNewChild(root, NULL, (xmlChar *) "Alarm", NULL);

		if (cal->alarm.tone)
			xmlNewChild(current, NULL, (xmlChar*) "AlarmAction", (xmlChar*) "DISPLAY");

		xmlNode *sub = xmlNewChild(current, NULL, (xmlChar*) "AlarmTrigger", NULL);
		xmlNewChild(sub, NULL, (xmlChar*) "Content", (xmlChar*) tmp); 
		xmlNewChild(sub, NULL, (xmlChar*) "Value", (xmlChar*) "DURATION");
		
		g_free(tmp);
	}
	
	// Summary
	if (cal->text) {
		current = xmlNewChild(root, NULL, (xmlChar*) "Summary", NULL);
		xmlNewChild(current, NULL, (xmlChar*) "Content", (xmlChar *) cal->text);
	}

	// Phone Number
	if (cal->phone_number && cal->type == GN_CALNOTE_CALL) {
		current = xmlNewChild(root, NULL, (xmlChar*) "Description", NULL);
		xmlNewChild(current, NULL, (xmlChar*) "Content", (xmlChar *) cal->phone_number);
	}

	// mlocation
	if (cal->mlocation && cal->type == GN_CALNOTE_MEETING) {
		current = xmlNewChild(root, NULL, (xmlChar*) "Location", NULL);
		xmlNewChild(current, NULL, (xmlChar*) "Content", (xmlChar*) cal->mlocation);
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
				break;
		}
		
		current = xmlNewChild(root, NULL, (xmlChar*) "RecurrenceRule", NULL);
		xmlNewChild(current, NULL, (xmlChar*) "Rule", (xmlChar*) tmp);

		// prepare "by day/day of month/..." string
		// TODO: BYMONTHDAY
		switch (cal->recurrence) {
			case GN_CALNOTE_DAILY:
				break;
			case GN_CALNOTE_2WEEKLY:
				tmp = g_strdup("INTERVAL=2");
				xmlNewChild(current, NULL, (xmlChar*) "Rule", (xmlChar*) tmp);
			case GN_CALNOTE_WEEKLY:
				tmp = g_strdup_printf("BYDAY=%s", gnokii_util_ttm2wday(&start_ttm));
				xmlNewChild(current, NULL, (xmlChar*) "Rule", (xmlChar*) tmp);
				break;
			case GN_CALNOTE_MONTHLY:
				break;
			case GN_CALNOTE_YEARLY:
				break;
			default:
				break;	
		}

		g_free(tmp);
	}
	
	*free_input = TRUE;
	*output = (char *)doc;
	*outpsize = sizeof(doc);

#ifndef HIDE_SENSITIVE
	osync_trace(TRACE_INTERNAL, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)doc));
#endif	
	
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
#ifndef HIDE_SENSITIVE
	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", osxml_write_to_string((xmlDoc *)input));
#endif	

	char *tmp;
	xmlNode *cur = NULL;
	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);

	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		goto error;
	}

	if (xmlStrcmp(root->name, (const xmlChar *) "vcal")) {
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
/*
   gnokii can not write this type of entry at the moment (version 0.6.11).
		else if (!strcasecmp(tmp, "Memo"))
			calnote->type = GN_CALNOTE_MEMO;
*/

		else
			// When no known type was found it will check later
			// for a valid type. 
			calnote->type = 0;
	}

	// DateStarted 
	cur = osxml_get_node(root, "DateStarted");
	if (cur) {

		tmp = (char*) xmlNodeGetContent(cur);

		sscanf(tmp, "%04u%02u%02uT%02u%02u%02uZ",
				&(calnote->time.year),
				&(calnote->time.month),
				&(calnote->time.day),
				&(calnote->time.hour),
				&(calnote->time.minute),
				&(calnote->time.second));

		// Nokia cellphones cannot handle seconds in calendar - so set it to ZERO
		calnote->time.second = 0;
	}
		

	// DateEnd
	/* XXX: Not supported by gnokii at the moment (version 0.6.11)... will be fixed! */
	cur = osxml_get_node(root, "DateEnd");
	if (cur) {

		tmp = (char*) xmlNodeGetContent(cur);

		sscanf(tmp, "%04u%02u%02uT%02u%02u%02uZ",
				&(calnote->end_time.year),
				&(calnote->end_time.month),
				&(calnote->end_time.day),
				&(calnote->end_time.hour),
				&(calnote->end_time.minute),
				&(calnote->end_time.second));

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
		time_t start_ttm, alarm_ttm;

		calnote->alarm.enabled = 1;
		
		tmp = osxml_find_node(cur, "AlarmAction");
		if (!strcasecmp(tmp, "DISPLAY"))
			calnote->alarm.tone = 1;
		
		// get AlarmTrigger root
		xmlNode *sub = osxml_get_node(cur, "AlarmTrigger");

		// get node with iCal duration
		sub = osxml_get_node(sub, "Content");

		tmp = (char *) xmlNodeGetContent(sub);

		// convert iCal duration string into seconds (before event)
		seconds_before = gnokii_util_alarmevent2secs(tmp);
		
		// convert start event in to seconds
		start_ttm = gnokii_util_timestamp2ttm(calnote->time);

		// timestamp for alarm
		alarm_ttm = start_ttm - seconds_before;

		// convert timestamp of alarm to gnokii timestamp
		calnote->alarm.timestamp = gnokii_util_ttm2timestamp(alarm_ttm);
	}

	// Summary
	cur = osxml_get_node(root, "Summary");
	if (cur)
		strncpy(calnote->text, (char *) xmlNodeGetContent(cur), sizeof(calnote->text)); 

	// meeting location
	cur = osxml_get_node(root, "Location");
	if (calnote->type == GN_CALNOTE_MEETING && cur)
		strncpy(calnote->mlocation, (char *) xmlNodeGetContent(cur), sizeof(calnote->mlocation));

	// PhoneNumber
	cur = osxml_get_node(root, "Description");
	if (calnote->type == GN_CALNOTE_CALL && cur)
		strncpy(calnote->phone_number, (char *) xmlNodeGetContent(cur), sizeof(calnote->phone_number));

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
		}

		// Nokia phones only support a interval of 2 for weeks
		if (calnote->recurrence == GN_CALNOTE_WEEKLY && interval == 2)
			calnote->recurrence = GN_CALNOTE_2WEEKLY;
	}

	// TODO: check for type which fits for given data if no type was set
	if (!calnote->type)
		calnote->type = GN_CALNOTE_REMINDER;

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

	char *output = NULL;
	gn_calnote *calnote = (gn_calnote *) osync_change_get_data(change);

	// Event Type
	output = g_strdup_printf("Type: %s\n", gnokii_util_caltype2string(calnote->type));
	// Summary
	output = g_strdup_printf("%sSummary: %s\n", output, calnote->text); 
	// Start
	output = g_strdup_printf("%sBegin: %02u.%02u.%04u", output, calnote->time.day,
			calnote->time.month, calnote->time.year);

	if (calnote->type != GN_CALNOTE_BIRTHDAY
			&& calnote->type != GN_CALNOTE_MEMO
			&& calnote->type != GN_CALNOTE_REMINDER) {
		output = g_strdup_printf("%s %02u:%02u:%02u", output,
				calnote->time.hour, calnote->time.minute,
				calnote->time.second);
	}

	output = g_strdup_printf("%s\n", output);
	// End
	if (calnote->end_time.year) {
		output = g_strdup_printf("%sEnd: %02u.%02u.%04u", output,
				calnote->time.day, calnote->time.month,
				calnote->time.year);

		if (calnote->type != GN_CALNOTE_BIRTHDAY
			&& calnote->type != GN_CALNOTE_MEMO
			&& calnote->type != GN_CALNOTE_REMINDER) {
				output = g_strdup_printf("%s %02u:%02u:%02u", output,
					calnote->end_time.hour, calnote->end_time.minute,
					calnote->end_time.second);
		}
	}

	output = g_strdup_printf("%s\n", output);
	// Alarm
	if (calnote->alarm.enabled) {
		output = g_strdup_printf("%sAlarm enabled.\n", output);

		if (calnote->alarm.tone)
			output = g_strdup_printf("%sAlarm with Ring tone.\n", output);

		output = g_strdup_printf("%sAlarm: %02u.%02u.%04u %02u:%02u:%02u\n", output,
				calnote->alarm.timestamp.day, calnote->alarm.timestamp.month,
				calnote->alarm.timestamp.year, calnote->alarm.timestamp.hour,
				calnote->alarm.timestamp.minute, calnote->alarm.timestamp.second);

	}
	// Meeting Location
	if (calnote->mlocation)
		output = g_strdup_printf("%sLocation: %s\n", output, calnote->mlocation);

	// Phone Number
	if (calnote->phone_number)
		output = g_strdup_printf("%sPhone Number: %s\n", output, calnote->phone_number);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, output);
	return output;
}


void get_info(OSyncEnv *env)
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

