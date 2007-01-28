/*
 * xml-vevent - A plugin for parsing vevent objects for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007  Daniel Gollub <dgollub@suse.de>
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

#include <opensync/opensync.h>
#include <opensync/opensync-merger.h>
#include <opensync/opensync-serializer.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-time.h>

#include <string.h> /* strcmp and strlen */
#include <stdio.h> /* printf  */

#include "vformat.h"
#include "xmlformat-vcal.h"

/*TODO: Refactor this and other xml-*.c code, to put all common code in the same place */


/* ******* Paramter ****** */
/*
static void handle_unknown_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown parameter %s", vformat_attribute_param_get_name(param));
	OSyncXMLField *property = xmlNewChild(xmlfield, NULL, (xmlChar*)"UnknownParam",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
	osxml_node_add(property, "ParamName", vformat_attribute_param_get_name(param));
}*/

static void handle_tzid_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "TimezoneID");
}

static void handle_value_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "Value");
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

static void handle_status_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "Status");
}


/***** Attributes *****/

/*
static OSyncXMLField *handle_unknown_attribute(OSyncXMLField *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown attribute %s", vformat_attribute_get_name(attr));
	OSyncXMLField *xmlfield = xmlNewChild(root, NULL, (xmlChar*)"UnknownNode", NULL);
	osxml_node_add(xmlfield, "NodeName", vformat_attribute_get_name(attr));
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osxml_node_add(xmlfield, "Content", retstr->str);
	}
	return xmlfield;
}
*/

static OSyncXMLField *_handle_attribute_content(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, const char *name, OSyncError **error) 
{ 
	osync_trace(TRACE_INTERNAL, "Handling %s attribute", name);
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, name, error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	} 
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0)); 
	return xmlfield; 
}


static OSyncXMLField *handle_prodid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "ProductID", error);
}


static OSyncXMLField *handle_method_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Method", error);
}

static OSyncXMLField *handle_dtstamp_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "DateCalendarCreated", error);
}

static OSyncXMLField *handle_percent_complete_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "PercentComplete", error);
}

static OSyncXMLField *handle_created_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "DateCalendarCreated", error);
}

static OSyncXMLField *handle_dtstart_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "DateStarted", error);
}

/* TODO
static OSyncXMLField *handle_rrule_attribute(OSyncXMLField *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling rrule attribute");
	OSyncXMLField *xmlfield = xmlNewChild(root, NULL, (xmlChar*)"RecurrenceRule", NULL);
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osxml_node_add(xmlfield, "Rule", retstr->str);
	}
	
	return xmlfield;
}
*/


static OSyncXMLField *handle_description_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Description", error);
}

static OSyncXMLField *handle_summary_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Summary", error);
}

static OSyncXMLField *handle_categories_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Categories attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Categories", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osync_xmlfield_add_key_value(xmlfield, "Category", retstr->str);
	}
	
	return xmlfield;
}

static OSyncXMLField *handle_class_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Class attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Class", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_due_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "DateDue", error);
}


static OSyncXMLField *handle_url_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Url", error);
}

static OSyncXMLField *handle_uid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Uid", error);
}


static OSyncXMLField *handle_priority_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Priority", error);
}

static OSyncXMLField *handle_sequence_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Sequence", error);
}

static OSyncXMLField *handle_last_modified_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "LastModified", error);
}

/*
   static OSyncXMLField *handle_rrule_attribute(OSyncXMLField *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	OSyncXMLField *xmlfield = xmlNewChild(root, NULL, (xmlChar*)"RecurrenceRule", NULL);
	osxml_node_add(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}*/

static OSyncXMLField *handle_rdate_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "RecurrenceDate", error);
}

static OSyncXMLField *handle_location_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Location", error);
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
	return _handle_attribute_content(xmlformat, attr, "Completed", error);
}

static OSyncXMLField *handle_organizer_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Organizer", error);
}

static OSyncXMLField *handle_recurid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "RecurrenceID", error);
}

static OSyncXMLField *handle_status_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Status", error);
}

static OSyncXMLField *handle_duration_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Duration", error);
}

static OSyncXMLField *handle_attach_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Attach", error);
}

static OSyncXMLField *handle_attendee_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Attendee", error);
}

static OSyncXMLField *handle_contact_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Contact", error);
}

static OSyncXMLField *handle_exdate_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "ExclusionDate", error);
}

static OSyncXMLField *handle_exrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "ExclusionRule", error);
}

static OSyncXMLField *handle_rstatus_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "RStatus", error);
}

static OSyncXMLField *handle_related_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Related", error);
}


static OSyncXMLField *handle_resources_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Resources", error);
}

static OSyncXMLField *handle_dtend_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "DateEnd", error);
}

static OSyncXMLField *handle_transp_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Transparency", error);
}

static OSyncXMLField *handle_calscale_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "CalendarScale", error);
}

/* TODO Timezone
static OSyncXMLField *handle_tzid_attribute(OSyncXMLField *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzid attribute");
	return xmlNewChild(root, NULL, (xmlChar*)"TimezoneID",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tz_location_attribute(OSyncXMLField *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tz location attribute");
	return xmlNewChild(root, NULL, (xmlChar*)"Location",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzoffsetfrom_location_attribute(OSyncXMLField *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzoffsetfrom attribute");
	return xmlNewChild(root, NULL, (xmlChar*)"TZOffsetFrom",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzoffsetto_location_attribute(OSyncXMLField *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzoffsetto attribute");
	return xmlNewChild(root, NULL, (xmlChar*)"TZOffsetTo",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzname_attribute(OSyncXMLField *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzname attribute");
	return xmlNewChild(root, NULL, (xmlChar*)"TimezoneName",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzdtstart_attribute(OSyncXMLField *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewChild(root, NULL, (xmlChar*)"DateStarted",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzrrule_attribute(OSyncXMLField *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzrrule attribute");
	OSyncXMLField *xmlfield = xmlNewChild(root, NULL, (xmlChar*)"RecurrenceRule", NULL);
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osxml_node_add(xmlfield, "Rule", retstr->str);
	}
	
	return xmlfield;
}

static OSyncXMLField *handle_tz_last_modified_attribute(OSyncXMLField *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewChild(root, NULL, (xmlChar*)"LastModified",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzurl_attribute(OSyncXMLField *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewChild(root, NULL, (xmlChar*)"TimezoneUrl",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzrdate_attribute(OSyncXMLField *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewChild(root, NULL, (xmlChar*)"TimezoneDate",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}
*/

