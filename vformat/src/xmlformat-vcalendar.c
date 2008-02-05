/*
 * xmlformat-vcalendar - common code for vcalendar10|20, xmlformat-vevent, -vnote, -vtodo
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007-2008  Christopher Stender <cstender@suse.de>
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

#include "xmlformat-vcalendar.h"

/* Briefing:
 * 
 * vcalendar10 <=> vCalendar <=> vcal
 * vcalendar20 <=> iCalendar <=> ical
 *
 * handle_vcal_*_parameter	// parameter handler which are used by vcalendar10 only
 * handle_vcal_*_attribute	// attribute handler which are used by vcalendar10 only
 * handle_xml_vcal_*_parameter	// xml parameter handler which are used by vcalendar10 only
 * handle_xml_vcal_*_attribute	// xml attribute handler which are used by vcalendar10 only
 *
 * handle_*_parameter		// parameter handler which are used by vcalendar20 (and vcalendar10)
 * handle_*_attribute		// attribute handler which are used by vcalendar20 (and vcalendar10)
 * handle_xml_*_parameter	// xml parameter handler which are used by vcalendar20 (and vcalendar10)
 * handle_xml_*_attribute	// xml attribute handler which are used by vcalendar20 (and vcalendar10)
 *
 * handle_alarm_*_attribute	// alarm attribute handler which are used by vcalendar20
 * handle_xml_alarm*_attribute	// xml alarm attribute handler which are used by vcalendar20
 *
 * handle_tz_*_attribute	// timezone attribute handler which are used by vcalendar20
 * handle_xml_tz_*_attribute	// xml timezone attribute handler which are used by vcalendar20
 */


/* BEGIN: vcalendar10 only parameters -> xml */
void handle_vcal_type_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	const char *type = vformat_attribute_param_get_nth_value(param, 0);

	// in case of type was within an alarm field, we call it FormatType
	if (!strncmp(osync_xmlfield_get_name(xmlfield), "Alarm", 5)) {
		if (!strcasecmp("PCM", type)) {
			osync_xmlfield_set_attr(xmlfield, "FormatType", "audio/basic");
		} else if (!strcasecmp("WAVE", type)) {
			osync_xmlfield_set_attr(xmlfield, "FormatType", "audio/x-wav");
		} else if (!strcasecmp("X-EPOCSOUND", type)) {
			osync_trace(TRACE_INTERNAL, "skipping %s parameter", type);
		} else {
			osync_xmlfield_set_attr(xmlfield, "FormatType", type);
		}
	} else {
		osync_xmlfield_set_attr(xmlfield, "Type", type);
	}
}

void handle_vcal_value_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	const char *value = vformat_attribute_param_get_nth_value(param, 0);

	// in case of value was within an alarm field, we call it AttachValue
	// TODO check if base64 and inline is working
	if (!strncmp(osync_xmlfield_get_name(xmlfield), "Alarm", 5)) {
		if (!strcasecmp("URL", value)) {
			osync_xmlfield_set_attr(xmlfield, "AttachValue", "URI");
		} else {
			osync_xmlfield_set_attr(xmlfield, "AttachValue", value);
		}
	} else {
		osync_xmlfield_set_attr(xmlfield, "Value", value);
	}
}

void handle_vcal_encoding_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Encoding", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_vcal_charset_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Charset", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_vcal_language_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Language", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_vcal_role_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Role", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_vcal_status_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Status", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_vcal_rsvp_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	const char *rsvp = vformat_attribute_param_get_nth_value(param, 0);

	if (!strcmp(rsvp, "YES")) {
		osync_xmlfield_set_attr(xmlfield, "Rsvp", "TRUE");
	} else if (!strcmp(rsvp, "NO")) {
		osync_xmlfield_set_attr(xmlfield, "Rsvp", "FALSE");
	} else {	
		osync_xmlfield_set_attr(xmlfield, "Rsvp", rsvp);
	}
}

void handle_vcal_expect_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Expect", vformat_attribute_param_get_nth_value(param, 0));
}
/* END: vcalendar10 only parameters-> xml */



