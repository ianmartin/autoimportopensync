/*
 * xmlformat-todo - A plugin for parsing vtodo objects for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007  Daniel Gollub <dgollub@suse.de>
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
 */
#include <opensync/opensync.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync_xml.h>

#include "xmlformat.h"
#include "xmlformat-vtodo.h"

/* ******* Paramter ****** */
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
	/* TODO handle FormaType in XSD */
	osync_xmlfield_set_attr(xmlfield, "Type", "FormaType");
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

/***** Attributes *****/
static OSyncXMLField *handle_dtstamp_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "DateCalendarCreated", error);
}

static OSyncXMLField *handle_percent_complete_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "PercentComplete", error);
}

static OSyncXMLField *handle_created_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "DateCalendarCreated", error);
}

static OSyncXMLField *handle_dtstart_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "DateStarted", error);
}

static OSyncXMLField *handle_description_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Description", error);
}

static OSyncXMLField *handle_summary_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Summary", error);
}

static OSyncXMLField *handle_due_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "DateDue", error);
}

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

   static OSyncXMLField *handle_rrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{ 
	return handle_attribute_simple_content(xmlformat, attr, "RecurrenceRule", error);
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

static OSyncXMLField *handle_completed_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Completed", error);
}

static OSyncXMLField *handle_organizer_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Organizer", error);
}

static OSyncXMLField *handle_recurid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "RecurrenceID", error);
}

static OSyncXMLField *handle_status_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Status", error);
}

static OSyncXMLField *handle_duration_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Duration", error);
}

static OSyncXMLField *handle_attach_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Attach", error);
}

static OSyncXMLField *handle_attendee_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Attendee", error);
}

static OSyncXMLField *handle_contact_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Contact", error);
}

static OSyncXMLField *handle_exdate_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "ExclusionDate", error);
}

static OSyncXMLField *handle_exrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "ExclusionRule", error);
}

static OSyncXMLField *handle_rstatus_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "RStatus", error);
}

static OSyncXMLField *handle_related_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Related", error);
}

static OSyncXMLField *handle_resources_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Resources", error);
}

/* VCALENDAR ONLY */
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

/* VCALENDAR ONLY */
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

static OSyncXMLField *handle_atrigger_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmTrigger", error);
}


static OSyncXMLField *handle_arepeat_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmRepeat", error);
}

/* FIXME... Duration wrong placed? in XSD */
static OSyncXMLField *handle_aduration_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Duration", error);
}

static OSyncXMLField *handle_aaction_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmAction", error);
}

/* TODO: Add alarm attach to XSD */ 
static OSyncXMLField *handle_aattach_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmAttach", error);
}

static OSyncXMLField *handle_adescription_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmDescription", error);
}

/* TODO: Add alarm attende to XSD */

static OSyncXMLField *handle_aattendee_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmAttendee", error);
}

/* TODO: Add alarm summary to XSD */

static OSyncXMLField *handle_asummary_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "AlarmSummary", error);
}

static void vtodo_parse_attributes(OSyncHookTables *hooks, GHashTable *table, OSyncXMLFormat *xmlformat, GHashTable *paramtable, GList **attributes)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, attributes);
	
	GList *a = NULL;
	for (a = *attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;
		
		osync_trace(TRACE_INTERNAL, "attributes:\"%s\"", vformat_attribute_get_name(attr));
		if (!strcmp(vformat_attribute_get_name(attr), "BEGIN")) {

			osync_trace(TRACE_INTERNAL, "%s: FOUND BEGIN", __func__);
		} else if (!strcmp(vformat_attribute_get_name(attr), "END")) {
			osync_trace(TRACE_EXIT, "%s: FOUND END", __func__);
			*attributes = a;
			return;
		} else
			handle_attribute(hooks, xmlformat, attr, NULL);
	}
	osync_trace(TRACE_EXIT, "%s: DONE", __func__);
}