static OSyncXMLField *handle_atrigger_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "AlarmTrigger", error);
}


static OSyncXMLField *handle_arepeat_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "AlarmRepeat", error);
}

/* FIXME... Duration wrong placed? in XSD */
static OSyncXMLField *handle_aduration_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "Duration", error);
}

static OSyncXMLField *handle_aaction_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "AlarmAction", error);
}

/* TODO: Add alarm attach to XSD */ 
static OSyncXMLField *handle_aattach_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "AlarmAttach", error);
}

static OSyncXMLField *handle_adescription_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "AlarmDescription", error);
}

/* TODO: Add alarm attende to XSD */

static OSyncXMLField *handle_aattendee_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "AlarmAttendee", error);
}

/* TODO: Add alarm summary to XSD */

static OSyncXMLField *handle_asummary_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return _handle_attribute_content(xmlformat, attr, "AlarmSummary", error);
}

static void vcalendar_handle_parameter(OSyncHookTables *hooks, OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, hooks, xmlfield, param);
	
	//Find the handler for this parameter
	void (* param_handler)(OSyncXMLField *, VFormatParam *);
	char *paramname = g_strdup_printf("%s=%s", vformat_attribute_param_get_name(param), vformat_attribute_param_get_nth_value(param, 0));
	param_handler = g_hash_table_lookup(hooks->parameters, paramname);
	g_free(paramname);

	if (!param_handler)
		param_handler = g_hash_table_lookup(hooks->parameters, vformat_attribute_param_get_name(param));
	
	if (param_handler == HANDLE_IGNORE) {
		printf("Ignored\n");
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	
	if (param_handler)
		param_handler(xmlfield, param);
//	else 
//		handle_unknown_parameter(current, param);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void vcal_handle_attribute(OSyncHookTables *hooks, OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s, %p)", __func__, hooks, xmlformat, attr, attr ? vformat_attribute_get_name(attr) : "None", error);
	OSyncXMLField *xmlfield = NULL;

	//Dont add empty stuff
	GList *v;
	for (v = vformat_attribute_get_values(attr); v; v = v->next) {
		char *value = v->data;
		if (strlen(value) != 0)
			goto has_value;
	}
	osync_trace(TRACE_EXIT, "%s: No values", __func__);
	return;
	
has_value:;
	
	//We need to find the handler for this attribute
	OSyncXMLField *(* attr_handler)(OSyncXMLFormat *, VFormatAttribute *, OSyncError **) = g_hash_table_lookup(hooks->table, vformat_attribute_get_name(attr));

	osync_trace(TRACE_INTERNAL, "Hook is: %p", attr_handler);
	if (attr_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	if (attr_handler)
		xmlfield = attr_handler(xmlformat, attr, error);
	/*
	else
		xmlfield = handle_unknown_attribute(xmlformat, attr, error);
	*/	

	//Handle all parameters of this attribute
	GList *params = vformat_attribute_get_params(attr);
	GList *p = NULL;
	for (p = params; p; p = p->next) {
		VFormatParam *param = p->data;
		vcalendar_handle_parameter(hooks, xmlfield, param);
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void vcal_parse_attributes(OSyncHookTables *hooks, GHashTable *table, OSyncXMLFormat *xmlformat, GHashTable *paramtable, GList **attributes)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, attributes);
	
	GList *a = NULL;
	for (a = *attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;

		if (!strcmp(vformat_attribute_get_name(attr), "BEGIN")) {
			//Handling supcomponent
			a = a->next;
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
		} else if (!strcmp(vformat_attribute_get_name(attr), "END")) {
			osync_trace(TRACE_EXIT, "%s: Found END", __func__);
			*attributes = a;
			return;
		} else
			vcal_handle_attribute(table, paramtable, attr, xmlfield);

	}
	osync_trace(TRACE_EXIT, "%s: Done", __func__);
}

static osync_bool needs_encoding(const unsigned char *tmp, const char *encoding)
{
	int i = 0;
	if (!strcmp(encoding, "QUOTED-PRINTABLE")) {
		while (tmp[i] != 0) {
			if (tmp[i] > 127 || tmp[i] == 10 || tmp[i] == 13)
				return TRUE;
			i++;
		}
	} else {
		return !g_utf8_validate((gchar*)tmp, -1, NULL);
	}
	return FALSE;
}

static osync_bool needs_charset(const unsigned char *tmp)
{
	int i = 0;
	while (tmp[i] != 0) {
		if (tmp[i] > 127)
			return TRUE;
		i++;
	}
	return FALSE;
}

static void add_value(VFormatAttribute *attr, OSyncXMLField *xmlfield, const char *name, const char *encoding)
{
	const char *tmp = osync_xmlfield_get_key_value(xmlfield, name);
	
	if (!tmp) {
		/* If there is no node with the given name, add an empty value to the list.
		 * This is necessary because some fields (N and ADR, for example) need
		 * a specific order of the values
		 */
		tmp = "";
	}
	
	if (needs_charset((unsigned char*)tmp))
		if (!vformat_attribute_has_param (attr, "CHARSET"))
			vformat_attribute_add_param_with_value(attr, "CHARSET", "UTF-8");
	
	if (needs_encoding((unsigned char*)tmp, encoding)) {
		if (!vformat_attribute_has_param (attr, "ENCODING"))
			vformat_attribute_add_param_with_value(attr, "ENCODING", encoding);
		vformat_attribute_add_value_decoded(attr, tmp, strlen(tmp) + 1);
	} else
		vformat_attribute_add_value(attr, tmp);
}

/*
static void xml_handle_unknown_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown xml parameter");
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, (char*)xmlfield->name, content);
	g_free(content);
}
*/

/*
static void handle_xml_category_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_value(attr, content);
	g_free(content);
}
*/


/*
static void handle_xml_rule_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_value(attr, content);
	g_free(content);
}

static void handle_xml_value_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "VALUE", content);
	g_free(content);
}

static void handle_xml_altrep_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "ALTREP", content);
	g_free(content);
}

static void handle_xml_cn_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "CN", content);
	g_free(content);
}

static void handle_xml_delegated_from_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "DELEGATED-FROM", content);
	g_free(content);
}

static void handle_xml_delegated_to_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "DELEGATED-TO", content);
	g_free(content);
}

static void handle_xml_dir_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "DIR", content);
	g_free(content);
}

static void handle_xml_format_type_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "FMTTYPE", content);
	g_free(content);
}

static void handle_xml_fb_type_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "FBTYPE", content);
	g_free(content);
}

static void handle_xml_member_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "MEMBER", content);
	g_free(content);
}

static void handle_xml_partstat_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "PARTSTAT", content);
	g_free(content);
}

static void handle_xml_range_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "RANGE", content);
	g_free(content);
}

static void handle_xml_reltype_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "RELTYPE", content);
	g_free(content);
}

static void handle_xml_related_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "RELATED", content);
	g_free(content);
}

static void handle_xml_role_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "ROLE", content);
	g_free(content);
}

static void handle_xml_rsvp_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "RSVP", content);
	g_free(content);
}

static void handle_xml_sent_by_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, "SENT-BY", content);
	g_free(content);
}
*/

/*
static VFormatAttribute *xml_handle_unknown_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown xml attribute %s", root->name);
	char *name = osxml_find_node(root, "NodeName");
	VFormatAttribute *attr = vformat_attribute_new(NULL, name);
	g_free(name);
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}
*/

static VFormatAttribute *handle_xml_prodid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling prodid xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "PRODID");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_method_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling method xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "METHOD");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
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

