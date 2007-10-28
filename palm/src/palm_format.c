/*
 * libopensync-palm-plugin - A palm plugin for opensync
 * Copyright (C) 2005  Armin Bauer <armin.bauer@opensync.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * 
 */

#include "palm_sync.h"
#include "palm_format.h"

#include <opensync/opensync-time.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-merger.h>
#include <opensync/opensync-serializer.h>

static char *conv_enc_palm_to_xml(const char *text) {
	char *ret;

	ret = g_convert(text, strlen(text), "utf8", "cp1252", NULL, NULL, NULL);

	osync_trace(TRACE_SENSITIVE, "%s(): %s -> %s", __func__, text ? text : "nil", ret ? ret : "nil");

	return ret;
}

static char *conv_enc_xml_to_palm(const char *text) {
	char *ret;

	ret = g_convert(text, strlen(text), "cp1252", "utf8", NULL, NULL, NULL);

	osync_trace(TRACE_SENSITIVE, "%s(): %s -> %s", __func__, text ? text : "nil", ret ? ret: "nil");

	return ret;
}

#if 0
static osync_bool conv_palm_event_to_xml(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
	PSyncEventEntry *entry = (PSyncEventEntry *)input;
	char *tmp = NULL;
	char *vtime = NULL;
	xmlNode *current = NULL;
	
	if (inpsize != sizeof(PSyncEventEntry)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		goto error;
	}

	osync_trace(TRACE_SENSITIVE, "codepage: %s\n", entry->codepage ? entry->codepage : "nil");
	osync_trace(TRACE_SENSITIVE, "event: %i\n alarm: %i\n",
		       entry->appointment.event, entry->appointment.alarm);	
	osync_trace(TRACE_SENSITIVE, "Start: %04d-%02d-%02d %02d-%02d-%02d",
			entry->appointment.begin.tm_year + 1900,
			entry->appointment.begin.tm_mon + 1,
			entry->appointment.begin.tm_mday,
			entry->appointment.begin.tm_hour,
			entry->appointment.begin.tm_min,
			entry->appointment.begin.tm_sec);

	osync_trace(TRACE_SENSITIVE, "End: %04d-%02d-%02d %02d-%02d-%02d",
			entry->appointment.end.tm_year + 1900,
			entry->appointment.end.tm_mon + 1,
			entry->appointment.end.tm_mday,
			entry->appointment.end.tm_hour,
			entry->appointment.end.tm_min,
			entry->appointment.end.tm_sec);

	osync_trace(TRACE_SENSITIVE, "advance: %i\n advanceUnits: %i\n repeatType: %i\n",
			entry->appointment.advance, entry->appointment.advanceUnits, entry->appointment.repeatType);
	osync_trace(TRACE_SENSITIVE, "repeatForever: %i\n repeatEnd.tm_year: %i\n repeatFrequency: %i\n",
			entry->appointment.repeatForever, entry->appointment.repeatEnd.tm_year, entry->appointment.repeatFrequency);
	osync_trace(TRACE_SENSITIVE, "repeatDay: %i\n repeatDays: %i %i %i %i %i %i %i\n repeatWeekstart: %i\n",
			entry->appointment.repeatDay, entry->appointment.repeatDays[0],
			entry->appointment.repeatDays[1], entry->appointment.repeatDays[2],
			entry->appointment.repeatDays[3], entry->appointment.repeatDays[4],
			entry->appointment.repeatDays[5], entry->appointment.repeatDays[6],
			entry->appointment.repeatWeekstart);
	osync_trace(TRACE_SENSITIVE, "execptions: %i\n tm_exception: NULL\n description: %s\n note: %s\n", entry->appointment.exceptions, entry->appointment.description ? entry->appointment.description : "nil", entry->appointment.note ? entry->appointment.note : "nil");	

	int i;
	for (i=0; i < entry->appointment.exceptions; i++) {
		osync_trace(TRACE_SENSITIVE, "exception[%i]: %04d-%02d-%02d", i,
				entry->appointment.exception[i].tm_year + 1900,
				entry->appointment.exception[i].tm_mon + 1,
				entry->appointment.exception[i].tm_mday);
	}

	//Create a new xml document
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "vcal");
	root = xmlNewTextChild(root, NULL, (xmlChar*)"Event", NULL);


	//Description
        if (entry->appointment.description) {
		//Convert from cp1252 -> utf8 ...
		tmp = conv_enc_palm_to_xml(entry->appointment.description);

		current = xmlNewTextChild(root, NULL, (xmlChar*)"Summary", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);

		g_free(tmp);
	}

	//Note
        if (entry->appointment.note) {
		//Convert from cp1252 -> utf8 ...
		tmp = conv_enc_palm_to_xml(entry->appointment.note);

		current = xmlNewTextChild(root, NULL, (xmlChar*)"Description", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);

		g_free(tmp);
	}

	// All Day Event
	if (entry->appointment.event == 1) {

		osync_trace(TRACE_SENSITIVE, "all-day event...");

		// Start
		tmp = osync_time_tm2vtime(&(entry->appointment.begin), FALSE);
		vtime = osync_time_datestamp(tmp);

		current = xmlNewTextChild(root, NULL, (xmlChar*)"DateStarted", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)vtime);
		//RFC2445: default value type is DATE-TIME. We alter to DATE
		xmlNewTextChild(current, NULL, (xmlChar*) "Value", (xmlChar*)"DATE");

		osync_trace(TRACE_SENSITIVE, "Start: %s", vtime ? vtime : "nil");

		g_free(tmp);
		g_free(vtime);
		
		// End
		tmp = osync_time_tm2vtime(&(entry->appointment.end), FALSE);
		vtime = osync_time_datestamp(tmp);

		current = xmlNewTextChild(root, NULL, (xmlChar*)"DateEnd", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)vtime);
		//RFC2445: default value type is DATE-TIME. We alter to DATE
		xmlNewTextChild(current, NULL, (xmlChar*) "Value", (xmlChar*)"DATE");

		osync_trace(TRACE_SENSITIVE, "End: %s", vtime ? vtime : "nil");

		g_free(tmp);
		g_free(vtime);
	} else {

		osync_trace(TRACE_SENSITIVE, "non-all-day event...");
		
		// Start
		tmp = osync_time_tm2vtime(&(entry->appointment.begin), FALSE);
		vtime = osync_time_vtime2utc(tmp);

		current = xmlNewTextChild(root, NULL, (xmlChar*)"DateStarted", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)vtime);

		osync_trace(TRACE_SENSITIVE, "Start: %s", vtime ? vtime : "nil");

		g_free(vtime);
		g_free(tmp);
		
		// End
		tmp = osync_time_tm2vtime(&(entry->appointment.end), FALSE);
		vtime = osync_time_vtime2utc(tmp);

		current = xmlNewTextChild(root, NULL, (xmlChar*)"DateEnd", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)vtime);

		osync_trace(TRACE_SENSITIVE, "End: %s", vtime ? vtime : "nil");

		g_free(vtime);
		g_free(tmp);
	}
		
	// Alarm
	if(entry->appointment.alarm) {
		xmlNode *alarm = xmlNewTextChild(root, NULL, (xmlChar*)"Alarm", NULL);

		osync_trace(TRACE_INTERNAL, "advance Unit: %i", entry->appointment.advanceUnits);
		switch(entry->appointment.advanceUnits) {
			case 1:
			case 4:	// TODO: check for devices if they need 4 (typo?)
				tmp = g_strdup_printf("-PT%iH", entry->appointment.advance);
				break;
			case 2:
				tmp = g_strdup_printf("-P%iD", entry->appointment.advance);
				break;
			case 0:	
			default:	
				tmp = g_strdup_printf("-PT%iM", entry->appointment.advance);
				break;

		}
		
		xmlNode *alarmtrigger = xmlNewTextChild(alarm, NULL, (xmlChar*) "AlarmTrigger", NULL);
		xmlNewTextChild(alarmtrigger, NULL, (xmlChar*) "Content", (xmlChar*) tmp);
		//XXX: This is not needed - value type DURATION is default (rfc2445 - 4.8.6.3 Trigger)
		// But kdepim-sync force us to do so.....
		xmlNewTextChild(alarmtrigger, NULL, (xmlChar*) "Value", (xmlChar*) "DURATION");

		g_free(tmp);
	}
	
	// Recurrence
	if (entry->appointment.repeatType != repeatNone) {
		
		int i;
		tmp = NULL;
		GString *rrulestr = g_string_new("");

		current = xmlNewTextChild(root, NULL, (xmlChar*) "RecurrenceRule", NULL);

		//Frequency
		switch (entry->appointment.repeatType) {
			case repeatDaily:
				tmp = g_strdup("FREQ=DAILY");
				break;
			case repeatWeekly:
				tmp = g_strdup("FREQ=WEEKLY");

				g_string_append(rrulestr, "BYDAY=");
				for(i = 0; i <= 6; i++) {
					if (entry->appointment.repeatDays[i]) {
						switch (i) {
							case 0:
								g_string_append(rrulestr, "SU,");
								break;
							case 1:
								g_string_append(rrulestr, "MO,");
								break;
							case 2:
								g_string_append(rrulestr, "TU,");
								break;
							case 3:
								g_string_append(rrulestr, "WE,");
								break;
							case 4:
								g_string_append(rrulestr, "TH,");
								break;
							case 5:
								g_string_append(rrulestr, "FR,");
								break;
							case 6:
								g_string_append(rrulestr, "SA,");
								break;
						}
					}
				}
				//Now remove the coma
				g_string_truncate(rrulestr, strlen(rrulestr->str) - 1);
				break;
			case repeatMonthlyByDate:
				tmp = g_strdup("FREQ=MONTHLY");
				g_string_append_printf(rrulestr, "BYMONTHDAY=%i", entry->appointment.begin.tm_mday);
				break;
			case repeatMonthlyByDay:
				tmp = g_strdup("FREQ=MONTHLY");

				int weekno = (entry->appointment.repeatDay / 7) + 1; 

				// TODO: 
				// testing 5 Week issuse?! :(
				if (weekno > 4)
					weekno = -1;

				g_string_append(rrulestr, "BYDAY=");
				switch (entry->appointment.repeatDay % 7) {
					case 0:
						g_string_append_printf(rrulestr, "%iSU", weekno);
						break;
					case 1:
						g_string_append_printf(rrulestr, "%iMO", weekno);
						break;
					case 2:
						g_string_append_printf(rrulestr, "%iTU", weekno);
						break;
					case 3:
						g_string_append_printf(rrulestr, "%iWE", weekno);
						break;
					case 4:
						g_string_append_printf(rrulestr, "%iTH", weekno);
						break;
					case 5:
						g_string_append_printf(rrulestr, "%iFR", weekno);
						break;
					case 6:
						g_string_append_printf(rrulestr, "%iSA", weekno);
						break;
				}
		
				break;
			case repeatYearly:
				tmp = g_strdup("FREQ=YEARLY");
				break;
			//repeatNone
			default:
				break;
		}
	
		// Frequencly
		xmlNewTextChild(current, NULL, (xmlChar*)"Rule", (xmlChar*) tmp);
		g_free(tmp);

		// Every #X YYYY
		if (strlen(rrulestr->str))
			xmlNewTextChild(current, NULL, (xmlChar*)"Rule", (xmlChar*) rrulestr->str);

		g_string_free(rrulestr, TRUE);

		// Interval
		// intveral value 1 is default and is not required as seperate field. would lead to conflicts.
		if (entry->appointment.repeatFrequency && entry->appointment.repeatFrequency != 1) {
			tmp = g_strdup_printf("INTERVAL=%i", entry->appointment.repeatFrequency);
			xmlNewTextChild(current, NULL, (xmlChar*)"Rule", (xmlChar*) tmp); 
			g_free(tmp);
		}

		// RepeatEnd
		if (!entry->appointment.repeatForever) {
			tmp = osync_time_tm2vtime(&(entry->appointment.repeatEnd), FALSE);
			vtime = g_strdup_printf("UNTIL=%s", tmp);
			xmlNewTextChild(current, NULL, (xmlChar*)"Rule", (xmlChar*) vtime);
			g_free(tmp);
			g_free(vtime);
		}

		// Exceptions
		if (entry->appointment.exceptions) {
			for (i = 0; i < entry->appointment.exceptions; i++) {
				current = xmlNewTextChild(root, NULL, (xmlChar*)"ExclusionDate", NULL);
				vtime = osync_time_tm2vtime(&(entry->appointment.exception[i]), FALSE);
				tmp = osync_time_datestamp(vtime);
				xmlNewTextChild(current, NULL, (xmlChar*) "Content", (xmlChar*) tmp);
				// workaround for date value property (rfc2445)
				// Palm device always stores exceptions as DATE default in rfc2445 is DATE-TIME
				xmlNewTextChild(current, NULL, (xmlChar*) "Value", (xmlChar*) "DATE");
				g_free(tmp);
				g_free(vtime);
			}
		}

	}
	// End of reccurence

	// Categories
	GList *c = NULL;
	current = NULL;
	for (c = entry->categories; c; c = c->next) {
		if (!current)
			current = xmlNewTextChild(root, NULL, (xmlChar*)"Categories", NULL);
		tmp = conv_enc_palm_to_xml((char *) c->data);
		osxml_node_add(current, "Category", tmp);
		g_free(tmp);
	}

	*free_input = TRUE;
	*output = (char *)doc;
	*outpsize = sizeof(doc);

	osync_trace(TRACE_SENSITIVE, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)doc) ? osxml_write_to_string((xmlDoc *)doc) : "nil");
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return FALSE;
}

