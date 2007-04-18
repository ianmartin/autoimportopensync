/*
 * xmlformat-vcard - A plugin for parsing vcard objects for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2006  Daniel Friedrich <daniel.friedrich@opensync.org>
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
#include "xmlformat.h"
#include "xmlformat-vcard.h"

static void handle_assistant_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Assistant parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Type", "Assistant");
}

static void handle_callback_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Callback parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Type", "Callback");
}

static void handle_company_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Company parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Type", "Company");
}

//static void handle_location_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
//{
//	osync_trace(TRACE_INTERNAL, "Handling Location parameter %s", vformat_attribute_param_get_name(param));
//	osync_xmlfield_set_attr(xmlfield, "Location", vformat_attribute_param_get_nth_value(param,0));
//}

static void handle_location_home_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Location parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Location", "Home");
}

static void handle_location_work_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Location parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Location", "Work");
}

static void handle_location_other_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Location parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Location", "Other");
}

static void handle_preferred_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Preferred parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Preferred", "true");
}

static void handle_radio_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Radio parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Type", "Radio");
}

static void handle_telex_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Telex parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Type", "Telex");
}

//static void handle_type_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
//{
//	
//	osync_trace(TRACE_INTERNAL, "Handling Type parameter %s\n", vformat_attribute_param_get_name(param));
//	
//	GList *v = vformat_attribute_param_get_values(param);
//	for (; v; v = v->next) {
//		osync_xmlfield_set_attr(xmlfield, "Type", v->data);
//	}
//}

static void handle_type_car_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Type parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Type", "Car");
}

static void handle_type_cellular_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Type parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Type", "Cellular");
}

static void handle_type_fax_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Type parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Type", "Fax");
}

static void handle_type_voice_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Type parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Type", "Voice");
}

static void handle_uislot_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Slot parameter %s", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "UI-Slot", vformat_attribute_param_get_nth_value(param, 0));
}

/* TODO: drop unknown parameters? */
//static void handle_unknown_parameter(OSyncXMLField *current, VFormatParam *param)
//{
//	osync_trace(TRACE_INTERNAL, "Handling unknown parameter %s", vformat_attribute_param_get_name(param));
//	xmlNode *property = xmlNewChild(current, NULL, (xmlChar*)"UnknownParam",
//		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
//	osxml_node_add(property, "ParamName", vformat_attribute_param_get_name(param));
//}

static OSyncXMLField *handle_address_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling address attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Address", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "PostOfficeBox", vformat_attribute_get_nth_value(attr, 0));
	osync_xmlfield_set_key_value(xmlfield, "ExtendedAddress", vformat_attribute_get_nth_value(attr, 1));
	osync_xmlfield_set_key_value(xmlfield, "Street", vformat_attribute_get_nth_value(attr, 2));
	osync_xmlfield_set_key_value(xmlfield, "Locality", vformat_attribute_get_nth_value(attr, 3));
	osync_xmlfield_set_key_value(xmlfield, "Region", vformat_attribute_get_nth_value(attr, 4));
	osync_xmlfield_set_key_value(xmlfield, "PostalCode", vformat_attribute_get_nth_value(attr, 5));
	osync_xmlfield_set_key_value(xmlfield, "Country", vformat_attribute_get_nth_value(attr, 6));	
	return xmlfield;
}

static OSyncXMLField *handle_agent_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	osync_trace(TRACE_INTERNAL, "Handling agent attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Agent", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	} 
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0)); 
	return xmlfield; 
} 

static OSyncXMLField *handle_aim_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling x-aim attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "IM-AIM", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_anniversary_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Anniversary attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Anniversary", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_assistant_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Assistant attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Assistant", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_birthday_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling birthday attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Birthday", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	char * datestamp = osync_time_datestamp(vformat_attribute_get_nth_value(attr, 0));
	osync_xmlfield_set_key_value(xmlfield, "Content", datestamp);
	g_free(datestamp);
	return xmlfield;
}

static OSyncXMLField *handle_blog_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling BlogUrl attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "BlogUrl", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_calendar_url_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling CalendarUrl attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "CalendarUrl", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_department_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling department attribute");
	OSyncXMLField *xmlfield = NULL;
	
	//We need to check first if the node already exists.
	OSyncXMLFieldList *list = osync_xmlformat_search_field(xmlformat, "Organization", NULL);
	xmlfield = osync_xmlfieldlist_item(list, 0);
	osync_xmlfieldlist_free(list);
	if(xmlfield == NULL) {
		xmlfield = osync_xmlfield_new(xmlformat, "Organization", error);
		if(!xmlfield) {
			osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
			return NULL;
		}
	}
	osync_xmlfield_set_key_value(xmlfield, "Department", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_email_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling EMail attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "EMail", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_file_as_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling FileAs attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "FileAs", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_formatted_name_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling formatted name attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "FormattedName", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_free_busy_url_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling FreeBusyUrl attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "FreeBusyUrl", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_gadu_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling gadu attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "IM-GaduGadu", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_groupwise_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling GroupwiseDirectory attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "GroupwiseDirectory", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_icq_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling IM-ICQ attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "IM-ICQ", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_irc_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling IRC attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "IRC", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_jabber_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Jabber attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "IM-Jabber", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_kde_organization_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Organization attribute");
	OSyncXMLField *xmlfield = NULL;

	//We need to check first if the node already exists.
	OSyncXMLFieldList *list = osync_xmlformat_search_field(xmlformat, "Organization", NULL);
	xmlfield = osync_xmlfieldlist_item(list, 0);
	osync_xmlfieldlist_free(list);
	if(xmlfield == NULL) {
		xmlfield = osync_xmlfield_new(xmlformat, "Organization", error);
		if(!xmlfield) {
			osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
			return NULL;
		}
	}
	osync_xmlfield_set_key_value(xmlfield, "Name", vformat_attribute_get_nth_value(attr, 0));
	osync_xmlfield_set_key_value(xmlfield, "Department", vformat_attribute_get_nth_value(attr, 1));
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	values = g_list_nth(values, 2);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osync_xmlfield_add_key_value(xmlfield, "Unit", retstr->str);
	}
	return xmlfield;
}