static OSyncConvCmpResult compare_todo(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftdata, rightdata);
	
	char* keys_content[] =  {"Content", NULL};
	OSyncXMLPoints points[] = {
		{"Summary", 		90, 	keys_content},
		{"DateStarted", 	10, 	keys_content},
		{"DateDue", 	10, 	keys_content},
		{NULL}
	};
	
	OSyncConvCmpResult ret = osync_xmlformat_compare((OSyncXMLFormat *)leftdata, (OSyncXMLFormat *)rightdata, points, 0, 100);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static void create_todo(char **data, unsigned int *size)
{
	OSyncError *error = NULL;
	*data = (char *)osync_xmlformat_new("todo", &error);
	if (!*data)
		osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

static void insert_attr_handler(GHashTable *table, const char *attrname, void* handler)
{
	g_hash_table_insert(table, (gpointer)attrname, handler);
}

static void *init_vtodo_to_xmlformat(VFormatType target)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);

	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));
	
	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);
	
	//VTODO components
	insert_attr_handler(hooks->attributes, "BEGIN", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "END", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "DTSTAMP", (void *)handle_dtstamp_attribute);
	insert_attr_handler(hooks->attributes, "DESCRIPTION", (void *)handle_description_attribute);
	insert_attr_handler(hooks->attributes, "SUMMARY", (void *)handle_summary_attribute);
	insert_attr_handler(hooks->attributes, "DUE", (void *)handle_due_attribute);
	insert_attr_handler(hooks->attributes, "DTSTART", (void *)handle_dtstart_attribute);
	insert_attr_handler(hooks->attributes, "PERCENT-COMPLETE", (void *)handle_percent_complete_attribute);
	insert_attr_handler(hooks->attributes, "CLASS", (void *)handle_class_attribute);
	insert_attr_handler(hooks->attributes, "CATEGORIES", (void *)handle_categories_attribute);
	insert_attr_handler(hooks->attributes, "PRIORITY", (void *)handle_priority_attribute);
	insert_attr_handler(hooks->attributes, "UID", (void *)handle_uid_attribute);
	insert_attr_handler(hooks->attributes, "URL", (void *)handle_url_attribute);
	insert_attr_handler(hooks->attributes, "SEQUENCE", (void *)handle_sequence_attribute);
	insert_attr_handler(hooks->attributes, "LAST-MODIFIED", (void *)handle_last_modified_attribute);
	insert_attr_handler(hooks->attributes, "CREATED", (void *)handle_created_attribute);
	insert_attr_handler(hooks->attributes, "RRULE", (void *)handle_rrule_attribute);
	insert_attr_handler(hooks->attributes, "RDATE", (void *)handle_rdate_attribute);
	insert_attr_handler(hooks->attributes, "LOCATION", (void *)handle_location_attribute);
	insert_attr_handler(hooks->attributes, "GEO", (void *)handle_geo_attribute);
	insert_attr_handler(hooks->attributes, "COMPLETED", (void *)handle_completed_attribute);
	insert_attr_handler(hooks->attributes, "ORGANIZER", (void *)handle_organizer_attribute);
	insert_attr_handler(hooks->attributes, "X-ORGANIZER", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "RECURRENCE-ID", (void *)handle_recurid_attribute);
	insert_attr_handler(hooks->attributes, "STATUS", (void *)handle_status_attribute);
	insert_attr_handler(hooks->attributes, "DURATION", (void *)handle_duration_attribute);
	insert_attr_handler(hooks->attributes, "ATTACH", (void *)handle_attach_attribute);
	insert_attr_handler(hooks->attributes, "ATTENDEE", (void *)handle_attendee_attribute);
	insert_attr_handler(hooks->attributes, "COMMENT", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "CONTACT", (void *)handle_contact_attribute);
	insert_attr_handler(hooks->attributes, "EXDATE", (void *)handle_exdate_attribute);
	insert_attr_handler(hooks->attributes, "EXRULE", (void *)handle_exrule_attribute);
	insert_attr_handler(hooks->attributes, "RSTATUS", (void *)handle_rstatus_attribute);
	insert_attr_handler(hooks->attributes, "RELATED-TO", (void *)handle_related_attribute);
	insert_attr_handler(hooks->attributes, "RESOURCES", (void *)handle_resources_attribute);
	insert_attr_handler(hooks->attributes, "X-LIC-ERROR", HANDLE_IGNORE);
	
	insert_attr_handler(hooks->parameters, "TZID", (void *)handle_tzid_parameter);
	insert_attr_handler(hooks->parameters, "VALUE", (void *)handle_value_parameter);
	insert_attr_handler(hooks->parameters, "ALTREP", (void *)handle_altrep_parameter);
	insert_attr_handler(hooks->parameters, "CN", (void *)handle_cn_parameter);
	insert_attr_handler(hooks->parameters, "DELEGATED-FROM", (void *)handle_delegated_from_parameter);
	insert_attr_handler(hooks->parameters, "DELEGATED-TO", (void *)handle_delegated_to_parameter);
	insert_attr_handler(hooks->parameters, "DIR", (void *)handle_dir_parameter);
	insert_attr_handler(hooks->parameters, "FMTTYPE", (void *)handle_format_type_parameter);
	insert_attr_handler(hooks->parameters, "MEMBER", (void *)handle_member_parameter);
	insert_attr_handler(hooks->parameters, "PARTSTAT", (void *)handle_partstat_parameter);
	insert_attr_handler(hooks->parameters, "RANGE", (void *)handle_range_parameter);
	//RELATED is for VALARM trigger
	insert_attr_handler(hooks->parameters, "RELATED", (void *)handle_related_parameter);
	insert_attr_handler(hooks->parameters, "RELTYPE", (void *)handle_reltype_parameter);
	insert_attr_handler(hooks->parameters, "ROLE", (void *)handle_role_parameter);
	insert_attr_handler(hooks->parameters, "RSVP", (void *)handle_rsvp_parameter);
	insert_attr_handler(hooks->parameters, "SENT-BY", (void *)handle_sent_by_parameter);
	insert_attr_handler(hooks->parameters, "X-LIC-ERROR", HANDLE_IGNORE);
	/* perhaps CUTYPE, ENCODING, LANGUAGE should be added */

	//VAlarm component
	if (target == VFORMAT_TODO_20) {
		insert_attr_handler(hooks->attributes, "TRIGGER", (void *)handle_atrigger_attribute);
		insert_attr_handler(hooks->attributes, "REPEAT", (void *)handle_arepeat_attribute);
		insert_attr_handler(hooks->attributes, "DURATION", (void *)handle_aduration_attribute);
		insert_attr_handler(hooks->attributes, "ACTION", (void *)handle_aaction_attribute);
		insert_attr_handler(hooks->attributes, "ATTACH", (void *)handle_aattach_attribute);
		insert_attr_handler(hooks->attributes, "DESCRIPTION", (void *)handle_adescription_attribute);
		insert_attr_handler(hooks->attributes, "ATTENDEE", (void *)handle_aattendee_attribute);
		insert_attr_handler(hooks->attributes, "SUMMARY", (void *)handle_asummary_attribute);
	} else if (target == VFORMAT_TODO_10) {
		insert_attr_handler(hooks->attributes, "AALARM", (void *)handle_aalarm_attribute);
		insert_attr_handler(hooks->attributes, "DALARM", (void *)handle_dalarm_attribute);
	}

	// FIXME: The functions below shoudn't be on alarmtable, but on another hash table
	insert_attr_handler(hooks->parameters, "TZID", (void *)handle_tzid_parameter);
	insert_attr_handler(hooks->parameters, "VALUE", (void *)handle_value_parameter);
	insert_attr_handler(hooks->parameters, "ALTREP", (void *)handle_altrep_parameter);
	insert_attr_handler(hooks->parameters, "CN", (void *)handle_cn_parameter);
	insert_attr_handler(hooks->parameters, "DELEGATED-FROM", (void *)handle_delegated_from_parameter);
	insert_attr_handler(hooks->parameters, "DELEGATED-TO", (void *)handle_delegated_to_parameter);
	insert_attr_handler(hooks->parameters, "DIR", (void *)handle_dir_parameter);
	insert_attr_handler(hooks->parameters, "FMTTYPE", (void *)handle_format_type_parameter);
	insert_attr_handler(hooks->parameters, "FBTYPE", (void *)handle_fb_type_parameter);
	insert_attr_handler(hooks->parameters, "MEMBER", (void *)handle_member_parameter);
	insert_attr_handler(hooks->parameters, "PARTSTAT", (void *)handle_partstat_parameter);
	insert_attr_handler(hooks->parameters, "RANGE", (void *)handle_range_parameter);
	insert_attr_handler(hooks->parameters, "RELATED", (void *)handle_related_parameter);
	insert_attr_handler(hooks->parameters, "RELTYPE", (void *)handle_reltype_parameter);
	insert_attr_handler(hooks->parameters, "ROLE", (void *)handle_role_parameter);
	insert_attr_handler(hooks->parameters, "RSVP", (void *)handle_rsvp_parameter);
	insert_attr_handler(hooks->parameters, "SENT-BY", (void *)handle_sent_by_parameter);
	insert_attr_handler(hooks->parameters, "X-LIC-ERROR", HANDLE_IGNORE);
	insert_attr_handler(hooks->parameters, "X-EVOLUTION-ALARM-UID", HANDLE_IGNORE);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static osync_bool conv_itodo_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);
	
	OSyncHookTables *hooks = init_vtodo_to_xmlformat(VFORMAT_TODO_20);
	
	osync_trace(TRACE_INTERNAL, "Input vcal is:\n%s", input);
	//Parse the vtodo
	VFormat *vtodo = vformat_new_from_string(input);

	OSyncXMLFormat *xmlformat = osync_xmlformat_new("todo", error);
	
	osync_trace(TRACE_INTERNAL, "parsing attributes");
	
	GList *attributes = vformat_get_attributes(vtodo);
	vtodo_parse_attributes(hooks, hooks->attributes, xmlformat, hooks->parameters, &attributes);
	
	g_hash_table_destroy(hooks->attributes);
	g_hash_table_destroy(hooks->parameters);
	g_free(hooks);
	
	*free_input = TRUE;
	*output = (char *)xmlformat;
	*outpsize = sizeof(xmlformat);

	osync_xmlformat_sort(xmlformat);
	
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "... Output XMLFormat is: \n%s", str);
	g_free(str);
	
	if (osync_xmlformat_validate(xmlformat) == FALSE)
		osync_trace(TRACE_INTERNAL, "XMLFORMAT TODO: Not valid!");
	else
		osync_trace(TRACE_INTERNAL, "XMLFORMAT TODO: Valid!");

	vformat_free(vtodo);
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}
	