static VFormatAttribute *handle_xml_url_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling url xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "URL");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_uid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling uid xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "UID");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_class_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling class xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CLASS");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_categories_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling categories xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CATEGORIES");
	vformat_add_attribute(vevent, attr);
	return attr;
}


static void xml_vcal_handle_parameter(OSyncHookTables *hooks, VFormatAttribute *attr, OSyncXMLField *xmlfield, int attr_nr)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s, %i)", __func__, hooks, attr, xmlfield, xmlfield ? osync_xmlfield_get_name(xmlfield) : "None", attr_nr);
	
	//Find the handler for this parameter
	const char* par_name = osync_xmlfield_get_nth_attr_name(xmlfield, attr_nr);
	const char* par_value = osync_xmlfield_get_nth_attr_value(xmlfield, attr_nr);
	
	void (* xml_param_handler)(VFormatAttribute *attr, OSyncXMLField *);
	char *paramname = g_strdup_printf("%s=%s", par_name, par_value);
	xml_param_handler = g_hash_table_lookup(hooks->parameters, paramname);
	g_free(paramname);
	
	if (!xml_param_handler)
		xml_param_handler = g_hash_table_lookup(hooks->parameters, par_name);
	
	if (xml_param_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}

	if (xml_param_handler)
		xml_param_handler(attr, xmlfield);
	else 
		printf("No handler found!!!\n");
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void xml_vcal_handle_attribute(OSyncHookTables *hooks, VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, hooks, vevent, xmlfield, xmlfield ? osync_xmlfield_get_name(xmlfield) : "None");
	
	VFormatAttribute *attr = NULL;
	
	//We need to find the handler for this attribute
	VFormatAttribute *(* xml_attr_handler)(VFormat *vevent, OSyncXMLField *xmlfield, const char *) = g_hash_table_lookup(hooks->attributes, osync_xmlfield_get_name(xmlfield));
	osync_trace(TRACE_INTERNAL, "xml hook is: %p", xml_attr_handler);
	if (xml_attr_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	if (xml_attr_handler)
		attr = xml_attr_handler(vevent, xmlfield, encoding);
	else {
		osync_trace(TRACE_EXIT, "%s: Ignored2", __func__);
		return;
	}
	
	//Handle all parameters of this attribute
	int i, c = osync_xmlfield_get_attr_count(xmlfield);
	for(i=0; i<c; i++) {
		xml_vcal_handle_parameter(hooks, attr, xmlfield, i);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);	
}

/*
void xml_parse_attribute(OSyncHookTables *hooks, GHashTable *table, OSyncXMLField **xmlfield, VFormat *vcal)
{
	osync_trace(TRACE_INTERNAL, "parsing xml attributes");
	OSyncXMLField *root = xmlfield;
	while (root) {
		if (!strcmp((char*)root->name, "Todo")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VTODO");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = root->children;
			xml_parse_attribute(hooks, hooks->comptable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VTODO");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)root->name, "Timezone")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VTIMEZONE");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = root->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VTIMEZONE");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)root->name, "Event")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VEVENT");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = root->children;
			xml_parse_attribute(hooks, hooks->comptable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VEVENT");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)root->name, "Journal")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VJOURNAL");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = root->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VJOURNAL");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)root->name, "DaylightSavings")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "DAYLIGHT");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = root->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "DAYLIGHT");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)root->name, "Standard")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "STANDARD");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = root->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "STANDARD");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)root->name, "Alarm")) {
			VFormatAttribute *attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VALARM");
			vformat_add_attribute(vcal, attr);
			OSyncXMLField *child = root->children;
			xml_parse_attribute(hooks, hooks->alarmtable, &child, vcal);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VALARM");
			vformat_add_attribute(vcal, attr);
		} else {
			xml_vcal_handle_attribute(table, vcal, root);
		}
		root = root->next;
	}
}
*/

static OSyncConvCmpResult compare_event(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftdata, rightdata);
	
	char* keys_content[] =  {"Content", NULL};
	OSyncXMLPoints points[] = {
		{"Summary", 		90, 	keys_content},
		{"DateTimeStart", 	10, 	keys_content},
		{"DateTimeEnd", 	10, 	keys_content},
		{NULL}
	};
	
	OSyncConvCmpResult ret = osync_xmlformat_compare((OSyncXMLFormat *)leftdata, (OSyncXMLFormat *)rightdata, points, 0, 100);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static void insert_attr_handler(GHashTable *table, const char *attrname, void* handler)
{
	g_hash_table_insert(table, (gpointer)attrname, handler);
}