/* BEGIN: vcalendar10 only attributes -> xml */
OSyncXMLField *handle_vcal_aalarm_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	osync_trace(TRACE_INTERNAL, "Handling aalarm attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "AlarmAudio", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	} 

	osync_xmlfield_set_key_value(xmlfield, "AlarmAction", "AUDIO");
	
	osync_xmlfield_set_key_value(xmlfield, "AlarmAttach", vformat_attribute_get_nth_value(attr, 3));
	osync_xmlfield_set_key_value(xmlfield, "AlarmRepeat", vformat_attribute_get_nth_value(attr, 2));
	osync_xmlfield_set_key_value(xmlfield, "AlarmRepeatDuration", vformat_attribute_get_nth_value(attr, 1));
	osync_xmlfield_set_key_value(xmlfield, "AlarmTrigger", vformat_attribute_get_nth_value(attr, 0)); 
	return xmlfield; 
}

OSyncXMLField *handle_vcal_dalarm_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	osync_trace(TRACE_INTERNAL, "Handling dalarm attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "AlarmDisplay", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	} 

	osync_xmlfield_set_key_value(xmlfield, "AlarmAction", "DISPLAY");
	osync_xmlfield_set_key_value(xmlfield, "AlarmAttach", vformat_attribute_get_nth_value(attr, 3));
	osync_xmlfield_set_key_value(xmlfield, "AlarmRepeat", vformat_attribute_get_nth_value(attr, 2));
	osync_xmlfield_set_key_value(xmlfield, "AlarmRepeatDuration", vformat_attribute_get_nth_value(attr, 1));
	osync_xmlfield_set_key_value(xmlfield, "AlarmTrigger", vformat_attribute_get_nth_value(attr, 0)); 
	return xmlfield; 
}

OSyncXMLField *handle_vcal_rrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling RecurrenceRule attribute");

	return convert_vcal_rrule_to_xml(xmlformat, attr, "RecurrenceRule", error);
}
/* END: vcalendar only attributes -> xml */



/* BEGIN: xml -> vcalendar10 parameters */
void handle_xml_vcal_rsvp_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling Rsvp xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "Rsvp");
	if (!strcmp(content, "TRUE")) {
		vformat_attribute_add_param_with_value(attr, "RSVP", "YES");
	} else if (!strcmp(content, "FALSE")) {
		vformat_attribute_add_param_with_value(attr, "RSVP", "NO");
	} else {
		vformat_attribute_add_param_with_value(attr, "RSVP", content);
	}
}
/* END: xml -> vcalendar10 parameters */



/* BEGIN: xml -> vcalendar10 only attributes */
VFormatAttribute *handle_xml_vcal_rrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling \"RRULE\" xml attribute");
	return convert_xml_rrule_to_vcal(vevent, xmlfield, "RRULE", encoding); 
}
/* END: xml -> vcalendar10 only attributes */



/* BEGIN: vcalendar20 parameters -> xml */
void handle_altrep_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "AlternativeTextRep", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_cn_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "CommonName", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_cutype_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "CUType", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_delegated_from_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "DelegatedFrom", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_delegated_to_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "DelegatedTo", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_dir_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Directory", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_encoding_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Encoding", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_format_type_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "FormatType", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_fb_type_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "FreeBusyType", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_language_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Language", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_member_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Member", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_partstat_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "PartStat", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_range_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter %s\n", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Range", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_trigrel_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter %s\n", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "RelatedType", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_reltype_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "RelationshipType", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_role_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Role", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_rsvp_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Rsvp", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_sent_by_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "SentBy", vformat_attribute_param_get_nth_value(param, 0));
}

void handle_tzid_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling %s parameter %s\n", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "TimezoneID", vformat_attribute_param_get_nth_value(param, 0));
}
/* END: vcalendar20 parameters -> xml */



/* BEGIN: vcalendar20 (and vcalendar10) attributes -> xml */
OSyncXMLField *handle_attach_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Attach", error);
}