static osync_bool conv_xml_to_palm_event(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);

	osync_trace(TRACE_SENSITIVE, "Input XML is:\n%s", osxml_write_to_string((xmlDoc *)input) ? osxml_write_to_string((xmlDoc *)input) : "nil");

	int i = 0;
	char *tmp = NULL;
	char *vtime = NULL;

	struct tm *start = NULL;
	struct tm *end = NULL;
	struct tm *tmptm = NULL;

	//Get the root node of the input document
	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		goto error;
	}
	
	if (xmlStrcmp(root->name, (const xmlChar *)"vcal")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xml root element");
		goto error;
	}

	/* Get the first child which should be Event */
	root = osxml_get_node(root, "Event");
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No Event child element");
		goto error;
	}

	/* Start the new entry */
	PSyncEventEntry *entry = osync_try_malloc0(sizeof(PSyncEventEntry), error);
	if (!entry)
		goto error;

	entry->appointment.event = 0;
	entry->appointment.alarm = 0;
	entry->appointment.advance = 0;
	entry->appointment.advanceUnits = 0;
	entry->appointment.repeatForever = 1;
	entry->appointment.repeatType = repeatNone;
	// XXX repeatFrequencly of ZERO will break your palm(-calendar) on recurrence entries
	entry->appointment.repeatFrequency = 1;
	entry->appointment.repeatWeekstart= 0;
	entry->appointment.exceptions = 0;
	entry->appointment.description = NULL;
	entry->appointment.note = NULL;

	//Description
	xmlNode *cur = osxml_get_node(root, "Description");
	if (cur) {
		tmp = (char *)xmlNodeGetContent(cur);
		entry->appointment.note = conv_enc_xml_to_palm(tmp); 
		g_free(tmp);
	}
	
	//Summary
	cur = osxml_get_node(root, "Summary");
	if (cur) {
		tmp = (char *)xmlNodeGetContent(cur);
		entry->appointment.description = conv_enc_xml_to_palm(tmp); 
		g_free(tmp);
	}

	//Start
	cur = osxml_get_node(root, "DateStarted");
	if (cur) {
		cur = osxml_get_node(cur, "Content");
		tmp = (char *)xmlNodeGetContent(cur);
		vtime = osync_time_vtime2localtime(tmp);	
		g_free(tmp);

		start = osync_time_vtime2tm(vtime);

		if (!osync_time_isdate(vtime)) {
			tmptm = osync_time_tm2localtime(start);
			g_free(start);
			start = tmptm;
		} else {
			entry->appointment.event = 1;
		}

		entry->appointment.begin = *start;
	
		osync_trace(TRACE_SENSITIVE, "DateStarted: %04d-%02d-%02d %02d:%02d:%02d [%s]",
				entry->appointment.begin.tm_year,
				entry->appointment.begin.tm_mon,
				entry->appointment.begin.tm_mday,
				entry->appointment.begin.tm_hour,
				entry->appointment.begin.tm_min,
				entry->appointment.begin.tm_sec,
				vtime ? vtime : "nil");

		g_free(vtime);
	}

	//End
	cur = osxml_get_node(root, "DateEnd");
	if (cur) {
		cur = osxml_get_node(cur, "Content");

		tmp = (char *)xmlNodeGetContent(cur);
		vtime = osync_time_vtime2localtime(tmp);	
		g_free(tmp);

		end = osync_time_vtime2tm(vtime);

		if (!osync_time_isdate(vtime)) {
			tmptm = osync_time_tm2localtime(end);
			g_free(end);
			end = tmptm;
		}
	
		entry->appointment.end = *end;
	
		osync_trace(TRACE_SENSITIVE, "DateEnd: %04d-%02d-%02d %02d:%02d:%02d [%s]",
				entry->appointment.end.tm_year,
				entry->appointment.end.tm_mon,
				entry->appointment.end.tm_mday,
				entry->appointment.end.tm_hour,
				entry->appointment.end.tm_min,
				entry->appointment.end.tm_sec,
				vtime ? vtime : "nil");

		g_free(vtime);
	}
	
	//Alarm
	cur = osxml_get_node(root, "Alarm");
	if (cur) {
		// enable alarm
		entry->appointment.alarm = 1;

		xmlNode *sub = osxml_get_node(cur, "AlarmTrigger");
		if (sub) {
			sub = osxml_get_node(sub, "Content");
			
			tmp = (char *) xmlNodeGetContent(sub);

			int is_signed = 0, is_digit = 0, digit = 0;
			int weeks = 0, days = 0, hours = 0, minutes = 0, seconds = 0;
			int i, advance_in_secs;

			for (i = 0; i < (int) strlen(tmp); i++) {

				switch(tmp[i])
				{
					case '-':
						is_signed = 1;
					case 'P':
					case 'T':	
					is_digit = 0;
					break;
					case 'W':
						is_digit = 0;
						weeks = digit;
						break;
					case 'D':
						is_digit = 0;
						days = digit;
						break;
					case 'H':
						is_digit = 0;
						hours = digit;
						break;
					case 'M':
						is_digit = 0;
						minutes = digit;
						break;
					case 'S':	
						is_digit = 0;
						seconds = digit; 
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

						sscanf((char*)(tmp+i), "%d", &digit);
						is_digit = 1;
						break;
				}
			}

			// TODO split this / modulize
			advance_in_secs = (weeks * 7 * 24 * 3600) + (days * 24 * 3600) + (hours * 3600) + (minutes * 60) + seconds; 
			osync_trace(TRACE_INTERNAL, "in seconds: %i", advance_in_secs);

			// use lowest used advance units
			if (weeks || days) {		// set days - weeks not supported as unit
				entry->appointment.advanceUnits = 4;
				entry->appointment.advance = advance_in_secs / 24 / 3600;
				if (weeks)
					entry->appointment.advance /= 7;
			}
			
			if (hours) {
				entry->appointment.advanceUnits = 2;
				entry->appointment.advance = advance_in_secs / 3600;
			}
			
			if (minutes || seconds)	{	// set minutes - seconds not supported as unit	
				entry->appointment.advanceUnits = 0;
  				entry->appointment.advance = advance_in_secs / 60;
			}

			if (!is_signed) 		// alarm after event start is not supported - disable alarm
				entry->appointment.alarm = 0;

			g_free(tmp);
		}
	}

	//recurrence
	cur = osxml_get_node(root, "RecurrenceRule");
	if (cur) {

		for (cur = cur->children; cur; cur = cur->next) {

			tmp = (char *) xmlNodeGetContent(cur);

			if (strstr(tmp, "DAILY")) {
				entry->appointment.repeatType = repeatDaily;
			} else if (strstr(tmp, "WEEKLY")) {
				entry->appointment.repeatType = repeatWeekly;
			} else if (strstr(tmp, "MONTHLY")) {
				// We suggest that the interval of the montly reccurence is by date.
				// Later we check for reccurence by day and change it if needed.
				entry->appointment.repeatType = repeatMonthlyByDate;
			} else if (strstr(tmp, "YEARLY")) {
				entry->appointment.repeatType = repeatYearly; 
			} else if (strstr(tmp, "BYDAY") && entry->appointment.repeatType == repeatWeekly) {
				gchar** weekdaystokens = g_strsplit(tmp, ",", 7);
				for (i=0; weekdaystokens[i] != NULL; i++) {
					if (strstr(weekdaystokens[i], "SU"))
						entry->appointment.repeatDays[0] = 1;
					else if (strstr(weekdaystokens[i], "MO"))
						entry->appointment.repeatDays[1] = 1;
					else if (strstr(weekdaystokens[i], "TU"))
						entry->appointment.repeatDays[2] = 1;
					else if (strstr(weekdaystokens[i], "WE"))
						entry->appointment.repeatDays[3] = 1;
					else if (strstr(weekdaystokens[i], "TH"))
						entry->appointment.repeatDays[4] = 1;
					else if (strstr(weekdaystokens[i], "FR"))
						entry->appointment.repeatDays[5] = 1;
					else if (strstr(weekdaystokens[i], "SA"))
						entry->appointment.repeatDays[6] = 1;
				}

			} else if (strstr(tmp, "BYDAY") && entry->appointment.repeatType == repeatMonthlyByDate) {
				// Change repeat Type of Montly recurrence if needed.
				entry->appointment.repeatType = repeatMonthlyByDay;

				int weekno = 1, wday = 0;
				char tmp_wday[] = "SU";

				sscanf(tmp, "BYDAY=%d%s", &weekno, tmp_wday);

				if (!strcmp("SU", tmp_wday)) 
					wday = 0;
				else if (!strcmp("MO", tmp_wday))
					wday = 1;
				else if (!strcmp("TU", tmp_wday))
					wday = 2;
				else if (!strcmp("WE", tmp_wday))
					wday = 3;
				else if (!strcmp("TH", tmp_wday))
					wday = 4;
				else if (!strcmp("FR", tmp_wday))
					wday = 5;
				else if (!strcmp("SA", tmp_wday))
					wday = 6;
				
				switch (weekno) {
					// first week
					case 1:
					case -5:	
						entry->appointment.repeatDay = 0 + wday;
						break;
					// second week	
					case 2:
					case -4:	
						entry->appointment.repeatDay = 7 + wday;
					break;
					case 3:
					case -3:	
						entry->appointment.repeatDay = 14 + wday;
						break;
					case 4:
					case -2:	
						entry->appointment.repeatDay = 21 + wday;
						break;
					// last week	
					default:	
						entry->appointment.repeatDay = 28 + wday;
						break;
				}

				osync_trace(TRACE_INTERNAL, "appointment.repeatDay: %i wday: %i", entry->appointment.repeatDay, wday);

						
			} else if (strstr(tmp, "UNTIL")) {
				struct tm *repeat_end = NULL;
				vtime = tmp; 
				vtime += strlen("UNTIL=");

				repeat_end = osync_time_vtime2tm(vtime);
							
				// when repeat_end is euqal or less then begin palm shows strange phenomenons
				if ((unsigned int) mktime(repeat_end) > (unsigned int) mktime(&(entry->appointment.begin))) {
					osync_trace(TRACE_INTERNAL, "UNTIL: %s", vtime ? vtime : "nil");
					entry->appointment.repeatEnd = *repeat_end;
					entry->appointment.repeatForever = 0;
				}
			} else if (strstr(tmp, "INTERVAL")) {
				int interval = 0;
				sscanf(tmp, "INTERVAL=%u", &interval);	
				entry->appointment.repeatFrequency = interval;
			}

			g_free(tmp);
		}
	}

	//Exceptions
	//There is a limit of execptions which may differ from palm to palm...
	//XXX Treo270: 5 Exceptions?
	xmlXPathObject *xobj = osxml_get_nodeset((xmlDoc *)input, "/vcal/Event/ExclusionDate");
	xmlNodeSet *nodes = xobj->nodesetval;
	entry->appointment.exception = g_malloc0(sizeof(struct tm) * nodes->nodeNr);

	for (i = 0; i < 5 && i < nodes->nodeNr; i++) {
		cur = nodes->nodeTab[i];

		struct tm *exclusion = NULL;
		cur = osxml_get_node(cur, "Content");
		vtime = (char *) xmlNodeGetContent(cur);


		exclusion = osync_time_vtime2tm(vtime);

		entry->appointment.exception[entry->appointment.exceptions] = *exclusion;
		entry->appointment.exceptions++;

		g_free(exclusion);
	}
	
	//Categories
	cur = osxml_get_node(root, "Categories");
	if (cur) {
		for (cur = cur->children; cur; cur = cur->next) {
			tmp = conv_enc_xml_to_palm((char*)xmlNodeGetContent(cur));
			entry->categories = g_list_append(entry->categories, tmp);
			g_free(tmp);
		}
	}
	
	*free_input = TRUE;
	*output = (void *)entry;
	*outpsize = sizeof(PSyncEventEntry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return FALSE;
}

