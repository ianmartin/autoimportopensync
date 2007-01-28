/*
 * xmlformat-event - A plugin for parsing vevent objects for the opensync framework
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

#include "vformat.h"
#include "xmlformat.h"
#include "xmlformat-vcal.h"

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

static void handle_status_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_xmlfield_set_attr(xmlfield, "Type", "Status");
}


/***** Attributes *****/
static OSyncXMLField *handle_prodid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "ProductID", error);
}

static OSyncXMLField *handle_method_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Method", error);
}

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

/* TODO
static OSyncXMLField *handle_rrule_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling rrule attribute");
	OSyncXMLField *xmlfield = xmlNewChild(xmlfield, NULL, (xmlChar*)"RecurrenceRule", NULL);
	
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

/*
   static OSyncXMLField *handle_rrule_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	OSyncXMLField *xmlfield = xmlNewChild(xmlfield, NULL, (xmlChar*)"RecurrenceRule", NULL);
	osxml_node_add(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}*/

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

static OSyncXMLField *handle_dtend_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "DateEnd", error);
}

static OSyncXMLField *handle_transp_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Transparency", error);
}

static OSyncXMLField *handle_calscale_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "CalendarScale", error);
}

/* TODO Timezone
static OSyncXMLField *handle_tzid_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzid attribute");
	return xmlNewChild(xmlfield, NULL, (xmlChar*)"TimezoneID",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tz_location_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tz location attribute");
	return xmlNewChild(xmlfield, NULL, (xmlChar*)"Location",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzoffsetfrom_location_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzoffsetfrom attribute");
	return xmlNewChild(xmlfield, NULL, (xmlChar*)"TZOffsetFrom",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzoffsetto_location_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzoffsetto attribute");
	return xmlNewChild(xmlfield, NULL, (xmlChar*)"TZOffsetTo",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzname_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzname attribute");
	return xmlNewChild(xmlfield, NULL, (xmlChar*)"TimezoneName",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzdtstart_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewChild(xmlfield, NULL, (xmlChar*)"DateStarted",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzrrule_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzrrule attribute");
	OSyncXMLField *xmlfield = xmlNewChild(xmlfield, NULL, (xmlChar*)"RecurrenceRule", NULL);
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osxml_node_add(xmlfield, "Rule", retstr->str);
	}
	
	return xmlfield;
}

static OSyncXMLField *handle_tz_last_modified_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewChild(xmlfield, NULL, (xmlChar*)"LastModified",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzurl_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewChild(xmlfield, NULL, (xmlChar*)"TimezoneUrl",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static OSyncXMLField *handle_tzrdate_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewChild(xmlfield, NULL, (xmlChar*)"TimezoneDate",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}
*/

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
	
	//todo attributes
	insert_attr_handler(hooks->attributes, "BEGIN", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "END", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "UID", HANDLE_IGNORE);	
	insert_attr_handler(hooks->attributes, "DTSTAMP", handle_dtstamp_attribute);
	insert_attr_handler(hooks->attributes, "DESCRIPTION", handle_description_attribute);
	insert_attr_handler(hooks->attributes, "SUMMARY", handle_summary_attribute);
	insert_attr_handler(hooks->attributes, "DUE", handle_due_attribute);
	insert_attr_handler(hooks->attributes, "DTSTART", handle_dtstart_attribute);
	insert_attr_handler(hooks->attributes, "PERCENT-COMPLETE", handle_percent_complete_attribute);
	insert_attr_handler(hooks->attributes, "CLASS", handle_class_attribute);
	insert_attr_handler(hooks->attributes, "CATEGORIES", handle_categories_attribute);
	insert_attr_handler(hooks->attributes, "PRIORITY", handle_priority_attribute);
	insert_attr_handler(hooks->attributes, "UID", handle_uid_attribute);
	insert_attr_handler(hooks->attributes, "URL", handle_url_attribute);
	insert_attr_handler(hooks->attributes, "SEQUENCE", handle_sequence_attribute);
	insert_attr_handler(hooks->attributes, "LAST-MODIFIED", handle_last_modified_attribute);
	insert_attr_handler(hooks->attributes, "CREATED", handle_created_attribute);
	insert_attr_handler(hooks->attributes, "DCREATED", handle_created_attribute);
