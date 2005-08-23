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


#if 0
/***********************************************************************
 *
 * Function:    tm2vcaldatetime
 *
 * Summary:   converts an tm struct to a vcal datetime string
 *
 ***********************************************************************/
char *tm2vcaldatetime(struct tm time)
{
	return g_strdup_printf("%04d%02d%02dT%02d%02d%02d", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
}

char *tm2vcaldate(struct tm time)
{
	return g_strdup_printf("%04d%02d%02d", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday);
}

char *escape_chars(char *str)
{
	gchar **array = g_strsplit(str, ",", 0);
	gchar *newstr = g_strjoinv("\\,", array);
	g_strfreev(array);
	g_free(str);
	return newstr;
}

/***********************************************************************
 *
 * Function:    vcaltime2tm
 *
 * Summary:   converts an vcal datetime stringto a tm struct
 *
 ***********************************************************************/
 struct tm vcaltime2tm(char *vcaltime)
 {
	char buffer[1024];
	struct tm time;

	//year
	strncpy(buffer, vcaltime, 4);
	buffer[4] = 0;
	time.tm_year = atoi(buffer) - 1900;

	//month
	strncpy(buffer, vcaltime + 4, 2);
	buffer[2] = 0;
	time.tm_mon = atoi(buffer) - 1;

	//day
	strncpy(buffer, vcaltime + 6, 2);
	buffer[2] = 0;
	time.tm_mday = atoi(buffer);

	if (strlen(vcaltime) != 8) {
		//hour
		strncpy(buffer, vcaltime + 9, 2);
		buffer[2] = 0;
		time.tm_hour = atoi(buffer);

		//minute
		strncpy(buffer, vcaltime + 11, 2);
		buffer[2] = 0;
		time.tm_min = atoi(buffer);

		//second
		strncpy(buffer, vcaltime + 13, 2);
		buffer[2] = 0;
		time.tm_sec = atoi(buffer);
	} else {
		time.tm_hour = 0;
		time.tm_min= 0;
		time.tm_sec = 0;
	}
	return time;
}

GString *calendar2vevent(palm_connection *conn, struct Appointment appointment)
{
	VObjectO *vcal;
	VObjectO *vevent;
	VObjectO *prop;
	GString *vcalstr;
	char *vcalptr;
	char buffer[1024];
	char *tmp = NULL;
	int i;

	palm_debug(conn, 2, "Translating calendar to vevent");
	palm_debug(conn, 3, "Calendar Entry:\nevent: %i\nbegin: %s\nend: %s\nalarm: %i\nadvance: %i\nadvanceUnits: %i\nrepeatType: %i\nrepeatForever: %i\nrepeatEnd %s:\nrepeatFrequency: %i\nrepeatDay: %i\nrepeatDays:\nrepeatWeekstart: %i\nexceptions: %i\nexception:\ndescription: %s\nnote: %s\n", appointment.event, tm2vcaldatetime(appointment.begin), tm2vcaldatetime(appointment.end), appointment.alarm, appointment.advance, appointment.advanceUnits, appointment.repeatType, appointment.repeatForever, tm2vcaldatetime(appointment.repeatEnd), appointment.repeatFrequency, appointment.repeatDay, appointment.repeatWeekstart, appointment.exceptions, appointment.description, appointment.note);

	vcal = newVObjectO(VCCalPropO);
  	vevent = addPropO(vcal, VCEventPropO);
	addPropValueO(vevent, VCVersionPropO, "2.0");

	//UTF8 conversion
	if (appointment.description) {
		tmp = g_convert(appointment.description, strlen(appointment.description), "utf8", conn->codepage, NULL, NULL, NULL);
		free(appointment.description);
		appointment.description = tmp;
	}

	if (appointment.note) {
		tmp = g_convert(appointment.note, strlen(appointment.note), "utf8", conn->codepage, NULL, NULL, NULL);
		free(appointment.note);
		appointment.note = tmp;
	}

	/* note */
	if(appointment.note && strlen(appointment.note))
		prop = addPropValueO(vevent, VCDescriptionPropO, appointment.note);

	/* description */
	if(appointment.description) {
		prop = addPropValueO(vevent, VCSummaryPropO, escape_chars(appointment.description));
	}

	/* begin and end*/
	if(appointment.event == 1) {
		prop = addPropValueO(vevent, VCDTstartPropO, tm2vcaldate(appointment.begin));
		addPropValueO(prop, VCValuePropO, "DATE");
		prop = addPropValueO(vevent, VCDTendPropO, tm2vcaldate(appointment.end));
		addPropValueO(prop, VCValuePropO, "DATE");
	} else {
		prop = addPropValueO(vevent, VCDTstartPropO, tm2vcaldatetime(appointment.begin));
		addPropValueO(prop, VCValuePropO, "DATE-TIME");
		prop = addPropValueO(vevent, VCDTendPropO, tm2vcaldatetime(appointment.end));
		addPropValueO(prop, VCValuePropO, "DATE-TIME");
	}

	/* alarm */
	if(appointment.alarm) {
		prop = addPropO(vevent, VCAlarmPropO);
		addPropValueO(prop, VCActionPropO, "DISPLAY");

		switch(appointment.advanceUnits) {
			case 0:
				snprintf(buffer, 1024, "-PT%iM", appointment.advance);
				break;
			case 1:
				snprintf(buffer, 1024, "-PT%iH", appointment.advance);
				break;
			case 2:
				snprintf(buffer, 1024, "-P%iD", appointment.advance);
				break;
		}

		if (appointment.description)
			addPropValueO(prop, VCDescriptionPropO, appointment.description);

		prop = addPropValueO(prop, VCTriggerPropO, buffer);
		addPropValueO(prop, VCRelatedPropO, "START");
		addPropValueO(prop, VCValuePropO, "DURATION");
	}

	//recurrence
	if (appointment.repeatType != repeatNone) {
		GString *rrulestr = g_string_new("");

		/*Frequency*/
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
			/* repeatNone */
			default:
				break;
		}

		/* interval */
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
		}
	}

	vcalptr = writeMemVObjectO(0,0,vcal);
	vcalstr = g_string_new(vcalptr);
	free(vcalptr);
	deleteVObjectO(vcal);

	palm_debug(conn, 3, "VCARD:\n%s", vcalstr->str);
	return vcalstr;
}