static void destroy_palm_event(char *input, unsigned int inpsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, input, inpsize);
	PSyncEventEntry *entry = (PSyncEventEntry *)input;
	g_assert(inpsize == sizeof(PSyncEventEntry));
	
	g_free(entry->codepage);
	
	g_free(entry->appointment.exception);
	g_free(entry->appointment.description);
	g_free(entry->appointment.note);
	
	GList *c = NULL;
	for (c = entry->categories; c; c = c->next) {
		g_free(c->data);
	}
	
	if (entry->categories)
		g_list_free(entry->categories);
	
	g_free(entry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool marshall_palm_event(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %i, %p)", __func__, input, inpsize, output, outpsize, error);

        int i;
        int tmp_size, osize;

        g_assert(inpsize == sizeof(PSyncEventEntry));
        PSyncEventEntry *event = (PSyncEventEntry*) input;

        // get size
        osize = sizeof(PSyncEventEntry) + 1; 

        if (event->codepage)
                osize += strlen(event->codepage);

	osize += 1;

        if (event->appointment.description)
                osize += strlen(event->appointment.description); 

	osize += 1;

        if (event->appointment.note)
                osize += strlen(event->appointment.note); 

	osize += 1;

	GList *c = NULL;
	for (c = event->categories; c; c = c->next) {
		osize += strlen((char *) c->data);
		osize += 1;
	}
	osize += 1;

	osize += (sizeof(struct tm) + 1) * event->appointment.exceptions; 

        char *outevent = g_malloc0(osize);
        if (!outevent)
                goto error;

        char *outdata = ((char *)outevent) + sizeof(PSyncEventEntry) + 1;
        memcpy(outevent, event, sizeof(PSyncEventEntry));

        /* event->codepage */
        if (event->codepage) {
                tmp_size = strlen(event->codepage);
                memcpy(outdata, event->codepage, tmp_size);
                outdata += tmp_size;
        }
        outdata += 1;

        /* description */
        if (event->appointment.description) {
                tmp_size = strlen(event->appointment.description);
                memcpy(outdata, event->appointment.description, tmp_size);
                outdata += tmp_size;
        }
        outdata += 1;

        /* note */
        if (event->appointment.note) {
                tmp_size = strlen(event->appointment.note);
                memcpy(outdata, event->appointment.note, tmp_size);
                outdata += tmp_size;
        }
        outdata += 1;

	/* exception */
	for (i=0; i < event->appointment.exceptions; i++) {
		struct tm *exception = &(event->appointment.exception[i]);
		memcpy(outdata, exception, sizeof(struct tm));
		outdata += sizeof(struct tm);
		outdata += 1;
	}

	/* glist stuff comes at the end */
	for (c = event->categories; c; c = c->next) {
		tmp_size = strlen((char *) c->data);
		memcpy(outdata, c->data, tmp_size);
		outdata += tmp_size;
		outdata += 1;
	}	
	outdata += 1;

	/*	
        for (i=0; i < osize; i++)
                osync_trace(TRACE_SENSITIVE, "[%i] %c", i, outevent[i]);
	*/		

        *output = (char*)outevent;
        *outpsize = osize;

	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT, "%s: FALSE", __func__);
	return FALSE;	
}