//	insert_attr_handler(hooks->attributes, "RRULE", handle_rrule_attribute);
	insert_attr_handler(hooks->attributes, "RDATE", handle_rdate_attribute);
	insert_attr_handler(hooks->attributes, "LOCATION", handle_location_attribute);
	insert_attr_handler(hooks->attributes, "GEO", handle_geo_attribute);
	insert_attr_handler(hooks->attributes, "COMPLETED", handle_completed_attribute);
	insert_attr_handler(hooks->attributes, "ORGANIZER", handle_organizer_attribute);
	insert_attr_handler(hooks->attributes, "ORGANIZER", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "X-ORGANIZER", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "RECURRENCE-ID", handle_recurid_attribute);
	insert_attr_handler(hooks->attributes, "STATUS", handle_status_attribute);
	insert_attr_handler(hooks->attributes, "DURATION", handle_duration_attribute);
	insert_attr_handler(hooks->attributes, "ATTACH", handle_attach_attribute);
	insert_attr_handler(hooks->attributes, "ATTENDEE", handle_attendee_attribute);
	insert_attr_handler(hooks->attributes, "COMMENT", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "CONTACT", handle_contact_attribute);
	insert_attr_handler(hooks->attributes, "EXDATE", handle_exdate_attribute);
	insert_attr_handler(hooks->attributes, "EXRULE", handle_exrule_attribute);
	insert_attr_handler(hooks->attributes, "RSTATUS", handle_rstatus_attribute);
	insert_attr_handler(hooks->attributes, "RELATED-TO", handle_related_attribute);
	insert_attr_handler(hooks->attributes, "RESOURCES", handle_resources_attribute);
	insert_attr_handler(hooks->attributes, "DTEND", handle_dtend_attribute);
	insert_attr_handler(hooks->attributes, "TRANSP", handle_transp_attribute);
	insert_attr_handler(hooks->attributes, "X-LIC-ERROR", HANDLE_IGNORE);
	
	insert_attr_handler(hooks->parameters, "TZID", handle_tzid_parameter);
	insert_attr_handler(hooks->parameters, "VALUE", handle_value_parameter);
	insert_attr_handler(hooks->parameters, "ALTREP", handle_altrep_parameter);
	insert_attr_handler(hooks->parameters, "CN", handle_cn_parameter);
	insert_attr_handler(hooks->parameters, "DELEGATED-FROM", handle_delegated_from_parameter);
	insert_attr_handler(hooks->parameters, "DELEGATED-TO", handle_delegated_to_parameter);
	insert_attr_handler(hooks->parameters, "DIR", handle_dir_parameter);
	insert_attr_handler(hooks->parameters, "FMTTYPE", handle_format_type_parameter);
	insert_attr_handler(hooks->parameters, "FBTYPE", handle_fb_type_parameter);
	insert_attr_handler(hooks->parameters, "MEMBER", handle_member_parameter);
	insert_attr_handler(hooks->parameters, "PARTSTAT", handle_partstat_parameter);
	insert_attr_handler(hooks->parameters, "RANGE", handle_range_parameter);
	insert_attr_handler(hooks->parameters, "RELATED", handle_related_parameter);
	insert_attr_handler(hooks->parameters, "RELTYPE", handle_reltype_parameter);
	insert_attr_handler(hooks->parameters, "ROLE", handle_role_parameter);
	insert_attr_handler(hooks->parameters, "RSVP", handle_rsvp_parameter);
	insert_attr_handler(hooks->parameters, "SENT-BY", handle_sent_by_parameter);
	insert_attr_handler(hooks->parameters, "X-LIC-ERROR", HANDLE_IGNORE);
	insert_attr_handler(hooks->parameters, "CHARSET", HANDLE_IGNORE);
	insert_attr_handler(hooks->parameters, "STATUS", handle_status_parameter);

	//vcal attributes
	insert_attr_handler(hooks->attributes, "PRODID", handle_prodid_attribute);
	insert_attr_handler(hooks->attributes, "PRODID", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "METHOD", handle_method_attribute);
	insert_attr_handler(hooks->attributes, "VERSION", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "ENCODING", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "CHARSET", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "BEGIN", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "END", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "CALSCALE", handle_calscale_attribute);
	insert_attr_handler(hooks->attributes, "X-LIC-ERROR", HANDLE_IGNORE);
	
	//Timezone
	/*
	insert_attr_handler(hooks->attributes, "TZID", handle_tzid_attribute);
	insert_attr_handler(hooks->attributes, "X-LIC-LOCATION", handle_tz_location_attribute);
	insert_attr_handler(hooks->attributes, "TZOFFSETFROM", handle_tzoffsetfrom_location_attribute);
	insert_attr_handler(hooks->attributes, "TZOFFSETTO", handle_tzoffsetto_location_attribute);
	insert_attr_handler(hooks->attributes, "TZNAME", handle_tzname_attribute);
	insert_attr_handler(hooks->attributes, "DTSTART", handle_tzdtstart_attribute);
	insert_attr_handler(hooks->attributes, "RRULE", handle_tzrrule_attribute);
	insert_attr_handler(hooks->attributes, "LAST-MODIFIED", handle_tz_last_modified_attribute);
	insert_attr_handler(hooks->attributes, "BEGIN", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "END", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "TZURL", handle_tzurl_attribute);
	insert_attr_handler(hooks->attributes, "COMMENT", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "RDATE", handle_tzrdate_attribute);
	*/

	//FIXME: The functions below shoudn't be on tztable, but on another hash table
	insert_attr_handler(hooks->parameters, "VALUE", handle_value_parameter);
	insert_attr_handler(hooks->parameters, "ALTREP", handle_altrep_parameter);
	insert_attr_handler(hooks->parameters, "CN", handle_cn_parameter);
	insert_attr_handler(hooks->parameters, "DELEGATED-FROM", handle_delegated_from_parameter);
	insert_attr_handler(hooks->parameters, "DELEGATED-TO", handle_delegated_to_parameter);
	insert_attr_handler(hooks->parameters, "DIR", handle_dir_parameter);
	insert_attr_handler(hooks->parameters, "FMTTYPE", handle_format_type_parameter);
	insert_attr_handler(hooks->parameters, "FBTYPE", handle_fb_type_parameter);
	insert_attr_handler(hooks->parameters, "MEMBER", handle_member_parameter);
	insert_attr_handler(hooks->parameters, "PARTSTAT", handle_partstat_parameter);
	insert_attr_handler(hooks->parameters, "RANGE", handle_range_parameter);
	insert_attr_handler(hooks->parameters, "RELATED", handle_related_parameter);
	insert_attr_handler(hooks->parameters, "RELTYPE", handle_reltype_parameter);
	insert_attr_handler(hooks->parameters, "ROLE", handle_role_parameter);
	insert_attr_handler(hooks->parameters, "RSVP", handle_rsvp_parameter);
	insert_attr_handler(hooks->parameters, "SENT-BY", handle_sent_by_parameter);
	insert_attr_handler(hooks->parameters, "X-LIC-ERROR", HANDLE_IGNORE);
	
	//VAlarm component
	if (target == VFORMAT_EVENT_20) {
		insert_attr_handler(hooks->attributes, "TRIGGER", handle_atrigger_attribute);
		insert_attr_handler(hooks->attributes, "REPEAT", handle_arepeat_attribute);
		insert_attr_handler(hooks->attributes, "DURATION", handle_aduration_attribute);
		insert_attr_handler(hooks->attributes, "ACTION", handle_aaction_attribute);
		insert_attr_handler(hooks->attributes, "ATTACH", handle_aattach_attribute);
		insert_attr_handler(hooks->attributes, "DESCRIPTION", handle_adescription_attribute);
		insert_attr_handler(hooks->attributes, "ATTENDEE", handle_aattendee_attribute);
		insert_attr_handler(hooks->attributes, "SUMMARY", handle_asummary_attribute);
	} else if (target == VFORMAT_EVENT_10) {
		insert_attr_handler(hooks->attributes, "AALARM", handle_aalarm_attribute);
		insert_attr_handler(hooks->attributes, "DALARM", handle_dalarm_attribute);
	}

	// FIXME: The functions below shoudn't be on alarmtable, but on another hash table
	insert_attr_handler(hooks->parameters, "TZID", handle_tzid_parameter);
	insert_attr_handler(hooks->parameters, "VALUE", handle_value_parameter);
	insert_attr_handler(hooks->parameters, "ALTREP", handle_altrep_parameter);
	insert_attr_handler(hooks->parameters, "CN", handle_cn_parameter);
	insert_attr_handler(hooks->parameters, "DELEGATED-FROM", handle_delegated_from_parameter);
	insert_attr_handler(hooks->parameters, "DELEGATED-TO", handle_delegated_to_parameter);
	insert_attr_handler(hooks->parameters, "DIR", handle_dir_parameter);
	insert_attr_handler(hooks->parameters, "FMTTYPE", handle_format_type_parameter);
	insert_attr_handler(hooks->parameters, "FBTYPE", handle_fb_type_parameter);
	insert_attr_handler(hooks->parameters, "MEMBER", handle_member_parameter);
	insert_attr_handler(hooks->parameters, "PARTSTAT", handle_partstat_parameter);
	insert_attr_handler(hooks->parameters, "RANGE", handle_range_parameter);
	insert_attr_handler(hooks->parameters, "RELATED", handle_related_parameter);
	insert_attr_handler(hooks->parameters, "RELTYPE", handle_reltype_parameter);
	insert_attr_handler(hooks->parameters, "ROLE", handle_role_parameter);
	insert_attr_handler(hooks->parameters, "RSVP", handle_rsvp_parameter);
	insert_attr_handler(hooks->parameters, "SENT-BY", handle_sent_by_parameter);
	insert_attr_handler(hooks->parameters, "X-LIC-ERROR", HANDLE_IGNORE);
	insert_attr_handler(hooks->parameters, "X-EVOLUTION-ALARM-UID", HANDLE_IGNORE);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}