static void *init_ical_to_xmlformat(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);

	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));
	
	hooks->table = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->tztable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->comptable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->compparamtable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->alarmtable = g_hash_table_new(g_str_hash, g_str_equal);
	
	//todo attributes
	insert_attr_handler(hooks->comptable, "BEGIN", HANDLE_IGNORE);
	insert_attr_handler(hooks->comptable, "END", HANDLE_IGNORE);
	insert_attr_handler(hooks->comptable, "UID", HANDLE_IGNORE);	
	insert_attr_handler(hooks->comptable, "DTSTAMP", handle_dtstamp_attribute);
	insert_attr_handler(hooks->comptable, "DESCRIPTION", handle_description_attribute);
	insert_attr_handler(hooks->comptable, "SUMMARY", handle_summary_attribute);
	insert_attr_handler(hooks->comptable, "DUE", handle_due_attribute);
	insert_attr_handler(hooks->comptable, "DTSTART", handle_dtstart_attribute);
	insert_attr_handler(hooks->comptable, "PERCENT-COMPLETE", handle_percent_complete_attribute);
	insert_attr_handler(hooks->comptable, "CLASS", handle_class_attribute);
	insert_attr_handler(hooks->comptable, "CATEGORIES", handle_categories_attribute);
	insert_attr_handler(hooks->comptable, "PRIORITY", handle_priority_attribute);
	insert_attr_handler(hooks->comptable, "UID", handle_uid_attribute);
	insert_attr_handler(hooks->comptable, "URL", handle_url_attribute);
	insert_attr_handler(hooks->comptable, "SEQUENCE", handle_sequence_attribute);
	insert_attr_handler(hooks->comptable, "LAST-MODIFIED", handle_last_modified_attribute);
	insert_attr_handler(hooks->comptable, "CREATED", handle_created_attribute);
	insert_attr_handler(hooks->comptable, "DCREATED", handle_created_attribute);