osync_bool demarshall_palm_event(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %i, %p)", __func__, input, inpsize, output, outpsize, error);

        int i;
        int tmp_size;
        char *pos = NULL;
        PSyncEventEntry *newevent = NULL;
        /* get PSyncEventEntry struct */
        g_assert(inpsize >= sizeof(PSyncEventEntry));
        PSyncEventEntry *event = (PSyncEventEntry *)input;

        /* get PSyncEventEntry data */
        newevent = g_malloc0(sizeof(PSyncEventEntry));
        if (!newevent)
                goto error;

        memcpy(newevent, event, sizeof(PSyncEventEntry));
        pos = (char *) event + sizeof(PSyncEventEntry) + 1;

	newevent->codepage = NULL;
	newevent->categories = NULL;
	newevent->appointment.note = NULL;
	newevent->appointment.description = NULL;

	/*
        for (i=0; i < inpsize; i++)
                osync_trace(TRACE_SENSITIVE, "[%i] %c", i, input[i]);
	*/	
	
        /* event->codepage */
        if ((tmp_size = strlen(pos)) > 0) {
                newevent->codepage = strdup(pos);
        	pos += tmp_size;
	}
	pos += 1;

        /* description */
        if ((tmp_size = strlen(pos)) > 0) {
                newevent->appointment.description = strdup(pos);
        	pos += tmp_size;
	}
	pos += 1;

        /* note */
        if ((tmp_size = strlen(pos)) > 0) {
                newevent->appointment.note = strdup(pos);
	        pos += tmp_size;
	}
	pos += 1;

	osync_trace(TRACE_INTERNAL, "exception: %i", newevent->appointment.exceptions);
        /* exception */
        newevent->appointment.exception = g_malloc0(sizeof(struct tm) * newevent->appointment.exceptions);
	for (i=0; i < newevent->appointment.exceptions; i++) {
		memcpy(&(newevent->appointment.exception[i]), pos, sizeof(struct tm));
		pos += sizeof(struct tm);
		pos += 1;
	}

	/* (glist) categories... */
	newevent->categories = NULL;
	while ((tmp_size = strlen(pos)) > 0) {
		newevent->categories = g_list_append(newevent->categories, g_strdup(pos));
		pos += tmp_size;
		pos += 1;
	}
	pos += 1;

	osync_trace(TRACE_SENSITIVE, "codepage: [%s]", newevent->codepage ? newevent->codepage : "nil");
			
	osync_trace(TRACE_SENSITIVE, "note: [%s] desc: [%s]",
			newevent->appointment.note ? newevent->appointment.note : "nil",
			newevent->appointment.description ? newevent->appointment.description : "nil");

        *output = (char*) newevent;
        *outpsize = sizeof(PSyncEventEntry);

	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
error:
	osync_trace(TRACE_EXIT, "%s: FALSE", __func__);
	return FALSE;	
}

static osync_bool conv_palm_todo_to_xml(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
	PSyncTodoEntry *entry = (PSyncTodoEntry *)input;
	char *tmp = NULL;
	char *date = NULL; 
	xmlNode *current = NULL;
	
	if (inpsize != sizeof(PSyncTodoEntry)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		goto error;
	}
	
	//Create a new xml document
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "vcal");
	root = xmlNewTextChild(root, NULL, (xmlChar*)"Todo", NULL);


	//Description
	if (entry->todo.note) {
		//Convert from cp1252 -> utf8 ...
		tmp = conv_enc_palm_to_xml(entry->todo.note); 

		current = xmlNewTextChild(root, NULL, (xmlChar*)"Description", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);

		g_free(tmp);
	}
	
	//Summary
	if (entry->todo.description) {
		//Convert from cp1252 -> utf8 ...
		tmp = conv_enc_palm_to_xml(entry->todo.description); 

		current = xmlNewTextChild(root, NULL, (xmlChar*)"Summary", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);

		g_free(tmp);
	}
	
	//priority
	if (entry->todo.priority) {
		//Palm device only have priority range from 1 to 5.
		//RFC2445 have the priority range from 0 - 9
		//FIXME: change offset value?
		tmp = g_strdup_printf("%i", entry->todo.priority + 2);
		current = xmlNewTextChild(root, NULL, (xmlChar*)"Priority", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
		g_free(tmp);
	}

	//due
	// RFC2445: 4.8.2.3 Date/Time Due 
	if (entry->todo.indefinite == 0) {
		tmp = osync_time_tm2vtime(&(entry->todo.due), FALSE); 
		// palm t|x is only able to handle datestamp (no timestamp!) dues
		date = osync_time_datestamp(tmp);
		current = xmlNewTextChild(root, NULL, (xmlChar*)"DateDue", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)date);
		// RFC2445 says the default valute type is DATE-TIME. But the Palm only
		// stores DATE as due date => alter VALUE to DATE
		xmlNewTextChild(current, NULL, (xmlChar*) "Value", (xmlChar*) "DATE");
		g_free(tmp);
		g_free(date);
	}

	//completed
	// RFC2445: 4.8.2.1 Date/Time Completed  
	if (entry->todo.complete) {
		//FIXME At the moment pilot-link 0.11.8 / 0.12.0-prev4 doesn't provide the completed timestamp.
		//As workaround we take the current timestamp. This will lead to conflicts on slow-syncs.
		time_t now = time(NULL);
		//RFC2445 says the timestamp must be UTC.
		tmp = osync_time_unix2vtime(&now); 
		current = xmlNewTextChild(root, NULL, (xmlChar*)"Completed", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
		g_free(tmp);
	}

	GList *c = NULL;
	current = NULL;
	for (c = entry->categories; c; c = c->next) {
		if (!current)
			current = xmlNewTextChild(root, NULL, (xmlChar*)"Categories", NULL);
		tmp = conv_enc_palm_to_xml((char *) c->data);
		osxml_node_add(current, "Category", tmp);
		g_free(tmp);
	}

	*free_input = TRUE;
	*output = (char *)doc;
	*outpsize = sizeof(doc);

	osync_trace(TRACE_SENSITIVE, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)doc) ? osxml_write_to_string((xmlDoc *)doc) : "nil");
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return FALSE;
}