static osync_bool conv_ical_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, error);
	
	OSyncHookTables *hooks = init_vcalendar_to_xmlformat(VFORMAT_EVENT_20); 

	
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
	osync_trace(TRACE_INTERNAL, "....Output XMLFormat is:\n%s", str);
	g_free(str);

	if (osync_xmlformat_validate(xmlformat) == FALSE)
		osync_trace(TRACE_INTERNAL, "XMLFORMAT EVENT: Not valid!");
	else
		osync_trace(TRACE_INTERNAL, "XMLFORMAT EVENT: VAILD");

	vformat_free(vcal);
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

static osync_bool conv_vcal_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
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


/*
static VFormatAttribute *xml_handle_unknown_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown xml attribute %s", xmlfield->name);
	char *name = osxml_find_node(xmlfield, "NodeName");
	VFormatAttribute *attr = vformat_attribute_new(NULL, name);
	g_free(name);
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}
*/

static VFormatAttribute *handle_xml_prodid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "PRODID", encoding);
}

static VFormatAttribute *handle_xml_method_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "METHOD", encoding);
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

static VFormatAttribute *handle_xml_dtstamp_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "DTSTAMP", encoding);
}

static VFormatAttribute *handle_xml_description_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "DESCRIPTION", encoding);
}

static VFormatAttribute *handle_xml_summary_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "SUMMARY", encoding);
}

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