/***********************************************************************
 *
 * Function:    isAAttributefO
 *
 * Summary:   vevent2calendar helper function for RRULE
 *
 ***********************************************************************/
char *isAAttributeOfO(char *rrule, char *pair)
{
	char* val = NULL;
	gchar** attrval;
	gchar **rruletokens = g_strsplit(rrule, ";", 0);
	int i;

	for(i=0; rruletokens[i]!=NULL; i++)
	{
		attrval = g_strsplit(rruletokens[i], "=", 2);
		val = g_strdup(attrval[1]);

		if(strcmp(attrval[0], pair) == 0) {
			g_strfreev(attrval);
			g_strfreev(rruletokens);
			return val;
		}
		g_strfreev(attrval);
		g_free(val);
	}
	return NULL;
}

/***********************************************************************
 *
 * Function:    vevent2calendar
 *
 * Summary:   converts an vevent card to an palm calendar record
 *
 ***********************************************************************/
void vevent2calendar(palm_connection *conn, palm_entry *entry, char *vevent)
{
	VObjectO *v, *t, *prop, *vcal;
	VObjectIteratorO iter, j;
	const char *n;
	char *buffer;
	char *attrValue;
	char *day;
	char *rrulestr;

	palm_debug(conn, 2, "converting vevent to calendar");

	registerMimeErrorHandlerO(VObjectOErrorHander);
	vcal = Parse_MIMEO(vevent, strlen(vevent));

	initPropIteratorO(&iter,vcal);

	memset(&(entry->appointment), 0, sizeof(entry->appointment));
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
	entry->appointment.description = "";
	entry->appointment.note = NULL;

	while(moreIterationO(&iter)) {
		v = nextVObjectO(&iter);
		n = vObjectNameO(v);

		if (n) {
			if (strcmp(n,VCEventPropO) == 0) {
				initPropIteratorO(&j,v);
				while(moreIterationO(&j)) {
					t = nextVObjectO(&j);
					n = vObjectNameO(t);
					attrValue = fakeCStringO(vObjectUStringZValueO(t));

					//Summary
					if(strcmp(n,VCSummaryPropO) == 0) {
						entry->appointment.description = g_strcompress(g_convert(attrValue, strlen(attrValue), conn->codepage ,"utf8", NULL, NULL, NULL));
					}

					//Note
					if(strcmp(n,VCDescriptionPropO) == 0) {
						entry->appointment.note = g_strcompress(g_convert(attrValue, strlen(attrValue), conn->codepage ,"utf8", NULL, NULL, NULL));
					}

					//Begin
					if(strcmp(n,VCDTstartPropO) == 0) {
						entry->appointment.begin = vcaltime2tm(attrValue);
						if (strlen(attrValue) == 8) {
							entry->appointment.event = 1;
						}
					}

					//End
					if(strcmp(n,VCDTendPropO) == 0)
						entry->appointment.end = vcaltime2tm(attrValue);

					//Alarm
					if(strcmp(n,VCAlarmPropO) == 0) {
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
					}

					//recurrence
					if(strcmp(n,VCRRulePropO) == 0) {
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
					}

					//Exceptions TODO
					if(strcmp(n,"EXDATE") == 0) {
						entry->appointment.exception[entry->appointment.exceptions] = vcaltime2tm(fakeCStringO(vObjectUStringZValueO(t)));
						entry->appointment.exceptions++;
					}

					if (attrValue)
						free(attrValue);
				}
			}
		}
	}

	palm_debug(conn, 2 , "end: vcal2calendar");
	palm_debug(conn, 3, "Calendar Entry:\nevent: %i\nbegin: %s\nend: %s\nalarm: %i\nadvance: %i\nadvanceUnits: %i\nrepeatType: %i\nrepeatForever: %i\nrepeatEnd %s:\nrepeatFrequency: %i\nrepeatDay:\nrepeatDays:\nrepeatWeekstart: %i\nexceptions: %i\nexception:\ndescription: %s\nnote: %s\n"
	, entry->appointment.event, tm2vcaldatetime(entry->appointment.begin), tm2vcaldatetime(entry->appointment.end), entry->appointment.alarm, entry->appointment.advance,
	 entry->appointment.advanceUnits, entry->appointment.repeatType, entry->appointment.repeatForever, tm2vcaldatetime(entry->appointment.repeatEnd), entry->appointment.repeatFrequency,
	  entry->appointment.repeatWeekstart, entry->appointment.exceptions,
	  entry->appointment.description, entry->appointment.note);

	deleteVObjectO(vcal);
}