static osync_bool conv_vtodo_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);
	
	OSyncHookTables *hooks = init_vtodo_to_xmlformat(VFORMAT_TODO_10);
	
	osync_trace(TRACE_INTERNAL, "Input vcal is:\n%s", input);
	//Parse the vtodo
	VFormat *vtodo = vformat_new_from_string(input);

	OSyncXMLFormat *xmlformat = osync_xmlformat_new("todo", error);
	
	osync_trace(TRACE_INTERNAL, "parsing attributes");
	
	GList *attributes = vformat_get_attributes(vtodo);
	vtodo_parse_attributes(hooks, hooks->attributes, xmlformat, hooks->parameters, &attributes);
	
	g_hash_table_destroy(hooks->attributes);
	g_hash_table_destroy(hooks->parameters);
	g_free(hooks);
	
	*free_input = TRUE;
	*output = (char *)xmlformat;
	*outpsize = sizeof(xmlformat);

	osync_xmlformat_sort(xmlformat);
	
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "... Output XMLFormat is: \n%s", str);
	g_free(str);
	
	if (osync_xmlformat_validate(xmlformat) == FALSE)
		osync_trace(TRACE_INTERNAL, "XMLFORMAT TODO: Not valid!");
	else
		osync_trace(TRACE_INTERNAL, "XMLFORMAT TODO: Valid!");

	vformat_free(vtodo);
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