static OSyncXMLField *handle_key_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Key attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Key", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_label_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling AddressLabel attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "AddressLabel", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_location_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Location attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Location", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Latitude", vformat_attribute_get_nth_value(attr, 0));
	osync_xmlfield_set_key_value(xmlfield, "Longitude", vformat_attribute_get_nth_value(attr, 1));
	return xmlfield;
}

static OSyncXMLField *handle_logo_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Logo attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Logo", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_mailer_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Mailer attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Mailer", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_manager_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Manager attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Manager", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_msn_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling MSN attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "IM-MSN", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_name_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling name attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Name", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "LastName", vformat_attribute_get_nth_value(attr, 0));
	osync_xmlfield_set_key_value(xmlfield, "FirstName", vformat_attribute_get_nth_value(attr, 1));
	osync_xmlfield_set_key_value(xmlfield, "Additional", vformat_attribute_get_nth_value(attr, 2));
	osync_xmlfield_set_key_value(xmlfield, "Prefix", vformat_attribute_get_nth_value(attr, 3));
	osync_xmlfield_set_key_value(xmlfield, "Suffix", vformat_attribute_get_nth_value(attr, 4));
	return xmlfield;
}

static OSyncXMLField *handle_nickname_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Nickname attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Nickname", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_note_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Note attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Note", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_office_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling office attribute");
	OSyncXMLField *xmlfield = NULL;
	
	//We need to check first if the node already exists.
	OSyncXMLFieldList *list = osync_xmlformat_search_field(xmlformat, "Organization", NULL);
	xmlfield = osync_xmlfieldlist_item(list, 0);
	osync_xmlfieldlist_free(list);
	if(xmlfield == NULL) {
		xmlfield = osync_xmlfield_new(xmlformat, "Organization", error);
		if(!xmlfield) {
			osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
			return NULL;
		}	
	}

	osync_xmlfield_set_key_value(xmlfield, "Unit", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_organization_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Organization attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Organization", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Name", vformat_attribute_get_nth_value(attr, 0));
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	values = g_list_nth(values, 1);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osync_xmlfield_add_key_value(xmlfield, "Unit", retstr->str);
	}
	return xmlfield;
}

static OSyncXMLField *handle_photo_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling photo attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Photo", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_profession_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling profession attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Profession", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_revision_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Revision attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Revision", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	char *revision = osync_time_timestamp(vformat_attribute_get_nth_value(attr, 0));
	osync_xmlfield_set_key_value(xmlfield, "Content", revision);
	g_free(revision);
	return xmlfield;
}

static OSyncXMLField *handle_role_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Role attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Role", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_sms_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling SMS attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "SMS", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_sound_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Sound attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Sound", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_spouse_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Spouse attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Spouse", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_telephone_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Telephone attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Telephone", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_timezone_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Timezone attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Timezone", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_title_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Title attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Title", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

/*
static OSyncXMLField *handle_unknown_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown attribute %s", vformat_attribute_get_name(attr));
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "UnknownNode", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "NodeName", vformat_attribute_get_name(attr));
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osync_xmlfield_add_key_value(xmlfield, "Content", retstr->str);
	}
	return xmlfield;
}
*/