/***********************************************************************
 *
 * Function:    todo2vcard
 *
 * Summary:   converts an palm todo record to an vcard
 *
 ***********************************************************************/
GString *todo2vcal(palm_connection *conn, struct ToDo todo, char *category)
{
	VObjectO *vcal;
	VObjectO *vtodo;
	VObjectO *prop;
	GString *vcalstr;
	char *vcalptr;
	char buffer[1024];
	time_t now;
	struct tm *now_tm;
	char *tmp;

	palm_debug(conn, 2, "Translating todo to vcard");
	palm_debug(conn, 2, "ToDo Entry:\nIndefinite: %i\nDue: %s\nPriority: %i\nComplete: %i\nDescription: %s\nNote: %s", todo.indefinite, tm2vcaldatetime(todo.due), todo.priority, todo.complete, todo.description, todo.note);

  	vcal = newVObjectO(VCCalPropO);
  	vtodo = addPropO(vcal, VCTodoPropO);
	addPropValueO(vtodo, VCVersionPropO, "2.0");

	//UTF8 conversion
	if (todo.description) {
		tmp = g_convert(todo.description, strlen(todo.description), "utf8", conn->codepage, NULL, NULL, NULL);
		free(todo.description);
		todo.description = tmp;
	}

	if (todo.note) {
		tmp = g_convert(todo.note, strlen(todo.note), "utf8", conn->codepage, NULL, NULL, NULL);
		free(todo.note);
		todo.note = tmp;
	}


	//priority
	if (todo.priority) {
		snprintf(buffer, 1024, "%i", todo.priority + 2);
		addPropValueO(vtodo, VCPriorityPropO, buffer);
	}

	//due
	if (todo.indefinite == 0) {
		prop = addPropValueO(vtodo, VCDuePropO, tm2vcaldatetime(todo.due));
		addPropValueO(prop, VCValuePropO, "DATE");
	}

	/* completed */
	if(todo.complete) {
		now = time(NULL);
		now_tm = gmtime(&now);
		addPropValueO(vtodo, VCCompletedPropO, tm2vcaldatetime(*now_tm));
		addPropValueO(vtodo, VCStatusPropO, "COMPLETED");
	}

	/* note */
	if(todo.note && strlen(todo.note)) {
		prop = addPropValueO(vtodo, VCDescriptionPropO, escape_chars(todo.note));
		//addPropValueO(prop, "ENCODING", "QUOTED-PRINTABLE");
	}

	/* description */
	if(todo.description) {
		addPropValueO(vtodo, VCSummaryPropO,  escape_chars(todo.description));
	}
	
	if (category) {
		addPropValueO(vtodo, "CATEGORIES", category);
	}
	
	vcalptr = writeMemVObjectO(0,0,vcal);
	vcalstr = g_string_new(vcalptr);
	free(vcalptr);
	deleteVObjectO(vcal);

	palm_debug(conn, 3, "VCARD:\n%s", vcalstr->str);
	return vcalstr;
}

