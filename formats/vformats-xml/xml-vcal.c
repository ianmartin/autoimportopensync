/*
 * xml-vcard - A plugin for parsing vcard objects for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
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
 
#include "xml-support.h"
#include "vformat.h"
#include "vcalical.h"
#include "xml-vcal.h"
#include <glib.h>

/*TODO: replace all g_hash_table_insert by functions with well-defined-type parameters
 * (like insert_xml_attr_handler() and insert_attr_handler()) */

/*TODO: Refactor this and other xml-*.c code, to put all common code in the same place */

typedef xmlNode *(* vattr_handler_t)(xmlNode *, VFormatAttribute *);
typedef VFormatAttribute *(* xml_attr_handler_t)(VFormat *vcard, xmlNode *root, const char *encoding);

static void handle_unknown_parameter(xmlNode *current, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown parameter %s", vformat_attribute_param_get_name(param));
	xmlNode *property = xmlNewTextChild(current, NULL, (xmlChar*)"UnknownParam",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
	osxml_node_add(property, "ParamName", vformat_attribute_param_get_name(param));
}

static void handle_tzid_parameter(xmlNode *current, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling tzid parameter");
	xmlNewTextChild(current, NULL, (xmlChar*)"TimezoneID",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_value_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"Value",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_altrep_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"AlternateRep",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_cn_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"CommonName",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_delegated_from_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"DelegatedFrom",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_delegated_to_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"DelegatedTo",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_dir_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"Directory",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_format_type_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"FormaType",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_fb_type_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"FreeBusyType",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_member_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"Member",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_partstat_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"PartStat",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_range_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"Range",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_related_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"Related",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_reltype_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"RelationType",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_role_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"Role",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_rsvp_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"RSVP",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_sent_by_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"SentBy",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static void handle_status_parameter(xmlNode *current, VFormatParam *param)
{
	xmlNewTextChild(current, NULL, (xmlChar*)"Status",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static xmlNode *handle_unknown_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown attribute %s", vformat_attribute_get_name(attr));
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"UnknownNode", NULL);
	osxml_node_add(current, "NodeName", vformat_attribute_get_name(attr));
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osxml_node_add(current, "Content", retstr->str);
	}
	return current;
}

static xmlNode *handle_prodid_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling prodid attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"ProductID", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_method_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling method attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Method", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_dtstamp_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling dtstamp attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"DateCalendarCreated", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_percent_complete_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling percent complete attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"PercentComplete", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_created_attribute(xmlNode *root, VFormatAttribute *attr)
{
	const char *tmp;
	char *timestamp;
	osync_trace(TRACE_INTERNAL, "Handling created attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"DateCreated", NULL);
	tmp = vformat_attribute_get_nth_value(attr, 0);
	timestamp = osync_time_timestamp(tmp);
	osxml_node_add(current, "Content", timestamp);
	g_free(timestamp);
	return current;
}

static xmlNode *handle_dtstart_attribute(xmlNode *root, VFormatAttribute *attr)
{
	const char *tmp;
	char *timestamp;
	osync_trace(TRACE_INTERNAL, "Handling dtstart attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"DateStarted", NULL);
	tmp = vformat_attribute_get_nth_value(attr, 0);
	timestamp = osync_time_timestamp(tmp);
	osxml_node_add(current, "Content", timestamp);
	g_free(timestamp);
	return current;
}

static xmlNode *handle_rrule_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling rrule attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"RecurrenceRule", NULL);

	GList *values = NULL; 
	osync_bool interval_isset = FALSE;
	GString *retstr = NULL;

	values = vformat_attribute_get_values_decoded(attr);

	for (; values; values = values->next) {
		retstr = (GString *)values->data;
		g_assert(retstr);
		osxml_node_add(current, "Rule", retstr->str);
		if (strstr(retstr->str, "INTERVAL"))
			interval_isset = TRUE;
	}

	if (!interval_isset)
		osxml_node_add(current, "Rule", "INTERVAL=1");

	return current;
}

static xmlNode *handle_vcal_rrule_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling rrule attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"RecurrenceRule", NULL);

	GList *values = NULL; 

	const char *rrule = vformat_attribute_get_nth_value(attr, 0);
	values = conv_vcal2ical_rrule(rrule);

	for (; values; values = values->next) {
		osxml_node_add(current, "Rule", (char *) values->data);
	}

	g_list_free(values);

	return current;
}

static xmlNode *handle_aalarm_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling aalarm attribute");

	time_t started, alarm;
	char *dtstarted = NULL, *duration = NULL;
	xmlNode *dtstartNode = NULL, *sub = NULL;
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Alarm", NULL);

	osxml_node_add(current, "AlarmAction", "AUDIO");
	osxml_node_add(current, "AlarmDescription", vformat_attribute_get_nth_value(attr, 1));

	/*
	osxml_node_add(current, "AlarmDuration", vformat_attribute_get_nth_value(attr, 1));
	osxml_node_add(current, "AlarmRepeat", vformat_attribute_get_nth_value(attr, 2));
	*/

	sub = xmlNewTextChild(current, NULL, (xmlChar*) "AlarmTrigger", NULL);

	// get timestamp of DateStarted or DateDue (for todos)
	if ((dtstartNode = osxml_get_node(root, "DateDue"))) {
		dtstarted = osxml_find_node(dtstartNode, "Content");
	} else if (( dtstartNode = osxml_get_node(root, "DateStarted"))) {
		dtstarted = osxml_find_node(dtstartNode, "Content");
	}

	/* TODO: This breaks the case if a localtime stamp + tzid
	   get synced to a vcal. This means that the alarm duration
	   _CAN_ be wrong when dtstarted is localtime and alarm is not localtime.

	   FIXME */  

	if (dtstarted) {
		started = osync_time_vtime2unix(dtstarted, 0);
		g_free(dtstarted);

		alarm = osync_time_vtime2unix(vformat_attribute_get_nth_value(attr, 0), 0);

		// convert offset in seconds to alarm duration
		duration = osync_time_sec2alarmdu(alarm - started);
		osxml_node_add(sub, "Content", duration);
		osxml_node_add(sub, "Value", "DURATION");
		osxml_node_add(sub, "Related", "START");
		g_free(duration);


	/* This happens only if a todo has a AALARM without any DateDue and DateStarted field.
    	   (This was found on a old SE mobile phone and is illegal.) */ 
	} else {
		osxml_node_add(sub, "Content", vformat_attribute_get_nth_value(attr, 0));
		osxml_node_add(sub, "Value", "DATE-TIME");
	}

	return current;
}

static xmlNode *handle_dalarm_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling dalarm attribute");

	time_t started, alarm;
	char *dtstarted = NULL, *duration = NULL;
	xmlNode *dtstartNode = NULL, *sub = NULL;
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Alarm", NULL);

	osxml_node_add(current, "AlarmDescription", vformat_attribute_get_nth_value(attr, 1));
	/*
	osxml_node_add(current, "AlarmDuration", vformat_attribute_get_nth_value(attr, 1));
	osxml_node_add(current, "AlarmRepeat", vformat_attribute_get_nth_value(attr, 2));
	*/

	osxml_node_add(current, "AlarmAction", "DISPLAY");

	sub = xmlNewTextChild(current, NULL, (xmlChar*) "AlarmTrigger", NULL);

	// get timestamp of DateStarted or DateDue (for todos)
	if ((dtstartNode = osxml_get_node(root, "DateDue"))) {
		dtstarted = osxml_find_node(dtstartNode, "Content");
	} else if (( dtstartNode = osxml_get_node(root, "DateStarted"))) {
		dtstarted = osxml_find_node(dtstartNode, "Content");
	}

	/* TODO: This breaks the case if a localtime stamp + tzid
	   get synced to a vcal. This means that the alarm duration
	   _CAN_ be wrong when dtstarted is localtime and alarm is not localtime.

	   FIXME */  

	if (dtstarted) {
		started = osync_time_vtime2unix(dtstarted, 0);
		g_free(dtstarted);

		alarm = osync_time_vtime2unix(vformat_attribute_get_nth_value(attr, 0), 0);

		// convert offset in seconds to alarm duration
		duration = osync_time_sec2alarmdu(alarm - started);
		osxml_node_add(sub, "Content", duration);
		osxml_node_add(sub, "Value", "DURATION");
		osxml_node_add(sub, "Related", "START");
		g_free(duration);


	/* This happens only if a todo has a AALARM without any DateDue and DateStarted field.
    	   (This was found on a old SE mobile phone and is illegal.) */ 
	} else {
		osxml_node_add(sub, "Content", vformat_attribute_get_nth_value(attr, 0));
		osxml_node_add(sub, "Value", "DATE-TIME");
	}


	return current;
}