OSyncXMLField *handle_attendee_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Attendee", error);
}

OSyncXMLField *handle_comment_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Comment", error);
}

OSyncXMLField *handle_created_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content_timestamp(xmlformat, attr, "Created", error);
}

OSyncXMLField *handle_dtstamp_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content_timestamp(xmlformat, attr, "DateCalendarCreated", error);
}

OSyncXMLField *handle_dtstart_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content_timestamp(xmlformat, attr, "DateStarted", error);
}

OSyncXMLField *handle_description_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Description", error);
}

OSyncXMLField *handle_summary_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Summary", error);
}

OSyncXMLField *handle_due_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content_timestamp(xmlformat, attr, "Due", error);
}

OSyncXMLField *handle_duration_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	osync_trace(TRACE_INTERNAL, "Handling Duration attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Duration", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}

	const char *duration = vformat_attribute_get_nth_value(attr, 0);

	// check if duration is positive or negative
	if (duration[0] == '-') {
		osync_xmlfield_add_key_value(xmlfield, "InAdvance", "TRUE");
	} else {
		osync_xmlfield_add_key_value(xmlfield, "InAdvance", "FALSE");
	}

	int i, end, digits;
	end = strlen(duration);
	char *value = NULL;
	for (i = 1; i < end; i++) {
		switch (duration[i]) {
			case 'W':
				osync_xmlfield_add_key_value(xmlfield, "Weeks", value);
				break;	
			case 'D':
				osync_xmlfield_add_key_value(xmlfield, "Days", value);
				break;
			case 'H':
				osync_xmlfield_add_key_value(xmlfield, "Hours", value);
				break;
			case 'M':
				osync_xmlfield_add_key_value(xmlfield, "Minutes", value);
				break;
			case 'S':
				osync_xmlfield_add_key_value(xmlfield, "Seconds", value);
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
				if (value)
					g_free(value);
				sscanf((duration+i),"%d",&digits);
				value = g_strdup_printf("%i", digits);
				i += strlen(value)-1;
				break;
			default:
				break;
		}	
	}

	if (value)
		g_free(value);

	return xmlfield;
}

OSyncXMLField *handle_priority_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Priority", error);
}

OSyncXMLField *handle_sequence_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Sequence", error);
}

OSyncXMLField *handle_last_modified_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content_timestamp(xmlformat, attr, "LastModified", error);
}

OSyncXMLField *handle_geo_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
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

OSyncXMLField *handle_prodid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "ProductID", error);
}

OSyncXMLField *handle_rdate_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "RecurrenceDateTime", error);
}

OSyncXMLField *handle_rnum_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "NumberRecurrences", error);
}

OSyncXMLField *handle_location_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Location", error);
}

OSyncXMLField *handle_completed_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Completed", error);
}

OSyncXMLField *handle_status_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Status", error);
}

OSyncXMLField *handle_exdate_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content_timestamp(xmlformat, attr, "ExceptionDateTime", error);
}

OSyncXMLField *handle_exrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	osync_trace(TRACE_INTERNAL, "Handling ExceptionRule attribute");

	return convert_ical_rrule_to_xml(xmlformat, attr, "ExceptionRule", error);
}

OSyncXMLField *handle_rstatus_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{
	osync_trace(TRACE_INTERNAL, "Handling RStatus attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "RStatus", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	osync_xmlfield_set_key_value(xmlfield, "StatusCode", vformat_attribute_get_nth_value(attr, 0));
	osync_xmlfield_set_key_value(xmlfield, "StatusDescription", vformat_attribute_get_nth_value(attr, 1));
	if (vformat_attribute_get_nth_value(attr, 2) != NULL)
		osync_xmlfield_set_key_value(xmlfield, "ExceptionData", vformat_attribute_get_nth_value(attr, 2));
	
	return xmlfield;
}

OSyncXMLField *handle_related_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Related", error);
}

OSyncXMLField *handle_resources_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Resources", error);
}