static VFormatAttribute *handle_xml_percent_complete_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "PERCENT-COMPLETE", encoding);
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

static VFormatAttribute *handle_xml_completed_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "COMPLETED", encoding);
}

static VFormatAttribute *handle_xml_organizer_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "ORGANIZER", encoding);
}

static VFormatAttribute *handle_xml_recurid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RECURRENCE-ID", encoding);
}

static VFormatAttribute *handle_xml_status_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "STATUS", encoding);
}

static VFormatAttribute *handle_xml_duration_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "DURATION", encoding);
}

static VFormatAttribute *handle_xml_attach_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "ATTACH", encoding);
}

static VFormatAttribute *handle_xml_attendee_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "ATTENDEE", encoding);
}

static VFormatAttribute *handle_xml_event_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "CONTACT", encoding);
}

static VFormatAttribute *handle_xml_exdate_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "EXDATE", encoding);
}

static VFormatAttribute *handle_xml_exrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "EXRULE", encoding);
}

static VFormatAttribute *handle_xml_rstatus_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RSTATUS", encoding);
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

static VFormatAttribute *handle_xml_calscale_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "CALSCALE", encoding);
}

/*
static VFormatAttribute *handle_xml_tzid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZID");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tz_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-LIC-LOCATION");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzoffsetfrom_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZOFFSETFROM");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzoffsetto_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZOFFSETTO");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzname_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZNAME");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzdtstart_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTSTART");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzrrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RRULE");
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tz_last_modified_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LAST-MODIFIED");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzurl_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZURL");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzrdate_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RDATE");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}
*/