static xmlNode *handle_description_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling description attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Description", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_summary_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling summary attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Summary", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_categories_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Categories attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Categories", NULL);
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osxml_node_add(current, "Category", retstr->str);
	}
	
	return current;
}

static xmlNode *handle_class_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Class attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Class", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_due_attribute(xmlNode *root, VFormatAttribute *attr)
{
	const char *tmp;
	char *timestamp = NULL;
	osync_trace(TRACE_INTERNAL, "Handling due attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"DateDue", NULL);
	tmp = vformat_attribute_get_nth_value(attr, 0);
	timestamp = osync_time_timestamp(tmp);
	osxml_node_add(current, "Content", timestamp);
	g_free(timestamp);
	return current;
}

static xmlNode *handle_url_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Url attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Url", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_priority_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling priority attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Priority", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_sequence_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling sequence attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Sequence", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_last_modified_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"LastModified", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_rdate_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"RecurrenceDate", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_location_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Location", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_geo_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling geo attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Geo", NULL);
	osxml_node_add(current, "Latitude", vformat_attribute_get_nth_value(attr, 0));
	osxml_node_add(current, "Longitude", vformat_attribute_get_nth_value(attr, 1));
	return current;
}

static xmlNode *handle_completed_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Completed", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_organizer_attribute(xmlNode *root, VFormatAttribute *attr)
{
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Organizer", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_recurid_attribute(xmlNode *root, VFormatAttribute *attr)
{
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"RecurrenceID", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_status_attribute(xmlNode *root, VFormatAttribute *attr)
{
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Status", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_duration_attribute(xmlNode *root, VFormatAttribute *attr)
{
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Duration", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_attach_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Attach", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_attendee_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Attendee", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_contact_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Contact", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_exdate_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = NULL;
	char *datestamp = NULL;
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		current = xmlNewTextChild(root, NULL, (xmlChar*)"ExclusionDate", NULL);
		if (!osync_time_isdate(retstr->str)) {
			datestamp = osync_time_datestamp(retstr->str);
		} else {
			datestamp = g_strdup(retstr->str);
		}
		osxml_node_add(current, "Content", datestamp);

		if (!osync_time_isdate(retstr->str))
			osxml_node_add(current, "Value", "DATE");

		g_free(datestamp);
		g_string_free(retstr, TRUE);
	}

	return current;
}

static xmlNode *handle_exrule_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"ExclusionRule", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_rstatus_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"RStatus", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_related_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Related", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_resources_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Resources", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_dtend_attribute(xmlNode *root, VFormatAttribute *attr)
{
	char *timestamp;
	const char *tmp;
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"DateEnd", NULL);
	tmp = vformat_attribute_get_nth_value(attr, 0);
	timestamp = osync_time_timestamp(tmp);
	osxml_node_add(current, "Content", timestamp);
	g_free(timestamp);
	return current;
}

static xmlNode *handle_transp_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Transparency", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_calscale_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"CalendarScale", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_tzid_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzid attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"TimezoneID",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_tz_location_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tz location attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"Location",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_tzoffsetfrom_location_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzoffsetfrom attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"TZOffsetFrom",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_tzoffsetto_location_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzoffsetto attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"TZOffsetTo",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_tzname_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzname attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"TimezoneName",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_tzdtstart_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"DateStarted",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_tzrrule_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzrrule attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"RecurrenceRule", NULL);
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osxml_node_add(current, "Rule", retstr->str);
	}
	
	return current;
}

static xmlNode *handle_tz_last_modified_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"LastModified",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_tzurl_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"TimezoneUrl",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_tzrdate_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"TimezoneDate",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_atrigger_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"AlarmTrigger", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_arepeat_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"AlarmRepeat",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_aduration_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"AlarmDuration",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_aaction_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"AlarmAction",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_aattach_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"AlarmAttach",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_adescription_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"AlarmDescription",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_aattendee_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"AlarmAttendee",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_asummary_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzdtstart attribute");
	return xmlNewTextChild(root, NULL, (xmlChar*)"AlarmSummary",
		(xmlChar*)vformat_attribute_get_nth_value(attr, 0));
}

static void vcard_handle_parameter(GHashTable *hooks, xmlNode *current, VFormatParam *param)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, hooks, current, param);
	
	//Find the handler for this parameter
	void (* param_handler)(xmlNode *, VFormatParam *);
	char *paramname = g_strdup_printf("%s=%s", vformat_attribute_param_get_name(param), vformat_attribute_param_get_nth_value(param, 0));
	param_handler = g_hash_table_lookup(hooks, paramname);
	g_free(paramname);
	if (!param_handler)
		param_handler = g_hash_table_lookup(hooks, vformat_attribute_param_get_name(param));
	
	if (param_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	
	if (param_handler)
		param_handler(current, param);
	else
		handle_unknown_parameter(current, param);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void vcal_handle_attribute(GHashTable *table, GHashTable *paramtable, xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, table, root, attr, attr ? vformat_attribute_get_name(attr) : "None");
	xmlNode *current = NULL;
	
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
	vattr_handler_t attr_handler = g_hash_table_lookup(table, vformat_attribute_get_name(attr));
	osync_trace(TRACE_INTERNAL, "Hook is: %p", attr_handler);
	if (attr_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	if (attr_handler)
		current = attr_handler(root, attr);
	else
		current = handle_unknown_attribute(root, attr);

	//Handle all parameters of this attribute
	GList *params = vformat_attribute_get_params(attr);
	GList *p = NULL;
	for (p = params; p; p = p->next) {
		VFormatParam *param = p->data;
		vcard_handle_parameter(paramtable, current, param);
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void vcal_parse_attributes(OSyncHooksTable *hooks, GHashTable *table, GHashTable *paramtable, GList **attributes, xmlNode *root)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, attributes, root);
	
	GList *a = NULL;
	for (a = *attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;
		
		if (!strcmp(vformat_attribute_get_name(attr), "BEGIN")) {
			//Handling supcomponent
			a = a->next;
			if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "VTIMEZONE")) {
				xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Timezone", NULL);
				vcal_parse_attributes(hooks, hooks->tztable, hooks->tztable, &a, current);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "DAYLIGHT")) {
				xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"DaylightSavings", NULL);
				vcal_parse_attributes(hooks, hooks->tztable, hooks->tztable, &a, current);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "STANDARD")) {
				xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Standard", NULL);
				vcal_parse_attributes(hooks, hooks->tztable, hooks->tztable, &a, current);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "VTODO")) {
				xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Todo", NULL);
				vcal_parse_attributes(hooks, hooks->comptable, hooks->compparamtable, &a, current);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "VEVENT")) {
				xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Event", NULL);
				vcal_parse_attributes(hooks, hooks->comptable, hooks->compparamtable, &a, current);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "VJOURNAL")) {
				xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Journal", NULL);
				vcal_parse_attributes(hooks, hooks->comptable, hooks->compparamtable, &a, current);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "VALARM")) {
				xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Alarm", NULL);
				vcal_parse_attributes(hooks, hooks->alarmtable, hooks->alarmtable, &a, current);
			}
		} else if (!strcmp(vformat_attribute_get_name(attr), "END")) {
			osync_trace(TRACE_EXIT, "%s: Found END", __func__);
			*attributes = a;
			return;
		} else
			vcal_handle_attribute(table, paramtable, root, attr);
	}
	osync_trace(TRACE_EXIT, "%s: Done", __func__);
}