static OSyncXMLField *handle_video_chat_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling VideoUrl attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "VideoUrl", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_wants_html_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling WantsHtml attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "WantsHtml", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_x_kde_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling X-KDE attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "KDE-Extension", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "ExtName", vformat_attribute_get_name(attr));
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_yahoo_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling IM-Yahoo attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "IM-Yahoo", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static osync_bool init_evolution_to_xmlformat(OSyncHookTables *hooks)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, hooks);
	
	g_hash_table_insert(hooks->attributes, "X-EVOLUTION-FILE-AS", handle_file_as_attribute);
	g_hash_table_insert(hooks->attributes, "X-EVOLUTION-MANAGER", handle_manager_attribute);
	g_hash_table_insert(hooks->attributes, "X-EVOLUTION-ASSISTANT", handle_assistant_attribute);
	g_hash_table_insert(hooks->attributes, "X-EVOLUTION-ANNIVERSARY", handle_anniversary_attribute);
	g_hash_table_insert(hooks->attributes, "X-EVOLUTION-SPOUSE", handle_spouse_attribute);
	g_hash_table_insert(hooks->attributes, "X-EVOLUTION-BLOG-URL", handle_blog_attribute);
	g_hash_table_insert(hooks->attributes, "CALURI", handle_calendar_url_attribute);
	g_hash_table_insert(hooks->attributes, "FBURL", handle_free_busy_url_attribute);
	g_hash_table_insert(hooks->attributes, "X-EVOLUTION-VIDEO-URL", handle_video_chat_attribute);
	g_hash_table_insert(hooks->attributes, "X-MOZILLA-HTML", handle_wants_html_attribute);
	g_hash_table_insert(hooks->attributes, "X-YAHOO", handle_yahoo_attribute);
	g_hash_table_insert(hooks->attributes, "X-ICQ", handle_icq_attribute);
	g_hash_table_insert(hooks->attributes, "X-GROUPWISE", handle_groupwise_attribute);
	g_hash_table_insert(hooks->attributes, "X-AIM", handle_aim_attribute);
	g_hash_table_insert(hooks->attributes, "X-JABBER", handle_jabber_attribute);
	g_hash_table_insert(hooks->attributes, "X-MSN", handle_msn_attribute);
	
	//Overwrite the role hook (evo2s role is more like a profession so we map it there)
	g_hash_table_insert(hooks->attributes, "ROLE", handle_profession_attribute);
	
	g_hash_table_insert(hooks->parameters, "X-EVOLUTION-UI-SLOT", handle_uislot_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=X-EVOLUTION-ASSISTANT", handle_assistant_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=X-EVOLUTION-CALLBACK", handle_callback_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=X-EVOLUTION-COMPANY", handle_company_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=X-EVOLUTION-TELEX", handle_telex_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=X-EVOLUTION-RADIO", handle_radio_parameter);

	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

static osync_bool init_kde_to_xmlformat(OSyncHookTables *hooks)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, hooks);
	
	g_hash_table_insert(hooks->attributes, "X-KADDRESSBOOK-CRYPTOENCRYPTPREF", handle_x_kde_attribute);
	g_hash_table_insert(hooks->attributes, "X-KADDRESSBOOK-CRYPTOPROTOPREF", handle_x_kde_attribute);
	g_hash_table_insert(hooks->attributes, "X-KADDRESSBOOK-CRYPTOSIGNPREF", handle_x_kde_attribute);
	g_hash_table_insert(hooks->attributes, "X-KADDRESSBOOK-OPENPGPFP", handle_x_kde_attribute);
	g_hash_table_insert(hooks->attributes, "X-KADDRESSBOOK-X-IMAddress", handle_x_kde_attribute);
	g_hash_table_insert(hooks->attributes, "X-KADDRESSBOOK-X-ManagersName", handle_manager_attribute);
	g_hash_table_insert(hooks->attributes, "X-KADDRESSBOOK-X-AssistantsName", handle_assistant_attribute);
	g_hash_table_insert(hooks->attributes, "X-KADDRESSBOOK-X-Anniversary", handle_anniversary_attribute);
	g_hash_table_insert(hooks->attributes, "X-KADDRESSBOOK-X-Department", handle_department_attribute);
	g_hash_table_insert(hooks->attributes, "X-KADDRESSBOOK-X-Office", handle_office_attribute);
	g_hash_table_insert(hooks->attributes, "X-KADDRESSBOOK-X-Profession", handle_profession_attribute);
	g_hash_table_insert(hooks->attributes, "X-KADDRESSBOOK-X-SpousesName", handle_spouse_attribute);
	g_hash_table_insert(hooks->attributes, "X-messaging/yahoo-All", handle_yahoo_attribute);
	g_hash_table_insert(hooks->attributes, "X-messaging/icq-All", handle_icq_attribute);
	g_hash_table_insert(hooks->attributes, "X-messaging/aim-All", handle_aim_attribute);
	g_hash_table_insert(hooks->attributes, "X-messaging/xmpp-All", handle_jabber_attribute);
	g_hash_table_insert(hooks->attributes, "X-messaging/msn-All", handle_msn_attribute);
	g_hash_table_insert(hooks->attributes, "X-messaging/gadu-All", handle_gadu_attribute);
	g_hash_table_insert(hooks->attributes, "X-messaging/irc-All", handle_irc_attribute);
	g_hash_table_insert(hooks->attributes, "X-messaging/sms-All", handle_sms_attribute);
	
	//Overwrite the organization hook
	g_hash_table_insert(hooks->attributes, "ORG", handle_kde_organization_attribute);
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

static OSyncHookTables *init_vcard_to_xmlformat(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	
	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));
	
	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);
	
	g_hash_table_insert(hooks->attributes, "FN", handle_formatted_name_attribute);
	g_hash_table_insert(hooks->attributes, "N", handle_name_attribute);
	g_hash_table_insert(hooks->attributes, "PHOTO", handle_photo_attribute);
	g_hash_table_insert(hooks->attributes, "BDAY", handle_birthday_attribute);
	g_hash_table_insert(hooks->attributes, "ADR", handle_address_attribute);
	g_hash_table_insert(hooks->attributes, "AGENT", handle_agent_attribute); 
	g_hash_table_insert(hooks->attributes, "LABEL", handle_label_attribute);
	g_hash_table_insert(hooks->attributes, "TEL", handle_telephone_attribute);
	g_hash_table_insert(hooks->attributes, "EMAIL", handle_email_attribute);
	g_hash_table_insert(hooks->attributes, "MAILER", handle_mailer_attribute);
	g_hash_table_insert(hooks->attributes, "TZ", handle_timezone_attribute);
	g_hash_table_insert(hooks->attributes, "GEO", handle_location_attribute);
	g_hash_table_insert(hooks->attributes, "TITLE", handle_title_attribute);
	g_hash_table_insert(hooks->attributes, "ROLE", handle_role_attribute);
	g_hash_table_insert(hooks->attributes, "LOGO", handle_logo_attribute);
	g_hash_table_insert(hooks->attributes, "ORG", handle_organization_attribute);
	g_hash_table_insert(hooks->attributes, "NOTE", handle_note_attribute);
	g_hash_table_insert(hooks->attributes, "REV", handle_revision_attribute);
	g_hash_table_insert(hooks->attributes, "SOUND", handle_sound_attribute);
	g_hash_table_insert(hooks->attributes, "URL", handle_url_attribute);
	g_hash_table_insert(hooks->attributes, "UID", handle_uid_attribute);
	g_hash_table_insert(hooks->attributes, "KEY", handle_key_attribute);
	g_hash_table_insert(hooks->attributes, "NICKNAME", handle_nickname_attribute);
	g_hash_table_insert(hooks->attributes, "CLASS", handle_class_attribute);
	g_hash_table_insert(hooks->attributes, "CATEGORIES", handle_categories_attribute);
	
	g_hash_table_insert(hooks->attributes, "VERSION", HANDLE_IGNORE);
	g_hash_table_insert(hooks->attributes, "BEGIN", HANDLE_IGNORE);
	g_hash_table_insert(hooks->attributes, "END", HANDLE_IGNORE);
	
	g_hash_table_insert(hooks->attributes, "ENCODING", HANDLE_IGNORE);
	g_hash_table_insert(hooks->attributes, "CHARSET", HANDLE_IGNORE);
	
	//g_hash_table_insert(hooks->parameters, "TYPE", handle_type_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=PREF", handle_preferred_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=HOME", handle_location_home_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=WORK", handle_location_work_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=OTHER", handle_location_other_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=VOICE", handle_type_voice_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=CELL", handle_type_cellular_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=FAX", handle_type_fax_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=CAR", handle_type_car_parameter);
	g_hash_table_insert(hooks->parameters, "VALUE", handle_value_parameter);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