/***********************************************************************
 *
 * Function:    vcal2todo
 *
 * Summary:   converts an vcard to an palm todo record
 *
 ***********************************************************************/
void vcal2todo(palm_connection *conn, palm_entry *entry, char *vcard)
{
	VObjectO *v, *t, *vcal;
	VObjectIteratorO iter, j;
	const char *n;
	char *attrValue;

	palm_debug(conn, 2, "converting vcal to todo");

	registerMimeErrorHandlerO(VObjectOErrorHander);
	vcal = Parse_MIMEO(vcard, strlen(vcard));

	initPropIteratorO(&iter,vcal);

	memset(&(entry->todo), 0, sizeof(entry->todo));
	entry->todo.priority = 0;
	entry->todo.complete = 0;
	entry->todo.description = "";
	entry->todo.note = "";
	entry->todo.indefinite = 1;

	while(moreIterationO(&iter)) {
		v = nextVObjectO(&iter);
		n = vObjectNameO(v);

		if (n) {
			if (strcmp(n,VCTodoPropO) == 0) {
				initPropIteratorO(&j,v);
				while(moreIterationO(&j)) {
					t = nextVObjectO(&j);
					n = vObjectNameO(t);
					attrValue = fakeCStringO(vObjectUStringZValueO(t));


					//Priority
					if(strcmp(n,VCPriorityPropO) == 0) {
						entry->todo.priority = atoi(attrValue) - 2;
						if (entry->todo.priority < 1) {
							//Never go lower than 1
							entry->todo.priority = 1;
						}
						if (atoi(attrValue) == 0) {
							//Default to priority 5
							entry->todo.priority = 5;
						}
					}

					//Complete
					if(strcmp(n,VCStatusPropO) == 0) {
						if(!strcmp(attrValue, "COMPLETED")) {
							entry->todo.complete = 1;
						}
					}

					//Summary
					if(strcmp(n,VCSummaryPropO) == 0) {
						entry->todo.description = g_strcompress(g_convert(attrValue, strlen(attrValue), conn->codepage ,"utf8", NULL, NULL, NULL));
					}

					//Note
					if(strcmp(n,VCDescriptionPropO) == 0) {
						entry->todo.note = g_strcompress(g_convert(attrValue, strlen(attrValue), conn->codepage ,"utf8", NULL, NULL, NULL));
					}

					//Due
					if(strcmp(n,VCDuePropO) == 0) {
						entry->todo.due = vcaltime2tm(attrValue);
						entry->todo.indefinite = 0;
					}

					//Categories
					if (!strcmp(n, "CATEGORIES")) {
						palm_debug(conn, 3, "GOT CATEGORIES: %s\n", attrValue);
						gchar **array = g_strsplit(g_strcompress(attrValue), ",", 0);
						int z = 0;
						while (array[z] != NULL) {
							palm_debug(conn, 3, "testing %s\n", array[z]);
							entry->catID = get_category_id_from_name(conn, array[z]);
							if (entry->catID != 0) {
								palm_debug(conn, 3, "Found category %i\n", entry->catID);
								break;
							}
							z++;
						}
						g_strfreev(array);
					}
					
					if (attrValue)
						free(attrValue);
				}
			}
		}
	}

	palm_debug(conn, 2 , "end: vcal2todo");
	palm_debug(conn, 2, "ToDo Entry:\nIndefinite: %i\nDue: %s\nPriority: %i\nComplete: %i\nDescription: %s\nNote: %s", entry->todo.indefinite, tm2vcaldatetime(entry->todo.due), entry->todo.priority, entry->todo.complete, entry->todo.description, entry->todo.note);

	deleteVObjectO(vcal);
}

