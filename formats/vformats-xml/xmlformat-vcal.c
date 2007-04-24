/*
 * xmlformat-vcal - A plugin for parsing vevent10 objects for the opensync framework
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

#include "xmlformat-vcal.h"

/* vcal recurrence rule to XMLFormat-event converter */

/* detect FREQUENCY MODIFIED */
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

			if (i < size-2 && !sscanf(rule[i+1], "%d", &count)) {
				g_string_append_printf(fm_buffer, " %s", rule[i+1]);
				i++;
			}

		} else {
			/* e.g. Day or 'LD' (Last day) */
			g_string_append(fm_buffer, rule[i]);
		}
	}

	return g_string_free(fm_buffer, FALSE);
}

/* detect the COUNT or UNTIL field */
static void convert_vcal_rrule_countuntil(OSyncXMLField *xmlfield, const char *duration_block)
{
	int count;
	char *until = NULL;

	/* COUNT: #20 */
	if (sscanf(duration_block, "#%d", &count) == 1) {
		osync_xmlfield_set_key_value(xmlfield, "Count", duration_block+1);
		return;
	}

	if (!osync_time_isdate(duration_block)) {

		/* Check if this duration_block is a localtime timestamp.
		 * If it is not UTC change the offset from 0 to the system UTC offset.·
		 * vcal doesn't store any TZ information. This means the device have to be
		 * in the same Timezone as the host.
		 */
		int offset;

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

/* detect the FREQUENCY field */
static int convert_vcal_rrule_frequency(OSyncXMLField *xmlfield, const char *rule)
{

        int frequency_state = 0;
	char next = *(rule + 1);
	char *frequency;

	/* get frequency: only D(1), W(2), MP(3), MD(4), YD(5) and YM(6) is allowed */
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

	osync_xmlfield_set_key_value(xmlfield, "Content", frequency);

	return frequency_state;
}

static void convert_vcal_rrule_to_xml(OSyncXMLField *xmlfield, const char *rule)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, xmlfield, rule);

	int frequency_state = 0, counter;
	char  *frequency_block = NULL, *freq_mod = NULL, *duration_block;

	gchar** blocks = g_strsplit(rule, " ", 256);

	/* count blocks */
	for(counter=0; blocks[counter]; counter++);

	frequency_block = blocks[0];
	duration_block = blocks[counter-1];

	/* FREQ */
	frequency_state = convert_vcal_rrule_frequency(xmlfield, frequency_block);

	/* Count/Until */
	convert_vcal_rrule_countuntil(xmlfield, duration_block);

	/* The interval is at the end of the block */
	frequency_block++;

	if (frequency_state > 2)
		frequency_block++;

	/* INTERVAL */
	osync_xmlfield_set_key_value(xmlfield, "Interval", frequency_block);

	/* Frequency modifier: BYDAY, BYMONTH, .... */
	if (counter > 2)
		freq_mod = convert_vcal_rrule_freqmod(xmlfield, blocks, counter, frequency_state);

	// TODO enum
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

/* ******* Paramter ****** */

static void handle_role_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "Role");
}

static void handle_status_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "Status");
}


/***** Attributes *****/
static OSyncXMLField *handle_prodid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "ProductID", error);
}

static OSyncXMLField *handle_created_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "DateCalendarCreated", error);
}

static OSyncXMLField *handle_dtstart_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "DateStarted", error);
}

static OSyncXMLField *handle_rrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
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

static OSyncXMLField *handle_description_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Description", error);
}

static OSyncXMLField *handle_summary_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Summary", error);
}

// XXX: vtodo only?
#if 0
static OSyncXMLField *handle_due_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Due", error);
}
#endif

static OSyncXMLField *handle_priority_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Priority", error);
}

static OSyncXMLField *handle_sequence_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Sequence", error);
}

static OSyncXMLField *handle_last_modified_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "LastModified", error);
}

static OSyncXMLField *handle_rdate_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "RecurrenceDate", error);
}

static OSyncXMLField *handle_location_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Location", error);
}

static OSyncXMLField *handle_geo_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
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

// XXX vtodo only?
#if 0
static OSyncXMLField *handle_completed_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Completed", error);
}
#endif

static OSyncXMLField *handle_status_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Status", error);
}