static osync_bool conv_vcal_to_xml(void *conv_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, conv_data, input, inpsize, output, outpsize, free_input, error);
	
	OSyncHooksTable *hooks = (OSyncHooksTable *)conv_data;
	
	osync_trace(TRACE_SENSITIVE, "Input vcal is:\n%s", input);
	
	/* The input is not null-terminated, but vformat_new_from_string() expects a null-terminated string */
	char *input_str = g_malloc(inpsize + 1);
	memcpy(input_str, input, inpsize);
	input_str[inpsize] = '\0';

	//Parse the vcard
	VFormat *vcal = vformat_new_from_string(input_str);
	g_free(input_str);
	
	osync_trace(TRACE_INTERNAL, "Creating xml doc");
	
	//Create a new xml document
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "vcal");
	
	osync_trace(TRACE_INTERNAL, "parsing attributes");
	
	//For every attribute we have call the handling hook
	GList *attributes = vformat_get_attributes(vcal);
	vcal_parse_attributes(hooks, hooks->table, hooks->table, &attributes, root);
	
	xmlChar *str = osxml_write_to_string(doc);
	osync_trace(TRACE_SENSITIVE, "Output XML is:\n%s", str);
	xmlFree(str);
	
	*free_input = TRUE;
	*output = (char *)doc;
	*outpsize = sizeof(doc);
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
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

static void add_value(VFormatAttribute *attr, xmlNode *parent, const char *name, const char *encoding)
{
	char *tmp = NULL;
	if (!name)
		tmp = (char*)xmlNodeGetContent(parent);
	else
		tmp = osxml_find_node(parent, name);
	
	if (!tmp)
		return;
	
	if (needs_charset((unsigned char*)tmp))
		if (!vformat_attribute_has_param (attr, "CHARSET"))
			vformat_attribute_add_param_with_value(attr, "CHARSET", "UTF-8");
	
	if (encoding && needs_encoding((unsigned char*)tmp, encoding)) {
		if (!vformat_attribute_has_param (attr, "ENCODING"))
			vformat_attribute_add_param_with_value(attr, "ENCODING", encoding);
		vformat_attribute_add_value_decoded(attr, tmp, strlen(tmp) + 1);
	} else
		vformat_attribute_add_value(attr, tmp);
	g_free(tmp);
}

static void xml_handle_unknown_parameter(VFormatAttribute *attr, xmlNode *current)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown xml parameter %s", current->name);
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, (char*)current->name, content);
	g_free(content);
}

static void handle_xml_category_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_value(attr, content);
	g_free(content);
}

static void handle_xml_rule_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_value(attr, content);
	g_free(content);
}

static void handle_xml_value_parameter(VFormatAttribute *attr, xmlNode *current)
{
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, "VALUE", content);
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

static VFormatAttribute *xml_handle_unknown_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown xml attribute %s", root->name);
	char *name = osxml_find_node(root, "NodeName");
	VFormatAttribute *attr = vformat_attribute_new(NULL, name);
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_prodid_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling prodid xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "PRODID");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_method_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling method xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "METHOD");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_geo_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling location xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "GEO");
	add_value(attr, root, "Latitude", encoding);
	add_value(attr, root, "Longitude", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_url_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling url xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "URL");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_uid_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling uid xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "UID");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_class_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling class xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CLASS");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_categories_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling categories xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CATEGORIES");
	vformat_add_attribute(vcard, attr);
	return attr;
}

static void xml_vcard_handle_parameter(GHashTable *table, VFormatAttribute *attr, xmlNode *current)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, table, attr, current, current ? (char *)current->name : "None");
	
	//Find the handler for this parameter
	void (* xml_param_handler)(VFormatAttribute *attr, xmlNode *);
	char *content = (char*)xmlNodeGetContent(current);
	char *paramname = g_strdup_printf("%s=%s", current->name, content);
	g_free(content);
	xml_param_handler = g_hash_table_lookup(table, paramname);
	g_free(paramname);
	if (!xml_param_handler)
		xml_param_handler = g_hash_table_lookup(table, current->name);
	
	if (xml_param_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	
	if (xml_param_handler)
		xml_param_handler(attr, current);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void xml_vcal_handle_attribute(GHashTable *table, VFormat *vcard, xmlNode *root)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, table, vcard, root, root ? (char *)root->name : "None");
	VFormatAttribute *attr = NULL;
	
	//We need to find the handler for this attribute
	xml_attr_handler_t xml_attr_handler = g_hash_table_lookup(table, root->name);
	osync_trace(TRACE_INTERNAL, "xml hook is: %p", xml_attr_handler);
	if (xml_attr_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	if (xml_attr_handler)
		/*FIXME: What the encoding parameter is supposed to be. Is it necessary? */
		attr = xml_attr_handler(vcard, root, NULL);
	else {
		osync_trace(TRACE_EXIT, "%s: Ignored2", __func__);
		return;
	}
	
	//Handle all parameters of this attribute
	xmlNode *child = root->xmlChildrenNode;
	while (child) {
		xml_vcard_handle_parameter(table, attr, child);
		child = child->next;
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static OSyncConvCmpResult compare_vevent(OSyncChange *leftchange, OSyncChange *rightchange)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftchange, rightchange);
	
	OSyncXMLScore score[] =
	{
	{10, "/vcal/Event/StartTime"},
	{10, "/vcal/Event/EndTime"},
	{100, "/vcal/Event/Summary"},
	{0, "/vcal/Event/Uid"},
	{0, "/vcal/Event/Revision"},
	{0, "/vcal/Event/DateCalendarCreated"},
	{0, "/vcal/Event/DateCreated"},
	{0, "/vcal/Event/LastModified"},
	{0, "/vcal/Event/Sequence"},
	{0, "/vcal/Event/Class[Content = \"PUBLIC\"]"},
	{0, "/vcal/Event/Priority"},
	{0, "/vcal/Event/Transparency[Content = \"OPAQUE\"]"},
	{0, "/vcal/Method"},
//	{0, "/vcal/Timezone"},
	{0, NULL}
	};
	
	OSyncConvCmpResult ret = osxml_compare((xmlDoc*)osync_change_get_data(leftchange), (xmlDoc*)osync_change_get_data(rightchange), score, 0, 99);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static OSyncConvCmpResult compare_vtodo(OSyncChange *leftchange, OSyncChange *rightchange)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftchange, rightchange);
	
	OSyncXMLScore score[] =
	{
	{100, "/vcal/Todo/Summary"},
	{0, "/vcal/Todo/Uid"},
	{0, "/vcal/Todo/Revision"},
	{0, "/vcal/Todo/DateCalendarCreated"},
	{0, "/vcal/Todo/DateCreated"},
	{0, "/vcal/Todo/LastModified"},
	{0, "/vcal/Todo/Sequence"},
	{0, "/vcal/Todo/Class[Content = \"PUBLIC\"]"},
	{0, "/vcal/Todo/Priority"},
	// ignore 'PercentComplete', because we aren't able to handle this at the moment
	{0, "/vcal/Todo/PercentComplete"},
	{0, "/vcal/Method"},
//	{0, "/vcal/Timezone"},
	{0, NULL}
	};
	
	OSyncConvCmpResult ret = osxml_compare((xmlDoc*)osync_change_get_data(leftchange), (xmlDoc*)osync_change_get_data(rightchange), score, 0, 99);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static char *print_vcal(OSyncChange *change)
{
	xmlDoc *doc = (xmlDoc *)osync_change_get_data(change);
	
	return (char *)osxml_write_to_string(doc);
}

static void destroy_xml(char *data, size_t size)
{
	xmlFreeDoc((xmlDoc *)data);
}

static void insert_attr_handler(GHashTable *table, const char *attrname, vattr_handler_t handler)
{
	g_hash_table_insert(table, (gpointer)attrname, handler);
}