GString *address2vcard(palm_connection *conn, struct Address address, char *category)
{
	VObjectO *nameprop = NULL, *prop = NULL, *addrprop = NULL;
	VObjectO *vcard;
	int i = 0;
	GString *vcardstr;
	char *vcardptr;
	gchar *label = NULL;
	gchar *fullname = NULL;

	palm_debug(conn, 2, "Converting address to vcard");

	vcard = newVObjectO(VCCardPropO);
	addPropValueO(vcard, VCVersionPropO, "2.1");

	for (i = 0; i < 19; i++) {
		if (address.entry[i]) {
		char *tmp;
			palm_debug(conn, 3, "Palm Entry: %i: %s", i, address.entry[i]);
			
			printf("test %s, %s, %i\n", conn->codepage, address.entry[i], (address.entry[i])[1]);
			tmp = g_convert(address.entry[i], strlen(address.entry[i]), "utf8", conn->codepage, NULL, NULL, NULL);
			free(address.entry[i]);
			address.entry[i] = tmp;

			switch (i) {
				case 0:
					//Last Name
					if (!nameprop)
						nameprop = addPropO(vcard, "N");
					addPropValueO(nameprop, VCFamilyNamePropO, address.entry[i]);
					fullname = g_strdup(address.entry[0]);
					break;
				case 1:
					//First Name
					if (!nameprop)
						nameprop = addPropO(vcard, "N");
					addPropValueO(nameprop, VCGivenNamePropO, address.entry[i]);
					if (fullname)
						g_free(fullname);
					fullname = g_strconcat(address.entry[1], " ", address.entry[0], NULL);
					break;
				case 2:
					//Company
					prop = addPropO(vcard, "ORG");
					addPropValueO(prop, "ORGNAME", address.entry[i]);
					break;
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
					palm_debug(conn, 3, "GOT TEL with phoneLabel %i", address.phoneLabel[i - 3]);
					switch (address.phoneLabel[i - 3]) {
						case 0:
							prop = addPropValueO(vcard, "TEL", address.entry[i]);
							addPropO(prop, "WORK");
							addPropO(prop, "VOICE");
							break;
						case 1:
							prop = addPropValueO(vcard, "TEL", address.entry[i]);
							addPropO(prop, "HOME");
							break;
						case 2:
							prop = addPropValueO(vcard, "TEL", address.entry[i]);
							addPropO(prop, "WORK");
							addPropO(prop, "FAX");
							break;
						case 3:
							prop = addPropValueO(vcard, "TEL", address.entry[i]);
							addPropO(prop, "VOICE");
							break;
						case 4:
							prop = addPropValueO(vcard, "EMAIL", address.entry[i]);
							addPropO(prop, "INTERNET");
							break;
						case 5:
							prop = addPropValueO(vcard, "TEL", address.entry[i]);
							addPropO(prop, "PREF");
							break;
						case 6:
							prop = addPropValueO(vcard, "TEL", address.entry[i]);
							addPropO(prop, "PAGER");
							break;
						case 7:
							prop = addPropValueO(vcard, "TEL", address.entry[i]);
							addPropO(prop, "CELL");
							break;
					}
					break;
				case 8:
					if (!addrprop) {
						addrprop = addPropO(vcard, "ADR");
						addPropValueO(addrprop, "ENCODING", "QUOTED-PRINTABLE");
					}
					addPropValueO(addrprop, "STREET", address.entry[i]);
					break;
				case 9:
					if (!addrprop) {
						addrprop = addPropO(vcard, "ADR");
						addPropValueO(addrprop, "ENCODING", "QUOTED-PRINTABLE");
					}
					addPropValueO(addrprop, "L", address.entry[i]);
					break;
				case 10:
					if (!addrprop) {
						addrprop = addPropO(vcard, "ADR");
						addPropValueO(addrprop, "ENCODING", "QUOTED-PRINTABLE");
					}
					addPropValueO(addrprop, "R", address.entry[i]);
					break;
				case 11:
					if (!addrprop) {
						addrprop = addPropO(vcard, "ADR");
						addPropValueO(addrprop, "ENCODING", "QUOTED-PRINTABLE");
					}
					addPropValueO(addrprop, "PC", address.entry[i]);
					break;
				case 12:
					if (!addrprop) {
						addrprop = addPropO(vcard, "ADR");
						addPropValueO(addrprop, "ENCODING", "QUOTED-PRINTABLE");
					}
					addPropValueO(addrprop, "C", address.entry[i]);
					break;
				case 13:
					prop = addPropValueO(vcard, "TITLE", address.entry[i]);
					break;
				case 18:
					prop = addPropValueO(vcard, "NOTE", address.entry[i]);
					addPropValueO(prop, "ENCODING", "QUOTED-PRINTABLE");
					break;
			}
		}
	}

	if (addrprop) {
		//Make the Label
		for (i = 8; i <= 12; i++) {
			if (address.entry[i]) {
				if (!label) {
					label = "";
				} else {
					label = g_strconcat(label, "\n", NULL);
				}
				label = g_strconcat(label, address.entry[i], NULL);
			}
		}
		prop = addPropValueO(vcard, "LABEL", label);
		addPropValueO(prop, "ENCODING", "QUOTED-PRINTABLE");
		addPropO(prop, "WORK");
	}

	if (category) {
		addPropValueO(vcard, "CATEGORIES", category);
	}
	
	//Write the Fullname
	if (fullname) {
		addPropValueO(vcard, VCFullNamePropO, fullname);
		g_free(fullname);
	}

	vcardptr = writeMemVObjectO(0,0,vcard);
	vcardstr = g_string_new(vcardptr);
	free(vcardptr);
	deleteVObjectO(vcard);

	palm_debug(conn, 3, "VCARD:\n%s", vcardstr->str);
	return vcardstr;
}
#endif