//parameters
static void handle_xml_tzid_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "TimezoneID", content);
	g_free(content);
}

static void handle_xml_altrep_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "ALTREP", content);
	g_free(content);
}

static void handle_xml_cn_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "CN", content);
	g_free(content);
}

static void handle_xml_delegated_from_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "DELEGATED-FROM", content);
	g_free(content);
}

static void handle_xml_delegated_to_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "DELEGATED-TO", content);
	g_free(content);
}

static void handle_xml_dir_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "DIR", content);
	g_free(content);
}

static void handle_xml_format_type_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "FMTTYPE", content);
	g_free(content);
}

static void handle_xml_fb_type_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "FBTYPE", content);
	g_free(content);
}

static void handle_xml_member_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "MEMBER", content);
	g_free(content);
}

static void handle_xml_partstat_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "PARTSTAT", content);
	g_free(content);
}

static void handle_xml_range_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "RANGE", content);
	g_free(content);
}

static void handle_xml_reltype_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "RELTYPE", content);
	g_free(content);
}

static void handle_xml_related_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "RELATED", content);
	g_free(content);
}

static void handle_xml_role_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "ROLE", content);
	g_free(content);
}

static void handle_xml_rsvp_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "RSVP", content);
	g_free(content);
}

static void handle_xml_sent_by_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "SENT-BY", content);
	g_free(content);
}