static OSyncXMLField *handle_attach_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Attach", error);
}

static OSyncXMLField *handle_attendee_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Attendee", error);
}

static OSyncXMLField *handle_exdate_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "ExclusionDate", error);
}

static OSyncXMLField *handle_exrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "ExclusionRule", error);
}

static OSyncXMLField *handle_related_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Related", error);
}

static OSyncXMLField *handle_resources_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Resources", error);
}

static OSyncXMLField *handle_dtend_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "DateEnd", error);
}

static OSyncXMLField *handle_transp_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
        char *value;
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "TimeTransparency", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	} 

        if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "1")) {
            value = "OPAQUE";
        } else {
            value = "TRANSPARENT";
        }

	osync_xmlfield_set_key_value(xmlfield, "Content", value); 
	return xmlfield; 

}

static OSyncXMLField *handle_aalarm_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
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

static OSyncXMLField *handle_dalarm_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
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

static void vcal_parse_attributes(OSyncHookTables *hooks, GHashTable *table, OSyncXMLFormat *xmlformat, GHashTable *paramtable, GList **attributes)
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


static void insert_attr_handler(GHashTable *table, const char *attrname, void* handler)
{
	g_hash_table_insert(table, (gpointer)attrname, handler);
}

static void *init_vcalendar_to_xmlformat(VFormatType target)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);

	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));
	
	/*
	hooks->table = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->tztable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->comptable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->compparamtable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->alarmtable = g_hash_table_new(g_str_hash, g_str_equal);
	*/

	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);
	
        // [vcal-1.0] vcal (same order as in spec!)
	insert_attr_handler(hooks->attributes, "BEGIN", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "END", HANDLE_IGNORE);

        // [vcal-1.0] calprop (same order as in spec!)
	insert_attr_handler(hooks->attributes, "DAYLIGHT", HANDLE_IGNORE); // TODO
	insert_attr_handler(hooks->attributes, "GEO", handle_geo_attribute);
	insert_attr_handler(hooks->attributes, "PRODID", handle_prodid_attribute);
	insert_attr_handler(hooks->attributes, "TZ", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "VERSION", HANDLE_IGNORE);

        // [vcal-1.0] simprop (same order as in spec!)
	insert_attr_handler(hooks->attributes, "ATTACH", handle_attach_attribute);
	insert_attr_handler(hooks->attributes, "ATTENDEE", handle_attendee_attribute);
	insert_attr_handler(hooks->attributes, "DCREATED", handle_created_attribute);
//	insert_attr_handler(hooks->attributes, "COMPLETED", handle_completed_attribute); // XXX: vtodo ONLY?
	insert_attr_handler(hooks->attributes, "DESCRIPTION", handle_description_attribute);