static osync_bool conv_xml_to_palm(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);

	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", osxml_write_to_string((xmlDoc *)input));
	
	//Get the root node of the input document
	xmlNode *root = osxml_node_get_root((xmlDoc *)input, "contact", error);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get root element of xml-contact");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	/* Start the new entry */
	PSyncPalmEntry *entry = osync_try_malloc0(sizeof(PSyncPalmEntry), error);
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
		entry->address.entry[0] = osxml_find_node(cur, "First");
		entry->address.entry[1] = osxml_find_node(cur, "First");
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
	/*for (i = 0; i < 19; i++) {
	  if (entry->address.entry[i]) {
	    char *tmp = g_convert(entry->address.entry[i], strlen(entry->address.entry[i]), conn->codepage ,"utf8", NULL, NULL, NULL);
	    free(entry->address.entry[i]);
	    entry->address.entry[i] = tmp;
	  }
	  if (entry->address.entry[i] && !strlen(entry->address.entry[i])) {
	    free(entry->address.entry[i]);
	    entry->address.entry[i] = NULL;
	    palm_debug(conn, 3, "Address %i: %s", i, entry->address.entry[i]);
	  }
	}*/
	
	*free_input = TRUE;
	*output = (void *)entry;
	*outpsize = sizeof(PSyncPalmEntry);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "palm-contact");
	/*osync_env_format_set_compare_func(env, "file", compare_file);
	osync_env_format_set_duplicate_func(env, "file", duplicate_file);
	osync_env_format_set_destroy_func(env, "file", destroy_file);
	osync_env_format_set_print_func(env, "file", print_file);
	osync_env_format_set_copy_func(env, "file", copy_file);
	osync_env_format_set_create_func(env, "file", create_file);
	osync_env_format_set_revision_func(env, "file", revision_file);*/

	//osync_env_register_converter(env, CONVERTER_CONV, "palm-contact", "vcard21", conv_palm_to_vcard);
	osync_env_register_converter(env, CONVERTER_CONV, "vcard21", "palm-contact", conv_xml_to_palm);
}
