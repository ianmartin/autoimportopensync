/*
 * xmlformat-recurrence - common code for recurrence implementation
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007  Daniel Gollub <dgollub@suse.de>
 * Copyright (C) 2007  Christopher Stender <cstender@suse.de>
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

#include "xmlformat-common.h"

/*
 * Basic Recurrence Rules (vCalendar)
 *
 * The functions below are necessary for converting a vCalendar Recurrence Rule 
 * to a xmlformat-event.
 *
 * Description:
 * convert_vcal_rrule_frequency      get frequency value
 * convert_vcal_rrule_freqmod        get frequency modifier
 * convert_vcal_rrule_countuntil     get count or until value
 * convert_vcal_rrule_to_xml         get interval and call functions above
 */

static int convert_vcal_rrule_frequency(OSyncXMLField *xmlfield, const char *rule)
{
        int frequency_state = 0;
	char next = *(rule + 1);
	char *frequency = NULL;

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

OSyncXMLField *convert_vcal_rrule_to_xml(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, const char *rulename, OSyncError **error)
{
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, rulename, error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}

	const char *rule = vformat_attribute_get_nth_value(attr, 0);

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

	return xmlfield;
}


/*
 * Basic Recurrence Rules
 *
 * The functions below are necessary for converting a xmlformat-event 
 * to a vCalendar Recurrence Rule.
 *
 * Description:
 * convert_rrule_vcal_frequency      get frequency value
 * convert_rrule_vcal_freqmod        get frequency modifier
 * convert_rrule_vcal_countuntil     get count or until value
 * convert_xml_rrule_to_vcal         get interval and call functions above
 */

static char *convert_rrule_vcal_frequency(OSyncXMLField *xmlfield, int frequency_state_id)
{
	const char *frequency = NULL;
	char *frequency_id = NULL;

	/* set Frequency */
	frequency = osync_xmlfield_get_key_value(xmlfield, "Frequency");

	/* get frequency: only D(1), W(2), MP(3), MD(4), YD(5) and YM(6) are allowed */
	if (!strcmp(frequency, "DAILY")) {
		frequency_id = "D";
	} else if (!strcmp(frequency, "WEEKLY")) {
		frequency_id = "W";
	} else if (!strcmp(frequency, "MONTHLY") && frequency_state_id == 3) {
		frequency_id = "MP";
	} else if (!strcmp(frequency, "MONTHLY") && frequency_state_id == 4) {
		frequency_id = "MD";
	} else if (!strcmp(frequency, "YEARLY") && frequency_state_id == 5) {
		frequency_id = "YD";
	} else if (!strcmp(frequency, "YEARLY") && frequency_state_id == 6) {
		frequency_id = "YM";
	} else {
		osync_trace(TRACE_INTERNAL, "invalid or missing frequency");
		return NULL;
	}

	return frequency_id;
	      	
}

static char *convert_rrule_vcal_freqmod(OSyncXMLField *xmlfield, gchar **rule, int size, int freqstate)
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

static char *convert_rrule_vcal_until(const char *until_utc)
{
	int offset = 0; 
	char *until = NULL;


	/* UNTIL: 20070515T120000 */
	/* It is UTC : change the offset from 0 to the system UTC offset.·
	 * vcal doesn't store any TZ information. This means the device have to be
	 * in the same Timezone as the host.
	 */

	struct tm *ttm = osync_time_vtime2tm(until_utc);
	offset = osync_time_timezone_diff(ttm);
	g_free(ttm);
	until = osync_time_vtime2localtime(until_utc, offset);

	return until;

}

VFormatAttribute *convert_xml_rrule_to_vcal(VFormat *vformat, OSyncXMLField *xmlfield, const char *rulename, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, rulename);

	int frequency_state_id = 0, counter = 0;
	char *rule = NULL;

	// Grad the latest key of the field and check if it is a frequency state
	const char *frequency_state = osync_xmlfield_get_nth_key_name(xmlfield, osync_xmlfield_get_key_count(xmlfield)-1);

	
	if (frequency_state) {
		if ( !strcmp(frequency_state, "ByDay"))
			frequency_state_id = 3;
		else if (!strcmp(frequency_state, "ByMonthDay"))
			frequency_state_id = 4;
		else if (!strcmp(frequency_state, "ByYearDay"))
			frequency_state_id = 5;
		else if (!strcmp(frequency_state, "ByMonth"))
			frequency_state_id = 6;
	}

	/* get Interval */
	const char *interval = osync_xmlfield_get_key_value(xmlfield, "Interval");

	/* set frequency */
	const char *frequency = convert_rrule_vcal_frequency(xmlfield, frequency_state_id);

	if (frequency && interval )
		rule = g_strdup_printf("%s%s", frequency, interval);

	// FIXME : handle advanced frequency_mod with a convert_rrule_vcal_freqmod
	// We have a frequency state
	if (frequency_state_id) {
		const char *freq_mod = osync_xmlfield_get_key_value(xmlfield, frequency_state);
		rule = g_strdup_printf("%s %s", rule, freq_mod);
	}

	const char *until_utc = osync_xmlfield_get_key_value(xmlfield, "Until");
	if (until_utc) {
		char *until = convert_rrule_vcal_until(until_utc);
		rule = g_strdup_printf("%s %s", rule, until);
	}

	const char *count = osync_xmlfield_get_key_value(xmlfield, "Count");
	if (count) {
		rule = g_strdup_printf("%s #%s", rule, count);
	}

	vformat_attribute_add_value(attr, rule);

	vformat_add_attribute(vformat, attr);
	return attr;	

} 
// End of Basic Recurrence Rule