//	insert_attr_handler(hooks->comptable, "RRULE", handle_rrule_attribute);
	insert_attr_handler(hooks->comptable, "RDATE", handle_rdate_attribute);
	insert_attr_handler(hooks->comptable, "LOCATION", handle_location_attribute);
	insert_attr_handler(hooks->comptable, "GEO", handle_geo_attribute);
	insert_attr_handler(hooks->comptable, "COMPLETED", handle_completed_attribute);
	insert_attr_handler(hooks->comptable, "ORGANIZER", handle_organizer_attribute);
	insert_attr_handler(hooks->comptable, "ORGANIZER", HANDLE_IGNORE);
	insert_attr_handler(hooks->comptable, "X-ORGANIZER", HANDLE_IGNORE);
	insert_attr_handler(hooks->comptable, "RECURRENCE-ID", handle_recurid_attribute);
	insert_attr_handler(hooks->comptable, "STATUS", handle_status_attribute);
	insert_attr_handler(hooks->comptable, "DURATION", handle_duration_attribute);
	insert_attr_handler(hooks->comptable, "ATTACH", handle_attach_attribute);
	insert_attr_handler(hooks->comptable, "ATTENDEE", handle_attendee_attribute);
	insert_attr_handler(hooks->comptable, "COMMENT", HANDLE_IGNORE);
	insert_attr_handler(hooks->comptable, "CONTACT", handle_contact_attribute);
	insert_attr_handler(hooks->comptable, "EXDATE", handle_exdate_attribute);
	insert_attr_handler(hooks->comptable, "EXRULE", handle_exrule_attribute);
	insert_attr_handler(hooks->comptable, "RSTATUS", handle_rstatus_attribute);
	insert_attr_handler(hooks->comptable, "RELATED-TO", handle_related_attribute);
	insert_attr_handler(hooks->comptable, "RESOURCES", handle_resources_attribute);
	insert_attr_handler(hooks->comptable, "DTEND", handle_dtend_attribute);
	insert_attr_handler(hooks->comptable, "TRANSP", handle_transp_attribute);
	insert_attr_handler(hooks->comptable, "X-LIC-ERROR", HANDLE_IGNORE);
	
	insert_attr_handler(hooks->compparamtable, "TZID", handle_tzid_parameter);
	insert_attr_handler(hooks->compparamtable, "VALUE", handle_value_parameter);
	insert_attr_handler(hooks->compparamtable, "ALTREP", handle_altrep_parameter);
	insert_attr_handler(hooks->compparamtable, "CN", handle_cn_parameter);
	insert_attr_handler(hooks->compparamtable, "DELEGATED-FROM", handle_delegated_from_parameter);
	insert_attr_handler(hooks->compparamtable, "DELEGATED-TO", handle_delegated_to_parameter);
	insert_attr_handler(hooks->compparamtable, "DIR", handle_dir_parameter);
	insert_attr_handler(hooks->compparamtable, "FMTTYPE", handle_format_type_parameter);
	insert_attr_handler(hooks->compparamtable, "FBTYPE", handle_fb_type_parameter);
	insert_attr_handler(hooks->compparamtable, "MEMBER", handle_member_parameter);
	insert_attr_handler(hooks->compparamtable, "PARTSTAT", handle_partstat_parameter);
	insert_attr_handler(hooks->compparamtable, "RANGE", handle_range_parameter);
	insert_attr_handler(hooks->compparamtable, "RELATED", handle_related_parameter);
	insert_attr_handler(hooks->compparamtable, "RELTYPE", handle_reltype_parameter);
	insert_attr_handler(hooks->compparamtable, "ROLE", handle_role_parameter);
	insert_attr_handler(hooks->compparamtable, "RSVP", handle_rsvp_parameter);
	insert_attr_handler(hooks->compparamtable, "SENT-BY", handle_sent_by_parameter);
	insert_attr_handler(hooks->compparamtable, "X-LIC-ERROR", HANDLE_IGNORE);
	insert_attr_handler(hooks->compparamtable, "CHARSET", HANDLE_IGNORE);
	insert_attr_handler(hooks->compparamtable, "STATUS", handle_status_parameter);

	//vcal attributes
	insert_attr_handler(hooks->table, "PRODID", handle_prodid_attribute);
	insert_attr_handler(hooks->table, "PRODID", HANDLE_IGNORE);
	insert_attr_handler(hooks->table, "METHOD", handle_method_attribute);
	insert_attr_handler(hooks->table, "VERSION", HANDLE_IGNORE);
	insert_attr_handler(hooks->table, "ENCODING", HANDLE_IGNORE);
	insert_attr_handler(hooks->table, "CHARSET", HANDLE_IGNORE);
	insert_attr_handler(hooks->table, "BEGIN", HANDLE_IGNORE);
	insert_attr_handler(hooks->table, "END", HANDLE_IGNORE);
	insert_attr_handler(hooks->table, "CALSCALE", handle_calscale_attribute);
	insert_attr_handler(hooks->table, "X-LIC-ERROR", HANDLE_IGNORE);
	
	//Timezone
	/*
	insert_attr_handler(hooks->tztable, "TZID", handle_tzid_attribute);
	insert_attr_handler(hooks->tztable, "X-LIC-LOCATION", handle_tz_location_attribute);
	insert_attr_handler(hooks->tztable, "TZOFFSETFROM", handle_tzoffsetfrom_location_attribute);
	insert_attr_handler(hooks->tztable, "TZOFFSETTO", handle_tzoffsetto_location_attribute);
	insert_attr_handler(hooks->tztable, "TZNAME", handle_tzname_attribute);
	insert_attr_handler(hooks->tztable, "DTSTART", handle_tzdtstart_attribute);
	insert_attr_handler(hooks->tztable, "RRULE", handle_tzrrule_attribute);
	insert_attr_handler(hooks->tztable, "LAST-MODIFIED", handle_tz_last_modified_attribute);
	insert_attr_handler(hooks->tztable, "BEGIN", HANDLE_IGNORE);
	insert_attr_handler(hooks->tztable, "END", HANDLE_IGNORE);
	insert_attr_handler(hooks->tztable, "TZURL", handle_tzurl_attribute);
	insert_attr_handler(hooks->tztable, "COMMENT", HANDLE_IGNORE);
	insert_attr_handler(hooks->tztable, "RDATE", handle_tzrdate_attribute);
	*/

	/*FIXME: The functions below shoudn't be on tztable, but on another hash table */
	insert_attr_handler(hooks->tztable, "VALUE", handle_value_parameter);
	insert_attr_handler(hooks->tztable, "ALTREP", handle_altrep_parameter);
	insert_attr_handler(hooks->tztable, "CN", handle_cn_parameter);
	insert_attr_handler(hooks->tztable, "DELEGATED-FROM", handle_delegated_from_parameter);
	insert_attr_handler(hooks->tztable, "DELEGATED-TO", handle_delegated_to_parameter);
	insert_attr_handler(hooks->tztable, "DIR", handle_dir_parameter);
	insert_attr_handler(hooks->tztable, "FMTTYPE", handle_format_type_parameter);
	insert_attr_handler(hooks->tztable, "FBTYPE", handle_fb_type_parameter);
	insert_attr_handler(hooks->tztable, "MEMBER", handle_member_parameter);
	insert_attr_handler(hooks->tztable, "PARTSTAT", handle_partstat_parameter);
	insert_attr_handler(hooks->tztable, "RANGE", handle_range_parameter);
	insert_attr_handler(hooks->tztable, "RELATED", handle_related_parameter);
	insert_attr_handler(hooks->tztable, "RELTYPE", handle_reltype_parameter);
	insert_attr_handler(hooks->tztable, "ROLE", handle_role_parameter);
	insert_attr_handler(hooks->tztable, "RSVP", handle_rsvp_parameter);
	insert_attr_handler(hooks->tztable, "SENT-BY", handle_sent_by_parameter);
	insert_attr_handler(hooks->tztable, "X-LIC-ERROR", HANDLE_IGNORE);
	
	//VAlarm component
	insert_attr_handler(hooks->alarmtable, "TRIGGER", handle_atrigger_attribute);
	insert_attr_handler(hooks->alarmtable, "REPEAT", handle_arepeat_attribute);
	insert_attr_handler(hooks->alarmtable, "DURATION", handle_aduration_attribute);
	insert_attr_handler(hooks->alarmtable, "ACTION", handle_aaction_attribute);
	insert_attr_handler(hooks->alarmtable, "ATTACH", handle_aattach_attribute);
	insert_attr_handler(hooks->alarmtable, "DESCRIPTION", handle_adescription_attribute);
	insert_attr_handler(hooks->alarmtable, "ATTENDEE", handle_aattendee_attribute);
	insert_attr_handler(hooks->alarmtable, "SUMMARY", handle_asummary_attribute);

	/*FIXME: The functions below shoudn't be on alarmtable, but on another hash table */
	insert_attr_handler(hooks->alarmtable, "TZID", handle_tzid_parameter);
	insert_attr_handler(hooks->alarmtable, "VALUE", handle_value_parameter);
	insert_attr_handler(hooks->alarmtable, "ALTREP", handle_altrep_parameter);
	insert_attr_handler(hooks->alarmtable, "CN", handle_cn_parameter);
	insert_attr_handler(hooks->alarmtable, "DELEGATED-FROM", handle_delegated_from_parameter);
	insert_attr_handler(hooks->alarmtable, "DELEGATED-TO", handle_delegated_to_parameter);
	insert_attr_handler(hooks->alarmtable, "DIR", handle_dir_parameter);
	insert_attr_handler(hooks->alarmtable, "FMTTYPE", handle_format_type_parameter);
	insert_attr_handler(hooks->alarmtable, "FBTYPE", handle_fb_type_parameter);
	insert_attr_handler(hooks->alarmtable, "MEMBER", handle_member_parameter);
	insert_attr_handler(hooks->alarmtable, "PARTSTAT", handle_partstat_parameter);
	insert_attr_handler(hooks->alarmtable, "RANGE", handle_range_parameter);
	insert_attr_handler(hooks->alarmtable, "RELATED", handle_related_parameter);
	insert_attr_handler(hooks->alarmtable, "RELTYPE", handle_reltype_parameter);
	insert_attr_handler(hooks->alarmtable, "ROLE", handle_role_parameter);
	insert_attr_handler(hooks->alarmtable, "RSVP", handle_rsvp_parameter);
	insert_attr_handler(hooks->alarmtable, "SENT-BY", handle_sent_by_parameter);
	insert_attr_handler(hooks->alarmtable, "X-LIC-ERROR", HANDLE_IGNORE);
	insert_attr_handler(hooks->alarmtable, "X-EVOLUTION-ALARM-UID", HANDLE_IGNORE);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}


static osync_bool conv_ical_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, error);
	
	OSyncHookTables *hooks = init_ical_to_xmlformat(); 
	
	osync_trace(TRACE_INTERNAL, "Input vcal is:\n%s", input);
	
	//Parse the vevent
	VFormat *vcal = vformat_new_from_string(input);
	
	OSyncXMLFormat *xmlformat = osync_xmlformat_new("event", error);

	
	osync_trace(TRACE_INTERNAL, "parsing attributes");
	
	//For every attribute we have call the handling hook
	GList *attributes = vformat_get_attributes(vcal);
	vcal_parse_attributes(hooks, hooks->table, hooks->table, &attributes, root);


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

	vformat_free(vcal);
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}