//	insert_attr_handler(hooks->attributes, "DUE", handle_due_attribute); // XXX: vtodo ONLY?
	insert_attr_handler(hooks->attributes, "DTEND", handle_dtend_attribute);
	insert_attr_handler(hooks->attributes, "EXRULE", handle_exrule_attribute);
	insert_attr_handler(hooks->attributes, "LAST-MODIFIED", handle_last_modified_attribute);
	insert_attr_handler(hooks->attributes, "LOCATION", handle_location_attribute);
	insert_attr_handler(hooks->attributes, "RNUM", HANDLE_IGNORE); // TODO
	insert_attr_handler(hooks->attributes, "PRIORITY", handle_priority_attribute);
	insert_attr_handler(hooks->attributes, "RELATED-TO", handle_related_attribute);
	insert_attr_handler(hooks->attributes, "RRULE", handle_rrule_attribute);
	insert_attr_handler(hooks->attributes, "SEQUENCE", handle_sequence_attribute);
	insert_attr_handler(hooks->attributes, "DTSTART", handle_dtstart_attribute);
	insert_attr_handler(hooks->attributes, "SUMMARY", handle_summary_attribute);
	insert_attr_handler(hooks->attributes, "TRANSP", handle_transp_attribute);
	insert_attr_handler(hooks->attributes, "URL", handle_url_attribute);
	insert_attr_handler(hooks->attributes, "UID", handle_uid_attribute);

        // [vcal-1.0] entprop (same order as in spec!)
	insert_attr_handler(hooks->attributes, "AALARM", handle_aalarm_attribute);
	insert_attr_handler(hooks->attributes, "CATEGORIES", handle_categories_attribute);
	insert_attr_handler(hooks->attributes, "CLASS", handle_class_attribute);
	insert_attr_handler(hooks->attributes, "DALARM", handle_dalarm_attribute);
	insert_attr_handler(hooks->attributes, "EXDATE", handle_exdate_attribute);
	insert_attr_handler(hooks->attributes, "MALARM", HANDLE_IGNORE);  // TODO
	insert_attr_handler(hooks->attributes, "PALARM", HANDLE_IGNORE);  // TODO
	insert_attr_handler(hooks->attributes, "RDATE", handle_rdate_attribute);
	insert_attr_handler(hooks->attributes, "RESOURCES", handle_resources_attribute);
	insert_attr_handler(hooks->attributes, "STATUS", handle_status_attribute);

        // [vcal-1.0] param (same order as in sepc!)
	insert_attr_handler(hooks->parameters, "TYPE", HANDLE_IGNORE); // TODO
	insert_attr_handler(hooks->parameters, "VALUE", handle_value_parameter);
	insert_attr_handler(hooks->parameters, "ENCODING", HANDLE_IGNORE); // TODO
	insert_attr_handler(hooks->parameters, "CHARSET", HANDLE_IGNORE); // TODO
	insert_attr_handler(hooks->parameters, "LANGUAGE", HANDLE_IGNORE); // TODO
	insert_attr_handler(hooks->parameters, "ROLE", handle_role_parameter);
	insert_attr_handler(hooks->parameters, "STATUS", handle_status_parameter);


// XXX: not quite sure if RSVP is part of vcal-1.0 .. some traces about in the spec...
//	insert_attr_handler(hooks->parameters, "RSVP", handle_rsvp_parameter);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

osync_bool conv_vcal_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, error);
	
	OSyncHookTables *hooks = init_vcalendar_to_xmlformat(VFORMAT_EVENT_10); 
	
	osync_trace(TRACE_INTERNAL, "Input vcal is:\n%s", input);
	
	//Parse the vevent
	VFormat *vcal = vformat_new_from_string(input);
	
	OSyncXMLFormat *xmlformat = osync_xmlformat_new("event", error);

	
	osync_trace(TRACE_INTERNAL, "parsing attributes");
	
	//For every attribute we have call the handling hook
	GList *attributes = vformat_get_attributes(vcal);
	vcal_parse_attributes(hooks, hooks->attributes, xmlformat, hooks->parameters, &attributes);
//static void vcal_parse_attributes(OSyncHookTables *hooks, GHashTable *table, OSyncXMLFormat *xmlformat, GHashTable *paramtable, GList **attributes)



	// TODO free more members...
	g_hash_table_destroy(hooks->attributes);
	g_hash_table_destroy(hooks->parameters);
	g_free(hooks);

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


	vformat_free(vcal);
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

// TODO
static VFormatAttribute *handle_xml_alarm_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
    return NULL;
}

static VFormatAttribute *handle_xml_prodid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "PRODID", encoding);
}

static VFormatAttribute *handle_xml_geo_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling location xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "GEO");
	add_value(attr, xmlfield, "Latitude", encoding);
	add_value(attr, xmlfield, "Longitude", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_description_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "DESCRIPTION", encoding);
}

static VFormatAttribute *handle_xml_summary_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "SUMMARY", encoding);
}

// XXX todo only?
#if 0
static VFormatAttribute *handle_xml_due_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
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
#endif

static VFormatAttribute *handle_xml_dtstart_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
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

static VFormatAttribute *handle_xml_priority_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "PRIORITY", encoding);
}

static VFormatAttribute *handle_xml_sequence_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "SEQUENCE", encoding);
}

static VFormatAttribute *handle_xml_last_modified_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "LAST-MODIFIED", encoding);
}

static VFormatAttribute *handle_xml_created_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "CREATED", encoding);
}

static VFormatAttribute *handle_xml_rrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	/* TODO */
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RRULE");
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_rdate_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RDATE", encoding);
}

static VFormatAttribute *handle_xml_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "LOCATION", encoding);
}

