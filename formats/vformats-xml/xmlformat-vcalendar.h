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

// vCalendar handler
OSyncXMLField *handle_vcal_aalarm_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_vcal_dalarm_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_vcal_rrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_vcal_due_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
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
OSyncXMLField *handle_arepeat_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_atrigger_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_attach_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_attendee_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_comment_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_created_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
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

// TIMEZONE handler
OSyncXMLField *handle_tzid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_tz_last_modified_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_tzurl_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_tzdtstart_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_tzoffsetto_location_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_tzoffsetfrom_location_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_tzrdate_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_tzrrule_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_tzname_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_tz_location_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);

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

void insert_attr_handler(GHashTable *table, const char *attrname, void* handler);

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

void insert_xml_attr_handler(GHashTable *table, const char *name, void *handler);

void vcalendar_parse_attributes(OSyncHookTables *hooks, GHashTable *table, OSyncXMLFormat *xmlformat, GHashTable *paramtable, GList **attributes);

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