//static void _generate_formatted_name(VFormat *vcard, OSyncXMLFormat *xmlformat, OSyncError **error)
//{
//	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, xmlformat);
//	VFormatAttribute *n = vformat_find_attribute(vcard, "N");
//	GList *v = vformat_attribute_get_values(n);
//	GString *fnentry;
//	fnentry = g_string_new("");
//
//	// NAME:LAST;FIRST;ADDITIONAL;PREFIX;SUFFIX
//	// FN:PREFIX FIRST ADDITIONAL LAST SUFFIX
//
//	int order[5] = {3, 1, 2, 0, 4};
//	int i = 0;
//	char *str = NULL;
//	for (i = 0; i < 5; i++) {
//		if ((str = g_list_nth_data(v, order[i])) && str[0]) {
//			if (fnentry->len != 0)
//				g_string_append(fnentry, " ");
//			g_string_append(fnentry, str);
//		}
//	}
//	
//	osync_trace(TRACE_INTERNAL, "Handling formattedname attribute");
//	
//	if (fnentry->len != 0) {
////xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"FormattedName", NULL);
////osxml_node_add(current, "Content", fnentry->str);
//OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "FormattedName", error);
//osync_xmlfield_set_key_value(xmlfield, "Content", fnentry->str);
//	} else {
//		osync_trace(TRACE_INTERNAL, "FN is empty!");
//	}
//			
//	g_string_free(fnentry,TRUE);
//	osync_trace(TRACE_EXIT, "%s", __func__);
//	return;
//}
//
//static void _generate_name_from_fn(VFormat *vcard, OSyncXMLFormat *xmlformat, OSyncError **error)
//{
//	/*
//	 * We copy FN to N:LASTNAME because we don't now how FN was build.
//	 * e.g. we don't know if FN is "PREFIX FIRST LAST" or "FIRST ADDITIONAL LAST"
//	 * With copying FN to N we prevent the vcard from being invalid.
//	 */
//
//	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, root);
//	VFormatAttribute *n = vformat_find_attribute(vcard, "FN");
//	char *fn = vformat_attribute_get_value(n);
//
//	osync_trace(TRACE_INTERNAL, "Handling name attribute");
//
//	if (strlen(fn) != 0) {
////xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Name", NULL);
////	osxml_node_add(current, "LastName", fn);
//OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Name", error);
//osync_xmlfield_set_key_value(xmlfield, "LastName", fn);
//	} else {
//		osync_trace(TRACE_INTERNAL, "Name is empty");
//	}
//
//	osync_trace(TRACE_EXIT, "%s", __func__);
//	return;
//}

static osync_bool conv_vcard_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %i, %p, %s, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);
	
	OSyncHookTables *hooks = init_vcard_to_xmlformat();
	
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
			
			if(strcmp(config_array[i], "VCARD_EXTENSION") == 0) {

				if(strcmp(config_array[i+1], "KDE") == 0)
					init_kde_to_xmlformat(hooks);
				else if(strcmp(config_array[i+1], "Evolution") == 0)
					init_evolution_to_xmlformat(hooks);	
					
			}else if(strcmp(config_array[i], "REMOVE_SINGLE_WHITESPACES")) {
				
				if(strcmp(config_array[i+1], "TRUE") == 0) {
					/* TODO: */	
				}
				
			}else if(strcmp(config_array[i], "VCARD_ENCODING")) {
				
				if(strcmp(config_array[i+1], "UTF-16") == 0)
					;
				/* TODO: what to do? :) */
			}
		}
		g_strfreev(config_array);
	}
	
	osync_trace(TRACE_INTERNAL, "Input Vcard is:\n%s", input);
	
	//Parse the vcard
	VFormat *vcard = vformat_new_from_string(input);

	osync_trace(TRACE_INTERNAL, "Creating xmlformat object");
	OSyncXMLFormat *xmlformat = osync_xmlformat_new("contact", error);
	
	osync_trace(TRACE_INTERNAL, "parsing attributes");

	//For every attribute we have call the handling hook
	GList *attributes = vformat_get_attributes(vcard);
	GList *a = NULL;
	for (a = attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;
		handle_attribute(hooks, xmlformat, attr, error);
	}

//	//Generate FormattedName from Name if it doesn't exist
//	if (!vformat_find_attribute(vcard, "FN") && vformat_find_attribute(vcard, "N")) {
//		_generate_formatted_name(vcard, xmlformat);
//	}
//
//	//Generate Name from FormattedName if it doesn't exist
//	if (!vformat_find_attribute(vcard, "N") && vformat_find_attribute(vcard, "FN")) {
//		_generate_name_from_fn(vcard, xmlformat);
//	}	

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
		osync_trace(TRACE_INTERNAL, "XMLFORMAT CONTACT: Not valid!");
	else
		osync_trace(TRACE_INTERNAL, "XMLFORMAT CONTACT: VAILD");

	vformat_free(vcard);
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

static void handle_xml_assistant_x_evolution_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling assistant xml parameter");
	vformat_attribute_add_param_with_value(attr, "TYPE", "X-EVOLUTION-ASSISTANT");
}

static void handle_xml_callback_x_evolution_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling callback xml parameter");
	vformat_attribute_add_param_with_value(attr, "TYPE", "X-EVOLUTION-CALLBACK");
}

//static void handle_xml_category_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
//{
//	osync_trace(TRACE_INTERNAL, "Handling category xml parameter");
//	char *content = (char*)xmlNodeGetContent(current);
//	vformat_attribute_add_value(attr, content);
//	g_free(content);
//}

static void handle_xml_company_x_evolution_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling company xml parameter");
	vformat_attribute_add_param_with_value(attr, "TYPE", "X-EVOLUTION-COMPANY");
}

//static void handle_xml_location_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
//{
//	osync_trace(TRACE_INTERNAL, "Handling location xml parameter");
//	const char *content = osync_xmlfield_get_attr(xmlfield, "Location");
//	vformat_attribute_add_param_with_value(attr, "TYPE", content);
//}