// XXX: vtodo only?
#if 0
static VFormatAttribute *handle_xml_completed_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "COMPLETED", encoding);
}
#endif

static VFormatAttribute *handle_xml_status_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "STATUS", encoding);
}

static VFormatAttribute *handle_xml_attach_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "ATTACH", encoding);
}

static VFormatAttribute *handle_xml_attendee_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "ATTENDEE", encoding);
}

static VFormatAttribute *handle_xml_exdate_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "EXDATE", encoding);
}

static VFormatAttribute *handle_xml_exrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "EXRULE", encoding);
}

static VFormatAttribute *handle_xml_related_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RELATED-TO", encoding);
}

static VFormatAttribute *handle_xml_resources_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RESOURCES", encoding);
}

static VFormatAttribute *handle_xml_dtend_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
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

static VFormatAttribute *handle_xml_transp_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "TRANSP", encoding);
}

// TODO
static VFormatAttribute *handle_xml_tzid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
    /*
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZID");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
    */
    return NULL;
}

static void insert_xml_attr_handler(GHashTable *table, const char *name, void *handler)
{
	g_hash_table_insert(table, (gpointer)name, handler);
}

static OSyncHookTables *init_xmlformat_to_vcal(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	
	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));
	
	/*
	hooks->table = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->tztable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->comptable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->alarmtable = g_hash_table_new(g_str_hash, g_str_equal);
	*/

	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);

        // [vcal-1.0] vcal (same order as in spec! and in the vcal counterpart)
        // XXX Placeholder for Daylight (needed at all?)
	insert_xml_attr_handler(hooks->attributes, "Geo", handle_xml_geo_attribute);
	insert_xml_attr_handler(hooks->attributes, "ProductID", handle_xml_prodid_attribute);
	insert_xml_attr_handler(hooks->attributes, "TimezoneID", handle_xml_tzid_attribute); // TODO
	insert_xml_attr_handler(hooks->attributes, "Version", HANDLE_IGNORE); // XXX

        // [vcal-1.0] simprop (same order as in spec! and in the vcal counterpart)
	insert_xml_attr_handler(hooks->attributes, "Attach", handle_xml_attach_attribute);
	insert_xml_attr_handler(hooks->attributes, "Attendee", handle_xml_attendee_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateCreated", handle_xml_created_attribute);
//	insert_xml_attr_handler(hooks->attributes, "Completed", handle_xml_completed_attribute); // XXX: vtodo ONLY?
	insert_xml_attr_handler(hooks->attributes, "Description", handle_xml_description_attribute);
//	insert_xml_attr_handler(hooks->attributes, "Due", handle_xml_due_attribute); // XXX: vtodo ONLY?
	insert_xml_attr_handler(hooks->attributes, "DateEnd", handle_xml_dtend_attribute);
	insert_xml_attr_handler(hooks->attributes, "ExclusionRule", handle_xml_exrule_attribute);
	insert_xml_attr_handler(hooks->attributes, "LastModified", handle_xml_last_modified_attribute);
	insert_xml_attr_handler(hooks->attributes, "Location", handle_xml_location_attribute);
        // XXX vcal counterpart: RNUM - what is it?
	insert_xml_attr_handler(hooks->attributes, "Priority", handle_xml_priority_attribute);
	insert_xml_attr_handler(hooks->attributes, "Related", handle_xml_related_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceRule", handle_xml_rrule_attribute);
	insert_xml_attr_handler(hooks->attributes, "Sequence", handle_xml_sequence_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateStarted", handle_xml_dtstart_attribute);
	insert_xml_attr_handler(hooks->attributes, "Summary", handle_xml_summary_attribute);
	insert_xml_attr_handler(hooks->attributes, "TimeTransparency", handle_xml_transp_attribute);
	insert_xml_attr_handler(hooks->attributes, "Url", handle_xml_url_attribute);
	insert_xml_attr_handler(hooks->attributes, "Uid", handle_xml_uid_attribute);

        // [vcal-1.0] entprop (same order as in spec! and in the vcal counterpart)
	insert_xml_attr_handler(hooks->attributes, "Alarm", handle_xml_alarm_attribute); // TODO
	insert_xml_attr_handler(hooks->attributes, "Categories", handle_xml_categories_attribute);
	insert_xml_attr_handler(hooks->attributes, "Class", handle_xml_class_attribute);
        // XXX vcal counterpart: DALARM ... already handeld by handle_xml_alarm_attribute
	insert_xml_attr_handler(hooks->attributes, "ExclusionDate", handle_xml_exdate_attribute);
        // XXX vcal counterpart: MALARM ... already handeld by handle_xml_alarm_attribute
        // XXX vcal counterpart: PALARM ... already handeld by handle_xml_alarm_attribute
	insert_xml_attr_handler(hooks->attributes, "RecurrenceDate", handle_xml_rdate_attribute);
	insert_xml_attr_handler(hooks->attributes, "Resources", handle_xml_resources_attribute);
	insert_xml_attr_handler(hooks->attributes, "Status", handle_xml_status_attribute);

        // [vcal-1.0] param (same order as in spec! and in the vcal counterpart)
        // They are all attributes... handle them as XML attribute.. not as element...

	//insert_xml_attr_handler(hooks->parameters, "Type", HANDLE_IGNORE); // TODO - Node correct? 
	//insert_xml_attr_handler(hooks->parameters, "Value", handle_xml_value_parameter);
        // XXX vcal counterpart: ENCODING ... XMLFormat-event is unify
        // XXX vcal counterpart: CHARSET ... XMLFormat-event is unify
        // TODO LANGUAGE is an XML attribute! how to handle attributes...
        // insert_xml_attr_handler(hooks->parameters, "Role", handle_xml_role_parameter); //ATTR!
        // XXX vcal: "STATUS" ... not as attribute! .. this time as attribute/paramter!

	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static osync_bool conv_xmlformat_to_vcalendar(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error, int target)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, error);

	OSyncHookTables *hooks = init_xmlformat_to_vcal();

	int i = 0;
	if (config) {
		gchar** config_array = g_strsplit_set(config, "=;", 0);
		for(i=0; config_array[i]; i+=2)
		{
			if(!config_array[i+1]) {
				osync_trace(TRACE_ERROR, "Error in the converter configuration.");
				g_hash_table_destroy(hooks->attributes);
				g_hash_table_destroy(hooks->parameters);
				g_free(hooks);
				g_strfreev(config_array);
				return FALSE;
			}
			
			if(strcmp(config_array[i], "VCAL_EXTENSION") == 0) {

				/*
				if(strcmp(config_array[i+1], "KDE") == 0)
					init_xmlformat_to_kde(hooks);
				else if(strcmp(config_array[i+1], "Evolution") == 0)
					init_xmlformat_to_evolution(hooks);
				*/	
					
			}else if(strcmp(config_array[i], "VCAL_ENCODING")) {
				
				if(strcmp(config_array[i+1], "UTF-16") == 0)
					;
				/* TODO: what to do? :) */
			}
		}
		g_strfreev(config_array);
	}

	OSyncXMLFormat *xmlformat = (OSyncXMLFormat *)input;
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "Input XMLFormat is:\n%s", str);
	g_free(str);

	//Make the new vcal
	VFormat *vcal = vformat_new();
	
	osync_trace(TRACE_INTERNAL, "parsing xml attributes");
	const char *std_encoding = NULL;
	if (target == VFORMAT_EVENT_10)
		std_encoding = "QUOTED-PRINTABLE";
	else
		std_encoding = "B";
	
	OSyncXMLField *xmlfield = osync_xmlformat_get_first_field(xmlformat);
	for(; xmlfield != NULL; xmlfield = osync_xmlfield_get_next(xmlfield)) {
		xml_handle_attribute(hooks, vcal, xmlfield, std_encoding);
	}
	
	g_hash_table_destroy(hooks->attributes);
	g_hash_table_destroy(hooks->parameters);
	g_free(hooks);

	*free_input = TRUE;
	*output = vformat_to_string(vcal, target);
	*outpsize = strlen(*output) + 1;

	vformat_free(vcal);

	osync_trace(TRACE_INTERNAL, "Output vcalendar is: \n%s", *output);

	osync_trace(TRACE_EXIT, "%s", __func__);
	
	return TRUE;
}

osync_bool conv_xmlformat_to_vcal(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	return conv_xmlformat_to_vcalendar(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_EVENT_10);
}