static osync_bool conv_xml_to_palm_todo(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);

	char *tmp = NULL;

	osync_trace(TRACE_SENSITIVE, "Input XML is:\n%s", osxml_write_to_string((xmlDoc *)input) ? osxml_write_to_string((xmlDoc *)input) : "nil");
	
	//Get the root node of the input document
	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		goto error;
	}
	
	if (xmlStrcmp(root->name, (const xmlChar *)"vcal")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xml root element");
		goto error;
	}

	/* Get the first child which should be Todo */
	root = osxml_get_node(root, "Todo");
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No Todo child element");
		goto error;
	}

	/* Start the new entry */
	PSyncTodoEntry *entry = osync_try_malloc0(sizeof(PSyncTodoEntry), error);
	if (!entry)
		goto error;

	entry->todo.priority = 0;
	entry->todo.complete = 0;
	entry->todo.description = "";
	entry->todo.note = "";
	entry->todo.indefinite = 1;

	//Priority
	xmlNode *cur = osxml_get_node(root, "Priority");
	if (cur) {
		char *prio = (char *)xmlNodeGetContent(cur);
		if (prio) {
			entry->todo.priority = atoi(prio) - 2;
			if (entry->todo.priority < 1) {
				//Never go lower than 1
				entry->todo.priority = 1;
			}
			if (atoi(prio) == 0) {
				//Default to priority 5
				entry->todo.priority = 5;
			}
			g_free(prio);
		}
	}
	
	//Completed
	cur = osxml_get_node(root, "Completed");
	if (cur)
		entry->todo.complete = 1;

	//Summary
	cur = osxml_get_node(root, "Summary");
	if (cur) {
		tmp = (char *)xmlNodeGetContent(cur);
		entry->todo.description = conv_enc_xml_to_palm(tmp);
		g_free(tmp);
	}
	
	//Description
	cur = osxml_get_node(root, "Description");
	if (cur) {
		tmp = (char *)xmlNodeGetContent(cur);
		entry->todo.note = conv_enc_xml_to_palm(tmp); 
		g_free(tmp);
	}
	
	//Due
	cur = osxml_get_node(root, "DateDue");
	if (cur) {
		struct tm *due = osync_time_vtime2tm((char *) xmlNodeGetContent(cur));
		entry->todo.due = *due;
		entry->todo.indefinite = 0;
		g_free(due);
	}

	//Categories
	cur = osxml_get_node(root, "Categories");
	if (cur) {
		for (cur = cur->children; cur; cur = cur->next) {
			tmp = conv_enc_xml_to_palm((char*)xmlNodeGetContent(cur));
			entry->categories = g_list_append(entry->categories, tmp);
			g_free(tmp);
		}
	}
	
	*free_input = TRUE;
	*output = (void *)entry;
	*outpsize = sizeof(PSyncTodoEntry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return FALSE;
}

static void destroy_palm_todo(char *input, unsigned int inpsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, input, inpsize);
	PSyncTodoEntry *entry = (PSyncTodoEntry *)input;
	g_assert(inpsize == sizeof(PSyncTodoEntry));

	g_free(entry->codepage);

	g_free(entry->todo.description);
	g_free(entry->todo.note);

	GList *c = NULL;
	for (c = entry->categories; c; c = c->next) {
		g_free(c->data);
	}
	
	if (entry->categories)
		g_list_free(entry->categories);
	
	g_free(entry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool marshall_palm_todo(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %i, %p)", __func__, input, inpsize, output, outpsize, error);
	
        int tmp_size, osize;

        g_assert(inpsize == sizeof(PSyncTodoEntry));
        PSyncTodoEntry *todo = (PSyncTodoEntry*) input;

        // get size
        osize = sizeof(PSyncTodoEntry) + 1; 

        if (todo->codepage)
                osize += strlen(todo->codepage);

	osize += 1;

        if (todo->todo.description)
                osize += strlen(todo->todo.description); 

	osize += 1;

        if (todo->todo.note)
                osize += strlen(todo->todo.note); 

	osize += 1;

	GList *c = NULL;
	for (c = todo->categories; c; c = c->next) {
		osize += strlen((char *) c->data);
		osize += 1;
	}

	osize += 1;


        char *outtodo = g_malloc0(osize);
        if (!outtodo)
                goto error;

        char *outdata = ((char *)outtodo) + sizeof(PSyncTodoEntry) + 1;
        memcpy(outtodo, todo, sizeof(PSyncTodoEntry));

        /* todo->codepage */
        if (todo->codepage) {
                tmp_size = strlen(todo->codepage);
                memcpy(outdata, todo->codepage, tmp_size);
                outdata += tmp_size;
        }
        outdata +=1;

        /* description */
        if (todo->todo.description) {
                tmp_size = strlen(todo->todo.description);
                memcpy(outdata, todo->todo.description, tmp_size);
                outdata += tmp_size;
        }
        outdata += 1;

        /* note */
        if (todo->todo.note) {
                tmp_size = strlen(todo->todo.note);
               memcpy(outdata, todo->todo.note, tmp_size);
                outdata += tmp_size;
        }
        outdata += 1;

	/* glist stuff comes at the end */
	for (c = todo->categories; c; c = c->next) {
		tmp_size = strlen((char *) c->data);
		memcpy(outdata, c->data, tmp_size);
		outdata += tmp_size;
		outdata += 1;
	}	
	outdata += 1;

	/*
	int i;   
        for (i=0; i < osize; i++)
                osync_trace(TRACE_SENSITIVE, "[%i] %c", i, outtodo[i]);
	*/	

        *output = (char*)outtodo;
        *outpsize = osize;

	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT, "%s: FALSE", __func__);
	return FALSE;	
}

osync_bool demarshall_palm_todo(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %i, %p)", __func__, input, inpsize, output, outpsize, error);
	
        int tmp_size;
        char *pos = NULL;
        PSyncTodoEntry *newtodo = NULL;
        /* get PSyncTodoEntry struct */
        g_assert(inpsize >= sizeof(PSyncTodoEntry));
        PSyncTodoEntry *todo = (PSyncTodoEntry *)input;

        /* get PSyncTodoEntry data */
        newtodo = g_malloc0(sizeof(PSyncTodoEntry));
        if (!newtodo)
                goto error;

        memcpy(newtodo, todo, sizeof(PSyncTodoEntry));
        pos = (char *) todo + sizeof(PSyncTodoEntry) + 1;

	newtodo->codepage = NULL;

	newtodo->todo.note = NULL;
	newtodo->todo.description = NULL;

	/*
	int i;	   
        for (i=0; i < inpsize; i++)
                osync_trace(TRACE_SENSITIVE, "[%i] %c", i, input[i]);
	*/	

        /* todo->codepage */
        if ((tmp_size = strlen(pos)) > 0) {
                newtodo->codepage = strdup(pos);
        	pos += tmp_size;
	}
	pos += 1;

        /* description */
        if ((tmp_size = strlen(pos)) > 0) {
                newtodo->todo.description = strdup(pos);
        	pos += tmp_size;
	}
	pos += 1;

        /* note */
        if ((tmp_size = strlen(pos)) > 0) {
                newtodo->todo.note = strdup(pos);
	        pos += tmp_size;
	}
	pos += 1;

	/* (glist) categories... */
	newtodo->categories = NULL;
	while ((tmp_size = strlen(pos)) > 0) {
		newtodo->categories = g_list_append(newtodo->categories, g_strdup(pos));
		pos += tmp_size;
		pos += 1;
	}
	pos += 1;

	osync_trace(TRACE_SENSITIVE, "codepage: [%s]", nnewtodo->codepage ? ewtodo->codepage : "nil");


	osync_trace(TRACE_SENSITIVE, "desc: [%s] note: [%s]",
			newtodo->todo.description ? newtodo->todo.description : "nil",
			newtodo->todo.note ? newtodo->todo.note : "nil");

        *output = (char*) newtodo;
        *outpsize = sizeof(PSyncTodoEntry);

	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
error:
	osync_trace(TRACE_EXIT, "%s: FALSE", __func__);
	return FALSE;	
}
#endif

static char *return_next_entry(PSyncContactEntry *entry, unsigned int i)
{	
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, entry, i);
	char *tmp = NULL;
	
	osync_trace(TRACE_SENSITIVE, "Entry: %s (%p)", entry->address.entry[i] ? entry->address.entry[i] : "nil", entry->address.entry[i]);
	if (entry->address.entry[i] && strlen(entry->address.entry[i]) > 0)
		tmp = conv_enc_palm_to_xml(entry->address.entry[i]);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return tmp;
}

static osync_bool has_entry(PSyncContactEntry *entry, unsigned int i)
{
	return entry->address.entry[i] ? TRUE : FALSE;
}


static osync_bool _palmcontact_address(OSyncXMLFormat *xmlformat, PSyncContactEntry *entry, OSyncError **error)
{
	if (!(has_entry(entry, entryAddress) 
		|| has_entry(entry, entryCity) || has_entry(entry, entryState) 
		|| has_entry(entry, entryZip) || has_entry(entry, entryCountry)))
		return TRUE;

	char *tmp_address = return_next_entry(entry, entryAddress);
	char *tmp_city = return_next_entry(entry, entryCity);
	char *tmp_state = return_next_entry(entry, entryState);
	char *tmp_zip = return_next_entry(entry, entryZip);
	char *tmp_country = return_next_entry(entry, entryCountry);

	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Address", error);
	if (!xmlfield)
		return FALSE;

	//Street
	if (tmp_address) {
		osync_xmlfield_set_key_value(xmlfield, "Street", tmp_address);
		g_free(tmp_address);
	}

	//City
	if (tmp_city) {
		osync_xmlfield_set_key_value(xmlfield, "City", tmp_city);
		g_free(tmp_city);
	}
	
	//Region
	if (tmp_state) {
		osync_xmlfield_set_key_value(xmlfield, "Region", tmp_state);
		g_free(tmp_state);
	}
	
	//Code
	if (tmp_zip) {
		osync_xmlfield_set_key_value(xmlfield, "PostalCode", tmp_zip);
		g_free(tmp_zip);
	}
	
	//Country
	if (tmp_country) {
		osync_xmlfield_set_key_value(xmlfield, "Country", tmp_country);
		g_free(tmp_country);
	}

	return TRUE;
}

