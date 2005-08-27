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
	time_t ttm = 0;
	
	if (inpsize != sizeof(PSyncEventEntry)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		goto error;
	}
	
	//Create a new xml document
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "vcal");
	root = xmlNewChild(root, NULL, (xmlChar*)"Event", NULL);

	//Description
	if (entry->appointment.description) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Description", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", entry->appointment.description);
	}
	
	//Note
	if (entry->appointment.note) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Summary", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", entry->appointment.note);
	}
	
	//Start and end time
	if (entry->appointment.event == 1) {
		ttm = mktime(&(entry->appointment.begin));
		tmp = g_strdup_printf("%i", (int)ttm);
		current = xmlNewChild(root, NULL, (xmlChar*)"DateStarted", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", tmp);
		g_free(tmp);
		
		time_t ttm = mktime(&(entry->appointment.end));
		tmp = g_strdup_printf("%i", (int)ttm);
		current = xmlNewChild(root, NULL, (xmlChar*)"DateEnd", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", tmp);
		g_free(tmp);
	} else {
		ttm = mktime(&(entry->appointment.begin));
		tmp = g_strdup_printf("%i", (int)ttm);
		current = xmlNewChild(root, NULL, (xmlChar*)"DateStarted", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", tmp);
		g_free(tmp);
		
		time_t ttm = mktime(&(entry->appointment.end));
		tmp = g_strdup_printf("%i", (int)ttm);
		current = xmlNewChild(root, NULL, (xmlChar*)"DateEnd", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", tmp);
		g_free(tmp);
	}
		
	/* alarm */
	if(entry->appointment.alarm) {
		xmlNode *alarm = xmlNewChild(root, NULL, (xmlChar*)"Alarm", NULL);
		osync_trace(TRACE_INTERNAL, "ADDED alarm node %p", alarm);
		
		/*switch(appointment.advanceUnits) {
			case 0:
				snprintf(buffer, 1024, "-PT%iM", appointment.advance);
				break;
			case 1:
				snprintf(buffer, 1024, "-PT%iH", appointment.advance);
				break;
			case 2:
				snprintf(buffer, 1024, "-P%iD", appointment.advance);
				break;
		}*/

		/*if (appointment.description)
			addPropValueO(prop, VCDescriptionPropO, appointment.description);

		prop = addPropValueO(prop, VCTriggerPropO, buffer);
		addPropValueO(prop, VCRelatedPropO, "START");
		addPropValueO(prop, VCValuePropO, "DURATION");*/
	}
	
	//recurrence
	if (entry->appointment.repeatType != repeatNone) {
		osync_trace(TRACE_INTERNAL, "Unhandled reccurrence");
		
		/*switch(appointment.advanceUnits) {
			case 0:
				snprintf(buffer, 1024, "-PT%iM", appointment.advance);
				break;
			case 1:
				snprintf(buffer, 1024, "-PT%iH", appointment.advance);
				break;
			case 2:
				snprintf(buffer, 1024, "-P%iD", appointment.advance);
				break;
		}*/

		/*if (appointment.description)
			addPropValueO(prop, VCDescriptionPropO, appointment.description);

		prop = addPropValueO(prop, VCTriggerPropO, buffer);
		addPropValueO(prop, VCRelatedPropO, "START");
		addPropValueO(prop, VCValuePropO, "DURATION");
		
		GString *rrulestr = g_string_new("");

		//Frequency
		switch (appointment.repeatType) {
			case repeatDaily:
				rrulestr = g_string_append(rrulestr, "FREQ=DAILY;");
				break;
			case repeatWeekly:
				g_string_append(rrulestr, "FREQ=WEEKLY;");
				g_string_append(rrulestr, "BYDAY=");
				for(i = 0; i < 6; i++) {
					if (appointment.repeatDays[i]) {
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
				g_string_append(rrulestr, ";");
				break;
			case repeatMonthlyByDate:
				g_string_append(rrulestr, "FREQ=MONTHLY;");
				g_string_append_printf(rrulestr, "BYMONTHDAY=%i;", appointment.begin.tm_mday);
				break;
			case repeatMonthlyByDay:
				g_string_append(rrulestr, "FREQ=MONTHLY;");
				g_string_append_printf(rrulestr, "BYSETPOS=%i;", (int)(appointment.repeatDay / 7) + 1);
				g_string_append(rrulestr, "BYDAY=");
				switch (appointment.repeatDay % 7) {
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
				//Now remove the coma
				g_string_truncate(rrulestr, strlen(rrulestr->str) - 1);
				g_string_append(rrulestr, ";");
				break;
			case repeatYearly:
				g_string_append(rrulestr, "FREQ=YEARLY;");
				break;
			//repeatNone
			default:
				break;
		}

		//interval
		if (appointment.repeatFrequency) {
			g_string_append_printf(rrulestr, "INTERVAL=%i;", appointment.repeatFrequency);
		}

		//repeatEnd
		if (!appointment.repeatForever) {
			g_string_append_printf(rrulestr, "UNTIL=%s;", tm2vcaldatetime(appointment.repeatEnd));
		}

		//remove ;
		g_string_truncate(rrulestr, strlen(rrulestr->str) - 1);

		addPropValueO(vevent, VCRRulePropO, strdup(rrulestr->str));
		g_string_free(rrulestr, FALSE);

		//exceptions
		if (appointment.exceptions) {
			for (i = 0; i < appointment.exceptions; i++) {
				prop = addPropValueO(vevent, "EXDATE", tm2vcaldate(appointment.exception[i]));
				addPropValueO(prop, "VALUE", "DATE");
			}
		}*/
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

static osync_bool conv_xml_to_palm_event(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
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
	entry->appointment.repeatFrequency = 0;
	entry->appointment.repeatWeekstart= 0;
	entry->appointment.exception = malloc(sizeof(struct tm) * 20);
	entry->appointment.exceptions = 0;
	entry->appointment.description = NULL;
	entry->appointment.note = NULL;

	//Summary
	xmlNode *cur = osxml_get_node(root, "Description");
	if (cur) {
		entry->appointment.description = (char *)xmlNodeGetContent(cur);
	}
	
	//Note
	cur = osxml_get_node(root, "Summary");
	if (cur) {
		entry->appointment.note = (char *)xmlNodeGetContent(cur);
	}

	//Start
	cur = osxml_get_node(root, "DateStarted");
	if (cur) {
		char *content = (char *)xmlNodeGetContent(cur);
		time_t ttm = atoi(content);
		struct tm *tm = gmtime(&ttm);
		entry->appointment.begin = *tm;
		g_free(tm);
		/*if (strlen(attrValue) == 8) {
			entry->appointment.event = 1;
		}*/
	}

	//End
	cur = osxml_get_node(root, "DateEnd");
	if (cur) {
		char *content = (char *)xmlNodeGetContent(cur);
		time_t ttm = atoi(content);
		struct tm *tm = gmtime(&ttm);
		entry->appointment.end = *tm;
		g_free(tm);
	}
	
	//Alarm
	cur = osxml_get_node(root, "Alarm");
	if (cur) {
		osync_trace(TRACE_INTERNAL, "Unhandled alarm");
	}
	/*if(strcmp(n,VCAlarmPropO) == 0) {
		entry->appointment.alarm = 1;
		if ((prop = isAPropertyOfO(t, VCTriggerPropO))) {
			buffer = fakeCStringO(vObjectUStringZValueO(prop));
			switch(buffer[strlen(buffer)-1])
			{
				case 'M':
					entry->appointment.advanceUnits = 0;
					break;
				case 'H':
					entry->appointment.advanceUnits = 1;
					break;
				case 'D':
					entry->appointment.advanceUnits = 2;
					break;
				default:
					entry->appointment.advanceUnits = 0;
			}
			buffer[strlen(buffer)-1] = 0;
			entry->appointment.advance = atoi(buffer + 3);
			free(buffer);
		}
	}*/

	//recurrence
	/*if(strcmp(n,VCRRulePropO) == 0) {
		buffer = fakeCStringO(vObjectUStringZValueO(t));
		//Frequency
		if ((rrulestr = isAAttributeOfO(buffer, VCFreqPropO))) {
			switch(rrulestr[0]) {
				case 'D':
					entry->appointment.repeatType = repeatDaily;
					break;
				case 'W':
					entry->appointment.repeatType = repeatWeekly;
					if ((day = isAAttributeOfO(buffer, "BYDAY"))) {
						gchar** weekdaystokens = g_strsplit(day, ",", 7);
						int j = 0;
						for(j = 0; weekdaystokens[j] != NULL; j++) {
							if(!strcmp(weekdaystokens[j], "SU")) {
								entry->appointment.repeatDays[0] = 1;
							} else if(!strcmp(weekdaystokens[j], "MO")) {
								entry->appointment.repeatDays[1] = 1;
							} else if(!strcmp(weekdaystokens[j], "TU")) {
								entry->appointment.repeatDays[2] = 1;
							} else if(!strcmp(weekdaystokens[j], "WE")) {
								entry->appointment.repeatDays[3] = 1;
							} else if(!strcmp(weekdaystokens[j], "TH")) {
								entry->appointment.repeatDays[4] = 1;
							} else if(!strcmp(weekdaystokens[j], "FR")) {
								entry->appointment.repeatDays[5] = 1;
							} else if(!strcmp(weekdaystokens[j], "SA")) {
								entry->appointment.repeatDays[6] = 1;
							}
						}
						g_free(day);
					}
					break;
				case 'M':
					//We now need to find out if by date or by day
					if ((day = isAAttributeOfO(buffer, "BYMONTHDAY"))) {
						//By date
						if (atoi(day) <= 0) {
							palm_debug(conn, 0, "Unsupported Recurrence Rule: Counting days from the end of the month");
						} else {
							entry->appointment.repeatType = repeatMonthlyByDate;
							//Now set the date
							entry->appointment.begin.tm_mday = atoi(day);
						}
						g_free(day);
					} else if ((day = isAAttributeOfO(buffer, "BYDAY"))) {
						//By day
						entry->appointment.repeatType = repeatMonthlyByDay;
						if (!strcmp(day, "SU")) {
							entry->appointment.repeatDay = (7 * atoi(isAAttributeOfO(buffer, "BYSETPOS"))) - 7;
						} else if (!strcmp(day, "MO")) {
							entry->appointment.repeatDay = (7 * atoi(isAAttributeOfO(buffer, "BYSETPOS"))) - 6;
						} else if (!strcmp(day, "TU")) {
							entry->appointment.repeatDay = (7 * atoi(isAAttributeOfO(buffer, "BYSETPOS"))) - 5;
						} else if (!strcmp(day, "WE")) {
							entry->appointment.repeatDay = (7 * atoi(isAAttributeOfO(buffer, "BYSETPOS"))) - 4;
						} else if (!strcmp(day, "TH")) {
							entry->appointment.repeatDay = (7 * atoi(isAAttributeOfO(buffer, "BYSETPOS"))) - 3;
						} else if (!strcmp(day, "FR")) {
							entry->appointment.repeatDay = (7 * atoi(isAAttributeOfO(buffer, "BYSETPOS"))) - 2;
						} else if (!strcmp(day, "SA")) {
							entry->appointment.repeatDay = (7 * atoi(isAAttributeOfO(buffer, "BYSETPOS"))) - 1;
						}
						g_free(day);
					}
					break;
				case 'Y':
					entry->appointment.repeatType = repeatYearly;
					break;
			}
			g_free(rrulestr);
		}
		//repeatEnd
		if ((rrulestr = isAAttributeOfO(buffer, VCUntilPropO))) {
			entry->appointment.repeatEnd = vcaltime2tm(rrulestr);
			entry->appointment.repeatForever = 0;
			g_free(rrulestr);
		}
		//For
		if ((rrulestr = isAAttributeOfO(buffer, "COUNT"))) {
			palm_debug(conn, 0, "Unable to translate \"COUNT\" Property. (\"For\" Recurrence in Evolution)");
			g_free(rrulestr);
		}
		//Intervall
		if ((rrulestr = isAAttributeOfO(buffer, VCIntervalPropO))) {
			entry->appointment.repeatFrequency = atoi(rrulestr);
			g_free(rrulestr);
		}
	}*/

	//Exceptions TODO
	/*if(strcmp(n,"EXDATE") == 0) {
		entry->appointment.exception[entry->appointment.exceptions] = vcaltime2tm(fakeCStringO(vObjectUStringZValueO(t)));
		entry->appointment.exceptions++;
	}*/
	
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
		xmlNewChild(current, NULL, (xmlChar*)"Content", entry->todo.description);
	}
	
	//Note
	if (entry->todo.note) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Summary", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", entry->todo.note);
	}
	
	//priority
	if (entry->todo.priority) {
		tmp = g_strdup_printf("%i", entry->todo.priority + 2);
		current = xmlNewChild(root, NULL, (xmlChar*)"Priority", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", tmp);
		g_free(tmp);
	}

	//due
	if (entry->todo.indefinite == 0) {
		time_t ttm = mktime(&(entry->todo.due));
		tmp = g_strdup_printf("%i", (int)ttm);
		current = xmlNewChild(root, NULL, (xmlChar*)"DateDue", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", tmp);
		g_free(tmp);
	}

	//completed
	if (entry->todo.complete) {
		time_t now = time(NULL);
		tmp = g_strdup_printf("%i", (int)now);
		current = xmlNewChild(root, NULL, (xmlChar*)"Completed", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", tmp);
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
	
	if (xmlStrcmp(root->name, (const xmlChar *)"contact")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xml root element");
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
		g_free(tm);
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
	int i;
	for (i = 3; i <= 7; i++) {
		tmp = return_next_entry(entry, i);
		if (tmp) {
			if (entry->address.phoneLabel[i - 3] == 4) {
				current = xmlNewChild(root, NULL, (xmlChar*)"Telephone", NULL);
			} else
				current = xmlNewChild(root, NULL, (xmlChar*)"Telephone", NULL);
		
			xmlNewChild(current, NULL, (xmlChar*)"Content", tmp);
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
		xmlNewChild(current, NULL, (xmlChar*)"Content", tmp);
		g_free(tmp);
	}
		
	//Note
	tmp = return_next_entry(entry, 18);
	if (tmp) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Note", tmp);
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
	xmlXPathObject *xobj = osxml_get_nodeset((xmlDoc *)input, "/Telephone");
	xmlNodeSet *nodes = xobj->nodesetval;
	int numnodes = (nodes) ? nodes->nodeNr : 0;
	for (i = 0; i < 4 && i < numnodes; i++) {
		cur = nodes->nodeTab[i];
		entry->address.entry[3 + i] = (char*)osxml_find_node(cur, "Content");

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
	xobj = osxml_get_nodeset((xmlDoc *)input, "/EMail");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;
	int n;
	for (n = 0; i < 4 && n < numnodes; n++) {
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
}
