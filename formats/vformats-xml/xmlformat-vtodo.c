/*
 * xmlformat-vtodo - convert vtodo* to xmlformat-todo and backwards
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
 *
 */

#include "xmlformat-vtodo.h"
#include "xmlformat-vtodo10.h"
#include "xmlformat-vtodo20.h"

/* ******* Paramter ****** */
static void handle_fb_type_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "FreeBusyType");
}

static void handle_member_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "Member");
}

static void handle_related_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "Related");
}

static void handle_reltype_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "RelationType");
}

static OSyncXMLField *handle_percent_complete_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "PercentComplete", error);
}

/* VCALENDAR ONLY */
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

void *init_vtodo_to_xmlformat(VFormatType target)
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
		insert_attr_handler(hooks->attributes, "RRULE", (void *)handle_rrule_attribute);
		insert_attr_handler(hooks->attributes, "TRIGGER", (void *)handle_atrigger_attribute);
		insert_attr_handler(hooks->attributes, "REPEAT", (void *)handle_arepeat_attribute);
		insert_attr_handler(hooks->attributes, "DURATION", (void *)handle_aduration_attribute);
		insert_attr_handler(hooks->attributes, "ACTION", (void *)handle_aaction_attribute);
		insert_attr_handler(hooks->attributes, "ATTACH", (void *)handle_aattach_attribute);
		insert_attr_handler(hooks->attributes, "DESCRIPTION", (void *)handle_adescription_attribute);
		insert_attr_handler(hooks->attributes, "ATTENDEE", (void *)handle_aattendee_attribute);
		insert_attr_handler(hooks->attributes, "SUMMARY", (void *)handle_asummary_attribute);
	} else if (target == VFORMAT_TODO_10) {
		insert_attr_handler(hooks->attributes, "RRULE", (void *)handle_vcal_rrule_attribute);
		insert_attr_handler(hooks->attributes, "AALARM", (void *)handle_vcal_aalarm_attribute);
		insert_attr_handler(hooks->attributes, "DALARM", (void *)handle_vcal_dalarm_attribute);
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
static OSyncHookTables *init_xmlformat_to_vtodo(void)
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

osync_bool conv_xmlformat_to_vtodo(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error, int target)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);
	
	OSyncHookTables *hooks = init_xmlformat_to_vtodo();
	
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


void get_conversion_info(OSyncFormatEnv *env)
{
	OSyncFormatConverter *conv = NULL;
	OSyncError *error = NULL;

	OSyncObjFormat *xmlformat = osync_format_env_find_objformat(env, "xmlformat-todo");
	OSyncObjFormat *itodo = osync_format_env_find_objformat(env, "vtodo20");
	OSyncObjFormat *vtodo = osync_format_env_find_objformat(env, "vtodo10");

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, itodo, conv_xmlformat_to_vtodo20, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, itodo, xmlformat, conv_vtodo20_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vtodo, conv_xmlformat_to_vtodo10, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vtodo, xmlformat, conv_vtodo10_to_xmlformat, &error);
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