/*
 * Basic & Extended Recurrence Rules (iCalendar)
 *
 * The functions below are necessary for converting an iCalendar Recurrence Rule 
 * to a xmlformat-event.
 *
 * Description:
 * convert_ical_rrule_to_xml         converts an ical rrule to xmlformat
 */

OSyncXMLField *convert_ical_rrule_to_xml(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, const char *rulename, OSyncError **error) 
{
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, rulename, error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}

	osync_bool extended = FALSE;

	typedef struct {
	  char *name;
	  char *value;
	} rrule_t;

	rrule_t rrules[14];
	memset(rrules, 0, sizeof(rrules));
	rrules[0].name = "Frequency";
	rrules[1].name = "Until";
	rrules[2].name = "Count";
	rrules[3].name = "Interval";
	rrules[4].name = "BySecond";
	rrules[5].name = "ByMinute";
	rrules[6].name = "ByHour";
	rrules[7].name = "ByDay";
	rrules[8].name = "ByMonthDay";
	rrules[9].name = "ByYearDay";
	rrules[10].name = "ByWeekNo";
	rrules[11].name = "ByMonth";
	rrules[12].name = "BySetPos";
	rrules[13].name = "WKST";
	
	// parse values
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = values->data;
		g_assert(retstr);

		if (strstr(retstr->str, "FREQ=")) {
			rrules[0].value = retstr->str + strlen("FREQ=");
		} else if (strstr(retstr->str, "UNTIL=")) {
			rrules[1].value = retstr->str + strlen("UNTIL=");
		} else if (strstr(retstr->str, "COUNT=")) {	
			rrules[2].value = retstr->str + strlen("COUNT=");
		} else if (strstr(retstr->str, "INTERVAL=")) {
			rrules[3].value = retstr->str + strlen("INTERVAL=");
		} else if (strstr(retstr->str, "BYSECOND=")) {
			rrules[4].value = retstr->str + strlen("BYSECOND=");
			extended = TRUE;
		} else if (strstr(retstr->str, "BYMINUTE=")) {
			rrules[5].value = retstr->str + strlen("BYMINUTE=");
			extended = TRUE;
		} else if (strstr(retstr->str, "BYHOUR=")) { 
			rrules[6].value = retstr->str + strlen("BYHOUR=");
			extended = TRUE;
		} else if (strstr(retstr->str, "BYDAY=")) { 
			rrules[7].value = retstr->str + strlen("BYDAY=");
		} else if (strstr(retstr->str, "BYMONTHDAY=")) { 
			rrules[8].value = retstr->str + strlen("BYMONTHDAY=");
		} else if (strstr(retstr->str, "BYYEARDAY=")) { 
			rrules[9].value = retstr->str + strlen("BYYEARDAY=");
		} else if (strstr(retstr->str, "BYWEEKNO=")) { 
			rrules[10].value = retstr->str + strlen("BYWEEKNO=");
			extended = TRUE;
		} else if (strstr(retstr->str, "BYMONTH=")) { 
			rrules[11].value = retstr->str + strlen("BYMONTH=");
		} else if (strstr(retstr->str, "BYSETPOS=")) { 
			rrules[12].value = retstr->str + strlen("BYSETPOS=");
			extended = TRUE;
		} else if (strstr(retstr->str, "WKST=")) { 
			rrules[13].value = retstr->str + strlen("WKST=");
			extended = TRUE;
		}
	}
	
	// rename xmlfield if extended is true
	if (extended) {
		if (!strcmp(rulename, "ExceptionRule"))
			osync_xmlfield_set_name(xmlfield, "ExceptionRuleExtended");	
		else if (!strcmp(rulename, "RecurrenceRule"))	
			osync_xmlfield_set_name(xmlfield, "RecurrenceRuleExtended");	
	}

	// set interval to 1, if it wasn't set before
	if (rrules[3].value == NULL)
		rrules[3].value = "1";

	// set count to 0, if neither count nor until were set before
	if (rrules[1].value == NULL && rrules[2].value == NULL)
		rrules[2].value = "0";

	int i;
	for (i = 0; i <= 13; i++) {
		if (rrules[i].value != NULL)
			osync_xmlfield_add_key_value(xmlfield, rrules[i].name, rrules[i].value);
	}

	return xmlfield;
}

VFormatAttribute *convert_xml_rrule_to_ical(VFormat *vformat, OSyncXMLField *xmlfield, const char *rulename, const char *encoding)
{
/*
	g_assert(vformat);
	g_assert(xmlfield);
	g_assert(name);

	osync_trace(TRACE_INTERNAL, "Handling \"%s\" xml attribute", name);
	VFormatAttribute *attr = vformat_attribute_new(NULL, name);
	add_values(attr, xmlfield, encoding);
	vformat_add_attribute(vformat, attr);
	return attr;
*/
	VFormatAttribute *attr = vformat_attribute_new(NULL, rulename);
	vformat_add_attribute(vformat, attr);
	return attr;
}