static osync_bool _palmcontact_name(OSyncXMLFormat *xmlformat, PSyncContactEntry *entry, OSyncError **error)
{
	if (!(has_entry(entry, entryLastname) || has_entry(entry, entryFirstname)))
		return TRUE;
	
	char *tmp_first = return_next_entry(entry, entryFirstname);
	char *tmp_last  = return_next_entry(entry, entryLastname);

	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Name", error);
	if (!xmlfield)
		return FALSE;

	//First Name
	if (tmp_first) {
		osync_xmlfield_set_key_value(xmlfield, "FirstName", tmp_first);
		g_free(tmp_first);
	}

	//Last Name
	if (tmp_last) {
		osync_xmlfield_set_key_value(xmlfield, "LastName", tmp_last);
		g_free(tmp_last);
	}

	return TRUE;
}

static osync_bool _palmcontact_organization(OSyncXMLFormat *xmlformat, PSyncContactEntry *entry, OSyncError **error)
{
	char *tmp = return_next_entry(entry, entryCompany);

	// Empty?
	if (!tmp)
		return TRUE;

	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Organization", error);
	if (!xmlfield)
		return FALSE;

	osync_xmlfield_set_key_value(xmlfield, "Name", tmp);
	g_free(tmp);

	return TRUE;
}

static osync_bool _palmcontact_telephone(OSyncXMLFormat *xmlformat, PSyncContactEntry *entry, OSyncError **error)
{
	int i;
	char *tmp = NULL;
	OSyncXMLField *xmlfield = NULL;
	for (i = entryPhone1; i <= entryPhone5; i++) {
		tmp = return_next_entry(entry, i);
		if (!tmp)
			continue;

		osync_trace(TRACE_SENSITIVE, "phone #%i: [%i][telephone type /email == 4]", i, entry->address.phoneLabel[i - 3]);

		xmlfield = NULL;

		if (entry->address.phoneLabel[i - 3] == 4)
			xmlfield = osync_xmlfield_new(xmlformat, "EMail", error);
		else
			xmlfield = osync_xmlfield_new(xmlformat, "Telephone", error);

		if (!xmlfield)
			return FALSE;
		
		osync_xmlfield_set_key_value(xmlfield, "Content", tmp);
		g_free(tmp);
		
		switch (entry->address.phoneLabel[i - 3]) {
			case 0:
				// Work
				osync_xmlfield_set_attr(xmlfield, "Location", "Work");
				break;
			case 1:
				// Home
				osync_xmlfield_set_attr(xmlfield, "Location", "Home");
				break;
			case 2:
				// Fax
				osync_xmlfield_set_attr(xmlfield, "Type", "Fax");
				break;
			case 3:
				// Other FIXME Voice!=Other .. Other is not part of xmlformat-contact xsd
				osync_xmlfield_set_attr(xmlfield, "Type", "Voice");
				break;
			case 4:
				// E Mail
				break;	
			case 5:
				// Main FIXME Mail != Prefered !?
				osync_xmlfield_set_attr(xmlfield, "Preferred", "true");
				break;
			case 6:
				// Pager
				osync_xmlfield_set_attr(xmlfield, "Type", "Pager");
				break;
			case 7:
				// Mobile / Cellular
				osync_xmlfield_set_attr(xmlfield, "Type", "Cellular");
				break;
		}
		
	}
	return TRUE;
}

//Title
static osync_bool _palmcontact_title(OSyncXMLFormat *xmlformat, PSyncContactEntry *entry, OSyncError **error)
{
	char *tmp = return_next_entry(entry, entryTitle);
	if (!tmp)
		return TRUE;

	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Title", error);
	if (!xmlfield)
		return FALSE;

	osync_xmlfield_set_key_value(xmlfield, "Content", tmp);
	g_free(tmp);

	return TRUE;
}
		

//Note
static osync_bool _palmcontact_note(OSyncXMLFormat *xmlformat, PSyncContactEntry *entry, OSyncError **error)
{
	char *tmp = return_next_entry(entry, entryNote);
	if (!tmp)
		return TRUE;


	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Note", error);
	if (!xmlfield)
		return FALSE;

	osync_xmlfield_set_key_value(xmlfield, "Content", tmp);
	g_free(tmp);

	return TRUE;
}

// Categories
static osync_bool _palmcontact_categories(OSyncXMLFormat *xmlformat, PSyncContactEntry *entry, OSyncError **error)
{
	GList *c = NULL;
	OSyncXMLField *xmlfield = NULL;

	for (c = entry->categories; c; c = c->next) {
		if (!xmlfield)
			xmlfield = osync_xmlfield_new(xmlformat, "Categories", error);

		if (!xmlfield)
			return FALSE;

		char *tmp = conv_enc_palm_to_xml((char *) c->data);
		osync_xmlfield_set_key_value(xmlfield, "Category", tmp);
		g_free(tmp);
	}

	return TRUE;
}

static osync_bool conv_palm_contact_to_xml(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %s, %p)", __func__, input, inpsize, output, outpsize, free_input, config ? config : "nil", error);

	PSyncContactEntry *entry = (PSyncContactEntry *)input;

	int i;
	char *tmp = NULL;
	xmlNode *current = NULL;
	
	if (inpsize != sizeof(PSyncContactEntry)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		goto error;
	}
	
	for (i = 0; i < 19; i++)
		osync_trace(TRACE_SENSITIVE, "entry %i: %s", i, entry->address.entry[i] ? entry->address.entry[i] : "nil");
	
	//Create a new xmlformat
	OSyncXMLFormat *xmlformat = osync_xmlformat_new("contact", error);
	OSyncXMLField *xmlfield = NULL;

	// Address
	if (!_palmcontact_address(xmlformat, entry, error))
		goto error;

	// Categories
	if (!_palmcontact_categories(xmlformat, entry, error))
		goto error;
	// Name
	if (!_palmcontact_name(xmlformat, entry, error))
		goto error;

	// Note
	if (!_palmcontact_note(xmlformat, entry, error))
		goto error;

	// Organization 
	if (!_palmcontact_organization(xmlformat, entry, error))
		goto error;

	// Title
	if (!_palmcontact_title(xmlformat, entry, error))
		goto error;

	// Telephone
	if (!_palmcontact_telephone(xmlformat, entry, error))
		goto error;

	
	*free_input = TRUE;
	*output = (char *)xmlformat;
	*outpsize = sizeof(xmlformat);

        unsigned int size;
        char *str;
        osync_xmlformat_assemble(xmlformat, &str, &size);
        osync_trace(TRACE_SENSITIVE, "Output XMLFormat is:\n%s", str);
        g_free(str);

        if (osync_xmlformat_validate(xmlformat) == FALSE)
                osync_trace(TRACE_INTERNAL, "XMLFORMAT CONTACT: Not valid!");
        else
                osync_trace(TRACE_INTERNAL, "XMLFORMAT CONTACT: VAILD");


	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return FALSE;
}


static void _xmlfield_name(PSyncContactEntry *entry, OSyncXMLField *xmlfield)
{
	entry->address.entry[0] = g_strdup(osync_xmlfield_get_key_value(xmlfield, "LastName"));
	entry->address.entry[1] = g_strdup(osync_xmlfield_get_key_value(xmlfield, "FirstName"));
}

static void _xmlfield_organization(PSyncContactEntry *entry, OSyncXMLField *xmlfield)
{
	entry->address.entry[2] = g_strdup(osync_xmlfield_get_key_value(xmlfield, "Name"));
}

static void _xmlfield_telephone(PSyncContactEntry *entry, OSyncXMLField *xmlfield, int *pos)
{
	entry->address.entry[3 + *pos] = g_strdup(osync_xmlfield_get_key_value(xmlfield, "Content"));

	const char *type = osync_xmlfield_get_attr(xmlfield, "Type");
	const char *location = osync_xmlfield_get_attr(xmlfield, "Location");
	const char *pref = osync_xmlfield_get_attr(xmlfield, "Preferred");

	if (location && !strcmp(location, "Work"))
		entry->address.phoneLabel[*pos] = 0;
	else if (location && !strcmp(location, "Home"))
		entry->address.phoneLabel[*pos] = 1;
	else if (type && !strcasecmp(type, "Fax"))
		entry->address.phoneLabel[*pos] = 2;
	else if (type && !strcmp(type, "Voice"))
		entry->address.phoneLabel[*pos] = 3;
	else if (pref && !strcmp(type, "true"))
		entry->address.phoneLabel[*pos] = 5;
	else if (type && !strcmp(type, "Pager"))
		entry->address.phoneLabel[*pos] = 6;
	else if (type && !strcmp(type, "Cellular"))
		entry->address.phoneLabel[*pos] = 7;

	(*pos)++;
}

static void _xmlfield_email(PSyncContactEntry *entry, OSyncXMLField *xmlfield, int *pos)
{
	entry->address.entry[3 + *pos] = g_strdup(osync_xmlfield_get_key_value(xmlfield, "Content")); 
	entry->address.phoneLabel[*pos] = 4;
}