static void *init_ical_to_xml(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	OSyncHooksTable *hooks = g_malloc0(sizeof(OSyncHooksTable));
	
	hooks->table = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->tztable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->comptable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->compparamtable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->alarmtable = g_hash_table_new(g_str_hash, g_str_equal);
	
	//todo attributes
	insert_attr_handler(hooks->comptable, "BEGIN", HANDLE_IGNORE);
	insert_attr_handler(hooks->comptable, "END", HANDLE_IGNORE);
	insert_attr_handler(hooks->comptable, "UID", HANDLE_IGNORE);	
	insert_attr_handler(hooks->comptable, "X-IRMC-LUID", HANDLE_IGNORE);
	insert_attr_handler(hooks->comptable, "X-SONYERICSSON-DST", HANDLE_IGNORE);
	insert_attr_handler(hooks->comptable, "DTSTAMP", handle_dtstamp_attribute);
	insert_attr_handler(hooks->comptable, "DESCRIPTION", handle_description_attribute);
	insert_attr_handler(hooks->comptable, "SUMMARY", handle_summary_attribute);
	insert_attr_handler(hooks->comptable, "DUE", handle_due_attribute);
	insert_attr_handler(hooks->comptable, "DTSTART", handle_dtstart_attribute);
	insert_attr_handler(hooks->comptable, "PERCENT-COMPLETE", handle_percent_complete_attribute);
	insert_attr_handler(hooks->comptable, "CLASS", handle_class_attribute);
	insert_attr_handler(hooks->comptable, "CATEGORIES", handle_categories_attribute);
	insert_attr_handler(hooks->comptable, "PRIORITY", handle_priority_attribute);
	insert_attr_handler(hooks->comptable, "URL", handle_url_attribute);
	insert_attr_handler(hooks->comptable, "SEQUENCE", handle_sequence_attribute);
	insert_attr_handler(hooks->comptable, "LAST-MODIFIED", handle_last_modified_attribute);
	insert_attr_handler(hooks->comptable, "CREATED", handle_created_attribute);
	insert_attr_handler(hooks->comptable, "DCREATED", handle_created_attribute);
	insert_attr_handler(hooks->comptable, "RRULE", handle_rrule_attribute);

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
	
	g_hash_table_insert(hooks->compparamtable, "TZID", handle_tzid_parameter);
	g_hash_table_insert(hooks->compparamtable, "VALUE", handle_value_parameter);
	g_hash_table_insert(hooks->compparamtable, "ALTREP", handle_altrep_parameter);
	g_hash_table_insert(hooks->compparamtable, "CN", handle_cn_parameter);
	g_hash_table_insert(hooks->compparamtable, "DELEGATED-FROM", handle_delegated_from_parameter);
	g_hash_table_insert(hooks->compparamtable, "DELEGATED-TO", handle_delegated_to_parameter);
	g_hash_table_insert(hooks->compparamtable, "DIR", handle_dir_parameter);
	g_hash_table_insert(hooks->compparamtable, "FMTTYPE", handle_format_type_parameter);
	g_hash_table_insert(hooks->compparamtable, "FBTYPE", handle_fb_type_parameter);
	g_hash_table_insert(hooks->compparamtable, "MEMBER", handle_member_parameter);
	g_hash_table_insert(hooks->compparamtable, "PARTSTAT", handle_partstat_parameter);
	g_hash_table_insert(hooks->compparamtable, "RANGE", handle_range_parameter);
	g_hash_table_insert(hooks->compparamtable, "RELATED", handle_related_parameter);
	g_hash_table_insert(hooks->compparamtable, "RELTYPE", handle_reltype_parameter);
	g_hash_table_insert(hooks->compparamtable, "ROLE", handle_role_parameter);
	g_hash_table_insert(hooks->compparamtable, "RSVP", handle_rsvp_parameter);
	g_hash_table_insert(hooks->compparamtable, "SENT-BY", handle_sent_by_parameter);
	g_hash_table_insert(hooks->compparamtable, "X-LIC-ERROR", HANDLE_IGNORE);
	g_hash_table_insert(hooks->compparamtable, "CHARSET", HANDLE_IGNORE);
	g_hash_table_insert(hooks->compparamtable, "STATUS", handle_status_parameter);

	//vcal attributes
	g_hash_table_insert(hooks->table, "PRODID", handle_prodid_attribute);
	g_hash_table_insert(hooks->table, "PRODID", HANDLE_IGNORE);
	g_hash_table_insert(hooks->table, "METHOD", handle_method_attribute);
	g_hash_table_insert(hooks->table, "VERSION", HANDLE_IGNORE);
	g_hash_table_insert(hooks->table, "ENCODING", HANDLE_IGNORE);
	g_hash_table_insert(hooks->table, "CHARSET", HANDLE_IGNORE);
	g_hash_table_insert(hooks->table, "BEGIN", HANDLE_IGNORE);
	g_hash_table_insert(hooks->table, "END", HANDLE_IGNORE);
	g_hash_table_insert(hooks->table, "CALSCALE", handle_calscale_attribute);
	g_hash_table_insert(hooks->table, "X-LIC-ERROR", HANDLE_IGNORE);
	
	//Timezone
	g_hash_table_insert(hooks->tztable, "TZID", handle_tzid_attribute);
	g_hash_table_insert(hooks->tztable, "X-LIC-LOCATION", handle_tz_location_attribute);
	g_hash_table_insert(hooks->tztable, "TZOFFSETFROM", handle_tzoffsetfrom_location_attribute);
	g_hash_table_insert(hooks->tztable, "TZOFFSETTO", handle_tzoffsetto_location_attribute);
	g_hash_table_insert(hooks->tztable, "TZNAME", handle_tzname_attribute);
	g_hash_table_insert(hooks->tztable, "DTSTART", handle_tzdtstart_attribute);
	g_hash_table_insert(hooks->tztable, "RRULE", handle_tzrrule_attribute);
	g_hash_table_insert(hooks->tztable, "LAST-MODIFIED", handle_tz_last_modified_attribute);
	g_hash_table_insert(hooks->tztable, "BEGIN", HANDLE_IGNORE);
	g_hash_table_insert(hooks->tztable, "END", HANDLE_IGNORE);
	g_hash_table_insert(hooks->tztable, "TZURL", handle_tzurl_attribute);
	g_hash_table_insert(hooks->tztable, "COMMENT", HANDLE_IGNORE);
	g_hash_table_insert(hooks->tztable, "RDATE", handle_tzrdate_attribute);

	/*FIXME: The functions below shoudn't be on tztable, but on another hash table */
	g_hash_table_insert(hooks->tztable, "VALUE", handle_value_parameter);
	g_hash_table_insert(hooks->tztable, "ALTREP", handle_altrep_parameter);
	g_hash_table_insert(hooks->tztable, "CN", handle_cn_parameter);
	g_hash_table_insert(hooks->tztable, "DELEGATED-FROM", handle_delegated_from_parameter);
	g_hash_table_insert(hooks->tztable, "DELEGATED-TO", handle_delegated_to_parameter);
	g_hash_table_insert(hooks->tztable, "DIR", handle_dir_parameter);
	g_hash_table_insert(hooks->tztable, "FMTTYPE", handle_format_type_parameter);
	g_hash_table_insert(hooks->tztable, "FBTYPE", handle_fb_type_parameter);
	g_hash_table_insert(hooks->tztable, "MEMBER", handle_member_parameter);
	g_hash_table_insert(hooks->tztable, "PARTSTAT", handle_partstat_parameter);
	g_hash_table_insert(hooks->tztable, "RANGE", handle_range_parameter);
	g_hash_table_insert(hooks->tztable, "RELATED", handle_related_parameter);
	g_hash_table_insert(hooks->tztable, "RELTYPE", handle_reltype_parameter);
	g_hash_table_insert(hooks->tztable, "ROLE", handle_role_parameter);
	g_hash_table_insert(hooks->tztable, "RSVP", handle_rsvp_parameter);
	g_hash_table_insert(hooks->tztable, "SENT-BY", handle_sent_by_parameter);
	g_hash_table_insert(hooks->tztable, "X-LIC-ERROR", HANDLE_IGNORE);
	
	//VAlarm component
	g_hash_table_insert(hooks->alarmtable, "TRIGGER", handle_atrigger_attribute);
	g_hash_table_insert(hooks->alarmtable, "REPEAT", handle_arepeat_attribute);
	g_hash_table_insert(hooks->alarmtable, "DURATION", handle_aduration_attribute);
	g_hash_table_insert(hooks->alarmtable, "ACTION", handle_aaction_attribute);
	g_hash_table_insert(hooks->alarmtable, "ATTACH", handle_aattach_attribute);
	g_hash_table_insert(hooks->alarmtable, "DESCRIPTION", handle_adescription_attribute);
	g_hash_table_insert(hooks->alarmtable, "ATTENDEE", handle_aattendee_attribute);
	g_hash_table_insert(hooks->alarmtable, "SUMMARY", handle_asummary_attribute);

	/*FIXME: The functions below shoudn't be on alarmtable, but on another hash table */
	g_hash_table_insert(hooks->alarmtable, "TZID", handle_tzid_parameter);
	g_hash_table_insert(hooks->alarmtable, "VALUE", handle_value_parameter);
	g_hash_table_insert(hooks->alarmtable, "ALTREP", handle_altrep_parameter);
	g_hash_table_insert(hooks->alarmtable, "CN", handle_cn_parameter);
	g_hash_table_insert(hooks->alarmtable, "DELEGATED-FROM", handle_delegated_from_parameter);
	g_hash_table_insert(hooks->alarmtable, "DELEGATED-TO", handle_delegated_to_parameter);
	g_hash_table_insert(hooks->alarmtable, "DIR", handle_dir_parameter);
	g_hash_table_insert(hooks->alarmtable, "FMTTYPE", handle_format_type_parameter);
	g_hash_table_insert(hooks->alarmtable, "FBTYPE", handle_fb_type_parameter);
	g_hash_table_insert(hooks->alarmtable, "MEMBER", handle_member_parameter);
	g_hash_table_insert(hooks->alarmtable, "PARTSTAT", handle_partstat_parameter);
	g_hash_table_insert(hooks->alarmtable, "RANGE", handle_range_parameter);
	g_hash_table_insert(hooks->alarmtable, "RELATED", handle_related_parameter);
	g_hash_table_insert(hooks->alarmtable, "RELTYPE", handle_reltype_parameter);
	g_hash_table_insert(hooks->alarmtable, "ROLE", handle_role_parameter);
	g_hash_table_insert(hooks->alarmtable, "RSVP", handle_rsvp_parameter);
	g_hash_table_insert(hooks->alarmtable, "SENT-BY", handle_sent_by_parameter);
	g_hash_table_insert(hooks->alarmtable, "X-LIC-ERROR", HANDLE_IGNORE);
	g_hash_table_insert(hooks->alarmtable, "X-EVOLUTION-ALARM-UID", HANDLE_IGNORE);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static void *init_vcal_to_xml(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	OSyncHooksTable *hooks = (OSyncHooksTable *)init_ical_to_xml();
	
	//vcal (event10) only
	insert_attr_handler(hooks->comptable, "RRULE", handle_vcal_rrule_attribute);
	insert_attr_handler(hooks->comptable, "AALARM", handle_aalarm_attribute);
	insert_attr_handler(hooks->comptable, "DALARM", handle_dalarm_attribute);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static void fin_vcal_to_xml(void *data)
{
	OSyncHooksTable *hooks = (OSyncHooksTable *)data;
	g_hash_table_destroy(hooks->table);
	g_free(hooks);
}



/* xml to vcal */
static VFormatAttribute *handle_xml_dtstamp_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTSTAMP");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_description_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DESCRIPTION");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_summary_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "SUMMARY");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