//attributes	
static VFormatAttribute *handle_xml_prodid_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "PRODID", encoding);
}

static VFormatAttribute *handle_xml_method_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "METHOD", encoding);
}

static VFormatAttribute *handle_xml_geo_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling location xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "GEO");
	add_value(attr, xmlfield, "Latitude", encoding);
	add_value(attr, xmlfield, "Longitude", encoding);
	vformat_add_attribute(vtodo, attr);
	return attr;
}

static VFormatAttribute *handle_xml_dtstamp_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "DTSTAMP", encoding);
}

static VFormatAttribute *handle_xml_description_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "DESCRIPTION", encoding);
}

static VFormatAttribute *handle_xml_summary_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "SUMMARY", encoding);
}

static VFormatAttribute *handle_xml_due_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	/*TODO timezone*/
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DUE");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vtodo, attr);
	return attr;
}

static VFormatAttribute *handle_xml_dtstart_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	/* TODO timezone */
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTSTART");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vtodo, attr);
	return attr;
}

static VFormatAttribute *handle_xml_percent_complete_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "PERCENT-COMPLETE", encoding);
}

static VFormatAttribute *handle_xml_priority_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "PRIORITY", encoding);
}

static VFormatAttribute *handle_xml_sequence_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "SEQUENCE", encoding);
}

static VFormatAttribute *handle_xml_last_modified_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "LAST-MODIFIED", encoding);
}

static VFormatAttribute *handle_xml_created_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "CREATED", encoding);
}

static VFormatAttribute *handle_xml_rrule_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	/* TODO */
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RRULE");
	vformat_add_attribute(vtodo, attr);
	return attr;
}

static VFormatAttribute *handle_xml_rdate_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "RDATE", encoding);
}

static VFormatAttribute *handle_xml_location_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "LOCATION", encoding);
}

static VFormatAttribute *handle_xml_completed_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "COMPLETED", encoding);
}

static VFormatAttribute *handle_xml_organizer_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "ORGANIZER", encoding);
}

static VFormatAttribute *handle_xml_recurid_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "RECURRENCE-ID", encoding);
}

static VFormatAttribute *handle_xml_status_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "STATUS", encoding);
}

static VFormatAttribute *handle_xml_duration_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "DURATION", encoding);
}

static VFormatAttribute *handle_xml_attach_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "ATTACH", encoding);
}

static VFormatAttribute *handle_xml_attendee_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "ATTENDEE", encoding);
}

static VFormatAttribute *handle_xml_event_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "CONTACT", encoding);
}

static VFormatAttribute *handle_xml_exdate_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "EXDATE", encoding);
}

static VFormatAttribute *handle_xml_exrule_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "EXRULE", encoding);
}

static VFormatAttribute *handle_xml_rstatus_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "RSTATUS", encoding);
}

static VFormatAttribute *handle_xml_related_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "RELATED-TO", encoding);
}

static VFormatAttribute *handle_xml_resources_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "RESOURCES", encoding);
}

static VFormatAttribute *handle_xml_dtend_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{

	/* TODO timezone */
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTEND");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vtodo, attr);
	return attr;
}

static VFormatAttribute *handle_xml_transp_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "TRANSP", encoding);
}

static VFormatAttribute *handle_xml_atrigger_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vtodo, xmlfield, "TRIGGER", encoding);
}

static VFormatAttribute *handle_xml_arepeat_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "REPEAT");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vtodo, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aduration_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DURATION");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vtodo, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aaction_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ACTION");
	/* FIXME add_Value() #3 NULL is wrong */
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vtodo, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aattach_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTACH");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vtodo, attr);
	return attr;
}

