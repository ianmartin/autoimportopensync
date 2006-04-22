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

#include <opensync/opensync-xml.h>

static char *return_next_entry(PSyncContactEntry *entry, unsigned int i)
{	
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, entry, i);
	char *tmp = NULL;
	
	osync_trace(TRACE_INTERNAL, "Entry: %p", entry->address.entry[i]);
	if (entry->address.entry[i]) {
		osync_trace(TRACE_INTERNAL, "Before: %s", entry->address.entry[i]);
		tmp = g_convert(entry->address.entry[i], strlen(entry->address.entry[i]), "utf8", entry->codepage, NULL, NULL, NULL);
	}
	osync_trace(TRACE_INTERNAL, "Palm Entry: %i: %s", i, tmp);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return tmp;
}

static osync_bool has_entry(PSyncContactEntry *entry, unsigned int i)
{
	return entry->address.entry[i] ? TRUE : FALSE;
}

static osync_bool conv_palm_event_to_xml(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
	PSyncEventEntry *entry = (PSyncEventEntry *)input;
	char *tmp = NULL;
	xmlNode *current = NULL;
	struct tm begin, end;
	
	if (inpsize != sizeof(PSyncEventEntry)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		goto error;
	}

	osync_trace(TRACE_INTERNAL, "event: %i\n begin: %i\n end: %i\n alarm: %i\n",
		       entry->appointment.event, entry->appointment.begin, entry->appointment.end, entry->appointment.alarm);	
	osync_trace(TRACE_INTERNAL, "advance: %i\n advanceUnits: %i\n repeatType: %i\n",
			entry->appointment.advance, entry->appointment.advanceUnits, entry->appointment.repeatType);
	osync_trace(TRACE_INTERNAL, "repeatForever: %i\n repeatEnd.tm_year: %i\n repeatFrequency: %i\n",
			entry->appointment.repeatForever, entry->appointment.repeatEnd.tm_year, entry->appointment.repeatFrequency);
	osync_trace(TRACE_INTERNAL, "repeatDay: %i\n repeatDays: %i %i %i %i %i %i %i\n repeatWeekstart: %i\n",
			entry->appointment.repeatDay, entry->appointment.repeatDays[0],
			entry->appointment.repeatDays[1], entry->appointment.repeatDays[2],
			entry->appointment.repeatDays[3], entry->appointment.repeatDays[4],
			entry->appointment.repeatDays[5], entry->appointment.repeatDays[6],
			entry->appointment.repeatWeekstart);
	osync_trace(TRACE_INTERNAL, "execptions: %i\n tm_exception: NULL\n descriotion: %s\n note: %s\n",
		       entry->appointment.exception, entry->appointment.description, entry->appointment.note);	
		       				

	//Create a new xml document
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "vcal");
	root = xmlNewChild(root, NULL, (xmlChar*)"Event", NULL);

	osync_trace(TRACE_INTERNAL, "note: \"%s\" event: %i", entry->appointment.note, entry->appointment.event);

	//Description
	current = xmlNewChild(root, NULL, (xmlChar*)"Summary", NULL);
	xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)entry->appointment.description);
	
	//Note
	current = xmlNewChild(root, NULL, (xmlChar*)"Description", NULL);
	xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)entry->appointment.note);

	//Start and end time
	osync_trace(TRACE_INTERNAL, "starttime: %i event: %i", entry->appointment.begin,
		entry->appointment.event); 
			
	if (entry->appointment.event == 1) {
		begin = entry->appointment.begin;

		tmp = g_strdup_printf("%04d%02d%02d", 
			begin.tm_year + 1900, begin.tm_mon + 1, begin.tm_mday);
		current = xmlNewChild(root, NULL, (xmlChar*)"DateStarted", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
		g_free(tmp);
		
		end = entry->appointment.end;
		tmp = g_strdup_printf("%04d%02d%02d", 
			end.tm_year + 1900, end.tm_mon + 1, end.tm_mday);
		current = xmlNewChild(root, NULL, (xmlChar*)"DateEnd", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
		g_free(tmp);
	} else {
		begin = entry->appointment.begin;

		tmp = g_strdup_printf("%04d%02d%02dT%02d%02d%02d", 
			begin.tm_year + 1900, begin.tm_mon + 1, begin.tm_mday,
			begin.tm_hour, begin.tm_min, begin.tm_sec);
		current = xmlNewChild(root, NULL, (xmlChar*)"DateStarted", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
		g_free(tmp);
		
		end = entry->appointment.end;
		tmp = g_strdup_printf("%04d%02d%02dT%02d%02d%02d", 
			end.tm_year + 1900, end.tm_mon + 1, end.tm_mday,
			end.tm_hour, end.tm_min, end.tm_sec);
		current = xmlNewChild(root, NULL, (xmlChar*)"DateEnd", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
		g_free(tmp);
	}
		
	// Alarm
	if(entry->appointment.alarm) {
		xmlNode *alarm = xmlNewChild(root, NULL, (xmlChar*)"Alarm", NULL);
		osync_trace(TRACE_INTERNAL, "ADDED alarm node: %i", alarm);
		
		switch(entry->appointment.advanceUnits) {
			case 0:
				tmp = g_strdup_printf("-PT%iM", entry->appointment.advance);
				break;
			case 4:
				tmp = g_strdup_printf("-PT%iH", entry->appointment.advance);
				break;
			case 2:
				tmp = g_strdup_printf("-P%iD", entry->appointment.advance);
				break;
		}
		
		xmlNode *alarmtrigger = xmlNewChild(alarm, NULL, (xmlChar*) "AlarmTrigger", NULL);
		xmlNewChild(alarmtrigger, NULL, (xmlChar*) "Content", (xmlChar*) tmp);
		xmlNewChild(alarmtrigger, NULL, (xmlChar*) "Value", (xmlChar*) "DURATION");

		g_free(tmp);
	}
	
	// recurrence
	if (entry->appointment.repeatType != repeatNone) {
		
		int i;
		GString *rrulestr = g_string_new("");

		current = xmlNewChild(root, NULL, (xmlChar*) "RecurrenceRule", NULL);

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

				// 5 Week issuse?! :(
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
	
		xmlNewChild(current, NULL, (xmlChar*)"Rule", (xmlChar*) tmp);

		if (strlen(rrulestr->str))
			xmlNewChild(current, NULL, (xmlChar*)"Rule", (xmlChar*) rrulestr->str);

		g_string_free(rrulestr, FALSE);

		//interval
		if (entry->appointment.repeatFrequency) {
			tmp = g_strdup_printf("INTERVAL=%i", entry->appointment.repeatFrequency);
			xmlNewChild(current, NULL, (xmlChar*)"Rule", (xmlChar*) tmp); 
		}

		//repeatEnd
		if (!entry->appointment.repeatForever) {
			tmp = g_strdup_printf("%04d%02d%02dT%02d%02d%02d",
					entry->appointment.repeatEnd.tm_year + 1900,
					entry->appointment.repeatEnd.tm_mon + 1,
					entry->appointment.repeatEnd.tm_mday,
					entry->appointment.repeatEnd.tm_hour,
					entry->appointment.repeatEnd.tm_min,
					entry->appointment.repeatEnd.tm_sec);
			tmp = g_strdup_printf("UNTIL=%s", tmp);
			xmlNewChild(current, NULL, (xmlChar*)"Rule", (xmlChar*) tmp);
		}

		// TODO: test
		//exceptions
		if (entry->appointment.exceptions) {
			for (i = 0; i < entry->appointment.exceptions; i++) {
				current = xmlNewChild(root, NULL, (xmlChar*)"ExclusionDate", NULL);
				tmp = g_strdup_printf("%04d%02d%02d",
						entry->appointment.exception[i].tm_year + 1900,
						entry->appointment.exception[i].tm_mon + 1,
						entry->appointment.exception[i].tm_mday);
				xmlNewChild(current, NULL, (xmlChar*) "Content", (xmlChar*) tmp);
				xmlNewChild(current, NULL, (xmlChar*) "Value", (xmlChar*) "DATE");
			}
		}

		g_free(tmp);
	}
	// end of reccurence

	// Categories
	GList *c = NULL;
	current = NULL;
	for (c = entry->categories; c; c = c->next) {
		if (!current)
			current = xmlNewChild(root, NULL, (xmlChar*)"Categories", NULL);
		osxml_node_add(current, "Category", (char *)c->data);
	}

	*free_input = TRUE;
	*output = (char *)doc;
	*outpsize = sizeof(doc);

	osync_trace(TRACE_INTERNAL, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)doc));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool conv_xml_to_palm_event(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);

	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", osxml_write_to_string((xmlDoc *)input));

	int i = 0;
	char *tmp = NULL;
	
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
	// repeatFrequencly of ZERO will break your palm(-calendar) on recurrence entries
	entry->appointment.repeatFrequency = 1;
	entry->appointment.repeatWeekstart= 0;
	entry->appointment.exception = malloc(sizeof(struct tm) * 20); // FIXME: realloc?
	entry->appointment.exceptions = 0;
	entry->appointment.description = NULL;
	entry->appointment.note = NULL;

	//Summary
	xmlNode *cur = osxml_get_node(root, "Description");
	if (cur) {
		entry->appointment.note = (char *)xmlNodeGetContent(cur);
	}
	
	//Note
	cur = osxml_get_node(root, "Summary");
	if (cur) {
		entry->appointment.description = (char *)xmlNodeGetContent(cur);
	}

	//Start
	cur = osxml_get_node(root, "DateStarted");
	if (cur) {
		cur = osxml_get_node(cur, "Content");
		tmp = (char *)xmlNodeGetContent(cur);
		struct tm start;

		if (strstr(tmp, "T")) {
			sscanf(tmp, "%04d%02d%02dT%02d%02d%02d%*01c", // TODO timezone
					&start.tm_year, &start.tm_mon, &start.tm_mday,
					&start.tm_hour, &start.tm_min, &start.tm_sec);
		} else {
			sscanf(tmp, "%04d%02d%02d",
					&start.tm_year, &start.tm_mon, &start.tm_mday);
			entry->appointment.event = 1;

			start.tm_hour = 0;
			start.tm_min = 0;
			start.tm_sec = 0;
		}

		osync_trace(TRACE_INTERNAL, "event: %i datestart node: %s, year: %i month: %i days: %i", 
				entry->appointment.event, tmp,
				start.tm_year, start.tm_mon, start.tm_mday);

		start.tm_year -= 1900;
		start.tm_mon -= 1;
		entry->appointment.begin = start;
		g_free(tmp);
	}

	//End
	cur = osxml_get_node(root, "DateEnd");
	if (cur) {
		cur = osxml_get_node(cur, "Content");
		tmp = (char *)xmlNodeGetContent(cur);
		struct tm end;
		
		if (strstr(tmp, "T"))
			sscanf(tmp, "%04d%02d%02dT%02d%02d%02d%*01c",
					&end.tm_year, &end.tm_mon, &end.tm_mday,
					&end.tm_hour, &end.tm_min, &end.tm_sec);
		else
			sscanf(tmp, "%04d%02d%02d",
					&end.tm_year, &end.tm_mon, &end.tm_mday);
	
		osync_trace(TRACE_INTERNAL, "dateend node: %s, year: %i month: %i days: %i", tmp,
				end.tm_year, end.tm_mon, end.tm_mday);


		end.tm_year -= 1900;
		end.tm_mon -= 1;

		entry->appointment.end = end;

		g_free(tmp);
	}
	
	//Alarm
	cur = osxml_get_node(root, "Alarm");
	if (cur) {
		osync_trace(TRACE_INTERNAL, "Alarm is enabled.");

		// enable alarm
		entry->appointment.alarm = 1;

		xmlNode *sub = osxml_get_node(cur, "AlarmTrigger");
		if (sub) {
			sub = osxml_get_node(sub, "Content");
			
			tmp = (char *) xmlNodeGetContent(sub);

			int is_signed = 0, is_digit = 0, digit = 0;
			int weeks = 0, days = 0, hours = 0, minutes = 0, seconds = 0;
			int i, advance_in_secs;

			// TODO: seperate function
			osync_trace(TRACE_INTERNAL, "alarm: ical %s", tmp); 
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

			osync_trace(TRACE_INTERNAL, "alarm: %i (unit: %i)", entry->appointment.advance, entry->appointment.advanceUnits); 

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
				
				// A month can have 5 <-------- WEEKS :( (palm cannot handle this?)
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

				osync_trace(TRACE_INTERNAL, "appointment.repeatDay: %i wday: %i", entry->appointment.repeatDay
						, wday);

			} else if (strstr(tmp, "UNTIL")) {
				struct tm repeat_end;
				if (strstr(tmp, "T"))
					sscanf(tmp, "UNTIL=%04d%02d%02dT%02d%02d%02d%*01c",	// TODO timezone
							&repeat_end.tm_year, &repeat_end.tm_mon, &repeat_end.tm_mday,
							&repeat_end.tm_hour, &repeat_end.tm_min, &repeat_end.tm_sec);
				else
					sscanf(tmp, "UNTIL=%04d%02d%02d", &repeat_end.tm_year, &repeat_end.tm_mon, &repeat_end.tm_mday);

				repeat_end.tm_year -= 1900;
				repeat_end.tm_mon -= 1;
							
				// when repeat_end is euqal or less then begin palm shows strange phenomenons
				if ((unsigned int) mktime(&repeat_end) > (unsigned int) mktime(&(entry->appointment.begin))) {
					osync_trace(TRACE_INTERNAL, "UNTIL: %s", tmp);
					entry->appointment.repeatEnd = repeat_end;
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
	//Treo270: 5 Exceptions?
	xmlXPathObject *xobj = osxml_get_nodeset((xmlDoc *)input, "/vcal/Event/ExclusionDate");
	xmlNodeSet *nodes = xobj->nodesetval;
	for (i = 0; i < 5 && i < nodes->nodeNr; i++) {
		cur = nodes->nodeTab[i];

		struct tm exclusion;
		cur = osxml_get_node(cur, "Content");
		tmp = (char *) xmlNodeGetContent(cur);

		sscanf(tmp, "%04d%02d%02d",
				&exclusion.tm_year, &exclusion.tm_mon, &exclusion.tm_mday);

		osync_trace(TRACE_INTERNAL, "exclusion node: %s, year: %i month: %i days: %i", tmp,
				exclusion.tm_year, exclusion.tm_mon, exclusion.tm_mday);


		exclusion.tm_year -= 1900;
		exclusion.tm_mon -= 1;
		
		entry->appointment.exception[entry->appointment.exceptions] = exclusion;
		entry->appointment.exceptions++;

		g_free(tmp);
	}
	
	//Categories
	cur = osxml_get_node(root, "Categories");
	if (cur) {
		for (cur = cur->children; cur; cur = cur->next) {
			entry->categories = g_list_append(entry->categories, (char*)xmlNodeGetContent(cur));
		}
	}
	
	*free_input = TRUE;
	*output = (void *)entry;
	*outpsize = sizeof(PSyncEventEntry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static void destroy_palm_event(char *input, size_t inpsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, input, inpsize);
	PSyncEventEntry *entry = (PSyncEventEntry *)input;
	g_assert(inpsize == sizeof(PSyncEventEntry));
	
	g_free(entry->uid);
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

static osync_bool conv_palm_todo_to_xml(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
	PSyncTodoEntry *entry = (PSyncTodoEntry *)input;
	char *tmp = NULL;
	xmlNode *current = NULL;
	
	if (inpsize != sizeof(PSyncTodoEntry)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		goto error;
	}
	
	//Create a new xml document
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "vcal");
	root = xmlNewChild(root, NULL, (xmlChar*)"Todo", NULL);

	//Description
	if (entry->todo.description) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Description", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)entry->todo.description);
	}
	
	//Note
	if (entry->todo.note) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Summary", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)entry->todo.note);
	}
	
	//priority
	if (entry->todo.priority) {
		tmp = g_strdup_printf("%i", entry->todo.priority + 2);
		current = xmlNewChild(root, NULL, (xmlChar*)"Priority", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
		g_free(tmp);
	}

	//due
	if (entry->todo.indefinite == 0) {
		time_t ttm = mktime(&(entry->todo.due));
		tmp = g_strdup_printf("%i", (int)ttm);
		current = xmlNewChild(root, NULL, (xmlChar*)"DateDue", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
		g_free(tmp);
	}

	//completed
	if (entry->todo.complete) {
		time_t now = time(NULL);
		tmp = g_strdup_printf("%i", (int)now);
		current = xmlNewChild(root, NULL, (xmlChar*)"Completed", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
		g_free(tmp);
	}

	GList *c = NULL;
	current = NULL;
	for (c = entry->categories; c; c = c->next) {
		if (!current)
			current = xmlNewChild(root, NULL, (xmlChar*)"Categories", NULL);
		osxml_node_add(current, "Category", (char *)c->data);
	}

	*free_input = TRUE;
	*output = (char *)doc;
	*outpsize = sizeof(doc);

	osync_trace(TRACE_INTERNAL, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)doc));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool conv_xml_to_palm_todo(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);

	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", osxml_write_to_string((xmlDoc *)input));
	
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
		entry->todo.description = (char *)xmlNodeGetContent(cur);
	}
	
	//Note
	cur = osxml_get_node(root, "Note");
	if (cur) {
		entry->todo.note = (char *)xmlNodeGetContent(cur);
	}
	
	//Due
	cur = osxml_get_node(root, "Due");
	if (cur) {
		char *content = (char *)xmlNodeGetContent(cur);
		time_t ttm = atoi(content);
		struct tm *tm = gmtime(&ttm);
		entry->todo.due = *tm;
		entry->todo.indefinite = 0;
	}

	//Categories
	cur = osxml_get_node(root, "Categories");
	if (cur) {
		for (cur = cur->children; cur; cur = cur->next) {
			entry->categories = g_list_append(entry->categories, (char*)xmlNodeGetContent(cur));
		}
	}
	
	*free_input = TRUE;
	*output = (void *)entry;
	*outpsize = sizeof(PSyncTodoEntry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static void destroy_palm_todo(char *input, size_t inpsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, input, inpsize);
	PSyncTodoEntry *entry = (PSyncTodoEntry *)input;
	g_assert(inpsize == sizeof(PSyncTodoEntry));
	
	g_free(entry->uid);
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

static osync_bool conv_palm_contact_to_xml(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
	PSyncContactEntry *entry = (PSyncContactEntry *)input;
	char *tmp = NULL;
	xmlNode *current = NULL;
	
	if (inpsize != sizeof(PSyncContactEntry)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		goto error;
	}
	
	int i = 0;
	for (i = 0; i < 19; i++) {
	  /*if (entry->address.entry[i]) {
	    char *tmp = g_convert(entry->address.entry[i], strlen(entry->address.entry[i]), conn->codepage ,"utf8", NULL, NULL, NULL);
	    free(entry->address.entry[i]);
	    entry->address.entry[i] = tmp;
	  }
	  if (entry->address.entry[i] && !strlen(entry->address.entry[i])) {
	    free(entry->address.entry[i]);
	    entry->address.entry[i] = NULL;
	    palm_debug(conn, 3, "Address %i: %s", i, entry->address.entry[i]);
	  }*/
	  osync_trace(TRACE_INTERNAL, "entry %i: %s", i, entry->address.entry[i]);
	}
	
	//Create a new xml document
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "contact");

	//Names
	if (has_entry(entry, 0) || has_entry(entry, 1)) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Name", NULL);
		//Last Name
		tmp = return_next_entry(entry, 0);
		if (tmp) {
			osxml_node_add(current, "LastName", tmp);
			g_free(tmp);
		}
	
		//First Name
		tmp = return_next_entry(entry, 1);
		if (tmp) {
			osxml_node_add(current, "FirstName", tmp);
			g_free(tmp);
		}
	}
	
	//Company
	tmp = return_next_entry(entry, 2);
	if (tmp) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Organization", NULL);
		osxml_node_add(current, "Name", tmp);
		g_free(tmp);
	}

	//Telephones and email
	for (i = 3; i <= 7; i++) {
		tmp = return_next_entry(entry, i);
		if (tmp) {
			if (entry->address.phoneLabel[i - 3] == 4) {
				current = xmlNewChild(root, NULL, (xmlChar*)"EMail", NULL);
			} else
				current = xmlNewChild(root, NULL, (xmlChar*)"Telephone", NULL);
		
			xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
			g_free(tmp);
			
			switch (entry->address.phoneLabel[i - 3]) {
				case 0:
					osxml_node_add(current, "Work", NULL);
					osxml_node_add(current, "Voice", NULL);
					break;
				case 1:
					osxml_node_add(current, "Home", NULL);
					break;
				case 2:
					osxml_node_add(current, "Work", NULL);
					osxml_node_add(current, "Fax", NULL);
					break;
				case 3:
					osxml_node_add(current, "Voice", NULL);
					break;
				case 5:
					osxml_node_add(current, "Pref", NULL);
					break;
				case 6:
					osxml_node_add(current, "Pager", NULL);
					break;
				case 7:
					osxml_node_add(current, "Cellular", NULL);
					break;
			}
		}
		
	}

	//Address
	if (has_entry(entry, 8) || has_entry(entry, 9) || has_entry(entry, 10) || has_entry(entry, 11) || has_entry(entry, 12)) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Address", NULL);
		//Street
		tmp = return_next_entry(entry, 8);
		if (tmp) {
			osxml_node_add(current, "Street", tmp);
			g_free(tmp);
		}
	
		//City
		tmp = return_next_entry(entry, 9);
		if (tmp) {
			osxml_node_add(current, "City", tmp);
			g_free(tmp);
		}
		
		//Region
		tmp = return_next_entry(entry, 10);
		if (tmp) {
			osxml_node_add(current, "Region", tmp);
			g_free(tmp);
		}
		
		//Code
		tmp = return_next_entry(entry, 11);
		if (tmp) {
			osxml_node_add(current, "PostalCode", tmp);
			g_free(tmp);
		}
		
		//Country
		tmp = return_next_entry(entry, 12);
		if (tmp) {
			osxml_node_add(current, "Country", tmp);
			g_free(tmp);
		}
	}
	
	//Title
	tmp = return_next_entry(entry, 13);
	if (tmp) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Title", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
		g_free(tmp);
	}
		
	//Note
	tmp = return_next_entry(entry, 18);
	if (tmp) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Note", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
		g_free(tmp);
	}

	GList *c = NULL;
	current = NULL;
	for (c = entry->categories; c; c = c->next) {
		if (!current)
			current = xmlNewChild(root, NULL, (xmlChar*)"Categories", NULL);
		osxml_node_add(current, "Category", (char *)c->data);
	}

	*free_input = TRUE;
	*output = (char *)doc;
	*outpsize = sizeof(doc);

	osync_trace(TRACE_INTERNAL, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)doc));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}