/* ical only */
static VFormatAttribute *handle_xml_due_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DUE");
	add_value(attr, root, "Content", encoding);
	vformat_attribute_add_param_with_value(attr, "TZID", osxml_find_node(root, "TimezoneID"));
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_dtstart_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTSTART");
	add_value(attr, root, "Content", encoding);
	vformat_attribute_add_param_with_value(attr, "TZID", osxml_find_node(root, "TimezoneID"));
	vformat_add_attribute(vcard, attr);
	return attr;
}
/* end ical only */

/* vcal only */
static VFormatAttribute *handle_vcal_xml_due_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DUE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_vcal_xml_dtstart_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTSTART");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}
/* end vcal only */

static VFormatAttribute *handle_xml_percent_complete_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "PERCENT-COMPLETE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_priority_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "PRIORITY");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_sequence_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "SEQUENCE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_last_modified_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LAST-MODIFIED");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_created_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CREATED");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_rrule_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RRULE");
	vformat_add_attribute(vcard, attr);
	return attr;
}

/* vcal only */
static VFormatAttribute *handle_vcal_xml_rrule_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RRULE");

	char *vcalrrule = NULL;
	GString *icalrrule = g_string_new("");
        xmlNode *child = root->xmlChildrenNode;
	while (child) {
		icalrrule = g_string_append(icalrrule, (char*)xmlNodeGetContent(child));
		if (child->next)
			icalrrule = g_string_append(icalrrule, ";"); 

		child = child->next;
	}

	vcalrrule = conv_ical2vcal_rrule(icalrrule->str);

	g_string_free(icalrrule, TRUE);

	vformat_attribute_add_value(attr, vcalrrule);

	vformat_add_attribute(vcard, attr);
	return attr;
}
/* end of vcal only */

static VFormatAttribute *handle_xml_rdate_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RDATE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_location_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LOCATION");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_completed_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "COMPLETED");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_organizer_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ORGANIZER");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_recurid_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RECURRENCE-ID");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_status_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "STATUS");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_duration_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DURATION");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_attach_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTACH");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_attendee_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTENDEE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_contact_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CONTACT");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_exdate_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "EXDATE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

/* vcal only */
static VFormatAttribute *handle_vcal_xml_exdate_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{

	xmlNode *dtstartNode = NULL;
	char *dtstart = NULL, *timestamp = NULL, *origex = NULL;
	GString *exdate = g_string_new("");
	VFormatAttribute *attr = NULL;

	if (!(attr = vformat_find_attribute(vcard, "EXDATE")))
		attr = vformat_attribute_new(NULL, "EXDATE");

	origex = (char *) xmlNodeGetContent(root);

	exdate = g_string_append(exdate, origex);

	if (!strstr(origex, "T")) {
		dtstartNode = osxml_get_node(root->parent->parent, "DateStarted"); 
		osync_trace(TRACE_INTERNAL, "dtstartNode pointer: %p", dtstartNode);
		dtstart = osxml_find_node(dtstartNode, "Content");
		timestamp = strstr(dtstart, "T");
		osync_trace(TRACE_INTERNAL, "append timestamp: %s", timestamp);
		exdate = g_string_append(exdate, timestamp); 
		g_free(dtstart);
	}
	
	vformat_attribute_add_value(attr, exdate->str);

	g_string_free(exdate, TRUE);

	if (!vformat_find_attribute(vcard, "EXDATE"))
		vformat_add_attribute(vcard, attr);
	return attr;
}
/* end of vcal only */

static VFormatAttribute *handle_xml_exrule_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "EXRULE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_rstatus_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RSTATUS");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_related_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RELATED-TO");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_resources_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RESOURCES");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

/* ical only */
static VFormatAttribute *handle_xml_dtend_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTEND");
	add_value(attr, root, "Content", encoding);
	vformat_attribute_add_param_with_value(attr, "TZID", osxml_find_node(root, "TimezoneID"));
	vformat_add_attribute(vcard, attr);
	return attr;
}
/* end ical only */

/* vcal only */
static VFormatAttribute *handle_vcal_xml_dtend_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTEND");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}
/* end vcal only */

static VFormatAttribute *handle_xml_transp_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TRANSP");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_calscale_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CALSCALE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzid_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZID");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tz_location_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-LIC-LOCATION");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzoffsetfrom_location_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZOFFSETFROM");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzoffsetto_location_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZOFFSETTO");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzname_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZNAME");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzdtstart_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTSTART");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzrrule_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RRULE");
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tz_last_modified_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LAST-MODIFIED");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzurl_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZURL");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_tzrdate_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RDATE");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