static void handle_xml_location_home_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling location xml parameter");
	vformat_attribute_add_param_with_value(attr, "TYPE", "HOME");
}

static void handle_xml_location_work_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling location xml parameter");
	vformat_attribute_add_param_with_value(attr, "TYPE", "WORK");
}

static void handle_xml_location_other_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling location xml parameter");
	vformat_attribute_add_param_with_value(attr, "TYPE", "OTHER");
}

static void handle_xml_preferred_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling Preferred xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "Preferred");
	if(strcmp("true", content) == 0)
		vformat_attribute_add_param_with_value(attr, "TYPE", "PREF");
}

static void handle_xml_radio_x_evolution_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling radio xml parameter");
	vformat_attribute_add_param_with_value(attr, "TYPE", "X-EVOLUTION-RADIO");
}

static void handle_xml_uislot_x_evolution_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling slot xml parameter");
	vformat_attribute_add_param_with_value(attr, "X-EVOLUTION-UI-SLOT", osync_xmlfield_get_attr(xmlfield, "UI-Slot"));
}

//static void handle_xml_unit_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
//{
//	osync_trace(TRACE_INTERNAL, "Handling unit xml parameter");
//	vformat_attribute_add_value(attr, osync_xmlfield_get_attr(xmlfield, "Unit"));
//}

static void handle_xml_telex_x_evolution_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling telex xml parameter");
	vformat_attribute_add_param_with_value(attr, "TYPE", "X-EVOLUTION-TELEX");
}

//static void handle_xml_type_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
//{
//	osync_trace(TRACE_INTERNAL, "Handling type xml parameter");
//	const char *content = osync_xmlfield_get_attr(xmlfield, "Type");
//	vformat_attribute_add_param_with_value(attr, "TYPE", content);
//}

static void handle_xml_type_car_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling type xml parameter");
	vformat_attribute_add_param_with_value(attr, "TYPE", "CAR");
}

static void handle_xml_type_cellular_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling type xml parameter");
	vformat_attribute_add_param_with_value(attr, "TYPE", "CELL");
}

static void handle_xml_type_fax_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling type xml parameter");
	vformat_attribute_add_param_with_value(attr, "TYPE", "FAX");
}

static void handle_xml_type_voice_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling type xml parameter");
	vformat_attribute_add_param_with_value(attr, "TYPE", "VOICE");
}

static void handle_xml_value_parameter(VFormatAttribute *attr, OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_INTERNAL, "Handling value xml parameter");
	const char *content = osync_xmlfield_get_attr(xmlfield, "Value");
	vformat_attribute_add_param_with_value(attr, "VALUE", content);
}

//static void handle_xml_unknown_parameter(VFormatAttribute *attr, xmlNode *current)
//{
//	osync_trace(TRACE_INTERNAL, "Handling unknown xml parameter %s", current->name);
//	char *content = (char*)xmlNodeGetContent(current);
//	vformat_attribute_add_param_with_value(attr, (char*)current->name, content);
//	g_free(content);
//}

static VFormatAttribute *handle_xml_address_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling address xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ADR");
	add_value(attr, xmlfield, "PostOfficeBox", encoding);
	add_value(attr, xmlfield, "ExtendedAddress", encoding);
	add_value(attr, xmlfield, "Street", encoding);
	add_value(attr, xmlfield, "Locality", encoding);
	add_value(attr, xmlfield, "Region", encoding);
	add_value(attr, xmlfield, "PostalCode", encoding);
	add_value(attr, xmlfield, "Country", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_agent_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding) 
{ 
		osync_trace(TRACE_INTERNAL, "Handling agent xml attribute"); 
		VFormatAttribute *attr = vformat_attribute_new(NULL, "AGENT"); 
		add_value(attr, xmlfield, "Content", encoding); 
		vformat_add_attribute(vcard, attr); 
		return attr; 
} 

static VFormatAttribute *handle_xml_aim_x_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling aim xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-AIM");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aim_x_messaging_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling aim xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/aim-All");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_anniversary_x_evolution_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling anniversary xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-EVOLUTION-ANNIVERSARY");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_anniversary_x_kaddressbook_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling anniversary xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-Anniversary");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_assistant_x_evolution_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling assistant xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-EVOLUTION-ASSISTANT");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_assistant_x_kaddressbook_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling assistant xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-AssistantsName");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_birthday_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling birthday xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "BDAY");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_blog_url_x_evolution_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling blog_url xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-EVOLUTION-BLOG-URL");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_calendar_url_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling calendar_url xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CALURI");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_email_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling email xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "EMAIL");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_file_as_x_evolution_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling file_as xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-EVOLUTION-FILE-AS");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_free_busy_url_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling free_busy_url xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "FBURL");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_formatted_name_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling formatted name xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "FN");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_gadu_x_messaging_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling msn xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/gadu-All");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_groupwise_x_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling groupwise xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-GROUPWISE");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_icq_x_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling icq xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-ICQ");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_icq_x_messaging_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling icq xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/icq-All");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_irc_x_messaging_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling msn xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/irc-All");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_jabber_x_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling jabber xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-JABBER");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_jabber_x_messaging_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling jabber xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/xmpp-All");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_key_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling key xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "KEY");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_label_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling label xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LABEL");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_location_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling location xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "GEO");
	add_value(attr, xmlfield, "Latitude", encoding);
	add_value(attr, xmlfield, "Longitude", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_logo_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling logo xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LOGO");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_attribute_add_param_with_value(attr, "ENCODING", "b");
	//vformat_attribute_add_param_with_value(attr, "TYPE", osxml_find_node(xmlfield, "Type"));
	vformat_attribute_add_param_with_value(attr, "TYPE", osync_xmlfield_get_attr(xmlfield, "Type"));
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_mailer_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling mailer xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "MAILER");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_name_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling name xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "N");
	add_value(attr, xmlfield, "LastName", encoding);
	add_value(attr, xmlfield, "FirstName", encoding);
	add_value(attr, xmlfield, "Additional", encoding);
	add_value(attr, xmlfield, "Prefix", encoding);
	add_value(attr, xmlfield, "Suffix", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_manager_x_evolution_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling manager xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-EVOLUTION-MANAGER");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_manager_x_kaddressbook_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling manager xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-ManagersName");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_msn_x_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling msn xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-MSN");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_msn_x_messaging_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling msn xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/msn-All");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}