static VFormatAttribute *handle_xml_atrigger_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "TRIGGER", encoding);
}

static VFormatAttribute *handle_xml_arepeat_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "REPEAT");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aduration_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DURATION");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aaction_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ACTION");
	/* FIXME add_Value() #3 NULL is wrong */
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aattach_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTACH");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_adescription_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DESCRIPTION");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aattendee_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTENDEE");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

static VFormatAttribute *handle_xml_asummary_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "SUMMARY");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
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

	
	/*
	hooks->table = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->tztable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->comptable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->alarmtable = g_hash_table_new(g_str_hash, g_str_equal);
	*/

	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);

	//todo attributes
	insert_xml_attr_handler(hooks->attributes, "Uid", handle_xml_uid_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateCalendarCreated", handle_xml_dtstamp_attribute);
	insert_xml_attr_handler(hooks->attributes, "Description", handle_xml_description_attribute);
	insert_xml_attr_handler(hooks->attributes, "Summary", handle_xml_summary_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateDue", handle_xml_due_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateStarted", handle_xml_dtstart_attribute);
	insert_xml_attr_handler(hooks->attributes, "PercentComplete", handle_xml_percent_complete_attribute);
	insert_xml_attr_handler(hooks->attributes, "Class", handle_xml_class_attribute);
	insert_xml_attr_handler(hooks->attributes, "Categories", handle_xml_categories_attribute);
	insert_xml_attr_handler(hooks->attributes, "Priority", handle_xml_priority_attribute);
	insert_xml_attr_handler(hooks->attributes, "Url", handle_xml_url_attribute);
	insert_xml_attr_handler(hooks->attributes, "Sequence", handle_xml_sequence_attribute);
	insert_xml_attr_handler(hooks->attributes, "LastModified", handle_xml_last_modified_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateCreated", handle_xml_created_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceRule", handle_xml_rrule_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceDate", handle_xml_rdate_attribute);
	insert_xml_attr_handler(hooks->attributes, "Location", handle_xml_location_attribute);
	insert_xml_attr_handler(hooks->attributes, "Geo", handle_xml_geo_attribute);
	insert_xml_attr_handler(hooks->attributes, "Completed", handle_xml_completed_attribute);
	insert_xml_attr_handler(hooks->attributes, "Organizer", handle_xml_organizer_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceID", handle_xml_recurid_attribute);
	insert_xml_attr_handler(hooks->attributes, "Status", handle_xml_status_attribute);
	insert_xml_attr_handler(hooks->attributes, "Duration", handle_xml_duration_attribute);
	insert_xml_attr_handler(hooks->attributes, "Attach", handle_xml_attach_attribute);
	insert_xml_attr_handler(hooks->attributes, "Attendee", handle_xml_attendee_attribute);
	insert_xml_attr_handler(hooks->attributes, "Contact", handle_xml_event_attribute);
	insert_xml_attr_handler(hooks->attributes, "ExclusionDate", handle_xml_exdate_attribute);
	insert_xml_attr_handler(hooks->attributes, "ExclusionRule", handle_xml_exrule_attribute);
	insert_xml_attr_handler(hooks->attributes, "RStatus", handle_xml_rstatus_attribute);
	insert_xml_attr_handler(hooks->attributes, "Related", handle_xml_related_attribute);
	insert_xml_attr_handler(hooks->attributes, "Resources", handle_xml_resources_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateEnd", handle_xml_dtend_attribute);
	insert_xml_attr_handler(hooks->attributes, "Transparency", handle_xml_transp_attribute);


	/*
	insert_xml_attr_handler(hooks->parameters, "Category", handle_xml_category_parameter);
	insert_xml_attr_handler(hooks->parameters, "Rule", handle_xml_rule_parameter);
	insert_xml_attr_handler(hooks->parameters, "Value", handle_xml_value_parameter);
	insert_xml_attr_handler(hooks->parameters, "AlternateRep", handle_xml_altrep_parameter);
	insert_xml_attr_handler(hooks->parameters, "CommonName", handle_xml_cn_parameter);
	insert_xml_attr_handler(hooks->parameters, "DelegatedFrom", handle_xml_delegated_from_parameter);
	insert_xml_attr_handler(hooks->parameters, "DelegatedTo", handle_xml_delegated_to_parameter);
	insert_xml_attr_handler(hooks->parameters, "Directory", handle_xml_dir_parameter);
	insert_xml_attr_handler(hooks->parameters, "FormaType", handle_xml_format_type_parameter);
	insert_xml_attr_handler(hooks->parameters, "FreeBusyType", handle_xml_fb_type_parameter);
	insert_xml_attr_handler(hooks->parameters, "Member", handle_xml_member_parameter);
	insert_xml_attr_handler(hooks->parameters, "PartStat", handle_xml_partstat_parameter);
	insert_xml_attr_handler(hooks->parameters, "Range", handle_xml_range_parameter);
	insert_xml_attr_handler(hooks->parameters, "Related", handle_xml_related_parameter);
	insert_xml_attr_handler(hooks->parameters, "RelationType", handle_xml_reltype_parameter);
	insert_xml_attr_handler(hooks->parameters, "Role", handle_xml_role_parameter);
	insert_xml_attr_handler(hooks->parameters, "RSVP", handle_xml_rsvp_parameter);
	insert_xml_attr_handler(hooks->parameters, "SentBy", handle_xml_sent_by_parameter);
	*/
	
	//vcal attributes
	insert_xml_attr_handler(hooks->attributes, "CalendarScale", handle_xml_calscale_attribute);
	insert_xml_attr_handler(hooks->attributes, "ProductID", handle_xml_prodid_attribute);
	insert_xml_attr_handler(hooks->attributes, "Method", handle_xml_method_attribute);
//	insert_xml_attr_handler(hooks->attributes, "UnknownNode", xml_handle_unknown_attribute);
//	insert_xml_attr_handler(hooks->attributes, "UnknownParameter", xml_handle_unknown_parameter);
	
	/*
	//Timezone
	insert_xml_attr_handler(hooks->attributes, "TimezoneID", handle_xml_tzid_attribute);
	insert_xml_attr_handler(hooks->attributes, "Location", handle_xml_tz_location_attribute);
	insert_xml_attr_handler(hooks->attributes, "TZOffsetFrom", handle_xml_tzoffsetfrom_location_attribute);
	insert_xml_attr_handler(hooks->attributes, "TZOffsetTo", handle_xml_tzoffsetto_location_attribute);
	insert_xml_attr_handler(hooks->attributes, "TimezoneName", handle_xml_tzname_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateStarted", handle_xml_tzdtstart_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceRule", handle_xml_tzrrule_attribute);
	insert_xml_attr_handler(hooks->attributes, "LastModified", handle_xml_tz_last_modified_attribute);
	insert_xml_attr_handler(hooks->attributes, "TimezoneUrl", handle_xml_tzurl_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceDate", handle_xml_tzrdate_attribute);
	*/

	/*
//	insert_xml_attr_handler(hooks->parameters, "Category", handle_xml_category_parameter);
//	insert_xml_attr_handler(hooks->parameters, "Rule", handle_xml_rule_parameter);
	insert_xml_attr_handler(hooks->parameters, "Value", handle_xml_value_parameter);
	insert_xml_attr_handler(hooks->parameters, "AlternateRep", handle_xml_altrep_parameter);
	insert_xml_attr_handler(hooks->parameters, "CommonName", handle_xml_cn_parameter);
	insert_xml_attr_handler(hooks->parameters, "DelegatedFrom", handle_xml_delegated_from_parameter);
	insert_xml_attr_handler(hooks->parameters, "DelegatedTo", handle_xml_delegated_to_parameter);
	insert_xml_attr_handler(hooks->parameters, "Directory", handle_xml_dir_parameter);
	insert_xml_attr_handler(hooks->parameters, "FormaType", handle_xml_format_type_parameter);
	insert_xml_attr_handler(hooks->parameters, "FreeBusyType", handle_xml_fb_type_parameter);
	insert_xml_attr_handler(hooks->parameters, "Member", handle_xml_member_parameter);
	insert_xml_attr_handler(hooks->parameters, "PartStat", handle_xml_partstat_parameter);
	insert_xml_attr_handler(hooks->parameters, "Range", handle_xml_range_parameter);
	insert_xml_attr_handler(hooks->parameters, "Related", handle_xml_related_parameter);
	insert_xml_attr_handler(hooks->parameters, "RelationType", handle_xml_reltype_parameter);
	insert_xml_attr_handler(hooks->parameters, "Role", handle_xml_role_parameter);
	insert_xml_attr_handler(hooks->parameters, "RSVP", handle_xml_rsvp_parameter);
	insert_xml_attr_handler(hooks->parameters, "SentBy", handle_xml_sent_by_parameter);
	*/
	
	//VAlarm component
	insert_xml_attr_handler(hooks->attributes, "AlarmTrigger", handle_xml_atrigger_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmRepeat", handle_xml_arepeat_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmDuration", handle_xml_aduration_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmAction", handle_xml_aaction_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmAttach", handle_xml_aattach_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmDescription", handle_xml_adescription_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmAttendee", handle_xml_aattendee_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmSummary", handle_xml_asummary_attribute);

	/*
	//FIXME: The functions below shouldn't be on alarmtable, but on other hash table
//	insert_xml_attr_handler(hooks->parameters, "Category", handle_xml_category_parameter);
	insert_xml_attr_handler(hooks->parameters, "Rule", handle_xml_rule_parameter);
	insert_xml_attr_handler(hooks->parameters, "Value", handle_xml_value_parameter);
	insert_xml_attr_handler(hooks->parameters, "AlternateRep", handle_xml_altrep_parameter);
	insert_xml_attr_handler(hooks->parameters, "CommonName", handle_xml_cn_parameter);
	insert_xml_attr_handler(hooks->parameters, "DelegatedFrom", handle_xml_delegated_from_parameter);
	insert_xml_attr_handler(hooks->parameters, "DelegatedTo", handle_xml_delegated_to_parameter);
	insert_xml_attr_handler(hooks->parameters, "Directory", handle_xml_dir_parameter);
	insert_xml_attr_handler(hooks->parameters, "FormaType", handle_xml_format_type_parameter);
	insert_xml_attr_handler(hooks->parameters, "FreeBusyType", handle_xml_fb_type_parameter);
	insert_xml_attr_handler(hooks->parameters, "Member", handle_xml_member_parameter);
	insert_xml_attr_handler(hooks->parameters, "PartStat", handle_xml_partstat_parameter);
	insert_xml_attr_handler(hooks->parameters, "Range", handle_xml_range_parameter);
	insert_xml_attr_handler(hooks->parameters, "Related", handle_xml_related_parameter);
	insert_xml_attr_handler(hooks->parameters, "RelationType", handle_xml_reltype_parameter);
	insert_xml_attr_handler(hooks->parameters, "Role", handle_xml_role_parameter);
	insert_xml_attr_handler(hooks->parameters, "RSVP", handle_xml_rsvp_parameter);
	insert_xml_attr_handler(hooks->parameters, "SentBy", handle_xml_sent_by_parameter);
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

static osync_bool conv_xmlformat_to_ical(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	return conv_xmlformat_to_vcalendar(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_EVENT_20);
}

static osync_bool conv_xmlformat_to_vcal(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	return conv_xmlformat_to_vcalendar(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_EVENT_10);
}

static void create_event(char **data, unsigned int *size)
{
	OSyncError *error = NULL;
	*data = (char *)osync_xmlformat_new("event", &error);
	if (!*data)
		osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(&error));
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
	
	OSyncObjFormat *format = osync_objformat_new("xmlformat-event", "event", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format xmlformat: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	
	osync_objformat_set_compare_func(format, compare_event);
	osync_objformat_set_destroy_func(format, destroy_xmlformat);
	osync_objformat_set_print_func(format, print_xmlformat);
	osync_objformat_set_copy_func(format, copy_xmlformat);
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
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vcal, xmlformat, conv_vcal_to_xmlformat, &error);
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