OSyncXMLField *handle_dtend_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content_timestamp(xmlformat, attr, "DateEnd", error);
}

OSyncXMLField *handle_transp_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "TimeTransparency", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	} 

	const char *transp = vformat_attribute_get_nth_value(attr, 0);
        if (!strcmp(transp, "0") || !strcmp(transp, "OPAQUE")) {
		osync_xmlfield_set_key_value(xmlfield, "Content", "OPAQUE"); 
        } else {
		osync_xmlfield_set_key_value(xmlfield, "Content", "TRANSPARENT"); 
        }

	return xmlfield; 

}

OSyncXMLField *handle_method_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Method", error);
}

OSyncXMLField *handle_percent_complete_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "PercentComplete", error);
}

OSyncXMLField *handle_rrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Recurrence Rule attribute");
	
	return convert_ical_rrule_to_xml(xmlformat, attr, "RecurrenceRule", error);
}

OSyncXMLField *handle_organizer_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Organizer", error);
}

OSyncXMLField *handle_recurid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "RecurrenceId", error);
}

OSyncXMLField *handle_contact_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Contact", error);
}

OSyncXMLField *handle_calscale_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "CalendarScale", error);
}
/* END: vcalendar20 (and vcalendar10) attributes -> xml */



/* BEGIN: xml -> vcalendar20 (and vcalendar10) parameters */
void handle_xml_tzid_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling TimezoneID xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "TimezoneID");
	vformat_attribute_add_param_with_value(attr, "TZID", content);
}

void handle_xml_altrep_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling AlternativeTextRep xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "AlternativeTextRep");
	vformat_attribute_add_param_with_value(attr, "ALTREP", content);
}

void handle_xml_cn_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling CommonName xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "CommonName");
	vformat_attribute_add_param_with_value(attr, "CN", content);
}

void handle_xml_cutype_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling CUType xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "CUType");
	vformat_attribute_add_param_with_value(attr, "CUTYPE", content);
}

void handle_xml_delegated_from_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling DelegatedFrom xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "DelegatedFrom");
	vformat_attribute_add_param_with_value(attr, "DELEGATED-FROM", content);
}

void handle_xml_delegated_to_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling DelegatedTo xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "DelegatedTo");
	vformat_attribute_add_param_with_value(attr, "DELEGATED-TO", content);
}

void handle_xml_dir_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling Directory xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "Directory");
	vformat_attribute_add_param_with_value(attr, "DIR", content);
}

void handle_xml_encoding_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	//FIXME if Encoding=B -> ENCODING=BASE64
	osync_trace(TRACE_INTERNAL, "Handling Encoding xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "Encoding");
	vformat_attribute_add_param_with_value(attr, "ENCODING", content);
}

void handle_xml_format_type_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling FormatType xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "FormatType");
	vformat_attribute_add_param_with_value(attr, "FMTTYPE", content);
}

void handle_xml_fb_type_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling FreeBusyType xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "FreeBusyType");
	vformat_attribute_add_param_with_value(attr, "FBTYPE", content);
}

void handle_xml_language_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling Language xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "Language");
	vformat_attribute_add_param_with_value(attr, "LANGUAGE", content);
}

void handle_xml_member_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling Member xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "Member");
	vformat_attribute_add_param_with_value(attr, "MEMBER", content);
}

void handle_xml_partstat_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling PartStat xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "PartStat");
	vformat_attribute_add_param_with_value(attr, "PARTSTAT", content);
}

void handle_xml_range_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling Range xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "Range");
	vformat_attribute_add_param_with_value(attr, "RANGE", content);
}

void handle_xml_related_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling RelatedType xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "RelatedType");
	vformat_attribute_add_param_with_value(attr, "RELATED", content);
}

void handle_xml_reltype_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling RelationshipType xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "RelationshipType");
	vformat_attribute_add_param_with_value(attr, "RELTYPE", content);
}

void handle_xml_role_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling Role xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "Role");
	vformat_attribute_add_param_with_value(attr, "ROLE", content);
}