/* vcal only */
static VFormatAttribute *handle_vcal_xml_alarm_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling reminder xml attribute");

	xmlNode *dtstart = NULL;
	xmlNode *trigger = osxml_get_node(root, "AlarmTrigger");
	char *action = NULL, *tmp = NULL, *value = NULL, *runtime = NULL;
	char *startvtime = NULL;
	time_t dtstarted;
	int duration;
	osync_bool isruntime = FALSE;

	VFormatAttribute *attr = vformat_attribute_new(NULL, "DALARM");

	dtstart = osxml_get_node(root->parent, "DateStarted"); 
	value = osxml_find_node(dtstart, "Value");

	if (value) {
		if (strstr(value, "DATE-TIME")) {
			isruntime = TRUE;
		}
	}

	startvtime = osxml_find_node(dtstart, "Content");

	/* Runtime */
	if (isruntime) {
		runtime = startvtime; 
	/* Duration */	
	} else {
		tmp = osxml_find_node(trigger, "Content");
		duration = osync_time_alarmdu2sec(tmp);
		g_free(tmp);

		tmp = osxml_find_node(dtstart, "Content");
		/* AlarmTrigger MUST be UTC (see rfc2445).
		   So there is an offset to UTC of 0 seconds. */
		if (osync_time_isutc(tmp))
			osync_trace(TRACE_INTERNAL, "WARNNING: timestamp is not UTC: %s", tmp);

		dtstarted = osync_time_vtime2unix(tmp, 0);

		g_free(tmp);

		dtstarted += duration;

		runtime = osync_time_unix2vtime(&dtstarted);
	}

	g_free(value);

	vformat_attribute_add_value(attr, runtime);
	
	add_value(attr, root, "AlarmDuration", encoding);
	add_value(attr, root, "AlarmRepeat", encoding);
	add_value(attr, root, "AlarmDescription", encoding);

	action = osxml_find_node(root, "AlarmAction");
	if (action) {
		if (!strcmp(action, "AUDIO")) {
			osync_trace(TRACE_INTERNAL, "Handling audo reminder xml attribute");

			attr = vformat_attribute_new(NULL, "AALARM");
			vformat_attribute_add_value(attr, runtime);
			add_value(attr, root, "AlarmDuration", encoding);
			add_value(attr, root, "AlarmRepeat", encoding);
			add_value(attr, root, "AlarmDescription", encoding);
		}
	}

	vformat_add_attribute(vcard, attr);
	g_free(action);
	g_free(runtime);

	return attr;
}
/* end of vcal only */

/* ical only */
static VFormatAttribute *handle_xml_atrigger_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TRIGGER");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_arepeat_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "REPEAT");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aduration_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DURATION");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aaction_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ACTION");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aattach_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTACH");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_adescription_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DESCRIPTION");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aattendee_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTENDEE");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_asummary_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "SUMMARY");
	add_value(attr, root, NULL, encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}
/* end of ical only */

void xml_parse_attribute(OSyncHooksTable *hooks, GHashTable *table, xmlNode **current, VFormat *vcal, VFormatType target)
{
	osync_trace(TRACE_INTERNAL, "parsing xml attributes");
	void *xml_param_handler = NULL;
	VFormatAttribute *attr = NULL;
	xmlNode *root = *current;
	while (root) {
		if (!strcmp((char*)root->name, "Todo")) {
			attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VTODO");
			vformat_add_attribute(vcal, attr);
			xmlNode *child = root->children;
			xml_parse_attribute(hooks, hooks->comptable, &child, vcal, target);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VTODO");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)root->name, "Timezone") && target != VFORMAT_EVENT_10) {
			attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VTIMEZONE");
			vformat_add_attribute(vcal, attr);
			xmlNode *child = root->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal, target);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VTIMEZONE");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)root->name, "Event")) {
			attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VEVENT");
			vformat_add_attribute(vcal, attr);
			xmlNode *child = root->children;
			xml_parse_attribute(hooks, hooks->comptable, &child, vcal, target);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VEVENT");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)root->name, "Journal")) {
			attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "VJOURNAL");
			vformat_add_attribute(vcal, attr);
			xmlNode *child = root->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal, target);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "VJOURNAL");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)root->name, "DaylightSavings")) {
			attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "DAYLIGHT");
			vformat_add_attribute(vcal, attr);
			xmlNode *child = root->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal, target);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "DAYLIGHT");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)root->name, "Standard")) {
			attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_attribute_add_value(attr, "STANDARD");
			vformat_add_attribute(vcal, attr);
			xmlNode *child = root->children;
			xml_parse_attribute(hooks, hooks->tztable, &child, vcal, target);
			attr = vformat_attribute_new(NULL, "END");
			vformat_attribute_add_value(attr, "STANDARD");
			vformat_add_attribute(vcal, attr);
		} else if (!strcmp((char*)root->name, "Alarm") && target != VFORMAT_EVENT_10) {

				xmlNode *child = root->children;
				attr = vformat_attribute_new(NULL, "BEGIN");
				vformat_attribute_add_value(attr, "VALARM");
				vformat_add_attribute(vcal, attr);
				xml_parse_attribute(hooks, hooks->alarmtable, &child, vcal, target);
				attr = vformat_attribute_new(NULL, "END");
				vformat_attribute_add_value(attr, "VALARM");
				vformat_add_attribute(vcal, attr);

		/* list of parameters which should NOT handle for vcal (event10) */
		} else if (!strcmp((char*)root->name, "ExclusionDate") && target == VFORMAT_EVENT_10) {
				xml_param_handler = g_hash_table_lookup(hooks->comptable, "Value");
				g_hash_table_insert(hooks->comptable, "Value", HANDLE_IGNORE);
				g_hash_table_insert(hooks->comptable, "Content", handle_vcal_xml_exdate_attribute); 

				xmlNode *child = root->children;
				xml_parse_attribute(hooks, hooks->comptable, &child, vcal, target);

				g_hash_table_insert(hooks->comptable, "Value", xml_param_handler);
				g_hash_table_remove(hooks->comptable, "Content"); 

		} else {
			xml_vcal_handle_attribute(table, vcal, root);
		}
		root = root->next;
	}
}


static void insert_xml_attr_handler(GHashTable *table, const char *name, xml_attr_handler_t handler)
{
	g_hash_table_insert(table, (gpointer)name, handler);
}