static VFormatAttribute *handle_xml_dtstamp_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTSTAMP");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_description_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DESCRIPTION");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_summary_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "SUMMARY");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_due_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DUE");
	add_value(attr, root, "Content", encoding);
//	char *tzid = osxml_find_node(root, "TimezoneID")
//	vformat_attribute_add_param_with_value(attr, "TZID", tzid);
//	g_free(tzid);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_dtstart_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTSTART");
	add_value(attr, root, "Content", encoding);
//	char *tzid = osxml_find_node(root, "TimezoneID")
//	vformat_attribute_add_param_with_value(attr, "TZID", tzid);
//	g_free(tzid);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_percent_complete_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "PERCENT-COMPLETE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_priority_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "PRIORITY");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_sequence_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "SEQUENCE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_last_modified_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LAST-MODIFIED");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_created_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CREATED");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_rrule_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RRULE");
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_rdate_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RDATE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_location_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LOCATION");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_completed_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "COMPLETED");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_organizer_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ORGANIZER");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_recurid_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RECURRENCE-ID");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_status_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "STATUS");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_duration_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DURATION");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_attach_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTACH");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_attendee_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTENDEE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_event_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CONTACT");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_exdate_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "EXDATE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_exrule_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "EXRULE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_rstatus_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RSTATUS");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_related_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RELATED-TO");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_resources_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RESOURCES");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_dtend_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTEND");
	add_value(attr, root, "Content", encoding);
//	char *tzid = osxml_find_node(root, "TimezoneID")
//	vformat_attribute_add_param_with_value(attr, "TZID", tzid);
//	g_free(tzid);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_transp_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TRANSP");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_calscale_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CALSCALE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzid_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZID");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tz_location_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-LIC-LOCATION");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzoffsetfrom_location_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZOFFSETFROM");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzoffsetto_location_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZOFFSETTO");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzname_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZNAME");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzdtstart_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTSTART");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzrrule_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RRULE");
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tz_last_modified_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LAST-MODIFIED");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzurl_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZURL");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzrdate_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RDATE");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_atrigger_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TRIGGER");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_arepeat_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "REPEAT");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aduration_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DURATION");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aaction_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ACTION");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aattach_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTACH");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_adescription_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DESCRIPTION");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aattendee_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTENDEE");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_asummary_attribute(VFormat *vevent, OSyncXMLField *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "SUMMARY");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static void insert_xml_attr_handler(GHashTable *table, const char *name, void *handler)
{
	g_hash_table_insert(table, (gpointer)name, handler);
}

static OSyncHookTables *init_xmlformat_to_ical(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	
	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));
	
	hooks->table = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->tztable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->comptable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->alarmtable = g_hash_table_new(g_str_hash, g_str_equal);
	
	//todo attributes
	insert_xml_attr_handler(hooks->comptable, "Uid", handle_xml_uid_attribute);
	insert_xml_attr_handler(hooks->comptable, "DateCalendarCreated", handle_xml_dtstamp_attribute);
	insert_xml_attr_handler(hooks->comptable, "Description", handle_xml_description_attribute);
	insert_xml_attr_handler(hooks->comptable, "Summary", handle_xml_summary_attribute);
	insert_xml_attr_handler(hooks->comptable, "DateDue", handle_xml_due_attribute);
	insert_xml_attr_handler(hooks->comptable, "DateStarted", handle_xml_dtstart_attribute);
	insert_xml_attr_handler(hooks->comptable, "PercentComplete", handle_xml_percent_complete_attribute);
	insert_xml_attr_handler(hooks->comptable, "Class", handle_xml_class_attribute);
	insert_xml_attr_handler(hooks->comptable, "Categories", handle_xml_categories_attribute);
	insert_xml_attr_handler(hooks->comptable, "Priority", handle_xml_priority_attribute);
	insert_xml_attr_handler(hooks->comptable, "Url", handle_xml_url_attribute);
	insert_xml_attr_handler(hooks->comptable, "Sequence", handle_xml_sequence_attribute);
	insert_xml_attr_handler(hooks->comptable, "LastModified", handle_xml_last_modified_attribute);
	insert_xml_attr_handler(hooks->comptable, "DateCreated", handle_xml_created_attribute);
	insert_xml_attr_handler(hooks->comptable, "RecurrenceRule", handle_xml_rrule_attribute);
	insert_xml_attr_handler(hooks->comptable, "RecurrenceDate", handle_xml_rdate_attribute);
	insert_xml_attr_handler(hooks->comptable, "Location", handle_xml_location_attribute);
	insert_xml_attr_handler(hooks->comptable, "Geo", handle_xml_geo_attribute);
	insert_xml_attr_handler(hooks->comptable, "Completed", handle_xml_completed_attribute);
	insert_xml_attr_handler(hooks->comptable, "Organizer", handle_xml_organizer_attribute);
	insert_xml_attr_handler(hooks->comptable, "RecurrenceID", handle_xml_recurid_attribute);
	insert_xml_attr_handler(hooks->comptable, "Status", handle_xml_status_attribute);
	insert_xml_attr_handler(hooks->comptable, "Duration", handle_xml_duration_attribute);
	insert_xml_attr_handler(hooks->comptable, "Attach", handle_xml_attach_attribute);
	insert_xml_attr_handler(hooks->comptable, "Attendee", handle_xml_attendee_attribute);
	insert_xml_attr_handler(hooks->comptable, "Contact", handle_xml_event_attribute);
	insert_xml_attr_handler(hooks->comptable, "ExclusionDate", handle_xml_exdate_attribute);
	insert_xml_attr_handler(hooks->comptable, "ExclusionRule", handle_xml_exrule_attribute);
	insert_xml_attr_handler(hooks->comptable, "RStatus", handle_xml_rstatus_attribute);
	insert_xml_attr_handler(hooks->comptable, "Related", handle_xml_related_attribute);
	insert_xml_attr_handler(hooks->comptable, "Resources", handle_xml_resources_attribute);
	insert_xml_attr_handler(hooks->comptable, "DateEnd", handle_xml_dtend_attribute);
	insert_xml_attr_handler(hooks->comptable, "Transparency", handle_xml_transp_attribute);


	/*FIXME: The functions below shouldn't be on comptable, but on other hash table */
	/*
	insert_xml_attr_handler(hooks->comptable, "Category", handle_xml_category_parameter);
	insert_xml_attr_handler(hooks->comptable, "Rule", handle_xml_rule_parameter);
	insert_xml_attr_handler(hooks->comptable, "Value", handle_xml_value_parameter);
	insert_xml_attr_handler(hooks->comptable, "AlternateRep", handle_xml_altrep_parameter);
	insert_xml_attr_handler(hooks->comptable, "CommonName", handle_xml_cn_parameter);
	insert_xml_attr_handler(hooks->comptable, "DelegatedFrom", handle_xml_delegated_from_parameter);
	insert_xml_attr_handler(hooks->comptable, "DelegatedTo", handle_xml_delegated_to_parameter);
	insert_xml_attr_handler(hooks->comptable, "Directory", handle_xml_dir_parameter);
	insert_xml_attr_handler(hooks->comptable, "FormaType", handle_xml_format_type_parameter);
	insert_xml_attr_handler(hooks->comptable, "FreeBusyType", handle_xml_fb_type_parameter);
	insert_xml_attr_handler(hooks->comptable, "Member", handle_xml_member_parameter);
	insert_xml_attr_handler(hooks->comptable, "PartStat", handle_xml_partstat_parameter);
	insert_xml_attr_handler(hooks->comptable, "Range", handle_xml_range_parameter);
	insert_xml_attr_handler(hooks->comptable, "Related", handle_xml_related_parameter);
	insert_xml_attr_handler(hooks->comptable, "RelationType", handle_xml_reltype_parameter);
	insert_xml_attr_handler(hooks->comptable, "Role", handle_xml_role_parameter);
	insert_xml_attr_handler(hooks->comptable, "RSVP", handle_xml_rsvp_parameter);
	insert_xml_attr_handler(hooks->comptable, "SentBy", handle_xml_sent_by_parameter);
	*/
	
	//vcal attributes
	insert_xml_attr_handler(hooks->table, "CalendarScale", handle_xml_calscale_attribute);
	insert_xml_attr_handler(hooks->table, "ProductID", handle_xml_prodid_attribute);
	insert_xml_attr_handler(hooks->table, "Method", handle_xml_method_attribute);