void handle_xml_rsvp_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling Rsvp xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "Rsvp");
	vformat_attribute_add_param_with_value(attr, "RSVP", content);
}

void handle_xml_sent_by_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling SentBy xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "SentBy");
	vformat_attribute_add_param_with_value(attr, "SENT-BY", content);
}

void handle_xml_value_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling Value xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "Value");
	vformat_attribute_add_param_with_value(attr, "VALUE", content);
}
/* END: xml -> vcalendar20 (and vcalendar10) parameters */



/* BEGIN: xml -> vcalendar20 (and vcalendar10) attributes */
VFormatAttribute *handle_xml_alarm_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	// TODO
	return NULL;
}

VFormatAttribute *handle_xml_prodid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "PRODID", encoding);
}

VFormatAttribute *handle_xml_method_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "METHOD", encoding);
}

VFormatAttribute *handle_xml_geo_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling location xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "GEO");
	add_value(attr, xmlfield, "Latitude", encoding);
	add_value(attr, xmlfield, "Longitude", encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_dtstamp_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "DTSTAMP", encoding);
}

VFormatAttribute *handle_xml_description_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "DESCRIPTION", encoding);
}

VFormatAttribute *handle_xml_summary_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "SUMMARY", encoding);
}

VFormatAttribute *handle_xml_due_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
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

VFormatAttribute *handle_xml_dtstart_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
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

VFormatAttribute *handle_xml_percent_complete_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "PERCENT-COMPLETE", encoding);
}

VFormatAttribute *handle_xml_priority_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "PRIORITY", encoding);
}

VFormatAttribute *handle_xml_sequence_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "SEQUENCE", encoding);
}

VFormatAttribute *handle_xml_last_modified_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "LAST-MODIFIED", encoding);
}

VFormatAttribute *handle_xml_created_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "CREATED", encoding);
}

VFormatAttribute *handle_xml_dcreated_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "DCREATED", encoding);
}

VFormatAttribute *handle_xml_rrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling \"RRULE\" xml attribute");
	return convert_xml_rrule_to_ical(vevent, xmlfield, "RRULE", encoding); 
}

VFormatAttribute *handle_xml_rdate_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RDATE", encoding);
}

VFormatAttribute *handle_xml_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "LOCATION", encoding);
}

VFormatAttribute *handle_xml_completed_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "COMPLETED", encoding);
}

VFormatAttribute *handle_xml_organizer_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "ORGANIZER", encoding);
}

VFormatAttribute *handle_xml_recurid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RECURRENCE-ID", encoding);
}

VFormatAttribute *handle_xml_status_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "STATUS", encoding);
}

VFormatAttribute *handle_xml_duration_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "DURATION", encoding);
}

VFormatAttribute *handle_xml_attach_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "ATTACH", encoding);
}

VFormatAttribute *handle_xml_attendee_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "ATTENDEE", encoding);
}

VFormatAttribute *handle_xml_event_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "CONTACT", encoding);
}

VFormatAttribute *handle_xml_exdate_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "EXDATE", encoding);
}

VFormatAttribute *handle_xml_exrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "EXRULE", encoding);
}

VFormatAttribute *handle_xml_rstatus_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RSTATUS", encoding);
}

VFormatAttribute *handle_xml_related_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RELATED-TO", encoding);
}

VFormatAttribute *handle_xml_resources_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "RESOURCES", encoding);
}

VFormatAttribute *handle_xml_dtend_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
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

VFormatAttribute *handle_xml_transp_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "TRANSP", encoding);
}

VFormatAttribute *handle_xml_calscale_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "CALSCALE", encoding);
}
/* END: xml -> vcalendar20 (and vcalendar10) attributes */