static osync_bool conv_xml_to_palm_contact(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);

	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", osxml_write_to_string((xmlDoc *)input));
	
	//Get the root node of the input document
	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		goto error;
	}
	
	if (xmlStrcmp(root->name, (const xmlChar *)"contact")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xml root element");
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
	
	//Name
	xmlNode *cur = osxml_get_node(root, "Name");
	if (cur) {
		entry->address.entry[0] = osxml_find_node(cur, "LastName");
		entry->address.entry[1] = osxml_find_node(cur, "FirstName");
	}

	//Company
	cur = osxml_get_node(root, "Organization");
	if (cur)
		entry->address.entry[2] = osxml_find_node(cur, "Name");

	//Telephone
	int i = 0;
	xmlXPathObject *xobj = osxml_get_nodeset((xmlDoc *)input, "/contact/Telephone");
	xmlNodeSet *nodes = xobj->nodesetval;
	int numnodes = (nodes) ? nodes->nodeNr : 0;
	osync_trace(TRACE_INTERNAL, "Found %i telephones", numnodes);
	
	for (i = 0; i < 5 && i < numnodes; i++) {
		cur = nodes->nodeTab[i];
		entry->address.entry[3 + i] = (char*)osxml_find_node(cur, "Content");

		osync_trace(TRACE_INTERNAL, "handling telephone. has work %i, home %i, voice %i", osxml_has_property(cur, "Work"), osxml_has_property(cur, "Home"), osxml_has_property(cur, "Voice"));

		if (osxml_has_property(cur, "Work") && osxml_has_property(cur, "Voice")) {
			entry->address.phoneLabel[i] = 0;
		} else if (osxml_has_property(cur, "HOME") && !(osxml_has_property(cur, "FAX"))) {
			entry->address.phoneLabel[i] = 1;
		} else if (osxml_has_property(cur, "FAX")) {
			entry->address.phoneLabel[i] = 2;
		} else if (!(osxml_has_property(cur, "WORK")) && !(osxml_has_property(cur, "HOME")) && osxml_has_property(cur, "VOICE")) {
			entry->address.phoneLabel[i] = 3;
		} else if (osxml_has_property(cur, "PREF") && !(osxml_has_property(cur, "FAX"))) {
			entry->address.phoneLabel[i] = 5;
		} else if (osxml_has_property(cur, "PAGER")) {
			entry->address.phoneLabel[i] = 6;
		} else if (osxml_has_property(cur, "CELL")) {
			entry->address.phoneLabel[i] = 7;
		} else osync_trace(TRACE_INTERNAL, "Unknown TEL entry");
	}
	xmlXPathFreeObject(xobj);
	
	//EMail
	xobj = osxml_get_nodeset((xmlDoc *)input, "/contact/EMail");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;
	osync_trace(TRACE_INTERNAL, "Found %i emails", numnodes);
	int n;
	for (n = 0; i < 5 && n < numnodes; n++) {
		cur = nodes->nodeTab[n];
		entry->address.entry[3 + i] = (char*)osxml_find_node(cur, "Content");
		entry->address.phoneLabel[i] = 4;
		i++;
	}
	xmlXPathFreeObject(xobj);
	
	//Address
	cur = osxml_get_node(root, "Organization");
	if (cur) {
		entry->address.entry[8] = osxml_find_node(cur, "Address");
		entry->address.entry[9] = osxml_find_node(cur, "City");
		entry->address.entry[10] = osxml_find_node(cur, "Region");
		entry->address.entry[11] = osxml_find_node(cur, "Code");
		entry->address.entry[12] = osxml_find_node(cur, "Country");
	}

	//Title
	cur = osxml_get_node(root, "Summary");
	if (cur)
		entry->address.entry[13] = (char*)xmlNodeGetContent(cur);

	//Note
	cur = osxml_get_node(root, "Note");
	if (cur)
		entry->address.entry[18] = (char*)xmlNodeGetContent(cur);
	
	//Categories
	cur = osxml_get_node(root, "Categories");
	if (cur) {
		for (cur = cur->children; cur; cur = cur->next) {
			entry->categories = g_list_append(entry->categories, (char*)xmlNodeGetContent(cur));
		}
	}

	/* Now convert to the charset */
	for (i = 0; i < 19; i++) {
	  /*if (entry->address.entry[i]) {
	    char *tmp = g_convert(entry->address.entry[i], strlen(entry->address.entry[i]), conn->codepage ,"utf8", NULL, NULL, NULL);
	    free(entry->address.entry[i]);
	    entry->address.entry[i] = tmp;
	  }
	  if (entry->address.entry[i] && !strlen(entry->address.entry[i])) {
	    free(entry->address.entry[i]);
	    entry->address.entry[i] = NULL;
	    palm_debug(conn, 3, "Address %i: %s", i, entry->address.entry[i]);
	  }*/
	  osync_trace(TRACE_INTERNAL, "entry %i: %s", i, entry->address.entry[i]);
	}
	
	*free_input = TRUE;
	*output = (void *)entry;
	*outpsize = sizeof(PSyncContactEntry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static void destroy_palm_contact(char *input, size_t inpsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, input, inpsize);
	PSyncContactEntry *entry = (PSyncContactEntry *)input;
	g_assert(inpsize == sizeof(PSyncContactEntry));
	
	g_free(entry->uid);
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
	
	if (entry->categories)
		g_list_free(entry->categories);
	
	g_free(entry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static osync_bool conv_palm_note_to_xml(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
	PSyncNoteEntry *entry = (PSyncNoteEntry *)input;
	xmlNode *current = NULL;
	
	if (inpsize != sizeof(PSyncNoteEntry)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		goto error;
	}
	
	//Create a new xml document
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "Note");

	// Summary & Body
	if (entry->memo.text) {
		gchar **splitMemo = g_strsplit(entry->memo.text, "\n", 2);
		current = xmlNewChild(root, NULL, (xmlChar*)"Summary", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)splitMemo[0]);

		current = xmlNewChild(root, NULL, (xmlChar*)"Body", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)splitMemo[1]);
	}

	GList *c = NULL;
	current = NULL;
	for (c = entry->categories; c; c = c->next) {
		if (!current)
			current = xmlNewChild(root, NULL, (xmlChar*)"Categories", NULL);
		osxml_node_add(current, "Category", (char *)c->data);
	}

	*free_input = TRUE;
	*output = (char *)doc;
	*outpsize = sizeof(doc);

	osync_trace(TRACE_INTERNAL, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)doc));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool conv_xml_to_palm_note(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);

	xmlNode *cur = NULL;

	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", osxml_write_to_string((xmlDoc *)input));

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
	if (cur) {
		entry->memo.text = (char *)xmlNodeGetContent(cur);
	}
	
	// Body
	cur = osxml_get_node(root, "Body"); 
	if (cur) {
		entry->memo.text = g_strdup_printf("%s\n%s", entry->memo.text, (char *)xmlNodeGetContent(cur));
	}
	
	//Categories
	cur = osxml_get_node(root, "Categories");
	if (cur) {
		for (cur = cur->children; cur; cur = cur->next) {
			entry->categories = g_list_append(entry->categories, (char*)xmlNodeGetContent(cur));
		}
	}
	
	*free_input = TRUE;
	*output = (void *)entry;
	*outpsize = sizeof(PSyncNoteEntry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static void destroy_palm_note(char *input, size_t inpsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, input, inpsize);
	PSyncNoteEntry *entry = (PSyncNoteEntry *)input;
	g_assert(inpsize == sizeof(PSyncNoteEntry));
	
	g_free(entry->uid);
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

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "palm-contact");
	osync_env_format_set_destroy_func(env, "palm-contact", destroy_palm_contact);

	osync_env_register_converter(env, CONVERTER_CONV, "palm-contact", "xml-contact", conv_palm_contact_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-contact", "palm-contact", conv_xml_to_palm_contact);
	
	osync_env_register_objtype(env, "todo");
	osync_env_register_objformat(env, "todo", "palm-todo");
	osync_env_format_set_destroy_func(env, "palm-todo", destroy_palm_todo);
	
	osync_env_register_converter(env, CONVERTER_CONV, "palm-todo", "xml-todo", conv_palm_todo_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-todo", "palm-todo", conv_xml_to_palm_todo);
	
	osync_env_register_objtype(env, "event");
	osync_env_register_objformat(env, "event", "palm-event");
	osync_env_format_set_destroy_func(env, "palm-event", destroy_palm_event);
	
	osync_env_register_converter(env, CONVERTER_CONV, "palm-event", "xml-event", conv_palm_event_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-event", "palm-event", conv_xml_to_palm_event);
	
	osync_env_register_objtype(env, "note");
	osync_env_register_objformat(env, "note", "palm-note");
	osync_env_format_set_destroy_func(env, "palm-note", destroy_palm_note);
	
	osync_env_register_converter(env, CONVERTER_CONV, "palm-note", "xml-note", conv_palm_note_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-note", "palm-note", conv_xml_to_palm_note);

}