static VFormatAttribute *handle_xml_nickname_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling nickname xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "NICKNAME");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_note_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling note xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "NOTE");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_organization_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling organization xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ORG");
	add_value(attr, xmlfield, "Name", encoding);
	add_values_from_nth_field_on(attr, xmlfield, encoding, 1);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_organization_x_kaddressbook_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling organization kde xml attribute");
	VFormatAttribute *org = NULL;
	VFormatAttribute *attr = NULL;
	
	int i, j = 0 , c = osync_xmlfield_get_key_count(xmlfield);
	for(i=0; i < c; i++)
	{
		const char *name = osync_xmlfield_get_nth_key_name(xmlfield, i);
		const char *content = osync_xmlfield_get_nth_key_value(xmlfield, i);

		if (!strcmp("Name", name)) {
			org = vformat_attribute_new(NULL, "ORG");
			vformat_attribute_add_value(org, content);
			vformat_add_attribute(vcard, org);
		}
		
		if (!strcmp("Department", name)) {
			attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-Department");
			vformat_attribute_add_value(attr, content);
			vformat_add_attribute(vcard, attr);
		}
		if (!strcmp("Unit", name)) {
			switch (j) {
				case 0:
					attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-Office");
					vformat_attribute_add_value(attr, content);
					vformat_add_attribute(vcard, attr);
					break;
				default:
					vformat_attribute_add_value(org, content);
			}
			j++;
		}
	}
	
	return attr;
}


static VFormatAttribute *handle_xml_photo_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling photo xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "PHOTO");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_attribute_add_param_with_value(attr, "ENCODING", "b");
	vformat_attribute_add_param_with_value(attr, "TYPE", osync_xmlfield_get_attr(xmlfield, "Type"));
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_profession_x_kaddressbook_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling profession xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-Profession");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_revision_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling revision xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "REV");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_role_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling role xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ROLE");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_sms_x_messaging_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling msn xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/sms-All");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_sound_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling sound xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "SOUND");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_attribute_add_param_with_value(attr, "ENCODING", "b");
	vformat_attribute_add_param_with_value(attr, "TYPE", osync_xmlfield_get_attr(xmlfield, "Type"));
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_spouse_x_evolution_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling spouse xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-EVOLUTION-SPOUSE");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_spouse_x_kaddressbook_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling spouse xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-SpousesName");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_telephone_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling telephone xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TEL");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_timezone_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling timezone xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZ");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_title_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling title xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TITLE");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

//static VFormatAttribute *handle_xml_unknown_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
//{
//	osync_trace(TRACE_INTERNAL, "Handling unknown xml attribute %s", osync_xmlfield_get_name(xmlfield));
//	char *name = osxml_find_node(root, "NodeName");
//	VFormatAttribute *attr = vformat_attribute_new(NULL, name);
//	add_value(attr, xmlfield, "Content", encoding);
//	vformat_add_attribute(vcard, attr);
//	return attr;
//}

static VFormatAttribute *handle_xml_video_url_x_evolution_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling videourl xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-EVOLUTION-VIDEO-URL");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_wants_html_x_mozilla_html_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling wants_html xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-MOZILLA-HTML");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_x_kde_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling X-KDE xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, osync_xmlfield_get_key_value(xmlfield, "ExtName"));
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}
static VFormatAttribute *handle_xml_yahoo_x_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling yahoo xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-YAHOO");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_yahoo_x_messaging_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling yahoo xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/yahoo-All");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static osync_bool init_xmlformat_to_evolution(OSyncHookTables *hooks)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, hooks);
	
	g_hash_table_insert(hooks->attributes, "FileAs", handle_xml_file_as_x_evolution_attribute);
	g_hash_table_insert(hooks->attributes, "Manager", handle_xml_manager_x_evolution_attribute);
	g_hash_table_insert(hooks->attributes, "Assistant", handle_xml_assistant_x_evolution_attribute);
	g_hash_table_insert(hooks->attributes, "Anniversary", handle_xml_anniversary_x_evolution_attribute);
	g_hash_table_insert(hooks->attributes, "Spouse", handle_xml_spouse_x_evolution_attribute);
	g_hash_table_insert(hooks->attributes, "BlogUrl", handle_xml_blog_url_x_evolution_attribute);
	g_hash_table_insert(hooks->attributes, "CalendarUrl", handle_xml_calendar_url_attribute);
	g_hash_table_insert(hooks->attributes, "FreeBusyUrl", handle_xml_free_busy_url_attribute);
	g_hash_table_insert(hooks->attributes, "VideoUrl", handle_xml_video_url_x_evolution_attribute);
	g_hash_table_insert(hooks->attributes, "WantsHtml", handle_xml_wants_html_x_mozilla_html_attribute);
	g_hash_table_insert(hooks->attributes, "IM-Yahoo", handle_xml_yahoo_x_attribute);
	g_hash_table_insert(hooks->attributes, "IM-ICQ", handle_xml_icq_x_attribute);
	g_hash_table_insert(hooks->attributes, "GroupwiseDirectory", handle_xml_groupwise_x_attribute);
	g_hash_table_insert(hooks->attributes, "IM-AIM", handle_xml_aim_x_attribute);
	g_hash_table_insert(hooks->attributes, "IM-Jabber", handle_xml_jabber_x_attribute);
	g_hash_table_insert(hooks->attributes, "IM-MSN", handle_xml_msn_x_attribute);

	//Overwrite Profession handler //We map the profession to the ROLE
	g_hash_table_insert(hooks->attributes, "Profession", handle_xml_role_attribute);
	
	g_hash_table_insert(hooks->parameters, "UI-Slot", handle_xml_uislot_x_evolution_parameter);
	g_hash_table_insert(hooks->parameters, "Type=Assistant", handle_xml_assistant_x_evolution_parameter);
	g_hash_table_insert(hooks->parameters, "Type=Callback", handle_xml_callback_x_evolution_parameter);
	g_hash_table_insert(hooks->parameters, "Type=Company", handle_xml_company_x_evolution_parameter);
	g_hash_table_insert(hooks->parameters, "Type=Telex", handle_xml_telex_x_evolution_parameter);
	g_hash_table_insert(hooks->parameters, "Type=Radio", handle_xml_radio_x_evolution_parameter);
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