/* BEGIN: alarm attributes (vcalendar20) -> xml */
void handle_alarm_action_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr) 
{ 
	// Rename xmlfield to AlarmDisplay/AlarmAudio/AlarmEmail/AlarmProcedure
	// We need this to make an own schema for each type.
	if(!strcmp(vformat_attribute_get_nth_value(attr, 0),"DISPLAY")) {
		osync_xmlfield_set_name(xmlfield, "AlarmDisplay");
	} else if(!strcmp(vformat_attribute_get_nth_value(attr, 0),"AUDIO")) {
		osync_xmlfield_set_name(xmlfield, "AlarmAudio");
	} else if(!strcmp(vformat_attribute_get_nth_value(attr, 0),"EMAIL")) {
		osync_xmlfield_set_name(xmlfield, "AlarmEmail");
	} else if(!strcmp(vformat_attribute_get_nth_value(attr, 0),"PROCEDURE")) {
		osync_xmlfield_set_name(xmlfield, "AlarmProcedure");
	}
}

void handle_alarm_attach_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr) 
{ 
	handle_simple_xmlfield(xmlfield, attr, "AlarmAttach");
}

void handle_alarm_attendee_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr) 
{ 
	handle_simple_xmlfield(xmlfield, attr, "AlarmAttendee");
}

void handle_alarm_description_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr) 
{ 
	handle_simple_xmlfield(xmlfield, attr, "AlarmDescription");
}

void handle_alarm_duration_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr) 
{ 
	handle_simple_xmlfield(xmlfield, attr, "AlarmRepeatDuration");
}

void handle_alarm_repeat_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr) 
{ 
	handle_simple_xmlfield(xmlfield, attr, "AlarmRepeat");
}

void handle_alarm_summary_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr) 
{ 
	handle_simple_xmlfield(xmlfield, attr, "AlarmSummary");
}

void handle_alarm_trigger_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr) 
{ 
	handle_simple_xmlfield(xmlfield, attr, "AlarmTrigger");
}
/* END: alarm attributes (vcalendar20) -> xml */



/* BEGIN: xml -> alarm attributes (vcalendar20) */
VFormatAttribute *handle_xml_alarm_trigger_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vevent, xmlfield, "TRIGGER", encoding);
}

VFormatAttribute *handle_xml_alarm_repeat_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "REPEAT");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_alarm_duration_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DURATION");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_alarm_action_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ACTION");
	/* FIXME add_Value() #3 NULL is wrong */
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_alarm_attach_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTACH");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_alarm_description_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DESCRIPTION");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_alarm_attendee_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ATTENDEE");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_alarm_summary_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "SUMMARY");
	/* FIXME add_Value() #3 NULL is wrong */

	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}
/* END: xml -> alarm attributes (vcalendar20) */



/* BEGIN: timezone attributes (vcalendar20) -> xml */
void handle_tz_comment_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	handle_simple_xmlfield(xmlfield, attr, "Comment");
}

void handle_tz_dtstart_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	handle_simple_xmlfield(xmlfield, attr, "DateTimeStart");
}

void handle_tz_id_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	// We need to set an attribute here, see vcalendar_parse_attributes
	osync_xmlfield_set_attr(xmlfield, "TimezoneID", vformat_attribute_get_nth_value(attr, 0));
}

void handle_tz_last_modified_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	handle_simple_xmlfield(xmlfield, attr, "LastModified");
}

void handle_tz_location_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	handle_simple_xmlfield(xmlfield, attr, "X-Location");
}

void handle_tz_name_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	handle_simple_xmlfield(xmlfield, attr, "TZName");
}

void handle_tz_offsetfrom_location_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	handle_simple_xmlfield(xmlfield, attr, "TZOffsetFrom");
}

void handle_tz_offsetto_location_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	handle_simple_xmlfield(xmlfield, attr, "TZOffsetTo");
}

void handle_tz_rdate_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	handle_simple_xmlfield(xmlfield, attr, "TimezoneDate");
}

void handle_tz_url_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr)
{
	handle_simple_xmlfield(xmlfield, attr, "TimezomeUrl");
}