static void _xmlfield_address(PSyncContactEntry *entry, OSyncXMLField *xmlfield)
{
	entry->address.entry[8] = g_strdup(osync_xmlfield_get_key_value(xmlfield, "Street"));
	entry->address.entry[9] = g_strdup(osync_xmlfield_get_key_value(xmlfield, "City"));
	entry->address.entry[10] = g_strdup(osync_xmlfield_get_key_value(xmlfield, "Region"));
	entry->address.entry[11] = g_strdup(osync_xmlfield_get_key_value(xmlfield, "PostalCode"));
	entry->address.entry[12] = g_strdup(osync_xmlfield_get_key_value(xmlfield, "Country"));
}

static void _xmlfield_title(PSyncContactEntry *entry, OSyncXMLField *xmlfield)
{
	entry->address.entry[13] = g_strdup(osync_xmlfield_get_key_value(xmlfield, "Content")); 
}

static void _xmlfield_note(PSyncContactEntry *entry, OSyncXMLField *xmlfield)
{
	entry->address.entry[18] = g_strdup(osync_xmlfield_get_key_value(xmlfield, "Content")); 
}

static void _xmlfield_category(PSyncContactEntry *entry, OSyncXMLField *xmlfield)
{

	int i, numnodes;
	numnodes = osync_xmlfield_get_key_count(xmlfield);
	for (i=0; i < numnodes; i++) {
		char *tmp = conv_enc_xml_to_palm(osync_xmlfield_get_nth_key_value(xmlfield, i));
		entry->categories = g_list_append(entry->categories, tmp);
		g_free(tmp);
	}
}