//	insert_xml_attr_handler(hooks->table, "UnknownNode", xml_handle_unknown_attribute);
//	insert_xml_attr_handler(hooks->table, "UnknownParameter", xml_handle_unknown_parameter);
	
	//Timezone
	insert_xml_attr_handler(hooks->tztable, "TimezoneID", handle_xml_tzid_attribute);
	insert_xml_attr_handler(hooks->tztable, "Location", handle_xml_tz_location_attribute);
	insert_xml_attr_handler(hooks->tztable, "TZOffsetFrom", handle_xml_tzoffsetfrom_location_attribute);
	insert_xml_attr_handler(hooks->tztable, "TZOffsetTo", handle_xml_tzoffsetto_location_attribute);
	insert_xml_attr_handler(hooks->tztable, "TimezoneName", handle_xml_tzname_attribute);
	insert_xml_attr_handler(hooks->tztable, "DateStarted", handle_xml_tzdtstart_attribute);
	insert_xml_attr_handler(hooks->tztable, "RecurrenceRule", handle_xml_tzrrule_attribute);
	insert_xml_attr_handler(hooks->tztable, "LastModified", handle_xml_tz_last_modified_attribute);
	insert_xml_attr_handler(hooks->tztable, "TimezoneUrl", handle_xml_tzurl_attribute);
	insert_xml_attr_handler(hooks->tztable, "RecurrenceDate", handle_xml_tzrdate_attribute);

	/*FIXME: The functions below shouldn't be on tztable, but on other hash table */
//	insert_xml_attr_handler(hooks->tztable, "Category", handle_xml_category_parameter);
//	insert_xml_attr_handler(hooks->tztable, "Rule", handle_xml_rule_parameter);
/*	
	insert_xml_attr_handler(hooks->tztable, "Value", handle_xml_value_parameter);
	insert_xml_attr_handler(hooks->tztable, "AlternateRep", handle_xml_altrep_parameter);
	insert_xml_attr_handler(hooks->tztable, "CommonName", handle_xml_cn_parameter);
	insert_xml_attr_handler(hooks->tztable, "DelegatedFrom", handle_xml_delegated_from_parameter);
	insert_xml_attr_handler(hooks->tztable, "DelegatedTo", handle_xml_delegated_to_parameter);
	insert_xml_attr_handler(hooks->tztable, "Directory", handle_xml_dir_parameter);
	insert_xml_attr_handler(hooks->tztable, "FormaType", handle_xml_format_type_parameter);
	insert_xml_attr_handler(hooks->tztable, "FreeBusyType", handle_xml_fb_type_parameter);
	insert_xml_attr_handler(hooks->tztable, "Member", handle_xml_member_parameter);
	insert_xml_attr_handler(hooks->tztable, "PartStat", handle_xml_partstat_parameter);
	insert_xml_attr_handler(hooks->tztable, "Range", handle_xml_range_parameter);
	insert_xml_attr_handler(hooks->tztable, "Related", handle_xml_related_parameter);
	insert_xml_attr_handler(hooks->tztable, "RelationType", handle_xml_reltype_parameter);
	insert_xml_attr_handler(hooks->tztable, "Role", handle_xml_role_parameter);
	insert_xml_attr_handler(hooks->tztable, "RSVP", handle_xml_rsvp_parameter);
	insert_xml_attr_handler(hooks->tztable, "SentBy", handle_xml_sent_by_parameter);
*/	
	
	//VAlarm component
	insert_xml_attr_handler(hooks->alarmtable, "AlarmTrigger", handle_xml_atrigger_attribute);
	insert_xml_attr_handler(hooks->alarmtable, "AlarmRepeat", handle_xml_arepeat_attribute);
	insert_xml_attr_handler(hooks->alarmtable, "AlarmDuration", handle_xml_aduration_attribute);
	insert_xml_attr_handler(hooks->alarmtable, "AlarmAction", handle_xml_aaction_attribute);
	insert_xml_attr_handler(hooks->alarmtable, "AlarmAttach", handle_xml_aattach_attribute);
	insert_xml_attr_handler(hooks->alarmtable, "AlarmDescription", handle_xml_adescription_attribute);
	insert_xml_attr_handler(hooks->alarmtable, "AlarmAttendee", handle_xml_aattendee_attribute);
	insert_xml_attr_handler(hooks->alarmtable, "AlarmSummary", handle_xml_asummary_attribute);

	/*FIXME: The functions below shouldn't be on alarmtable, but on other hash table */
