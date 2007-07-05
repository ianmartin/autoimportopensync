/*
 * xmlformat-vcalendar - common code for xmlformat-vevent*, -vnote*, -vtodo*
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007  Daniel Gollub <dgollub@suse.de>
 * Copyright (C) 2007  Christopher Stender <cstender@suse.de>
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

#ifndef XMLFORMAT_VCALENDAR_H_
#define XMLFORMAT_VCALENDAR_H_

#include "xmlformat-common.h"

// Handler functions pointer
typedef void (* param_handler_fn) (OSyncXMLField *xmlfield, VFormatParam *param);
typedef OSyncXMLField * (* attr_handler_fn) (OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
typedef void (* attr_component_handler_fn) (OSyncXMLField *xmlfield, VFormatAttribute *attr);

// vCalendar handler
OSyncXMLField *handle_vcal_aalarm_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_vcal_dalarm_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_vcal_rrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
void handle_vcal_type_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_vcal_value_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_vcal_encoding_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_vcal_charset_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_vcal_language_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_vcal_role_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_vcal_status_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_vcal_rsvp_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_vcal_expect_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_related_parameter(OSyncXMLField *xmlfield, VFormatParam *param); //FIXME

// vCalendar and iCalendar handler
OSyncXMLField *handle_attach_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_attendee_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_comment_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_created_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_due_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_dtstamp_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_dtstart_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_description_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_summary_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_duration_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_priority_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_sequence_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_last_modified_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_geo_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_prodid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_rdate_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_location_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_completed_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_status_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_exdate_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_exrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_rstatus_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_related_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_resources_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_dtend_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_transp_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_method_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_percent_complete_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_rrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_organizer_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_recurid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_contact_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 

// Alarm component handler
void handle_aaction_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_adescription_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_atrigger_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_aattach_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_aduration_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_arepeat_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_aattendee_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_asummary_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);

// TIMEZONE handler
void handle_tzid_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_tz_last_modified_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_tzurl_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_tzdtstart_attribute(OSyncXMLField *xmlformat, VFormatAttribute *attr);
void handle_tzoffsetto_location_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_tzoffsetfrom_location_attribute(OSyncXMLField *xmlformat, VFormatAttribute *attr);
void handle_tzrdate_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_tzrrule_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_tzname_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);
void handle_tz_location_attribute(OSyncXMLField *xmlfield, VFormatAttribute *attr);

// parameter
void handle_range_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_tzid_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_cn_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_dir_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_sent_by_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_language_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_altrep_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_format_type_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_encoding_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_role_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_partstat_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_delegated_from_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_delegated_to_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_cutype_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_rsvp_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_reltype_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_member_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_fb_type_parameter(OSyncXMLField *xmlfield, VFormatParam *param);
void handle_trigrel_parameter(OSyncXMLField *xmlfield, VFormatParam *param);

void insert_param_handler(GHashTable *table, const char *paramname, param_handler_fn handler);
void insert_attr_handler(GHashTable *table, const char *attrname, attr_handler_fn handler);
void insert_attr_component_handler(GHashTable *table, const char *attrname, attr_component_handler_fn handler);


VFormatAttribute *handle_xml_alarm_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_prodid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_method_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_alarm_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_geo_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_dtstamp_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_description_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_summary_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_due_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_dtstart_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_percent_complete_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_priority_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_sequence_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_last_modified_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_created_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_rrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_rdate_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_completed_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_organizer_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_recurid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_status_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_duration_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_attach_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_attendee_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_event_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_exdate_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_exrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_rstatus_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_related_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_resources_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_dtend_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_transp_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_calscale_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_tzid_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_tz_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_tzoffsetfrom_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_tzoffsetto_location_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_tzname_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_tzdtstart_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_tzrrule_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_tz_last_modified_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_tzurl_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_tzrdate_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_atrigger_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_arepeat_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_aduration_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_aaction_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_aattach_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_adescription_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_aattendee_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_asummary_attribute(VFormat *vevent, OSyncXMLField *xmlfield, const char *encoding);

void handle_xml_tzid_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_altrep_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_cn_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_delegated_from_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_delegated_to_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_dir_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_format_type_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_fb_type_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_member_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_partstat_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_range_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_reltype_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_related_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_role_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_rsvp_parameter(VFormatAttribute *attr, xmlNode *current);
void handle_xml_sent_by_parameter(VFormatAttribute *attr, xmlNode *current);

void insert_xml_attr_handler(GHashTable *table, const char *name, void *handler);

void vcalendar_parse_attributes(OSyncXMLFormat *xmlformat, GList **attributes, OSyncHookTables *hooks, GHashTable *attrtable, GHashTable *paramtable);

/*
typedef struct OSyncHookTables OSyncHookTables;


struct OSyncHookTables {
	GHashTable *table;
	GHashTable *tztable;
	GHashTable *comptable;
	GHashTable *compparamtable;
	GHashTable *alarmtable;

	GHashTable *parameters;
	GHashTable *attributes;
};

#define HANDLE_IGNORE (void *)1
*/
#endif //XMLFORMAT_VCALENDAR_H_