static osync_bool conv_xml_to_vcal(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error, VFormatType target)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
	
	xmlChar *str = osxml_write_to_string((xmlDoc *)input);
	osync_trace(TRACE_SENSITIVE, "Input XML is:\n%s", str);
	xmlFree(str);
	
	//Get the root node of the input document
	xmlNode *root = osxml_node_get_root((xmlDoc *)input, "vcal", error);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get root element of xml-contact");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	//Make the new vcard
	VFormat *vcal = vformat_new();
	
	OSyncHooksTable *hooks = (OSyncHooksTable *)user_data;
	/*  vevent10 / vevent20  */
	if (target == VFORMAT_EVENT_10) {
		/* RRULE */
		insert_xml_attr_handler(hooks->comptable, "RecurrenceRule", handle_vcal_xml_rrule_attribute);
		insert_xml_attr_handler(hooks->comptable, "ExclusionDate", handle_vcal_xml_exdate_attribute);
		insert_xml_attr_handler(hooks->comptable, "Alarm", handle_vcal_xml_alarm_attribute);
		g_hash_table_insert(hooks->comptable, "Rule", HANDLE_IGNORE);

		/* vcal attributes */
		insert_xml_attr_handler(hooks->comptable, "DateEnd", handle_vcal_xml_dtend_attribute);
		insert_xml_attr_handler(hooks->comptable, "DateDue", handle_vcal_xml_due_attribute);
		insert_xml_attr_handler(hooks->comptable, "DateStarted", handle_vcal_xml_dtstart_attribute);


	} else {
		/* RRULE */
		insert_xml_attr_handler(hooks->comptable, "RecurrenceRule", handle_xml_rrule_attribute);
		insert_xml_attr_handler(hooks->comptable, "ExclusionDate", handle_xml_exdate_attribute);
		g_hash_table_insert(hooks->comptable, "Rule", handle_xml_rule_parameter);

		/* ical attributes */
		g_hash_table_insert(hooks->table, "Method", handle_xml_method_attribute);
		insert_xml_attr_handler(hooks->comptable, "DateEnd", handle_xml_dtend_attribute);
		insert_xml_attr_handler(hooks->comptable, "DateDue", handle_xml_due_attribute);
		insert_xml_attr_handler(hooks->comptable, "DateStarted", handle_xml_dtstart_attribute);

		/* Timezone */
		g_hash_table_insert(hooks->tztable, "TimezoneID", handle_xml_tzid_attribute);
		g_hash_table_insert(hooks->tztable, "Location", handle_xml_tz_location_attribute);
		g_hash_table_insert(hooks->tztable, "TZOffsetFrom", handle_xml_tzoffsetfrom_location_attribute);
		g_hash_table_insert(hooks->tztable, "TZOffsetTo", handle_xml_tzoffsetto_location_attribute);
		g_hash_table_insert(hooks->tztable, "TimezoneName", handle_xml_tzname_attribute);
		g_hash_table_insert(hooks->tztable, "DateStarted", handle_xml_tzdtstart_attribute);
		g_hash_table_insert(hooks->tztable, "RecurrenceRule", handle_xml_tzrrule_attribute);
		g_hash_table_insert(hooks->tztable, "LastModified", handle_xml_tz_last_modified_attribute);
		g_hash_table_insert(hooks->tztable, "TimezoneUrl", handle_xml_tzurl_attribute);
		g_hash_table_insert(hooks->tztable, "RecurrenceDate", handle_xml_tzrdate_attribute);

		/* VAlarm component */
		g_hash_table_insert(hooks->alarmtable, "AlarmTrigger", handle_xml_atrigger_attribute);
		g_hash_table_insert(hooks->alarmtable, "AlarmRepeat", handle_xml_arepeat_attribute);
		g_hash_table_insert(hooks->alarmtable, "AlarmDuration", handle_xml_aduration_attribute);
		g_hash_table_insert(hooks->alarmtable, "AlarmAction", handle_xml_aaction_attribute);
		g_hash_table_insert(hooks->alarmtable, "AlarmAttach", handle_xml_aattach_attribute);
		g_hash_table_insert(hooks->alarmtable, "AlarmDescription", handle_xml_adescription_attribute);
		g_hash_table_insert(hooks->alarmtable, "AlarmAttendee", handle_xml_aattendee_attribute);
		g_hash_table_insert(hooks->alarmtable, "AlarmSummary", handle_xml_asummary_attribute);

	}
	
	xml_parse_attribute(hooks, hooks->table, &root, vcal, target);
	
	*free_input = TRUE;
	*output = vformat_to_string(vcal, target);
	osync_trace(TRACE_SENSITIVE, "vevent output is: \n%s", *output);
	*outpsize = strlen(*output);
	osync_trace(TRACE_EXIT, "%s", __func__);
	
	return TRUE;
}

static osync_bool conv_xml_to_vevent10(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	return conv_xml_to_vcal(user_data, input, inpsize, output, outpsize, free_input, error, VFORMAT_EVENT_10);
}

static osync_bool conv_xml_to_vevent20(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	return conv_xml_to_vcal(user_data, input, inpsize, output, outpsize, free_input, error, VFORMAT_EVENT_20);
}

static osync_bool conv_xml_to_vtodo10(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	return conv_xml_to_vcal(user_data, input, inpsize, output, outpsize, free_input, error, VFORMAT_TODO_10);
}

static osync_bool conv_xml_to_vtodo20(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	return conv_xml_to_vcal(user_data, input, inpsize, output, outpsize, free_input, error, VFORMAT_TODO_20);
}