// this function get called by vcalendar_parse_component 
static OSyncXMLField *convert_ical_tz_rrule_to_xml(OSyncXMLFormat *xmlformat, VFormatAttribute *attr) 
{
	osync_trace(TRACE_INTERNAL, "Handling TimezoneRule attribute");

	return convert_ical_rrule_to_xml(xmlformat, attr, "TimezoneRule", NULL);
}
/* END: timezone attributes (vcalendar20) -> xml */



/* BEGIN: xml -> timezone attributes (vcalendar20) */
VFormatAttribute *handle_xml_tz_id_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	/* TODO implement XMLFormat TZID handler */
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZID");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tz_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-LIC-LOCATION");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tz_offsetfrom_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZOFFSETFROM");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tz_offsetto_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZOFFSETTO");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tz_name_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZNAME");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tz_dtstart_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DTSTART");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tz_rrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RRULE");
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tz_last_modified_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LAST-MODIFIED");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tz_url_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZURL");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}

VFormatAttribute *handle_xml_tz_rdate_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "RDATE");
	add_value(attr, xmlfield, NULL, encoding);
	vformat_add_attribute(vevent, attr);
	return attr;
}
/* END: xml -> timezone attributes (vcalendar20) */






/* The following functions are used by vcalendar 10, vcalender20 and vnote11 */
static void vcalendar_parse_component(OSyncXMLField *xmlfield, GList **attributes, OSyncHookTables *hooks, GHashTable *attrtable, GHashTable *paramtable)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, xmlfield, attributes, hooks, attrtable, paramtable);

	GList *a = NULL;
	for (a = *attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;

		if (!strcmp(vformat_attribute_get_name(attr), "BEGIN")) {
			osync_trace(TRACE_EXIT, "%s: Found BEGIN:%s", __func__, vformat_attribute_get_nth_value(attr, 0));
			*attributes = a->prev;

			// Sorting is required to have a valid XMLFormat.
			osync_xmlfield_sort(xmlfield);

			return;
		} else if (!strcmp(vformat_attribute_get_name(attr), "END")) {
			osync_trace(TRACE_EXIT, "%s: Found END:%s", __func__, vformat_attribute_get_nth_value(attr, 0));
			*attributes = a;

			// Sorting is required to have a valid XMLFormat.
			osync_xmlfield_sort(xmlfield);

			return;
		} else {
			osync_trace(TRACE_INTERNAL, "Attribute: \"%s\"", vformat_attribute_get_name(attr));
			handle_component_attribute(attrtable, paramtable, xmlfield, attr, NULL);
		}
	}
}

static void vcalendar_parse_component_tz(OSyncXMLField *xmlfield, GList **attributes, OSyncHookTables *hooks, GHashTable *attrtable, GHashTable *paramtable, OSyncXMLFormat *xmlformat)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p, %p)", __func__, xmlfield, attributes, hooks, attrtable, paramtable, xmlformat);

	GList *a = NULL;
	for (a = *attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;

		if (!strcmp(vformat_attribute_get_name(attr), "BEGIN")) {
			osync_trace(TRACE_EXIT, "%s: Found BEGIN:%s", __func__, vformat_attribute_get_nth_value(attr, 0));
			*attributes = a->prev;

			// Sorting is required to have a valid XMLFormat.
			osync_xmlfield_sort(xmlfield);

			return;
		} else if (!strcmp(vformat_attribute_get_name(attr), "END")) {
			osync_trace(TRACE_EXIT, "%s: Found END:%s", __func__, vformat_attribute_get_nth_value(attr, 0));
			*attributes = a;

			// Sorting is required to have a valid XMLFormat.
			osync_xmlfield_sort(xmlfield);

			return;

		// We have to create a new root node 'TimezoneRule'
		} else if (xmlformat && !strcmp(vformat_attribute_get_name(attr), "RRULE")) {
			OSyncXMLField *tzrrule = convert_ical_tz_rrule_to_xml(xmlformat, attr);
			osync_xmlfield_set_attr(tzrrule, "TZComponent", osync_xmlfield_get_attr(xmlfield, "TZComponent"));
			osync_xmlfield_set_attr(tzrrule, "TimezoneID", osync_xmlfield_get_attr(xmlfield, "TimezoneID"));
		} else {
			osync_trace(TRACE_INTERNAL, "Attribute: \"%s\"", vformat_attribute_get_name(attr));
			handle_component_attribute(attrtable, paramtable, xmlfield, attr, NULL);
		}
	}
}