//	insert_xml_attr_handler(hooks->alarmtable, "Category", handle_xml_category_parameter);
	/*
	insert_xml_attr_handler(hooks->alarmtable, "Rule", handle_xml_rule_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "Value", handle_xml_value_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "AlternateRep", handle_xml_altrep_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "CommonName", handle_xml_cn_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "DelegatedFrom", handle_xml_delegated_from_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "DelegatedTo", handle_xml_delegated_to_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "Directory", handle_xml_dir_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "FormaType", handle_xml_format_type_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "FreeBusyType", handle_xml_fb_type_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "Member", handle_xml_member_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "PartStat", handle_xml_partstat_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "Range", handle_xml_range_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "Related", handle_xml_related_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "RelationType", handle_xml_reltype_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "Role", handle_xml_role_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "RSVP", handle_xml_rsvp_parameter);
	insert_xml_attr_handler(hooks->alarmtable, "SentBy", handle_xml_sent_by_parameter);
	*/
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static osync_bool conv_xmlformat_to_vcalendar(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error, int target)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, error);

	OSyncHookTables *hooks = init_xmlformat_to_ical();

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
	
	osync_trace(TRACE_INTERNAL, "parsing cml attributes");
	const char *std_encoding = NULL;
	if (target == VFORMAT_EVENT_10)
		std_encoding = "QUOTED-PRINTABLE";
	else
		std_encoding = "B";
	
	OSyncXMLField *xmlfield = osync_xmlformat_get_first_field(xmlformat);
	for(; xmlfield != NULL; xmlfield = osync_xmlfield_get_next(xmlfield)) {
		xml_vcal_handle_attribute(hooks, vcal, xmlfield, std_encoding);
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

static osync_bool conv_xmlformat_to_ical(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	return conv_xmlformat_to_vcalendar(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_EVENT_20);
}

static osync_bool conv_xmlformat_to_vcal(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	return conv_xmlformat_to_vcalendar(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_EVENT_10);
}

static void destroy_event(char *input, size_t inpsize)
{
	osync_xmlformat_unref((OSyncXMLFormat *)input);
}

static osync_bool copy_event(const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error)
{
	/* TODO: we can do that faster with a osync_xmlformat_copy() function */
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p)", __func__, input, inpsize, output, outpsize, error);
	OSyncXMLFormat *xmlformat = NULL;

	char *buffer = NULL;
	unsigned int size;
	
	osync_xmlformat_assemble((OSyncXMLFormat *) input, &buffer, &size);
	xmlformat = osync_xmlformat_parse(buffer, size, error);
	if (!xmlformat) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	*output = (char *) xmlformat;
	*outpsize = size;

	g_free(buffer);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

static void create_event(char **data, unsigned int *size)
{
	OSyncError *error = NULL;
	*data = (char *)osync_xmlformat_new("event", &error);
	if (!*data)
		osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

static char *print_event(const char *data, unsigned int size)
{
	char *buffer;
	unsigned int i;
	if(osync_xmlformat_assemble((OSyncXMLFormat *)data, &buffer, &i) == TRUE)
		return buffer;
	else
		return NULL;
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

static osync_bool marshal_xmlformat(const char *input, unsigned int inpsize, OSyncMessage *message, OSyncError **error)
{
	char *buffer;
	unsigned int size;
	
	if(!osync_xmlformat_assemble((OSyncXMLFormat *)input, &buffer, &size))
		return FALSE;

	osync_message_write_buffer(message, buffer, (int)size);
	
	return TRUE;
}

static osync_bool demarshal_xmlformat(OSyncMessage *message, char **output, unsigned int *outpsize, OSyncError **error)
{
	char *buffer = NULL;
	unsigned int size = 0;
	osync_message_read_buffer(message, (void **)&buffer, (int *)&size);
	
	OSyncXMLFormat *xmlformat = osync_xmlformat_parse((char *)buffer, size, error);
	if (!xmlformat) {
		osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	*output = (char*)xmlformat;
	*outpsize = 0; /* TODO: the compiler do not know the size of OSyncXMLFormat : 0 is ok? */
	return TRUE;
}

void get_format_info(OSyncFormatEnv *env)
{
	OSyncError *error = NULL;
	
	OSyncObjFormat *format = osync_objformat_new("xmlformat-event", "event", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format xmlformat: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	
	osync_objformat_set_compare_func(format, compare_event);
	osync_objformat_set_destroy_func(format, destroy_event);
	osync_objformat_set_print_func(format, print_event);
	osync_objformat_set_copy_func(format, copy_event);
	osync_objformat_set_create_func(format, create_event);
	
	osync_objformat_set_revision_func(format, get_revision);
	
	osync_objformat_must_marshal(format);
	osync_objformat_set_marshal_func(format, marshal_xmlformat);
	osync_objformat_set_demarshal_func(format, demarshal_xmlformat);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	
	
	format = osync_objformat_new("ical", "event", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format ical: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	
	
	/*
	format = osync_objformat_new("vcal", "event", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format vcal: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);	
	*/
}

void get_conversion_info(OSyncFormatEnv *env)
{
	OSyncFormatConverter *conv;
	OSyncError *error = NULL;
	
	OSyncObjFormat *xmlformat = osync_format_env_find_objformat(env, "xmlformat-event");
	OSyncObjFormat *ical = osync_format_env_find_objformat(env, "vevent20");
	OSyncObjFormat *vcal = osync_format_env_find_objformat(env, "vevent10");
	
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, ical, conv_xmlformat_to_ical, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, ical, xmlformat, conv_ical_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vcal, conv_xmlformat_to_vcal, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
//	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vcal, xmlformat, conv_vcal_to_xmlformat, &error);
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vcal, xmlformat, conv_ical_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
}

int get_version(void)
{
	return 1;
}