static VFormatAttribute *handle_xml_adescription_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DESCRIPTION");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vtodo, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aattendee_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTENDEE");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vtodo, attr);
	return attr;
}

static VFormatAttribute *handle_xml_asummary_attribute(VFormat *vtodo, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "SUMMARY");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vtodo, attr);
	return attr;
}

static void insert_xml_attr_handler(GHashTable *table, const char *name, void *handler)
{
	g_hash_table_insert(table, (gpointer)name, handler);
}

static OSyncHookTables *init_xmlformat_to_itodo(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	
	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));

	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);

	//VTODO component
	insert_xml_attr_handler(hooks->attributes, "Uid", (void *)handle_xml_uid_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateCalendarCreated", (void *)handle_xml_dtstamp_attribute);
	insert_xml_attr_handler(hooks->attributes, "Description", (void *)handle_xml_description_attribute);
	insert_xml_attr_handler(hooks->attributes, "Summary", (void *)handle_xml_summary_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateEnd", (void *)handle_xml_dtend_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateDue", (void *)handle_xml_due_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateStarted", (void *)handle_xml_dtstart_attribute);
	insert_xml_attr_handler(hooks->attributes, "PercentComplete", (void *)handle_xml_percent_complete_attribute);
	insert_xml_attr_handler(hooks->attributes, "Class", (void *)handle_xml_class_attribute);
	insert_xml_attr_handler(hooks->attributes, "Categories", (void *)handle_xml_categories_attribute);
	insert_xml_attr_handler(hooks->attributes, "Priority", (void *)handle_xml_priority_attribute);
	insert_xml_attr_handler(hooks->attributes, "Url", (void *)handle_xml_url_attribute);
	insert_xml_attr_handler(hooks->attributes, "Sequence", (void *)handle_xml_sequence_attribute);
	insert_xml_attr_handler(hooks->attributes, "LastModified", (void *)handle_xml_last_modified_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateCreated", (void *)handle_xml_created_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceRule", (void *)handle_xml_rrule_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceDate", (void *)handle_xml_rdate_attribute);
	insert_xml_attr_handler(hooks->attributes, "Location", (void *)handle_xml_location_attribute);
	insert_xml_attr_handler(hooks->attributes, "Geo", (void *)handle_xml_geo_attribute);
	insert_xml_attr_handler(hooks->attributes, "Completed", (void *)handle_xml_completed_attribute);
	insert_xml_attr_handler(hooks->attributes, "Organizer", (void *)handle_xml_organizer_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceID", (void *)handle_xml_recurid_attribute);
	insert_xml_attr_handler(hooks->attributes, "Status", (void *)handle_xml_status_attribute);
	insert_xml_attr_handler(hooks->attributes, "Duration", (void *)handle_xml_duration_attribute);
	insert_xml_attr_handler(hooks->attributes, "Attach", (void *)handle_xml_attach_attribute);
	insert_xml_attr_handler(hooks->attributes, "Attendee", (void *)handle_xml_attendee_attribute);
	insert_xml_attr_handler(hooks->attributes, "Contact", (void *)handle_xml_event_attribute);
	insert_xml_attr_handler(hooks->attributes, "ExclusionDate", (void *)handle_xml_exdate_attribute);
	insert_xml_attr_handler(hooks->attributes, "ExclusionRule", (void *)handle_xml_exrule_attribute);
	insert_xml_attr_handler(hooks->attributes, "RStatus", (void *)handle_xml_rstatus_attribute);
	insert_xml_attr_handler(hooks->attributes, "Related", (void *)handle_xml_related_attribute);
	insert_xml_attr_handler(hooks->attributes, "Resources", (void *)handle_xml_resources_attribute);
	insert_xml_attr_handler(hooks->attributes, "ProductID", (void *)handle_xml_prodid_attribute);
	insert_xml_attr_handler(hooks->attributes, "Method", (void *)handle_xml_method_attribute);
	insert_xml_attr_handler(hooks->attributes, "Transparent", (void *)handle_xml_transp_attribute);

	insert_xml_attr_handler(hooks->parameters, "TimezoneID", (void *)handle_xml_tzid_parameter);
	insert_xml_attr_handler(hooks->parameters, "AlternateRep", (void *)handle_xml_altrep_parameter);
	insert_xml_attr_handler(hooks->parameters, "CommonName", (void *)handle_xml_cn_parameter);
	insert_xml_attr_handler(hooks->parameters, "DelegatedFrom", (void *)handle_xml_delegated_from_parameter);
	insert_xml_attr_handler(hooks->parameters, "DelegatedTo", (void *)handle_xml_delegated_to_parameter);
	insert_xml_attr_handler(hooks->parameters, "Directory", (void *)handle_xml_dir_parameter);
	insert_xml_attr_handler(hooks->parameters, "FormaType", (void *)handle_xml_format_type_parameter);
	insert_xml_attr_handler(hooks->parameters, "Member", (void *)handle_xml_member_parameter);
	insert_xml_attr_handler(hooks->parameters, "PartStat", (void *)handle_xml_partstat_parameter);
	insert_xml_attr_handler(hooks->parameters, "Range", (void *)handle_xml_range_parameter);
	insert_xml_attr_handler(hooks->parameters, "Related", (void *)handle_xml_related_parameter);
	insert_xml_attr_handler(hooks->parameters, "RelationType", (void *)handle_xml_reltype_parameter);
	insert_xml_attr_handler(hooks->parameters, "Role", (void *)handle_xml_role_parameter);
	insert_xml_attr_handler(hooks->parameters, "RSVP", (void *)handle_xml_rsvp_parameter);
	insert_xml_attr_handler(hooks->parameters, "SentBy", (void *)handle_xml_sent_by_parameter);
	
	//VAlarm component
	insert_xml_attr_handler(hooks->attributes, "AlarmTrigger", (void *)handle_xml_atrigger_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmRepeat", (void *)handle_xml_arepeat_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmDuration", (void *)handle_xml_aduration_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmAction", (void *)handle_xml_aaction_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmAttach", (void *)handle_xml_aattach_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmDescription", (void *)handle_xml_adescription_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmAttendee", (void *)handle_xml_aattendee_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmSummary", (void *)handle_xml_asummary_attribute);

        //FreeBusy component
	insert_xml_attr_handler(hooks->parameters, "Type", (void *)handle_xml_fb_type_parameter);


	//FIXME: The functions below shouldn't be on alarmtable, but on other hash table
	insert_xml_attr_handler(hooks->parameters, "TimezoneID", (void *)handle_xml_tzid_parameter);
	insert_xml_attr_handler(hooks->parameters, "AlternateRep", (void *)handle_xml_altrep_parameter);
	insert_xml_attr_handler(hooks->parameters, "CommonName", (void *)handle_xml_cn_parameter);
	insert_xml_attr_handler(hooks->parameters, "DelegatedFrom", (void *)handle_xml_delegated_from_parameter);
	insert_xml_attr_handler(hooks->parameters, "DelegatedTo", (void *)handle_xml_delegated_to_parameter);
	insert_xml_attr_handler(hooks->parameters, "Directory", (void *)handle_xml_dir_parameter);
	insert_xml_attr_handler(hooks->parameters, "FormaType", (void *)handle_xml_format_type_parameter);
	insert_xml_attr_handler(hooks->parameters, "Member", (void *)handle_xml_member_parameter);
	insert_xml_attr_handler(hooks->parameters, "PartStat", (void *)handle_xml_partstat_parameter);
	insert_xml_attr_handler(hooks->parameters, "Range", (void *)handle_xml_range_parameter);
	insert_xml_attr_handler(hooks->parameters, "Related", (void *)handle_xml_related_parameter);
	insert_xml_attr_handler(hooks->parameters, "RelationType", (void *)handle_xml_reltype_parameter);
	insert_xml_attr_handler(hooks->parameters, "Role", (void *)handle_xml_role_parameter);
	insert_xml_attr_handler(hooks->parameters, "RSVP", (void *)handle_xml_rsvp_parameter);
	insert_xml_attr_handler(hooks->parameters, "SentBy", (void *)handle_xml_sent_by_parameter);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static osync_bool conv_xmlformat_to_vtodo_both(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error, int target)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);
	
	OSyncHookTables *hooks = init_xmlformat_to_itodo();
	
	int i = 0;
	if (config) {
		gchar ** config_array = g_strsplit_set(config, "=;", 0);
		for (i=0; config_array[i]; i+=2)
		{
			/*TODO: what's the meaning of config? */
		}
		g_strfreev(config_array);
	}

	// Print input XMLFormat into terminal
	OSyncXMLFormat *xmlformat = (OSyncXMLFormat *)input;
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "Input XMLFormat is:\n%s", str);
	g_free(str);

	//Parsing xml attributes
	VFormat *vtodo = vformat_new();
	osync_trace(TRACE_INTERNAL, "parsing xml attributes");
	const char *std_encoding = NULL;
	if (target == VFORMAT_TODO_10)
		std_encoding = "QUOTED-PRINTABLE";
	else
		std_encoding = "B";
	
	OSyncXMLField *xmlfield = osync_xmlformat_get_first_field(xmlformat);
	for(; xmlfield != NULL; xmlfield = osync_xmlfield_get_next(xmlfield)) {
		xml_handle_attribute(hooks, vtodo, xmlfield, std_encoding);
	}
	
	g_hash_table_destroy(hooks->attributes);
	g_hash_table_destroy(hooks->parameters);
	g_free(hooks);

	*free_input = TRUE;
	*output = vformat_to_string (vtodo, target);
	*outpsize = strlen(*output) + 1;
	
	vformat_free(vtodo);	
	
	osync_trace(TRACE_INTERNAL, "Output vtodo is : \n%s", *output);
	osync_trace(TRACE_EXIT, "%s", __func__);
	
	return TRUE;
}

static osync_bool conv_xmlformat_to_itodo(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	return conv_xmlformat_to_vtodo_both(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_EVENT_20);
}

static osync_bool conv_xmlformat_to_vtodo(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	return conv_xmlformat_to_vtodo_both(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_EVENT_10);
}


static time_t get_revision(const char *data, unsigned int size, OSyncError **error)
{	
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, data, size, error);
	
	OSyncXMLFieldList *fieldlist = osync_xmlformat_search_field((OSyncXMLFormat *)data, "LastModified", NULL);

	int length = osync_xmlfieldlist_get_length(fieldlist);
	if (length != 1) {
		osync_xmlfieldlist_free(fieldlist);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find the revision.");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return -1;
	}

	OSyncXMLField *xmlfield = osync_xmlfieldlist_item(fieldlist, 0);
	osync_xmlfieldlist_free(fieldlist);
	
	const char *revision = osync_xmlfield_get_nth_key_value(xmlfield, 0);
	osync_trace(TRACE_INTERNAL, "About to convert string %s", revision);
	time_t time = vformat_time_to_unix(revision);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, time);
	return time;
}

