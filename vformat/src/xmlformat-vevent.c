/*
 * xmlformat-vevent - convert vevent* to xmlformat-event and backwards
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

#include "xmlformat-vevent.h"

static OSyncHookTables *init_vcalendar_to_xmlformat(VFormatType target)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);

	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));
	
	// create hashtables for attributes and parameters
	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->tztable = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->alarmtable = g_hash_table_new(g_str_hash, g_str_equal);


	if (target == VFORMAT_EVENT_10 || target == VFORMAT_TODO_10) {

	/* vCalendar
	 * ---------
	 * The following attributes and parameters are described in vcal-10.txt.
	 * You can download this file from http://www.imc.org/pdi/vcal-10.txt
	 */

        // [vcal-1.0] param (same order as in sepc!)
	insert_param_handler(hooks->parameters, "TYPE", handle_vcal_type_parameter);
	insert_param_handler(hooks->parameters, "VALUE", handle_vcal_value_parameter); 
	// QUOTED-PRINTABLE is already handled by vformat.c
	insert_param_handler(hooks->parameters, "ENCODING=QUOTED-PRINTABLE", HANDLE_IGNORE);
	insert_param_handler(hooks->parameters, "ENCODING", handle_vcal_encoding_parameter);
	// CHARSET is already handled by vformat.c
	insert_param_handler(hooks->parameters, "CHARSET", HANDLE_IGNORE); // handle_vcal_charset_parameter
	insert_param_handler(hooks->parameters, "LANGUAGE", handle_vcal_language_parameter);
	insert_param_handler(hooks->parameters, "ROLE", handle_vcal_role_parameter); // (ATTENDEE)
	insert_param_handler(hooks->parameters, "STATUS", handle_vcal_status_parameter); // (ATTENDEE)
	insert_param_handler(hooks->parameters, "RSVP", handle_vcal_rsvp_parameter); // (ATTENDEE)
	insert_param_handler(hooks->parameters, "EXPECT", handle_vcal_expect_parameter); // (ATTENDEE)

        // [vcal-1.0] vcal (same order as in spec!)
	insert_attr_handler(hooks->attributes, "BEGIN", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "END", HANDLE_IGNORE);

        // [vcal-1.0] calprop (same order as in spec!)
	insert_attr_handler(hooks->attributes, "DAYLIGHT", HANDLE_IGNORE); // TODO
	insert_attr_handler(hooks->attributes, "GEO", handle_geo_attribute);
	insert_attr_handler(hooks->attributes, "PRODID", handle_prodid_attribute);
	insert_attr_handler(hooks->attributes, "TZ", HANDLE_IGNORE); // TODO
	insert_attr_handler(hooks->attributes, "VERSION", HANDLE_IGNORE);

        // [vcal-1.0] simprop (same order as in spec!)
	insert_attr_handler(hooks->attributes, "ATTACH", handle_attach_attribute);
	insert_attr_handler(hooks->attributes, "ATTENDEE", handle_attendee_attribute);
	insert_attr_handler(hooks->attributes, "DCREATED", handle_created_attribute);
	insert_attr_handler(hooks->attributes, "COMPLETED", handle_completed_attribute); //vtodo only
	insert_attr_handler(hooks->attributes, "DESCRIPTION", handle_description_attribute);
	insert_attr_handler(hooks->attributes, "DUE", handle_due_attribute); //vtodo only
	insert_attr_handler(hooks->attributes, "DTEND", handle_dtend_attribute);
	insert_attr_handler(hooks->attributes, "EXRULE", handle_exrule_attribute);
	insert_attr_handler(hooks->attributes, "LAST-MODIFIED", handle_last_modified_attribute);
	insert_attr_handler(hooks->attributes, "LOCATION", handle_location_attribute);
	insert_attr_handler(hooks->attributes, "RNUM", handle_rnum_attribute);
	insert_attr_handler(hooks->attributes, "PRIORITY", handle_priority_attribute);
	insert_attr_handler(hooks->attributes, "RELATED-TO", handle_related_attribute);
	insert_attr_handler(hooks->attributes, "RRULE", handle_vcal_rrule_attribute);
	insert_attr_handler(hooks->attributes, "SEQUENCE", handle_sequence_attribute);
	insert_attr_handler(hooks->attributes, "DTSTART", handle_dtstart_attribute);
	insert_attr_handler(hooks->attributes, "SUMMARY", handle_summary_attribute);
	insert_attr_handler(hooks->attributes, "TRANSP", handle_transp_attribute);
	insert_attr_handler(hooks->attributes, "URL", handle_url_attribute);
	insert_attr_handler(hooks->attributes, "UID", handle_uid_attribute);
	// X-Word

        // [vcal-1.0] entprop (same order as in spec!)
	insert_attr_handler(hooks->attributes, "AALARM", handle_vcal_alarm_attribute);
	insert_attr_handler(hooks->attributes, "CATEGORIES", handle_categories_attribute);
	insert_attr_handler(hooks->attributes, "CLASS", handle_class_attribute);
	insert_attr_handler(hooks->attributes, "DALARM", handle_vcal_alarm_attribute);
	insert_attr_handler(hooks->attributes, "EXDATE", handle_exdate_attribute);
	insert_attr_handler(hooks->attributes, "MALARM", handle_vcal_alarm_attribute);
	insert_attr_handler(hooks->attributes, "PALARM", handle_vcal_alarm_attribute);
	insert_attr_handler(hooks->attributes, "RDATE", handle_rdate_attribute);
	insert_attr_handler(hooks->attributes, "RESOURCES", handle_resources_attribute);
	insert_attr_handler(hooks->attributes, "STATUS", handle_status_attribute);

	} else if (target == VFORMAT_EVENT_20 || target == VFORMAT_TODO_20 || target == VFORMAT_JOURNAL) {

	/* iCalendar
	 * ---------
	 * The following attributes and parameters are described in RFC 2445
	 * ERRATA: http://www.rfc-editor.org/cgi-bin/errataSearch.pl?rfc=2445
	 */

	// [RFC 2445] 3.2 Parameters (same order as in spec!)
	// NOTE: non required
	// charset -> defined in [RFC 2046]
	// method -> must be the same as METHOD in the iCalendar object
	// component -> must be specified if the iCal object contains more than one component
	// optinfo -> optional information

	// [RFC 2445] 4.1.4 Character Set
	// There is not a property parameter to declare the character set used
	// in a property value. The default character set for an iCalendar
	// object is UTF-8 as defined in [RFC 2279].
	// The "charset" Content-Type parameter can be used in MIME transports
	// to specify any other IANA registered character set.

	// [RFC 2445] 4.2 Property Parameters (same order as in spec!)
	insert_param_handler(hooks->parameters, "ALTREP", handle_altrep_parameter); // altrepparam
	insert_param_handler(hooks->parameters, "CN", handle_cn_parameter); // cnparam
	insert_param_handler(hooks->parameters, "CUTYPE", handle_cutype_parameter); // cutypeparam
	insert_param_handler(hooks->parameters, "DELEGATED-FROM", handle_delegated_from_parameter); // delfromparam
	insert_param_handler(hooks->parameters, "DELEGATED-TO", handle_delegated_to_parameter); // deltoparam
	insert_param_handler(hooks->parameters, "DIR", handle_dir_parameter); // dirparam
	insert_param_handler(hooks->parameters, "ENCODING", handle_encoding_parameter); // encodingparam
	insert_param_handler(hooks->parameters, "FMTTYPE", handle_format_type_parameter); // fmttypeparam
	insert_param_handler(hooks->parameters, "FMTYPE", handle_format_type_parameter); // same as fmttypeparam -> see Errata
	insert_param_handler(hooks->parameters, "FBTYPE", handle_fb_type_parameter); // fbtypeparam -> TODO xsd
	insert_param_handler(hooks->parameters, "LANGUAGE", handle_language_parameter); // languageparam
	insert_param_handler(hooks->parameters, "MEMBER", handle_member_parameter); // memberparam
	insert_param_handler(hooks->parameters, "PARTSTAT", handle_partstat_parameter); // partstatparam
	insert_param_handler(hooks->parameters, "RANGE", handle_range_parameter); // rangeparam
	insert_param_handler(hooks->parameters, "RELATED", handle_trigrel_parameter); // trigrelparam
	insert_param_handler(hooks->parameters, "RELTYPE", handle_reltype_parameter); // reltypeparam
	insert_param_handler(hooks->parameters, "ROLE", handle_role_parameter); // roleparam
	insert_param_handler(hooks->parameters, "RSVP", handle_rsvp_parameter); // rsvpparam
	insert_param_handler(hooks->parameters, "SENT-BY", handle_sent_by_parameter); // sentbyparam
	insert_param_handler(hooks->parameters, "TZID", handle_tzid_parameter); // tzidparam
	insert_param_handler(hooks->parameters, "VALUE", handle_value_parameter); // valuetypeparam -> TODO fix xsd to allow all value types
	// ianaparam 
	// xparam


	// [RFC 2445] icalobject (same order as in spec!)
	// BEGIN:VCALENDAR
	// -> icalbody
	// END:VCALENDAR


	// [RFC 2445] icalbody (same order as in spec!)
	// icalbody = calprops component

	
	// [RFC 2445] calprop (same order as in spec!)
	// calprops = 2*
	// NOTE: required, but most not occur more than once
	insert_attr_handler(hooks->attributes, "PRODID", handle_prodid_attribute);
	insert_attr_handler(hooks->attributes, "VERSION", HANDLE_IGNORE);
	// NOTE: optional, but MUST NOT occur than once
	insert_attr_handler(hooks->attributes, "CALSCALE", handle_calscale_attribute);
	insert_attr_handler(hooks->attributes, "METHOD", handle_method_attribute);
	// x-prop


	// [RFC 2445] component (same order as in spec!)	
	// component = 1*(eventc / todoc / journalc / freebusyc / timezonec / iana-comp / x-comp)
	

	// [RFC 2445] iana-comp (same order as in spec!) -> TODO
	// BEGIN : iana-token
	// 1*contentline
	// END : iana-token
	

	// [RFC 2445] x-comp (same order as in spec!) -> TODO
	// BEGIN : x-name
	// 1*contentline
	// END : x-name


        // [RFC 2445] eventc (same order as in spec!)
	insert_attr_handler(hooks->attributes, "BEGIN", HANDLE_IGNORE);
	// -> eventprop *alarmc
	insert_attr_handler(hooks->attributes, "END", HANDLE_IGNORE);


	// [RFC 2445] eventprop (same order as in spec!)
	// NOTE: optional, but most not occur more than once
	insert_attr_handler(hooks->attributes, "CLASS", handle_class_attribute);
	insert_attr_handler(hooks->attributes, "CREATED", handle_created_attribute);
	insert_attr_handler(hooks->attributes, "DESCRIPTION", handle_description_attribute);
	insert_attr_handler(hooks->attributes, "DTSTART", handle_dtstart_attribute);
	insert_attr_handler(hooks->attributes, "GEO", handle_geo_attribute);
	insert_attr_handler(hooks->attributes, "LAST-MODIFIED", handle_last_modified_attribute);
	insert_attr_handler(hooks->attributes, "LOCATION", handle_location_attribute);
	insert_attr_handler(hooks->attributes, "ORGANIZER", handle_organizer_attribute);
	insert_attr_handler(hooks->attributes, "PRIORITY", handle_priority_attribute); 
	insert_attr_handler(hooks->attributes, "DTSTAMP", handle_dtstamp_attribute);
	insert_attr_handler(hooks->attributes, "SEQUENCE", handle_sequence_attribute);
	insert_attr_handler(hooks->attributes, "STATUS", handle_status_attribute);
	insert_attr_handler(hooks->attributes, "SUMMARY", handle_summary_attribute);
	insert_attr_handler(hooks->attributes, "TRANSP", handle_transp_attribute);
	insert_attr_handler(hooks->attributes, "UID", handle_uid_attribute);
	insert_attr_handler(hooks->attributes, "URL", handle_url_attribute);
	insert_attr_handler(hooks->attributes, "RECURRENCE-ID", handle_recurid_attribute);
	// NOTE: dtend or duration may appear in a eventprop
	insert_attr_handler(hooks->attributes, "DTEND", handle_dtend_attribute);
	insert_attr_handler(hooks->attributes, "DURATION", handle_duration_attribute);
	// NOTE: optional and may occur more than once
	insert_attr_handler(hooks->attributes, "ATTACH", handle_attach_attribute);
	insert_attr_handler(hooks->attributes, "ATTENDEE", handle_attendee_attribute);
	insert_attr_handler(hooks->attributes, "CATEGORIES", handle_categories_attribute);
	insert_attr_handler(hooks->attributes, "COMMENT", handle_comment_attribute);
	insert_attr_handler(hooks->attributes, "CONTACT", handle_contact_attribute);
	insert_attr_handler(hooks->attributes, "EXDATE", handle_exdate_attribute);
	insert_attr_handler(hooks->attributes, "EXRULE", handle_exrule_attribute);
	insert_attr_handler(hooks->attributes, "REQUEST-STATUS", handle_rstatus_attribute);
	insert_attr_handler(hooks->attributes, "RELATED-TO", handle_related_attribute);
	insert_attr_handler(hooks->attributes, "RESOURCES", handle_resources_attribute);
	insert_attr_handler(hooks->attributes, "RDATE", handle_rdate_attribute);
	insert_attr_handler(hooks->attributes, "RRULE", handle_rrule_attribute);
	// x-prop


	// [RFC 2445] todoc (same order as in spec!)
	// BEGIN
	// -> todoprop *alarmc
	// END


	// [RFC 2445] todoprop (same order as in spec!)
	// NOTE: optional, but most not occur more than once
	// class
	insert_attr_handler(hooks->attributes, "COMPLETED", handle_completed_attribute);
	// created
	// description
	// dtstamp
	// dtstart
	// geo
	// last-mod
	// location
	// organizer
	insert_attr_handler(hooks->attributes, "PERCENT-COMPLETE", handle_percent_complete_attribute);
	// priority
	// recurid
	// seq
	// status
	// summary
	// uid
	// url
	// NOTE: due or duration may appear in a todoprop
	insert_attr_handler(hooks->attributes, "DUE", handle_due_attribute);
	// duration
	// NOTE: optional and may occur more than once
	// attach
	// attendee
	// categories
	// comment
	// contact
	// exdate
	// exrule
	// rstatus
	// related
	// resources
	// rdate
	// rrule
	// x-prop


	// [RFC 2445] journalc (same order as in spec!)
	// BEGIN
	// -> jourprop
	// END


	// [RFC 2445] jourprop (same order as in spec!)
	// NOTE: optional, but most not occur more than once
	// class
	// created
	// description
	// dtstart
	// dtstamp
	// last-mod
	// organizer
	// recurid
	// seq
	// status
	// summary
	// uid
	// url
	// NOTE: optional and may occur more than once
	// attach
	// attendee
	// categories
	// comment
	// contact
	// exdate
	// exrule
	// related
	// rdate
	// rule
	// rstatus
	// x-prop
	 

	// [RFC 2445] freebusyc (same order as in spec)
	// BEGIN
	// -> fbprop
	// END


	// [RFC 2445] fbprop (same order as in spec!)
	// NOTE: optional, but most not occur more than once
	// contact
	// dtstart
	// dtend
	// duration
	// dtstamp
	// organizer
	// uid
	// url
	// NOTE: optional and may occur more than once
	// attendee
	// comment
	insert_attr_handler(hooks->attributes, "FREEBUSY", HANDLE_IGNORE); // TODO
	// rstatus
	// x-prop


	// [RFC 2445] timezonec (same order as in spec!)
	insert_attr_component_handler(hooks->tztable, "BEGIN", HANDLE_IGNORE);
	// NOTE: tzid is required, but most not occur more than once
	insert_attr_component_handler(hooks->tztable, "TZID", handle_tz_id_attribute);
	// NOTE: lastmod and tzurl are optional, but most not occur more than once
	insert_attr_component_handler(hooks->tztable, "LAST-MODIFIED", handle_tz_last_modified_attribute);
	insert_attr_component_handler(hooks->tztable, "TZURL", handle_tz_url_attribute);
	// NOTE: one of 'standardc' or 'daylightc' MUST occur and each MAY occur more than once
	// -> standardc / daylightc
	// NOTE: optional and may occur more than once
	// x-prop
	insert_attr_component_handler(hooks->tztable, "END", HANDLE_IGNORE);


	// [RFC 2445] standardc (same order as in spec!)
	// BEGIN
	// -> tzprop
	// END


	// [RFC 2445] daylightc (same order as in spec!)
	// BEGIN
	// -> tzprop
	// END


	// [RFC 2445] tzprop (same order as in spec!)
	// NOTE: all required, but they most not occur more than once
	insert_attr_component_handler(hooks->tztable, "DTSTART", handle_tz_dtstart_attribute);
	insert_attr_component_handler(hooks->tztable, "TZOFFSETTO", handle_tz_offsetto_location_attribute);
	insert_attr_component_handler(hooks->tztable, "TZOFFSETFROM", handle_tz_offsetfrom_location_attribute);
	// NOTE: optional and may occur more than once
	insert_attr_component_handler(hooks->tztable, "COMMENT", handle_tz_comment_attribute);
	insert_attr_component_handler(hooks->tztable, "RDATE", handle_tz_rdate_attribute);
	insert_attr_component_handler(hooks->tztable, "RRULE", HANDLE_IGNORE); // NOTE: we call it in vcalendar_parse_component
	insert_attr_component_handler(hooks->tztable, "TZNAME", handle_tz_name_attribute);
	insert_attr_component_handler(hooks->tztable, "X-LIC-LOCATION", handle_tz_location_attribute);
	// x-prop


	// [RFC 2445] alarmc (same order as in spec!)
	insert_attr_component_handler(hooks->alarmtable, "BEGIN", HANDLE_IGNORE);
	// -> audioprop / dispprop / emailprop / procprop
	insert_attr_component_handler(hooks->alarmtable, "END", HANDLE_IGNORE);


	// [RFC 2445] audioprop (same order as in spec!)
	// NOTE: action and trigger are required, but must not occur more than once
	insert_attr_component_handler(hooks->alarmtable, "ACTION", handle_alarm_action_attribute);
	insert_attr_component_handler(hooks->alarmtable, "TRIGGER", handle_alarm_trigger_attribute);
	// NOTE: duration and repeat are both optional, and MUST NOT occur
	// NOTE: more than once each, but if one occurs, so MUST the other
	insert_attr_component_handler(hooks->alarmtable, "DURATION", handle_alarm_duration_attribute);
	insert_attr_component_handler(hooks->alarmtable, "REPEAT", handle_alarm_repeat_attribute);
	// NOTE: optional, but must not occur more than once
	insert_attr_component_handler(hooks->alarmtable, "ATTACH", handle_alarm_attach_attribute);
	// NOTE: optional and may occur more than once
	// xprop


	// [RFC 2445] dispprop (same order as in spec!)
	// NOTE: all required, but must not occur more than once
	// action
	insert_attr_component_handler(hooks->alarmtable, "DESCRIPTION", handle_alarm_description_attribute);
	// trigger
	// NOTE: duration and repeat are both optional, and MUST NOT occur
	// NOTE: more than once each, but if one occurs, so MUST the other
	// duration
	// repeat
	// NOTE: optional and may occur more than once
	// x-prop


	// [RFC 2445] emailprop (same order as in spec!)
	// NOTE: all required, but must not occur more than once
	// action
	// description
	// trigger
	insert_attr_component_handler(hooks->alarmtable, "SUMMARY", handle_alarm_summary_attribute);
	// NOTE: required and may occur more than once
	insert_attr_component_handler(hooks->alarmtable, "ATTENDEE", handle_alarm_attendee_attribute);
	// NOTE: duration and repeat are both optional, and MUST NOT occur
	// NOTE: more than once each, but if one occurs, so MUST the other
	// duration
	// repeat
	// NOTE: optional and may occur more than once
	// attach
	// x-prop


	// [RFC 2445] procprop (same order as in spec!)
	// NOTE: all required, but must not occur more than once
	// action
	// attach
	// trigger
	// NOTE: duration and repeat are both optional, and MUST NOT occur
	// NOTE: more than once each, but if one occurs, so MUST the other
	// duration
	// repeat
	// NOTE: optional, but must not occur more than once
	// description
	// NOTE: optional and may occur more than once
	// x-prop

	}
	

	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}