static osync_bool init_xmlformat_to_kde(OSyncHookTables *hooks)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, hooks);
	
	g_hash_table_insert(hooks->attributes, "Manager", handle_xml_manager_x_kaddressbook_attribute);
	g_hash_table_insert(hooks->attributes, "Assistant", handle_xml_assistant_x_kaddressbook_attribute);
	g_hash_table_insert(hooks->attributes, "Anniversary", handle_xml_anniversary_x_kaddressbook_attribute);
	g_hash_table_insert(hooks->attributes, "Organization", handle_xml_organization_x_kaddressbook_attribute);
	g_hash_table_insert(hooks->attributes, "Profession", handle_xml_profession_x_kaddressbook_attribute);
	g_hash_table_insert(hooks->attributes, "Spouse", handle_xml_spouse_x_kaddressbook_attribute);
	g_hash_table_insert(hooks->attributes, "IM-Yahoo", handle_xml_yahoo_x_messaging_attribute);
	g_hash_table_insert(hooks->attributes, "IM-ICQ", handle_xml_icq_x_messaging_attribute);
	g_hash_table_insert(hooks->attributes, "IM-AIM", handle_xml_aim_x_messaging_attribute);
	g_hash_table_insert(hooks->attributes, "IM-Jabber", handle_xml_jabber_x_messaging_attribute);
	g_hash_table_insert(hooks->attributes, "IM-MSN", handle_xml_msn_x_messaging_attribute);
	g_hash_table_insert(hooks->attributes, "IM-GaduGadu", handle_xml_gadu_x_messaging_attribute);
	g_hash_table_insert(hooks->attributes, "IRC", handle_xml_irc_x_messaging_attribute);
	g_hash_table_insert(hooks->attributes, "SMS", handle_xml_sms_x_messaging_attribute);

	g_hash_table_insert(hooks->attributes, "KDE-Extension", handle_xml_x_kde_attribute);
	//Overwrite the uid and revision handler
	g_hash_table_insert(hooks->attributes, "Uid", HANDLE_IGNORE);
	g_hash_table_insert(hooks->attributes, "Revision", HANDLE_IGNORE);
	
	g_hash_table_insert(hooks->parameters, "Unit", HANDLE_IGNORE);
	g_hash_table_insert(hooks->parameters, "Name", HANDLE_IGNORE);
	g_hash_table_insert(hooks->parameters, "Department", HANDLE_IGNORE);
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

static OSyncHookTables *init_xmlformat_to_vcard(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	
	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));
	
	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);
	
	g_hash_table_insert(hooks->attributes, "FormattedName", handle_xml_formatted_name_attribute);
	g_hash_table_insert(hooks->attributes, "Name", handle_xml_name_attribute);
	g_hash_table_insert(hooks->attributes, "Photo", handle_xml_photo_attribute);
	g_hash_table_insert(hooks->attributes, "Birthday", handle_xml_birthday_attribute);
	g_hash_table_insert(hooks->attributes, "Address", handle_xml_address_attribute);
	g_hash_table_insert(hooks->attributes, "AddressLabel", handle_xml_label_attribute);
	g_hash_table_insert(hooks->attributes, "Agent", handle_xml_agent_attribute); 
	g_hash_table_insert(hooks->attributes, "Telephone", handle_xml_telephone_attribute);
	g_hash_table_insert(hooks->attributes, "EMail", handle_xml_email_attribute);
	g_hash_table_insert(hooks->attributes, "Mailer", handle_xml_mailer_attribute);
	g_hash_table_insert(hooks->attributes, "Timezone", handle_xml_timezone_attribute);
	g_hash_table_insert(hooks->attributes, "Location", handle_xml_location_attribute);
	g_hash_table_insert(hooks->attributes, "Title", handle_xml_title_attribute);
	g_hash_table_insert(hooks->attributes, "Role", handle_xml_role_attribute);
	g_hash_table_insert(hooks->attributes, "Logo", handle_xml_logo_attribute);
	g_hash_table_insert(hooks->attributes, "Organization", handle_xml_organization_attribute);
	g_hash_table_insert(hooks->attributes, "Note", handle_xml_note_attribute);
	g_hash_table_insert(hooks->attributes, "Revision", handle_xml_revision_attribute);
	g_hash_table_insert(hooks->attributes, "Sound", handle_xml_sound_attribute);
	g_hash_table_insert(hooks->attributes, "Url", handle_xml_url_attribute);
	g_hash_table_insert(hooks->attributes, "Uid", handle_xml_uid_attribute);
	g_hash_table_insert(hooks->attributes, "Key", handle_xml_key_attribute);
	g_hash_table_insert(hooks->attributes, "Nickname", handle_xml_nickname_attribute);
	g_hash_table_insert(hooks->attributes, "Class", handle_xml_class_attribute);
	g_hash_table_insert(hooks->attributes, "Categories", handle_xml_categories_attribute);
//	g_hash_table_insert(hooks->attributes, "UnknownNode", xml_handle_unknown_attribute);
	
//	g_hash_table_insert(hooks->parameters, "Type", handle_xml_type_parameter);
	g_hash_table_insert(hooks->parameters, "Type=Voice", handle_xml_type_voice_parameter);
	g_hash_table_insert(hooks->parameters, "Type=Cellular", handle_xml_type_cellular_parameter);
	g_hash_table_insert(hooks->parameters, "Type=Fax", handle_xml_type_fax_parameter);
	g_hash_table_insert(hooks->parameters, "Type=Car", handle_xml_type_car_parameter);