void get_format_info(OSyncFormatEnv *env)
{
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("xmlformat-todo", "todo", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format xmlfomat: %s", osync_error_print(&error));
		return;
	}

	osync_objformat_set_compare_func(format, compare_todo);
	osync_objformat_set_destroy_func(format, destroy_xmlformat);
	osync_objformat_set_print_func(format, print_xmlformat);
	osync_objformat_set_copy_func(format, copy_xmlformat);
	osync_objformat_set_create_func(format, create_todo);

	osync_objformat_set_revision_func(format, get_revision);

//	osync_objformat_must_marshal(format);
	osync_objformat_set_marshal_func(format, marshal_xmlformat);
	osync_objformat_set_demarshal_func(format, demarshal_xmlformat);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);

}

void get_conversion_info(OSyncFormatEnv *env)
{
	OSyncFormatConverter *conv = NULL;
	OSyncError *error = NULL;

	OSyncObjFormat *xmlformat = osync_format_env_find_objformat(env, "xmlformat-todo");
	OSyncObjFormat *itodo = osync_format_env_find_objformat(env, "vtodo20");
	OSyncObjFormat *vtodo = osync_format_env_find_objformat(env, "vtodo10");

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, itodo, conv_xmlformat_to_itodo, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, itodo, xmlformat, conv_itodo_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vtodo, conv_xmlformat_to_vtodo, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vtodo, xmlformat, conv_vtodo_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
}

int get_version(void)
{
	return 1;
}