static OSyncHookTables *init_xmlformat_to_vcalendar(VFormatType target)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);

	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));

	// create hashtables for attributes and parameters
	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);

	if (target == VFORMAT_EVENT_10 || target == VFORMAT_TODO_10) {

	/* vCalendar
	 * ---------
	 * The following attributes and parameters are described in vcal-10.txt.
	 * You can download this file from http://www.imc.org/pdi/vcal-10.txt
	 */


        // [vcal-1.0] param (same order as in spec!)
	// special parameter handler for alarms
	insert_xml_attr_handler(hooks->parameters, "FormatType", handle_xml_vcal_formattype_parameter); // (AALARM)
	insert_xml_attr_handler(hooks->parameters, "AttachValue", handle_xml_vcal_attachvalue_parameter); // (AALRM)
	// TODO -> TYPE 
	insert_xml_attr_handler(hooks->parameters, "Value", handle_xml_vcal_value_parameter);
	insert_xml_attr_handler(hooks->parameters, "Encoding", handle_xml_encoding_parameter);
	// TODO -> CHARSET
	insert_xml_attr_handler(hooks->parameters, "Language", handle_xml_language_parameter);
	insert_xml_attr_handler(hooks->parameters, "Role", handle_xml_role_parameter); // (ATTENDEE)
	// TODO -> STATUS // (ATTENDEE)
	insert_xml_attr_handler(hooks->parameters, "Rsvp", handle_xml_vcal_rsvp_parameter); // (ATTENDEE)
	// TODO -> EXPECT // (ATTENDEE)


        // [vcal-1.0] calprop (same order as in spec!)
	// TODO -> DAYLIGHT
	// TODO -> GEO (must be placed before BEGIN attribute!) --> handle_xml_geo_attribute
	// PRODID -> vformat.c
	// TODO -> TZ (must be placed before BEGIN attribute!)
	// VERSION -> vformat.c


        // [vcal-1.0] simprop (same order as in spec!)
	insert_xml_attr_handler(hooks->attributes, "Attach", handle_xml_attach_attribute);
	insert_xml_attr_handler(hooks->attributes, "Attendee", handle_xml_attendee_attribute);
	insert_xml_attr_handler(hooks->attributes, "Created", handle_xml_dcreated_attribute); // vcal only
	insert_xml_attr_handler(hooks->attributes, "Completed", handle_xml_completed_attribute); // vtodo only
	insert_xml_attr_handler(hooks->attributes, "Description", handle_xml_description_attribute);
	insert_xml_attr_handler(hooks->attributes, "Due", handle_xml_due_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateEnd", handle_xml_dtend_attribute);
	insert_xml_attr_handler(hooks->attributes, "ExclusionRule", handle_xml_exrule_attribute);
	insert_xml_attr_handler(hooks->attributes, "LastModified", handle_xml_last_modified_attribute);
	insert_xml_attr_handler(hooks->attributes, "Location", handle_xml_location_attribute);
	// TODO -> RNUM
	insert_xml_attr_handler(hooks->attributes, "Priority", handle_xml_priority_attribute);
	insert_xml_attr_handler(hooks->attributes, "Related", handle_xml_related_attribute); // rename -> related to
	insert_xml_attr_handler(hooks->attributes, "RecurrenceRule", handle_xml_vcal_rrule_attribute);
	insert_xml_attr_handler(hooks->attributes, "Sequence", handle_xml_sequence_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateStarted", handle_xml_dtstart_attribute);
	insert_xml_attr_handler(hooks->attributes, "Summary", handle_xml_summary_attribute);
	insert_xml_attr_handler(hooks->attributes, "TimeTransparency", handle_xml_transp_attribute);
	insert_xml_attr_handler(hooks->attributes, "Url", handle_xml_url_attribute);
	insert_xml_attr_handler(hooks->attributes, "Uid", handle_xml_uid_attribute);
	// X- word


        // [vcal-1.0] entprop (same order as in spec!)
	insert_xml_attr_handler(hooks->attributes, "AlarmAudio", handle_xml_vcal_alarm_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmDisplay", handle_xml_vcal_alarm_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmEmail", handle_xml_vcal_alarm_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmProcedure", handle_xml_vcal_alarm_attribute);
	insert_xml_attr_handler(hooks->attributes, "Categories", handle_xml_categories_attribute);
	insert_xml_attr_handler(hooks->attributes, "Class", handle_xml_class_attribute);
	insert_xml_attr_handler(hooks->attributes, "ExclusionDate", handle_xml_exdate_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceDate", handle_xml_rdate_attribute);
	insert_xml_attr_handler(hooks->attributes, "Resources", handle_xml_resources_attribute);
	insert_xml_attr_handler(hooks->attributes, "Status", handle_xml_status_attribute);


	} else if (target == VFORMAT_EVENT_20 || target == VFORMAT_TODO_20 || target == VFORMAT_JOURNAL) {


	/* iCalendar
	 * ---------
	 * The following attributes and parameters are described in RFC 2445
	 * ERRATA: http://www.rfc-editor.org/cgi-bin/errataSearch.pl?rfc=2445
	 */

	//insert_xml_attr_handler(hooks->attributes, "CalendarScale", handle_xml_calscale_attribute);
	//insert_xml_attr_handler(hooks->attributes, "ProductID", handle_xml_prodid_attribute);
	insert_xml_attr_handler(hooks->attributes, "Method", handle_xml_method_attribute);


	// [RFC 2445] 4.2 Property Parameters (ordered by name!)
	insert_xml_attr_handler(hooks->parameters, "AlternativeTextRep", handle_xml_altrep_parameter);
	insert_xml_attr_handler(hooks->parameters, "CommonName", handle_xml_cn_parameter);
	insert_xml_attr_handler(hooks->parameters, "CUType", handle_xml_cutype_parameter);
	insert_xml_attr_handler(hooks->parameters, "DelegatedFrom", handle_xml_delegated_from_parameter);
	insert_xml_attr_handler(hooks->parameters, "DelegatedTo", handle_xml_delegated_to_parameter);
	insert_xml_attr_handler(hooks->parameters, "Directory", handle_xml_dir_parameter);
	insert_xml_attr_handler(hooks->parameters, "Encoding", handle_xml_encoding_parameter);
	insert_xml_attr_handler(hooks->parameters, "FormatType", handle_xml_format_type_parameter);
	insert_xml_attr_handler(hooks->parameters, "FreeBusyType", handle_xml_fb_type_parameter); // TODO test
	insert_xml_attr_handler(hooks->parameters, "Language", handle_xml_language_parameter);
	insert_xml_attr_handler(hooks->parameters, "Member", handle_xml_member_parameter);
	insert_xml_attr_handler(hooks->parameters, "PartStat", handle_xml_partstat_parameter);
	insert_xml_attr_handler(hooks->parameters, "Range", handle_xml_range_parameter);
	insert_xml_attr_handler(hooks->parameters, "RelatedType", handle_xml_related_parameter);
	insert_xml_attr_handler(hooks->parameters, "RelationshipType", handle_xml_reltype_parameter);
	insert_xml_attr_handler(hooks->parameters, "Role", handle_xml_role_parameter);
	insert_xml_attr_handler(hooks->parameters, "Rsvp", handle_xml_rsvp_parameter);
	insert_xml_attr_handler(hooks->parameters, "SentBy", handle_xml_sent_by_parameter);
	insert_xml_attr_handler(hooks->parameters, "TimezoneID", handle_xml_tzid_parameter);
	insert_xml_attr_handler(hooks->parameters, "Value", handle_xml_value_parameter);
	

	// [RFC 2445] calprop (same order as in spec!)
	// PRODID -> vformat.c
	// VERSION -> vformat.c
	// CALSCALE -> TODO (must be placed before BEGIN attribute!)
	// METHOD -> TODO (must be placed before BEGIN attribute!)
	// x-prop


	// [RFC 2445] all ical props (ordered by name!)
	insert_xml_attr_handler(hooks->attributes, "Attach", handle_xml_attach_attribute);
	insert_xml_attr_handler(hooks->attributes, "Attendee", handle_xml_attendee_attribute);
	insert_xml_attr_handler(hooks->attributes, "Categories", handle_xml_categories_attribute);
	insert_xml_attr_handler(hooks->attributes, "Class", handle_xml_class_attribute);
	// TODO -> comment
	insert_xml_attr_handler(hooks->attributes, "Completed", handle_xml_completed_attribute);
	insert_xml_attr_handler(hooks->attributes, "Contact", handle_xml_event_attribute); // ical only
	insert_xml_attr_handler(hooks->attributes, "DateCalendarCreated", handle_xml_dtstamp_attribute);
	insert_xml_attr_handler(hooks->attributes, "Created", handle_xml_created_attribute); // ical only
	insert_xml_attr_handler(hooks->attributes, "Due", handle_xml_due_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateEnd", handle_xml_dtend_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateStarted", handle_xml_dtstart_attribute);
	insert_xml_attr_handler(hooks->attributes, "Description", handle_xml_description_attribute);
	insert_xml_attr_handler(hooks->attributes, "Duration", handle_xml_duration_attribute); // ical only
	insert_xml_attr_handler(hooks->attributes, "ExclusionDate", handle_xml_exdate_attribute);
	insert_xml_attr_handler(hooks->attributes, "ExclusionRule", handle_xml_exrule_attribute);
	// TODO -> freebusy
	insert_xml_attr_handler(hooks->attributes, "Geo", handle_xml_geo_attribute);
	insert_xml_attr_handler(hooks->attributes, "LastModified", handle_xml_last_modified_attribute);
	insert_xml_attr_handler(hooks->attributes, "Location", handle_xml_location_attribute);
	insert_xml_attr_handler(hooks->attributes, "Organizer", handle_xml_organizer_attribute); // ical only
	insert_xml_attr_handler(hooks->attributes, "PercentComplete", handle_xml_percent_complete_attribute); // ical only
	insert_xml_attr_handler(hooks->attributes, "Priority", handle_xml_priority_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceId", handle_xml_recurid_attribute); // ical only
	insert_xml_attr_handler(hooks->attributes, "RecurrenceDate", handle_xml_rdate_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceRule", handle_xml_rrule_attribute);
	insert_xml_attr_handler(hooks->attributes, "Related", handle_xml_related_attribute); // rename -> related to
	insert_xml_attr_handler(hooks->attributes, "Resources", handle_xml_resources_attribute);
	insert_xml_attr_handler(hooks->attributes, "RStatus", handle_xml_rstatus_attribute); // ical only
	insert_xml_attr_handler(hooks->attributes, "Sequence", handle_xml_sequence_attribute);
	insert_xml_attr_handler(hooks->attributes, "Status", handle_xml_status_attribute);
	insert_xml_attr_handler(hooks->attributes, "Summary", handle_xml_summary_attribute);
	insert_xml_attr_handler(hooks->attributes, "TimeTransparency", handle_xml_transp_attribute);
	insert_xml_attr_handler(hooks->attributes, "Uid", handle_xml_uid_attribute);
	insert_xml_attr_handler(hooks->attributes, "Url", handle_xml_url_attribute);

	}

	//Timezone TODO
	/*
	insert_xml_attr_handler(hooks->attributes, "TimezoneID", handle_xml_tz_id_attribute);
	insert_xml_attr_handler(hooks->attributes, "Location", handle_xml_tz_location_attribute);
	insert_xml_attr_handler(hooks->attributes, "TZOffsetFrom", handle_xml_tz_offsetfrom_location_attribute);
	insert_xml_attr_handler(hooks->attributes, "TZOffsetTo", handle_xml_tz_offsetto_location_attribute);
	insert_xml_attr_handler(hooks->attributes, "TimezoneName", handle_xml_tz_name_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateStarted", handle_xml_tz_dtstart_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceRule", handle_xml_tz_rrule_attribute);
	insert_xml_attr_handler(hooks->attributes, "LastModified", handle_xml_tz_last_modified_attribute);
	insert_xml_attr_handler(hooks->attributes, "TimezoneUrl", handle_xml_tz_url_attribute);
	insert_xml_attr_handler(hooks->attributes, "RecurrenceDate", handle_xml_tz_rdate_attribute);
	*/

	// [RFC 2445] alarm props (audio, display, email, procedure)
	insert_xml_attr_handler(hooks->attributes, "AlarmTrigger", handle_xml_alarm_trigger_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmRepeat", handle_xml_alarm_repeat_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmDuration", handle_xml_alarm_duration_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmAction", handle_xml_alarm_action_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmAttach", handle_xml_alarm_attach_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmDescription", handle_xml_alarm_description_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmAttendee", handle_xml_alarm_attendee_attribute);
	insert_xml_attr_handler(hooks->attributes, "AlarmSummary", handle_xml_alarm_summary_attribute);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static osync_bool conv_vcalendar_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error, int target)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, error);

	OSyncHookTables *hooks = init_vcalendar_to_xmlformat(target);
	OSyncXMLFormat *xmlformat = NULL; 

	// create a new xmlformat object
	if (target == VFORMAT_EVENT_10) {
		osync_trace(TRACE_SENSITIVE, "Input is vevent10:\n%s", input);
		xmlformat = osync_xmlformat_new("event", error);
	} else if (target == VFORMAT_EVENT_20) {
		osync_trace(TRACE_SENSITIVE, "Input is vevent20:\n%s", input);
		xmlformat = osync_xmlformat_new("event", error);
	} else if (target == VFORMAT_TODO_10) {
		osync_trace(TRACE_SENSITIVE, "Input is vtodo10:\n%s", input);
		xmlformat = osync_xmlformat_new("todo", error);
	} else if (target == VFORMAT_TODO_20) {
		osync_trace(TRACE_SENSITIVE, "Input is vtodo20:\n%s", input);
		xmlformat = osync_xmlformat_new("todo", error);
	} else if (target == VFORMAT_JOURNAL) {
		osync_trace(TRACE_SENSITIVE, "Input is vjournal:\n%s", input);
		xmlformat = osync_xmlformat_new("note", error);

	}

	// Parse the vcalendar
	VFormat *vcalendar = vformat_new_from_string(input);
	osync_trace(TRACE_INTERNAL, "Parsing attributes");
	
	// For every attribute we have call the handling hook
	GList *attributes = vformat_get_attributes(vcalendar);

	vcalendar_parse_attributes(xmlformat, &attributes, hooks, hooks->attributes, hooks->parameters);

	g_hash_table_destroy(hooks->attributes);
	g_hash_table_destroy(hooks->parameters);
	g_hash_table_destroy(hooks->tztable);
	g_hash_table_destroy(hooks->alarmtable);
	g_free(hooks);

	*free_input = TRUE;
	*output = (char *)xmlformat;
	*outpsize = osync_xmlformat_size();

	// XXX: remove this later?
	osync_xmlformat_sort(xmlformat);
	
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(xmlformat, &str, &size);
	osync_trace(TRACE_SENSITIVE, "Output XMLFormat is:\n%s", str);
	g_free(str);

	vformat_free(vcalendar);
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

static osync_bool conv_xmlformat_to_vcalendar(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error, int target)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, error);

	OSyncHookTables *hooks = init_xmlformat_to_vcalendar(target);

	// TODO register extensions
	/*
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

				//if(strcmp(config_array[i+1], "KDE") == 0)
				//	init_xmlformat_to_kde(hooks);
				//else if(strcmp(config_array[i+1], "Evolution") == 0)
				//	init_xmlformat_to_evolution(hooks);
					
			}else if(strcmp(config_array[i], "VCAL_ENCODING")) {
				
				if(strcmp(config_array[i+1], "UTF-16") == 0)
					;
				// TODO: what to do? :)
			}
		}
		g_strfreev(config_array);
	}
	*/

	OSyncXMLFormat *xmlformat = (OSyncXMLFormat *)input;
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(xmlformat, &str, &size);
	osync_trace(TRACE_SENSITIVE, "Input XMLFormat is:\n%s", str);
	g_free(str);

	// set default encoding
	const char *std_encoding = NULL;
	if (target == VFORMAT_EVENT_10 || target == VFORMAT_TODO_10)
		std_encoding = "QUOTED-PRINTABLE";
	else
		std_encoding = "B";

	// create a new vcalendar
	VFormat *vcalendar = vformat_new();

	// start parsing/generating attributes
	osync_trace(TRACE_INTERNAL, "parsing xml attributes");

	VFormatAttribute *attr = NULL;

	// prepare vcalendar container
	if (target == VFORMAT_EVENT_10 || target == VFORMAT_TODO_10) {

		// TODO: Handle DAYLIGHT attribute
		// TODO: Handle GEO attribute

		attr = vformat_attribute_new(NULL, "PRODID");
		vformat_attribute_add_value(attr, "-//OpenSync//NONSGML OpenSync vformat 0.3//EN");
		vformat_add_attribute(vcalendar, attr);
	
		// TODO: Handle TZ attribute
	
		attr = vformat_attribute_new(NULL, "VERSION");
		vformat_attribute_add_value(attr, "1.0");
		vformat_add_attribute(vcalendar, attr);

	} else if (target == VFORMAT_EVENT_20 || target == VFORMAT_TODO_20 || target == VFORMAT_JOURNAL) {

		attr = vformat_attribute_new(NULL, "PRODID");
		vformat_attribute_add_value(attr, "-//OpenSync//NONSGML OpenSync vformat 0.3//EN");
		vformat_add_attribute(vcalendar, attr);

		attr = vformat_attribute_new(NULL, "VERSION");
		vformat_attribute_add_value(attr, "2.0");
		vformat_add_attribute(vcalendar, attr);

		// TODO: Handle CALSCALE attribute
		// TODO: Handle METHOD attribute
		//  FIXME : handle errors or missing field
		OSyncXMLFieldList *list = osync_xmlformat_search_field(xmlformat, "Method", error, NULL);
		if (list) {
			OSyncXMLField *xmlfield = osync_xmlfieldlist_item(list, 0);
			osync_xmlfieldlist_free(list);
			if(xmlfield != NULL) {
			       xml_handle_attribute(hooks, vcalendar, xmlfield, std_encoding);
			}
		}

		// TODO: Handle VTIMEZONE component

	}

	// set begin attribute
	attr = vformat_attribute_new(NULL, "BEGIN");
	if (target == VFORMAT_EVENT_10 || target == VFORMAT_EVENT_20) {
		vformat_attribute_add_value(attr, "VEVENT");
	} else if (target == VFORMAT_TODO_10 || target == VFORMAT_TODO_20) {
		vformat_attribute_add_value(attr, "VTODO");
	} else if (target == VFORMAT_JOURNAL) {
		vformat_attribute_add_value(attr, "VJOURNAL");
	}
	vformat_add_attribute(vcalendar, attr);

	OSyncXMLField *xmlfield = osync_xmlformat_get_first_field(xmlformat);
	for(; xmlfield != NULL; xmlfield = osync_xmlfield_get_next(xmlfield)) {

		// we need to call xml_handle_component_attribute for alarm nodes
		if (strstr(osync_xmlfield_get_name(xmlfield), "Alarm") && target != VFORMAT_EVENT_10) {
			VFormatAttribute *start_attr = vformat_attribute_new(NULL, "BEGIN");
			vformat_add_attribute_with_value(vcalendar, start_attr, "VALARM"); 
			xml_handle_component_attribute(hooks, vcalendar, xmlfield, std_encoding);
			VFormatAttribute *end_attr = vformat_attribute_new(NULL, "END");
			vformat_add_attribute_with_value(vcalendar, end_attr, "VALARM"); 

		// skip method -> TODO
		} else if (strstr(osync_xmlfield_get_name(xmlfield), "Method")) {
			osync_trace(TRACE_INTERNAL, "Skipping %s", osync_xmlfield_get_name(xmlfield));

		// skip timezone -> TODO
		} else if (strstr(osync_xmlfield_get_name(xmlfield), "Timezone")) {
			osync_trace(TRACE_INTERNAL, "Skipping %s", osync_xmlfield_get_name(xmlfield));

		// default
		} else {
			xml_handle_attribute(hooks, vcalendar, xmlfield, std_encoding);
		}
	}

	osync_trace(TRACE_INTERNAL, "parsing xml attributes finished");
	
	// free hash tables
	g_hash_table_destroy(hooks->attributes);
	g_hash_table_destroy(hooks->parameters);
	g_free(hooks);

	*free_input = TRUE;
	*output = vformat_to_string(vcalendar, target);
	*outpsize = strlen(*output);

	vformat_free(vcalendar);

	if (target == VFORMAT_EVENT_10) {
		osync_trace(TRACE_SENSITIVE, "Output is vevent10:\n%s", *output);
	} else if (target == VFORMAT_EVENT_20) {
		osync_trace(TRACE_SENSITIVE, "Output is vevent20:\n%s", *output);
	} else if (target == VFORMAT_TODO_10) {
		osync_trace(TRACE_SENSITIVE, "Output is vtodo10:\n%s", *output);
	} else if (target == VFORMAT_TODO_20) {
		osync_trace(TRACE_SENSITIVE, "Output is vtodo20:\n%s", *output);
	} else if (target == VFORMAT_JOURNAL) {
		osync_trace(TRACE_SENSITIVE, "Output is vjournal:\n%s", *output);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	
	return TRUE;
}

osync_bool conv_xmlformat_to_vjournal(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	return conv_xmlformat_to_vcalendar(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_JOURNAL);
}

osync_bool conv_vjournal_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	return conv_vcalendar_to_xmlformat(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_JOURNAL);
}

osync_bool conv_xmlformat_to_vtodo10(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	return conv_xmlformat_to_vcalendar(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_TODO_10);
}

osync_bool conv_xmlformat_to_vtodo20(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	return conv_xmlformat_to_vcalendar(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_TODO_20);
}

osync_bool conv_vtodo10_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	return conv_vcalendar_to_xmlformat(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_TODO_10);
}

osync_bool conv_vtodo20_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	return conv_vcalendar_to_xmlformat(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_TODO_20);
}

osync_bool conv_xmlformat_to_vevent10(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	return conv_xmlformat_to_vcalendar(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_EVENT_10);
}

osync_bool conv_xmlformat_to_vevent20(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	return conv_xmlformat_to_vcalendar(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_EVENT_20);
}

osync_bool conv_vevent10_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	return conv_vcalendar_to_xmlformat(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_EVENT_10);
}

osync_bool conv_vevent20_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	return conv_vcalendar_to_xmlformat(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_EVENT_20);
}