//	g_hash_table_insert(hooks->parameters, "Location", handle_xml_location_parameter);
	g_hash_table_insert(hooks->parameters, "Location=Home", handle_xml_location_home_parameter);
	g_hash_table_insert(hooks->parameters, "Location=Work", handle_xml_location_work_parameter);
	g_hash_table_insert(hooks->parameters, "Location=Other", handle_xml_location_other_parameter);
	
	g_hash_table_insert(hooks->parameters, "Preferred", handle_xml_preferred_parameter);
	g_hash_table_insert(hooks->parameters, "Value", handle_xml_value_parameter);
	

	g_hash_table_insert(hooks->parameters, "TYPE=HOME", handle_location_home_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=WORK", handle_location_work_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=OTHER", handle_location_other_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=VOICE", handle_type_voice_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=CELL", handle_type_cellular_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=FAX", handle_type_fax_parameter);
	g_hash_table_insert(hooks->parameters, "TYPE=CAR", handle_type_car_parameter);
	
//	g_hash_table_insert(hooks->parameters, "Category", handle_xml_category_parameter);
//	g_hash_table_insert(hooks->parameters, "Unit", handle_xml_unit_parameter);
//	g_hash_table_insert(hooks->parameters, "UnknownParameter", xml_handle_unknown_parameter);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return hooks;
}

static osync_bool conv_xmlformat_to_vcard(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error, int target)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, error);

	OSyncHookTables *hooks = init_xmlformat_to_vcard();

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
			
			if(strcmp(config_array[i], "VCARD_EXTENSION") == 0) {

				if(strcmp(config_array[i+1], "KDE") == 0)
					init_xmlformat_to_kde(hooks);
				else if(strcmp(config_array[i+1], "Evolution") == 0)
					init_xmlformat_to_evolution(hooks);
					
			}else if(strcmp(config_array[i], "VCARD_ENCODING")) {
				
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

	//Make the new vcard
	VFormat *vcard = vformat_new();
	
	osync_trace(TRACE_INTERNAL, "parsing cml attributes");
	const char *std_encoding = NULL;
	if (target == VFORMAT_CARD_21)
		std_encoding = "QUOTED-PRINTABLE";
	else
		std_encoding = "B";
	
	OSyncXMLField *xmlfield = osync_xmlformat_get_first_field(xmlformat);
	for(; xmlfield != NULL; xmlfield = osync_xmlfield_get_next(xmlfield)) {
		xml_handle_attribute(hooks, vcard, xmlfield, std_encoding);
	}
	
	g_hash_table_destroy(hooks->attributes);
	g_hash_table_destroy(hooks->parameters);
	g_free(hooks);

	*free_input = TRUE;
	*output = vformat_to_string(vcard, target);
	*outpsize = strlen(*output) + 1;

	vformat_free(vcard);

	osync_trace(TRACE_INTERNAL, "Output vcard is: \n%s", *output);

	osync_trace(TRACE_EXIT, "%s", __func__);
	
	return TRUE;
}


static osync_bool conv_xmlformat_to_vcard30(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	return conv_xmlformat_to_vcard(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_CARD_30);
}

static osync_bool conv_xmlformat_to_vcard21(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	return conv_xmlformat_to_vcard(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_CARD_21);
}

static OSyncConvCmpResult compare_contact(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %i)", __func__, leftdata, leftsize, rightdata, rightsize);
	
	char* keys_content[] =  {"Content", NULL};
	char* keys_name[] = {"FirstName", "LastName", NULL};
	OSyncXMLPoints points[] = {
		{"Name", 		90, 	keys_name},
		{"Telephone", 	10, 	keys_content},
		{"EMail", 		10, 	keys_content},
		{NULL}
	};

	OSyncConvCmpResult ret = osync_xmlformat_compare((OSyncXMLFormat *)leftdata, (OSyncXMLFormat *)rightdata, points, 0, 100);
		
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static void create_contact(char **data, unsigned int *size)
{
	OSyncError *error = NULL;
	*data = (char *)osync_xmlformat_new("contact", &error);
	if (!*data)
		osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

static time_t get_revision(const char *data, unsigned int size, OSyncError **error)
{	
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, data, size, error);
	
	OSyncXMLFieldList *fieldlist = osync_xmlformat_search_field((OSyncXMLFormat *)data, "Revision", NULL);

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
	
	OSyncObjFormat *format = osync_objformat_new("xmlformat-contact", "contact", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format xmlformat: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	
	osync_objformat_set_compare_func(format, compare_contact);
	osync_objformat_set_destroy_func(format, destroy_xmlformat);
	osync_objformat_set_print_func(format, print_xmlformat);
	osync_objformat_set_copy_func(format, copy_xmlformat);
	osync_objformat_set_create_func(format, create_contact);
	
	osync_objformat_set_revision_func(format, get_revision);
	
	osync_objformat_must_marshal(format);
	osync_objformat_set_marshal_func(format, marshal_xmlformat);
	osync_objformat_set_demarshal_func(format, demarshal_xmlformat);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	
	
	format = osync_objformat_new("vcard21", "contact", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format vcard21: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	
	
	format = osync_objformat_new("vcard30", "contact", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format vcard30: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);	
}

void get_conversion_info(OSyncFormatEnv *env)
{
	OSyncFormatConverter *conv;
	OSyncError *error = NULL;
	
	OSyncObjFormat *xmlformat = osync_format_env_find_objformat(env, "xmlformat-contact");
	OSyncObjFormat *vcard21 = osync_format_env_find_objformat(env, "vcard21");
	OSyncObjFormat *vcard30 = osync_format_env_find_objformat(env, "vcard30");
	
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vcard21, conv_xmlformat_to_vcard21, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vcard21, xmlformat, conv_vcard_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vcard30, conv_xmlformat_to_vcard30, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vcard30, xmlformat, conv_vcard_to_xmlformat, &error);
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