static osync_bool conv_xml_to_palm_contact(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %s, %p)", __func__, input, inpsize, output, outpsize, free_input, config ? config : "nil", error);

	OSyncXMLFormat *xmlformat = (OSyncXMLFormat *)input;
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(xmlformat, &str, &size);
	osync_trace(TRACE_SENSITIVE, "Input XMLFormat is:\n%s", str);
	g_free(str);

	char *tmp = NULL;
	
	if (strcmp("contact", osync_xmlformat_get_objtype(xmlformat))) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xmlformat: %s",  
				osync_xmlformat_get_objtype(xmlformat));
		goto error;
	}

	//Get the first field 
	OSyncXMLField *xmlfield = osync_xmlformat_get_first_field(xmlformat);

	if (!xmlfield) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get first xmlfield");
		goto error;
	}

	/* Start the new entry */
	PSyncContactEntry *entry = osync_try_malloc0(sizeof(PSyncContactEntry), error);
	if (!entry)
		goto error;

	/* Set the entries */
	entry->address.phoneLabel[0] = 0;
	entry->address.phoneLabel[1] = 1;
	entry->address.phoneLabel[2] = 2;
	entry->address.phoneLabel[3] = 3;
	entry->address.phoneLabel[4] = 4;
	entry->address.showPhone = 0;
	
	int pos = 0;

	/* TODO This is ugly - since this is sorted this could be done much faster */
	for (; xmlfield; xmlfield = osync_xmlfield_get_next(xmlfield)) {
		osync_trace(TRACE_INTERNAL, "Field: %s", osync_xmlfield_get_name(xmlfield));

		if (!strcmp("Name", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_name(entry, xmlfield);
		if (!strcmp("Organization", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_name(entry, xmlfield);
		else if (!strcmp("Telephone", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_telephone(entry, xmlfield, &pos);
		else if (!strcmp("EMail", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_email(entry, xmlfield, &pos);
		else if (!strcmp("Address", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_address(entry, xmlfield);
		else if (!strcmp("Title", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_title(entry, xmlfield);
		else if (!strcmp("Note", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_note(entry, xmlfield);
		else if (!strcmp("Categories", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_category(entry, xmlfield);

	}


	int i;
	/* Now convert to the charset */
	for (i = 0; i < 19; i++) {
	  if (entry->address.entry[i]) {
		// FIXME cp1252 hardcoded? entry->codepage is null ... how to get this in the palm env?		  
		char *tmp = conv_enc_xml_to_palm(entry->address.entry[i]);
		g_free(entry->address.entry[i]);
		entry->address.entry[i] = tmp;
	 	osync_trace(TRACE_SENSITIVE, "entry %i: %s", i, entry->address.entry[i] ? entry->address.entry[i] : "nil");
	  }
	}
	
	osync_trace(TRACE_INTERNAL, "DONE");
	
	*free_input = TRUE;
	*output = (void *)entry;
	*outpsize = sizeof(PSyncContactEntry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return FALSE;
}

static void destroy_palm_contact(char *input, unsigned int inpsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, input, inpsize);
	PSyncContactEntry *entry = (PSyncContactEntry *)input;
	g_assert(inpsize == sizeof(PSyncContactEntry));
	
	g_free(entry->codepage);
	
	int i = 0;
	for (i = 0; i < 19; i++) {
		if (entry->address.entry[i])
			g_free(entry->address.entry[i]);
	}
	
	GList *c = NULL;
	for (c = entry->categories; c; c = c->next) {
		g_free(c->data);
	}
	
	g_list_free(entry->categories);
	
	g_free(entry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static osync_bool marshal_palm_contact(const char *input, unsigned int inpsize, OSyncMessage *message, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p)", __func__, input, inpsize, message, error);
	g_assert(inpsize == sizeof(PSyncContactEntry));
	
	PSyncContactEntry *contact = (PSyncContactEntry*)input;
	
	osync_message_write_buffer(message, &(contact), sizeof(PSyncContactEntry));
	osync_message_write_string(message, contact->codepage);
	
	int i = 0;
	for (i = 0; i < 19; i++) {
		osync_message_write_string(message, contact->address.entry[i]);
	}

	osync_message_write_int(message, g_list_length(contact->categories));
	GList *c = NULL;
	for (c = contact->categories; c; c = c->next) {
		osync_message_write_string(message, (char *)c->data);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;	
}

static osync_bool demarshal_palm_contact(OSyncMessage *message, char **output, unsigned int *outpsize, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, message, output, outpsize, error);
	
	void *buffer = NULL;
	int size = 0;
	osync_message_read_buffer(message, &(buffer), &size);
	PSyncContactEntry *contact = buffer;
	osync_message_read_string(message, &(contact->codepage));
	
	int i = 0;
	for (i = 0; i < 19; i++) {
		osync_message_read_string(message, &(contact->address.entry[i]));
	}
	
	int num = 0;
	osync_message_read_int(message, &num);
	
	char *category = NULL;
	contact->categories = NULL;
	for (i = 0; i < num; i++) {
		osync_message_read_string(message, &category);
		contact->categories = g_list_append(contact->categories, category);
	}
			
	*output = (char*)contact;
	*outpsize = sizeof(PSyncContactEntry);

	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

#if 0
static osync_bool conv_palm_note_to_xml(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %s, %p)", __func__, input, inpsize, output, outpsize, free_input, config ? config : "nil", error);
	PSyncNoteEntry *entry = (PSyncNoteEntry *)input;

	char *tmp = NULL;
	xmlNode *current = NULL;
	
	if (inpsize != sizeof(PSyncNoteEntry)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		goto error;
	}
	
	//Create a new xml document
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "Note");

	// convert memo.text from cp1252 -> utf8 
	tmp = g_strdup(entry->memo.text);
	g_free(entry->memo.text);
	entry->memo.text = conv_enc_xml_to_palm(tmp);
	g_free(tmp);

	// Summary & Body
	if (entry->memo.text) {
		gchar **splitMemo = g_strsplit(entry->memo.text, "\n", 2);

		current = xmlNewTextChild(root, NULL, (xmlChar*)"Summary", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)splitMemo[0]);

		current = xmlNewTextChild(root, NULL, (xmlChar*)"Body", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)splitMemo[1]);

		g_strfreev(splitMemo);
	}

	GList *c = NULL;
	current = NULL;
	for (c = entry->categories; c; c = c->next) {
		if (!current)
			current = xmlNewTextChild(root, NULL, (xmlChar*)"Categories", NULL);
		tmp = conv_enc_palm_to_xml((char *) c->data);
		osxml_node_add(current, "Category", tmp);
		g_free(tmp);
	}

	*free_input = TRUE;
	*output = (char *)doc;
	*outpsize = sizeof(doc);

	osync_trace(TRACE_SENSITIVE, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)doc) ? osxml_write_to_string((xmlDoc *)doc) : "nil");
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return FALSE;
}


static osync_bool conv_xml_to_palm_note(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %s, %p)", __func__, input, inpsize, output, outpsize, free_input, config ? config : "nil", error);

	char *tmp = NULL;
	xmlNode *cur = NULL;
	GString *memo = g_string_new("");

	osync_trace(TRACE_SENSITIVE, "Input XML is:\n%s", osxml_write_to_string((xmlDoc *)input) ? osxml_write_to_string((xmlDoc *)input) : "nil");

	//Get the root node of the input document
	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		goto error;
	}
	
	if (xmlStrcmp(root->name, (const xmlChar *)"Note")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xml root element");
		goto error;
	}

	/* Start the new entry */
	PSyncNoteEntry *entry = osync_try_malloc0(sizeof(PSyncNoteEntry), error);
	if (!entry)
		goto error;

	entry->memo.text = "";

	// Summary 
	cur = osxml_get_node(root, "Summary");
	if (cur)
		memo = g_string_append(memo, (char *)xmlNodeGetContent(cur));
	
	// Body
	cur = osxml_get_node(root, "Body"); 
	if (cur)
		memo = g_string_append(memo, (char *)xmlNodeGetContent(cur));

	entry->memo.text = (char *) g_string_free(memo, FALSE);

	// convert memo.text from utf8 -> cp1252
	tmp = g_strdup(entry->memo.text);
	g_free(entry->memo.text);
	entry->memo.text = conv_enc_xml_to_palm(tmp);
	g_free(tmp);
	
	//Categories
	cur = osxml_get_node(root, "Categories");
	if (cur) {
		for (cur = cur->children; cur; cur = cur->next) {
			tmp = conv_enc_xml_to_palm((char*)xmlNodeGetContent(cur));
			entry->categories = g_list_append(entry->categories, g_strdup(tmp));
			g_free(tmp);
		}
	}
	
	*free_input = TRUE;
	*output = (void *)entry;
	*outpsize = sizeof(PSyncNoteEntry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return FALSE;
}

static void destroy_palm_note(char *input, unsigned int inpsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, input, inpsize);
	PSyncNoteEntry *entry = (PSyncNoteEntry *)input;
	g_assert(inpsize == sizeof(PSyncNoteEntry));
	
	g_free(entry->codepage);
	
	g_free(entry->memo.text);
	
	GList *c = NULL;
	for (c = entry->categories; c; c = c->next) {
		g_free(c->data);
	}
	
	if (entry->categories)
		g_list_free(entry->categories);
	
	g_free(entry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool marshall_palm_note(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %i, %p)", __func__, input, inpsize, output, outpsize, error);
		
        int tmp_size, osize;

        g_assert(inpsize == sizeof(PSyncNoteEntry));
        PSyncNoteEntry *note = (PSyncNoteEntry*) input;

        // get size
        osize = sizeof(PSyncNoteEntry) + 1; 

        if (note->codepage)
                osize += strlen(note->codepage);

	osize += 1;

        if (note->memo.text)
                osize += strlen(note->memo.text); 

	osize += 1;

	GList *c = NULL;
	for (c = note->categories; c; c = c->next) {
		osize += strlen((char *) c->data);
		osize += 1;
	}

	osize += 1;


        char *outnote = g_malloc0(osize);
        if (!outnote)
                goto error;

        char *outdata = ((char *)outnote) + sizeof(PSyncNoteEntry) + 1;
        memcpy(outnote, note, sizeof(PSyncNoteEntry));

        /* note->codepage */
        if (note->codepage) {
                tmp_size = strlen(note->codepage);
                memcpy(outdata, note->codepage, tmp_size);
                outdata += tmp_size;
        }
        outdata +=1;

        /* description */
        if (note->memo.text) {
                tmp_size = strlen(note->memo.text);
                memcpy(outdata, note->memo.text, tmp_size);
                outdata += tmp_size;
        }
        outdata += 1;

	/* glist stuff comes at the end */
	for (c = note->categories; c; c = c->next) {
		tmp_size = strlen((char *) c->data);
		memcpy(outdata, c->data, tmp_size);
		outdata += tmp_size;
		outdata += 1;
	}	
	outdata += 1;


	/*
	int i;
        for (i=0; i < osize; i++)
                osync_trace(TRACE_SENSITIVE, "[%i] %c", i, outnote[i]);
	*/	

        *output = (char*)outnote;
        *outpsize = osize;

	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT, "%s: FALSE", __func__);
	return FALSE;	
}

osync_bool demarshall_palm_note(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %i, %p)", __func__, input, inpsize, output, outpsize, error);
		
        int tmp_size;
        char *pos = NULL;
        PSyncNoteEntry *newnote = NULL;
        /* get PSyncNoteEntry struct */
        g_assert(inpsize >= sizeof(PSyncNoteEntry));
        PSyncNoteEntry *note = (PSyncNoteEntry *)input;

        /* get PSyncNoteEntry data */
        newnote = g_malloc0(sizeof(PSyncNoteEntry));
        if (!newnote)
                goto error;

        memcpy(newnote, note, sizeof(PSyncNoteEntry));
        pos = (char *) note + sizeof(PSyncNoteEntry) + 1;

	newnote->codepage = NULL;

	newnote->memo.text = NULL;
	
	/*
	int i;
        for (i=0; i < inpsize; i++)
                osync_trace(TRACE_SENSITIVE, "[%i] %c", i, input[i]);
	*/	


        /* note->codepage */
        if ((tmp_size = strlen(pos)) > 0) {
                newnote->codepage = strdup(pos);
        	pos += tmp_size;
	}
	pos += 1;

        /* description */
        if ((tmp_size = strlen(pos)) > 0) {
                newnote->memo.text = strdup(pos);
        	pos += tmp_size;
	}
	pos += 1;

	/* (glist) categories... */
	newnote->categories = NULL;
	while ((tmp_size = strlen(pos)) > 0) {
		newnote->categories = g_list_append(newnote->categories, g_strdup(pos));
		pos += tmp_size;
		pos += 1;
	}

	pos += 1;

	osync_trace(TRACE_SENSITIVE, "codepage: [%s]", newnote->codepage ? newnote->codepage : "nil");

	osync_trace(TRACE_SENSITIVE, "memo.text: [%s]",
			newnote->memo.text ? newnote->memo.text : "nil");

        *output = (char*) newnote;
        *outpsize = sizeof(PSyncNoteEntry);

	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
error:
	osync_trace(TRACE_EXIT, "%s: FALSE", __func__);
	return FALSE;	
}
#endif

osync_bool get_format_info(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncObjFormat *format = osync_objformat_new("palm-contact", "contact", error);
	if (!format)
		return FALSE;
	
	osync_objformat_set_destroy_func(format, destroy_palm_contact);
	osync_objformat_set_marshal_func(format, marshal_palm_contact);
	osync_objformat_set_demarshal_func(format, demarshal_palm_contact);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	return TRUE;
}

osync_bool get_conversion_info(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncObjFormat *palmContact = osync_format_env_find_objformat(env, "palm-contact");
	if (!palmContact) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find palm-contact format");
		return FALSE;
	}
	
	OSyncObjFormat *xmlContact = osync_format_env_find_objformat(env, "xmlformat-contact");
	if (!xmlContact) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find xmlformat-contact format");
		return FALSE;
	}
	
	OSyncFormatConverter *conv = osync_converter_new(OSYNC_CONVERTER_CONV, palmContact, xmlContact, conv_palm_contact_to_xml, error);
	if (!conv)
		return FALSE;
	
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlContact, palmContact, conv_xml_to_palm_contact, error);
	if (!conv)
		return FALSE;
	
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	return TRUE;
}

int get_version(void)
{
	return 1;
}





/*
void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "");
	osync_env_format_set_destroy_func(env, "palm-contact", destroy_palm_contact);
	osync_env_format_set_marshall_func(env, "palm-contact", marshall_palm_contact);
	osync_env_format_set_demarshall_func(env, "palm-contact", demarshall_palm_contact);

	osync_env_register_converter(env, CONVERTER_CONV, "palm-contact", "xmlformat-contact", conv_palm_contact_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xmlformat-contact", "palm-contact", conv_xml_to_palm_contact);

	osync_env_register_objtype(env, "todo");
	osync_env_register_objformat(env, "todo", "palm-todo");
	osync_env_format_set_destroy_func(env, "palm-todo", destroy_palm_todo);
	osync_env_format_set_marshall_func(env, "palm-todo", marshall_palm_todo);
	osync_env_format_set_demarshall_func(env, "palm-todo", demarshall_palm_todo);
	
	osync_env_register_converter(env, CONVERTER_CONV, "palm-todo", "xml-todo", conv_palm_todo_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-todo", "palm-todo", conv_xml_to_palm_todo);

	osync_env_register_objtype(env, "event");
	osync_env_register_objformat(env, "event", "palm-event");
	osync_env_format_set_destroy_func(env, "palm-event", destroy_palm_event);
	osync_env_format_set_marshall_func(env, "palm-event", marshall_palm_event);
	osync_env_format_set_demarshall_func(env, "palm-event", demarshall_palm_event);
	
	osync_env_register_converter(env, CONVERTER_CONV, "palm-event", "xml-event", conv_palm_event_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-event", "palm-event", conv_xml_to_palm_event);

	osync_env_register_objtype(env, "note");
	osync_env_register_objformat(env, "note", "palm-note");
	osync_env_format_set_destroy_func(env, "palm-note", destroy_palm_note);
	osync_env_format_set_marshall_func(env, "palm-note", marshall_palm_note);
	osync_env_format_set_demarshall_func(env, "palm-note", demarshall_palm_note);
	
	osync_env_register_converter(env, CONVERTER_CONV, "palm-note", "xml-note", conv_palm_note_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-note", "palm-note", conv_xml_to_palm_note);

}*/