static void *init_xml_to_vcal(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	
	OSyncHooksTable *hooks = g_malloc0(sizeof(OSyncHooksTable));
	
	hooks->table = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->tztable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->comptable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->alarmtable = g_hash_table_new(g_str_hash, g_str_equal);
	
	//todo attributes
	insert_xml_attr_handler(hooks->comptable, "Uid", handle_xml_uid_attribute);
	insert_xml_attr_handler(hooks->comptable, "DateCalendarCreated", handle_xml_dtstamp_attribute);
	insert_xml_attr_handler(hooks->comptable, "Description", handle_xml_description_attribute);
	insert_xml_attr_handler(hooks->comptable, "Summary", handle_xml_summary_attribute);
	insert_xml_attr_handler(hooks->comptable, "PercentComplete", handle_xml_percent_complete_attribute);
	insert_xml_attr_handler(hooks->comptable, "Class", handle_xml_class_attribute);
	insert_xml_attr_handler(hooks->comptable, "Categories", handle_xml_categories_attribute);
	insert_xml_attr_handler(hooks->comptable, "Priority", handle_xml_priority_attribute);
	insert_xml_attr_handler(hooks->comptable, "Url", handle_xml_url_attribute);
	insert_xml_attr_handler(hooks->comptable, "Sequence", handle_xml_sequence_attribute);
	insert_xml_attr_handler(hooks->comptable, "LastModified", handle_xml_last_modified_attribute);
	insert_xml_attr_handler(hooks->comptable, "DateCreated", handle_xml_created_attribute);
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
	insert_xml_attr_handler(hooks->comptable, "Contact", handle_xml_contact_attribute);
	insert_xml_attr_handler(hooks->comptable, "ExclusionRule", handle_xml_exrule_attribute);
	insert_xml_attr_handler(hooks->comptable, "RStatus", handle_xml_rstatus_attribute);
	insert_xml_attr_handler(hooks->comptable, "Related", handle_xml_related_attribute);
	insert_xml_attr_handler(hooks->comptable, "Resources", handle_xml_resources_attribute);
	insert_xml_attr_handler(hooks->comptable, "Transparency", handle_xml_transp_attribute);


	/*FIXME: The functions below shouldn't be on comptable, but on other hash table */
	g_hash_table_insert(hooks->comptable, "Category", handle_xml_category_parameter);
	g_hash_table_insert(hooks->comptable, "Value", handle_xml_value_parameter);
	g_hash_table_insert(hooks->comptable, "AlternateRep", handle_xml_altrep_parameter);
	g_hash_table_insert(hooks->comptable, "CommonName", handle_xml_cn_parameter);
	g_hash_table_insert(hooks->comptable, "DelegatedFrom", handle_xml_delegated_from_parameter);
	g_hash_table_insert(hooks->comptable, "DelegatedTo", handle_xml_delegated_to_parameter);
	g_hash_table_insert(hooks->comptable, "Directory", handle_xml_dir_parameter);
	g_hash_table_insert(hooks->comptable, "FormaType", handle_xml_format_type_parameter);
	g_hash_table_insert(hooks->comptable, "FreeBusyType", handle_xml_fb_type_parameter);
	g_hash_table_insert(hooks->comptable, "Member", handle_xml_member_parameter);
	g_hash_table_insert(hooks->comptable, "PartStat", handle_xml_partstat_parameter);
	g_hash_table_insert(hooks->comptable, "Range", handle_xml_range_parameter);
	g_hash_table_insert(hooks->comptable, "Related", handle_xml_related_parameter);
	g_hash_table_insert(hooks->comptable, "RelationType", handle_xml_reltype_parameter);
	g_hash_table_insert(hooks->comptable, "Role", handle_xml_role_parameter);
	g_hash_table_insert(hooks->comptable, "RSVP", handle_xml_rsvp_parameter);
	g_hash_table_insert(hooks->comptable, "SentBy", handle_xml_sent_by_parameter);
	
	//vcal attributes
	g_hash_table_insert(hooks->table, "CalendarScale", handle_xml_calscale_attribute);
	g_hash_table_insert(hooks->table, "ProductID", handle_xml_prodid_attribute);
	g_hash_table_insert(hooks->table, "UnknownNode", xml_handle_unknown_attribute);
	g_hash_table_insert(hooks->table, "UnknownParameter", xml_handle_unknown_parameter);
	
	/*FIXME: The functions below shouldn't be on tztable, but on other hash table */
	g_hash_table_insert(hooks->tztable, "Category", handle_xml_category_parameter);
	g_hash_table_insert(hooks->tztable, "Rule", handle_xml_rule_parameter);
	g_hash_table_insert(hooks->tztable, "Value", handle_xml_value_parameter);
	g_hash_table_insert(hooks->tztable, "AlternateRep", handle_xml_altrep_parameter);
	g_hash_table_insert(hooks->tztable, "CommonName", handle_xml_cn_parameter);
	g_hash_table_insert(hooks->tztable, "DelegatedFrom", handle_xml_delegated_from_parameter);
	g_hash_table_insert(hooks->tztable, "DelegatedTo", handle_xml_delegated_to_parameter);
	g_hash_table_insert(hooks->tztable, "Directory", handle_xml_dir_parameter);
	g_hash_table_insert(hooks->tztable, "FormaType", handle_xml_format_type_parameter);
	g_hash_table_insert(hooks->tztable, "FreeBusyType", handle_xml_fb_type_parameter);
	g_hash_table_insert(hooks->tztable, "Member", handle_xml_member_parameter);
	g_hash_table_insert(hooks->tztable, "PartStat", handle_xml_partstat_parameter);
	g_hash_table_insert(hooks->tztable, "Range", handle_xml_range_parameter);
	g_hash_table_insert(hooks->tztable, "Related", handle_xml_related_parameter);
	g_hash_table_insert(hooks->tztable, "RelationType", handle_xml_reltype_parameter);
	g_hash_table_insert(hooks->tztable, "Role", handle_xml_role_parameter);
	g_hash_table_insert(hooks->tztable, "RSVP", handle_xml_rsvp_parameter);
	g_hash_table_insert(hooks->tztable, "SentBy", handle_xml_sent_by_parameter);
	

	/*FIXME: The functions below shouldn't be on alarmtable, but on other hash table */
	g_hash_table_insert(hooks->alarmtable, "Category", handle_xml_category_parameter);
	g_hash_table_insert(hooks->alarmtable, "Rule", handle_xml_rule_parameter);
	g_hash_table_insert(hooks->alarmtable, "Value", handle_xml_value_parameter);
	g_hash_table_insert(hooks->alarmtable, "AlternateRep", handle_xml_altrep_parameter);
	g_hash_table_insert(hooks->alarmtable, "CommonName", handle_xml_cn_parameter);
	g_hash_table_insert(hooks->alarmtable, "DelegatedFrom", handle_xml_delegated_from_parameter);
	g_hash_table_insert(hooks->alarmtable, "DelegatedTo", handle_xml_delegated_to_parameter);
	g_hash_table_insert(hooks->alarmtable, "Directory", handle_xml_dir_parameter);
	g_hash_table_insert(hooks->alarmtable, "FormaType", handle_xml_format_type_parameter);
	g_hash_table_insert(hooks->alarmtable, "FreeBusyType", handle_xml_fb_type_parameter);
	g_hash_table_insert(hooks->alarmtable, "Member", handle_xml_member_parameter);
	g_hash_table_insert(hooks->alarmtable, "PartStat", handle_xml_partstat_parameter);
	g_hash_table_insert(hooks->alarmtable, "Range", handle_xml_range_parameter);
	g_hash_table_insert(hooks->alarmtable, "Related", handle_xml_related_parameter);
	g_hash_table_insert(hooks->alarmtable, "RelationType", handle_xml_reltype_parameter);
	g_hash_table_insert(hooks->alarmtable, "Role", handle_xml_role_parameter);
	g_hash_table_insert(hooks->alarmtable, "RSVP", handle_xml_rsvp_parameter);
	g_hash_table_insert(hooks->alarmtable, "SentBy", handle_xml_sent_by_parameter);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static void fin_xml_to_vcal(void *data)
{
	OSyncHooksTable *hooks = (OSyncHooksTable *)data;
	g_hash_table_destroy(hooks->table);
	g_free(hooks);
}

static time_t get_revision(OSyncChange *change, const char *path, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, change, error);
	
	xmlDoc *doc = (xmlDoc *)osync_change_get_data(change);
	
	xmlXPathObject *xobj = osxml_get_nodeset(doc, path);
		
	xmlNodeSet *nodes = xobj->nodesetval;
		
	int size = (nodes) ? nodes->nodeNr : 0;
	if (size != 1) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find the revision");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return -1;
	}
	
	char *revision = (char*)xmlNodeGetContent(nodes->nodeTab[0]);
	
	time_t time = vformat_time_to_unix(revision);
	g_free(revision);
	xmlXPathFreeObject(xobj);
	osync_trace(TRACE_EXIT, "%s: %i", __func__, time);
	return time;
}

static time_t get_todo_revision(OSyncChange *change, OSyncError **error)
{
	return get_revision(change, "/vcal/Todo/LastModified", error);
}

static time_t get_event_revision(OSyncChange *change, OSyncError **error)
{
	return get_revision(change, "/vcal/Event/LastModified", error);
}

void get_info(OSyncEnv *env)
{
	//Calendar
	osync_env_register_objtype(env, "event");
	osync_env_register_objformat(env, "event", "xml-event");
	osync_env_format_set_compare_func(env, "xml-event", compare_vevent);
	osync_env_format_set_destroy_func(env, "xml-event", destroy_xml);
	osync_env_format_set_print_func(env, "xml-event", print_vcal);
	osync_env_format_set_copy_func(env, "xml-event", osxml_copy);
	osync_env_format_set_revision_func(env, "xml-event", get_event_revision);
	osync_env_format_set_marshall_func(env, "xml-event", osxml_marshall);
	osync_env_format_set_demarshall_func(env, "xml-event", osxml_demarshall);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vevent10", "xml-event", conv_vcal_to_xml);
	osync_env_converter_set_init(env, "vevent10", "xml-event", init_vcal_to_xml, fin_vcal_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-event", "vevent10", conv_xml_to_vevent10);
	osync_env_converter_set_init(env, "xml-event", "vevent10", init_xml_to_vcal, fin_xml_to_vcal);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vevent20", "xml-event", conv_vcal_to_xml);
	osync_env_converter_set_init(env, "vevent20", "xml-event", init_ical_to_xml, fin_vcal_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-event", "vevent20", conv_xml_to_vevent20);
	osync_env_converter_set_init(env, "xml-event", "vevent20", init_xml_to_vcal, fin_xml_to_vcal);
	
	//Todo
	osync_env_register_objtype(env, "todo");
	osync_env_register_objformat(env, "todo", "xml-todo");
	osync_env_format_set_compare_func(env, "xml-todo", compare_vtodo);
	osync_env_format_set_destroy_func(env, "xml-todo", destroy_xml);
	osync_env_format_set_print_func(env, "xml-todo", print_vcal);
	osync_env_format_set_copy_func(env, "xml-todo", osxml_copy);
	osync_env_format_set_revision_func(env, "xml-todo", get_todo_revision);
	osync_env_format_set_marshall_func(env, "xml-todo", osxml_marshall);
	osync_env_format_set_demarshall_func(env, "xml-todo", osxml_demarshall);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vtodo10", "xml-todo", conv_vcal_to_xml);
	osync_env_converter_set_init(env, "vtodo10", "xml-todo", init_vcal_to_xml, fin_vcal_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-todo", "vtodo10", conv_xml_to_vtodo10);
	osync_env_converter_set_init(env, "xml-todo", "vtodo10", init_xml_to_vcal, fin_xml_to_vcal);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vtodo20", "xml-todo", conv_vcal_to_xml);
	osync_env_converter_set_init(env, "vtodo20", "xml-todo", init_ical_to_xml, fin_vcal_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-todo", "vtodo20", conv_xml_to_vtodo20);
	osync_env_converter_set_init(env, "xml-todo", "vtodo20", init_xml_to_vcal, fin_xml_to_vcal);
}