void vcalendar_parse_attributes(OSyncXMLFormat *xmlformat, GList **attributes, OSyncHookTables *hooks, GHashTable *attrtable, GHashTable *paramtable)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, xmlformat, attributes, hooks, attrtable, paramtable);
	
	const char *tzid = NULL;
	GList *a = NULL;
	for (a = *attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;

		// Component VTIMEZONE (incl. DAYLIGHT/STANDARD) and VALARM need another hooktable
		if (!strcmp(vformat_attribute_get_name(attr), "BEGIN")) {
			
			const char *component = vformat_attribute_get_nth_value(attr, 0);
			
			osync_trace(TRACE_INTERNAL, "Attribute: \"BEGIN\", Component:\"%s\"", component);

			// We create another xmlfield which acts as a root node for the components
			OSyncXMLField *component_xmlfield = NULL;

			// vcalendar_parse_component use component specific hooktables
			if (!strcmp(component, "VALARM")) {
				a = a->next;
				component_xmlfield = osync_xmlfield_new(xmlformat, "Alarm", NULL);
				vcalendar_parse_component(component_xmlfield, &a, hooks, hooks->alarmtable, paramtable);
			} else if (!strcmp(component, "VTIMEZONE")) {
				a = a->next;
				component_xmlfield = osync_xmlfield_new(xmlformat, "Timezone", NULL);
				vcalendar_parse_component(component_xmlfield, &a, hooks, hooks->tztable, paramtable);
				tzid = osync_xmlfield_get_nth_attr_value(component_xmlfield, 0);
			} else if (!strcmp(component, "STANDARD")) {
				a = a->next;
				component_xmlfield = osync_xmlfield_new(xmlformat, "TimezoneComponent", NULL);
				osync_xmlfield_set_attr(component_xmlfield, "TZComponent", "Standard");
				osync_xmlfield_set_attr(component_xmlfield, "TimezoneID", tzid);
				// We call vcalendar_parse_component_tz, because it creates a new root node 'TimezoneRule' and need an OSyncXMLFormat.
				vcalendar_parse_component_tz(component_xmlfield, &a, hooks, hooks->tztable, paramtable, xmlformat);
			} else if (!strcmp(component, "DAYLIGHT")) {
				a = a->next;
				component_xmlfield = osync_xmlfield_new(xmlformat, "TimezoneComponent", NULL);
				osync_xmlfield_set_attr(component_xmlfield, "TZComponent", "Daylight");
				osync_xmlfield_set_attr(component_xmlfield, "TimezoneID", tzid);
				// We call vcalendar_parse_component_tz, because it creates a new root node 'TimezoneRule' and need an OSyncXMLFormat.
				vcalendar_parse_component_tz(component_xmlfield, &a, hooks, hooks->tztable, paramtable, xmlformat);
			}

		} else if (!strcmp(vformat_attribute_get_name(attr), "END")) {
			osync_trace(TRACE_INTERNAL, "Attribute: \"END\", Component:\"%s\"", vformat_attribute_get_nth_value(attr, 0));
			*attributes = a;
		} else {
			osync_trace(TRACE_INTERNAL, "Attribute: \"%s\"", vformat_attribute_get_name(attr));
			handle_attribute(attrtable, paramtable, xmlformat, attr, NULL);
		}
	}

	osync_trace(TRACE_EXIT, "%s: Done", __func__);
}



/* Review and remove old and unused code */
/*
static void xml_handle_unknown_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown xml parameter");
	char *content = (char*)OSyncXMLFieldGetContent(xmlfield);
	vformat_attribute_add_param_with_value(attr, (char*)xmlfield->name, content);
	g_free(content);
}

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

