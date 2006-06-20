/*
 * xml-vcard - A plugin for parsing vcard objects for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2006  Daniel Friedrich <daniel.friedrich@opensync.org>
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

#include "opensync-xml.h"
#include "vformat.h"
#include "xml-vcard.h"
#include "xml-support.c" //only for xmlcompaire
#include "merger/opensync_xmlformat.h"
#include "merger/opensync_xmlformat_internals.h"
#include "merger/opensync_xmlfield.h"
#include <glib.h>

//static void handle_unknown_parameter(OSyncXMLField *current, VFormatParam *param)
//{
//	osync_trace(TRACE_INTERNAL, "Handling unknown parameter %s", vformat_attribute_param_get_name(param));
//	xmlNode *property = xmlNewChild(current, NULL, (xmlChar*)"UnknownParam",
//		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
//	osxml_node_add(property, "ParamName", vformat_attribute_param_get_name(param));
//}

//static void handle_type_parameter(OSyncXMLField *current, VFormatParam *param)
//{
//	osync_trace(TRACE_INTERNAL, "Handling type parameter %s", vformat_attribute_param_get_name(param));
//	
//	GList *v = vformat_attribute_param_get_values(param);
//	for (; v; v = v->next) {
//		xmlNewChild(current, NULL, (xmlChar*)"Type", (xmlChar*)v->data);
//	}
	
//}

static OSyncXMLField *handle_formatted_name_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling formatted name attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "FormattedName");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_name_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling name attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Name");
	osync_xmlfield_set_key_value(xmlfield, "LastName", vformat_attribute_get_nth_value(attr, 0));
	osync_xmlfield_set_key_value(xmlfield, "FirstName", vformat_attribute_get_nth_value(attr, 1));
	osync_xmlfield_set_key_value(xmlfield, "Additional", vformat_attribute_get_nth_value(attr, 2));
	osync_xmlfield_set_key_value(xmlfield, "Prefix", vformat_attribute_get_nth_value(attr, 3));
	osync_xmlfield_set_key_value(xmlfield, "Suffix", vformat_attribute_get_nth_value(attr, 4));
	return xmlfield;
}

static OSyncXMLField *handle_photo_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling photo attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Photo");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_birthday_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling birthday attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Birthday");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_address_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling address attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Address");
	osync_xmlfield_set_key_value(xmlfield, "PostalBox", vformat_attribute_get_nth_value(attr, 0));
	osync_xmlfield_set_key_value(xmlfield, "ExtendedAddress", vformat_attribute_get_nth_value(attr, 1));
	osync_xmlfield_set_key_value(xmlfield, "Street", vformat_attribute_get_nth_value(attr, 2));
	osync_xmlfield_set_key_value(xmlfield, "City", vformat_attribute_get_nth_value(attr, 3));
	osync_xmlfield_set_key_value(xmlfield, "Region", vformat_attribute_get_nth_value(attr, 4));
	osync_xmlfield_set_key_value(xmlfield, "PostalCode", vformat_attribute_get_nth_value(attr, 5));
	osync_xmlfield_set_key_value(xmlfield, "Country", vformat_attribute_get_nth_value(attr, 6));
	return xmlfield;
}

static OSyncXMLField *handle_label_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling AddressLabel attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "AddressLabel");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_telephone_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Telephone attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Telephone");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_email_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling EMail attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "EMail");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_mailer_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Mailer attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Mailer");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_timezone_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Timezone attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Timezone");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_location_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Location attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Location");
	osync_xmlfield_set_key_value(xmlfield, "Latitude", vformat_attribute_get_nth_value(attr, 0));
	osync_xmlfield_set_key_value(xmlfield, "Longitude", vformat_attribute_get_nth_value(attr, 1));
	return xmlfield;
}

static OSyncXMLField *handle_title_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Title attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Title");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_role_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Role attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Role");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_logo_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Logo attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Logo");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_organization_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Organization attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Organization");
	osync_xmlfield_set_key_value(xmlfield, "Name", vformat_attribute_get_nth_value(attr, 0));
	osync_xmlfield_set_key_value(xmlfield, "Department", vformat_attribute_get_nth_value(attr, 1));
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	values = g_list_nth(values, 2);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osync_xmlfield_set_key_value(xmlfield, "Unit", retstr->str);
	}
	return xmlfield;
}

static OSyncXMLField *handle_note_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Note attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Note");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_revision_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Revision attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Revision");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_sound_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Sound attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Sound");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_url_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Url attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Url");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_uid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Uid attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Uid");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_key_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Key attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Key");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_nickname_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Nickname attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Nickname");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_class_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Class attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Class");
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

static OSyncXMLField *handle_categories_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Categories attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Categories");
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osync_xmlfield_set_key_value(xmlfield, "Category", retstr->str);
	}
	
	return xmlfield;
}

//static OSyncXMLField *handle_unknown_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
//{
//	osync_trace(TRACE_INTERNAL, "Handling unknown attribute %s", vformat_attribute_get_name(attr));
//	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "UnknownNode");
//	osync_xmlfield_set_key_value(xmlfield, "NodeName", vformat_attribute_get_name(attr));
//	GList *values = vformat_attribute_get_values_decoded(attr);
//	for (; values; values = values->next) {
//		GString *retstr = (GString *)values->data;
//		g_assert(retstr);
//		osync_xmlfield_set_key_value(xmlfield, "Content", retstr->str);
//	}
//	return xmlfield;
//}

//static void vcard_handle_parameter(GHashTable *hooks, OSyncXMLField *current, VFormatParam *param)
//{
//	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, hooks, current, param);
//	
//	//Find the handler for this parameter
//	void (* param_handler)(OSyncXMLField *, VFormatParam *);
//	char *paramname = g_strdup_printf("%s=%s", vformat_attribute_param_get_name(param), vformat_attribute_param_get_nth_value(param, 0));
//	param_handler = g_hash_table_lookup(hooks, paramname);
//	g_free(paramname);
//	if (!param_handler)
//		param_handler = g_hash_table_lookup(hooks, vformat_attribute_param_get_name(param));
//	
//	if (param_handler == HANDLE_IGNORE) {
//		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
//		return;
//	}
//	
//	if (param_handler)
//		param_handler(current, param);
//	else
//		handle_unknown_parameter(current, param);
//	
//	osync_trace(TRACE_EXIT, "%s", __func__);
//}

static void vcard_handle_attribute(GHashTable *hooks, OSyncXMLFormat *xmlformat, VFormatAttribute *attr)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, hooks, xmlformat, attr, attr ? vformat_attribute_get_name(attr) : "None");
//	xmlNode *current = NULL;
	OSyncXMLField *current;

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
	OSyncXMLField *(* attr_handler)(OSyncXMLFormat *, VFormatAttribute *) = g_hash_table_lookup(hooks, vformat_attribute_get_name(attr));
	osync_trace(TRACE_INTERNAL, "Hook is: %p", attr_handler);
	if (attr_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	if (attr_handler)
		current = attr_handler(xmlformat, attr);
//	else
//		current = handle_unknown_attribute(xmlformat, attr);

	//Handle all parameters of this attribute
//	GList *params = vformat_attribute_get_params(attr);
//	GList *p = NULL;
//	for (p = params; p; p = p->next) {
//		VFormatParam *param = p->data;
//		vcard_handle_parameter(hooks, current, param);
//	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void *init_vcard_to_xmlformat(void);

static osync_bool conv_vcard_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, error);

	GHashTable *hooks = (GHashTable *)init_vcard_to_xmlformat();

	osync_trace(TRACE_INTERNAL, "Input Vcard is:\n%s", input);
	
	/* The input is not null-terminated, but vformat_new_from_string() expects a null-terminated string */
	char *input_str = g_malloc(inpsize + 1);
	memcpy(input_str, input, inpsize);
	input_str[inpsize] = '\0';

	//Parse the vcard
	VFormat *vcard = vformat_new_from_string(input_str);

	g_free(input_str);

	osync_trace(TRACE_INTERNAL, "Creating xml doc");

	//Create a new xml document
	OSyncXMLFormat *xmlformat = osync_xmlformat_new("contact");
//	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
//	xmlNode *root = osxml_node_add_root(doc, "contact");
	
	osync_trace(TRACE_INTERNAL, "parsing attributes");

	//For every attribute we have call the handling hook
	GList *attributes = vformat_get_attributes(vcard);
	GList *a = NULL;
	for (a = attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;
		vcard_handle_attribute(hooks, xmlformat, attr);
	}

	int size;
	char *str;
	osync_xmlformat_assemble(xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "Output XML is:\n%s", str);
// TODO: xmlFree(str);


	*output = (char *)xmlformat;
	*outpsize = sizeof(xmlformat);
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

static void add_value(VFormatAttribute *attr, OSyncXMLField *xmlfield, const char *name, const char *encoding)
{
//	char *tmp = osxml_find_node(parent, name);
	char *tmp = (char *)name;
	
	if (!tmp) {
		/* If there is no node with the given name, add an empty value to the list.
		 * This is necessary because some fields (N and ADR, for example) need
		 * a specific order of the values
		 */
		tmp = g_strdup("");
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
	g_free(tmp);
}

static void handle_xml_type_parameter(VFormatAttribute *attr, xmlNode *current)
{
	osync_trace(TRACE_INTERNAL, "Handling type xml parameter");
	char *content = (char*)xmlNodeGetContent(current);
	VFormatParam *param = vformat_attribute_param_new("TYPE");
	vformat_attribute_param_add_value(param, content);
	vformat_attribute_add_param (attr, param);
	g_free(content);
}

static void handle_xml_category_parameter(VFormatAttribute *attr, xmlNode *current)
{
	osync_trace(TRACE_INTERNAL, "Handling category xml parameter");
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_value(attr, content);
	g_free(content);
}

static void handle_xml_unit_parameter(VFormatAttribute *attr, xmlNode *current)
{
	osync_trace(TRACE_INTERNAL, "Handling unit xml parameter");
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_value(attr, content);
	g_free(content);
}

static void xml_handle_unknown_parameter(VFormatAttribute *attr, xmlNode *current)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown xml parameter %s", current->name);
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, (char*)current->name, content);
	g_free(content);
}

//static void xml_vcard_handle_parameter(OSyncHookTables *hooks, VFormatAttribute *attr, xmlNode *current)
//{
//	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, hooks, attr, current, current ? (char *)current->name : "None");
//	
//	//Find the handler for this parameter
//	void (* xml_param_handler)(VFormatAttribute *attr, xmlNode *);
//	char *content = (char*)xmlNodeGetContent(current);
//	char *paramname = g_strdup_printf("%s=%s", current->name, content);
//	g_free(content);
//	xml_param_handler = g_hash_table_lookup(hooks->parameters, paramname);
//	g_free(paramname);
//	if (!xml_param_handler)
//		xml_param_handler = g_hash_table_lookup(hooks->parameters, current->name);
//	
//	if (xml_param_handler == HANDLE_IGNORE) {
//		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
//		return;
//	}
//	
//	if (xml_param_handler)
//		xml_param_handler(attr, current);
//	
//	osync_trace(TRACE_EXIT, "%s", __func__);
//}

//static VFormatAttribute *xml_handle_unknown_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
//{
//	osync_trace(TRACE_INTERNAL, "Handling unknown xml attribute %s", osync_xmlfield_get_name(xmlfield));
//	char *name = osxml_find_node(root, "NodeName");
//	VFormatAttribute *attr = vformat_attribute_new(NULL, name);
//	add_value(attr, xmlfield, "Content", encoding);
//	vformat_add_attribute(vcard, attr);
//	return attr;
//}

static VFormatAttribute *handle_xml_formatted_name_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling formatted name xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "FN");
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

static void xml_vcard_handle_attribute(OSyncHookTables *hooks, VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, hooks, vcard, xmlfield, xmlfield ? osync_xmlfield_get_name(xmlfield) : "None");
	VFormatAttribute *attr = NULL;
	
	//We need to find the handler for this attribute
	VFormatAttribute *(* xml_attr_handler)(VFormat *vcard, OSyncXMLField *xmlfield, const char *) = g_hash_table_lookup(hooks->attributes, osync_xmlfield_get_name(xmlfield));
	osync_trace(TRACE_INTERNAL, "xml hook is: %p", xml_attr_handler);
	if (xml_attr_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	if (xml_attr_handler)
		attr = xml_attr_handler(vcard, xmlfield, encoding);
	else {
		osync_trace(TRACE_EXIT, "%s: Ignored2", __func__);
		return;
	}
	
	//TODO: Handle all parameters of this attribute
//	xmlNode *child = root->xmlChildrenNode;
//	while (child) {
//		xml_vcard_handle_parameter(hooks, attr, child);
//		child = child->next;
//	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static VFormatAttribute *handle_xml_photo_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling photo xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "PHOTO");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_attribute_add_param_with_value(attr, "ENCODING", "b");
//	vformat_attribute_add_param_with_value(attr, "TYPE", osxml_find_node(xmlfield, "Type"));
	vformat_attribute_add_param_with_value(attr, "TYPE", osync_xmlfield_get_attr(xmlfield, "Type"));
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

static VFormatAttribute *handle_xml_address_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling address xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ADR");
	add_value(attr, xmlfield, "PostalBox", encoding);
	add_value(attr, xmlfield, "ExtendedAddress", encoding);
	add_value(attr, xmlfield, "Street", encoding);
	add_value(attr, xmlfield, "City", encoding);
	add_value(attr, xmlfield, "Region", encoding);
	add_value(attr, xmlfield, "PostalCode", encoding);
	add_value(attr, xmlfield, "Country", encoding);
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

static VFormatAttribute *handle_xml_telephone_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling telephone xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TEL");
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

static VFormatAttribute *handle_xml_mailer_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling mailer xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "MAILER");
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

static VFormatAttribute *handle_xml_location_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling location xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "GEO");
	add_value(attr, xmlfield, "Latitude", encoding);
	add_value(attr, xmlfield, "Longitude", encoding);
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

static VFormatAttribute *handle_xml_role_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling role xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ROLE");
	add_value(attr, xmlfield, "Content", encoding);
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

static VFormatAttribute *handle_xml_organization_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling organization xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ORG");
	add_value(attr, xmlfield, "Name", encoding);
	add_value(attr, xmlfield, "Department", encoding);
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

static VFormatAttribute *handle_xml_revision_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling revision xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "REV");
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
	//vformat_attribute_add_param_with_value(attr, "TYPE", osxml_find_node(xmlfield, "Type"));
	vformat_attribute_add_param_with_value(attr, "TYPE", osync_xmlfield_get_attr(xmlfield, "Type"));
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_url_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling url xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "URL");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_uid_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling uid xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "UID");
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

static VFormatAttribute *handle_xml_nickname_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling nickname xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "NICKNAME");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_class_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling class xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CLASS");
	add_value(attr, xmlfield, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_categories_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling categories xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CATEGORIES");
	vformat_add_attribute(vcard, attr);
	return attr;
}

static osync_bool conv_xmlformat_to_vcard(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error, int target)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);

	OSyncXMLFormat *xmlformat;
	OSyncXMLField *xmlfield;
	
	xmlformat = (OSyncXMLFormat *)input;

//	xmlChar *str = osxml_write_to_string((xmlDoc *)input);
//	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", str);
//	xmlFree(str);

//	//Get the root node of the input document
//	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);
//	if (!root) {
//		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
//		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
//		return FALSE;
//	}

	
//	if (xmlStrcmp(root->name, (const xmlChar *)"contact")) {
//		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xml root element");
//		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
//		return FALSE;
//	}


	//Make the new vcard
	VFormat *vcard = vformat_new();
	
	osync_trace(TRACE_INTERNAL, "parsing cml attributes");
	const char *std_encoding = NULL;
	if (target == VFORMAT_CARD_21)
		std_encoding = "QUOTED-PRINTABLE";
	else
		std_encoding = "B";
	
//	if (root)
//		root = root->children;
//	while (root) {
//		xml_vcard_handle_attribute((OSyncHookTables *)user_data, vcard, root, std_encoding);
//		root = root->next;
//	}

	xmlfield = osync_xmlformat_get_first_field(xmlformat);
	while(xmlfield)
	{
		xml_vcard_handle_attribute((OSyncHookTables *)user_data, vcard, xmlfield, std_encoding);
	}
	
	*free_input = TRUE;
	*output = vformat_to_string(vcard, target);
	osync_trace(TRACE_INTERNAL, "vcard output is: \n%s", *output);
	*outpsize = strlen(*output) + 1;

	osync_trace(TRACE_EXIT, "%s", __func__);
	
	return TRUE;
}

//static osync_bool conv_xmlformat_to_vcard30(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
//{
//	return conv_xmlformat_to_vcard(user_data, input, inpsize, output, outpsize, free_input, error, VFORMAT_CARD_30);
//}
//
//static osync_bool conv_xmlformat_to_vcard21(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
//{
//	return conv_xmlformat_to_vcard(user_data, input, inpsize, output, outpsize, free_input, error, VFORMAT_CARD_21);
//}

static OSyncConvCmpResult compare_contact(OSyncChange *leftchange, OSyncChange *rightchange)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftchange, rightchange);
	
//	OSyncXMLScore score[] =
//	{
//		//{30, "/contact/FullName"},
//		//{20, "/contact/Telephone"},
//		//{20, "/contact/Address"},
//		//{1, "/contact/UnknownNode"},
//		//{0, "/contact/*/Slot"},
//		//{0, "/contact/*/Type"},
//		{0,		"/contact/Class"},
//		{0,		"/contact/FileAs"},
//		{100,	"/contact/Name"},
//		{0,		"/contact/Revision"},
//		{0,		"/contact/Uid"},
//		{0,		"/contact/WantsHtml"},
//		//{0, "/contact/UnknownNode[NodeName=\"X-IRMC-LUID\"]"},
//		{0, NULL}
//	};

//	OSyncXMLFormat *xmlformat_left = (OSyncXMLFormat *) osync_change_get_data(leftchange);
//	OSyncXMLFormat *xmlformat_right = (OSyncXMLFormat *) osync_change_get_data(rightchange);

//	OSyncConvCmpResult ret = osxml_compare(xmlformat_left->doc, xmlformat_right->doc, score, 0, 99);
	

	OSyncConvCmpResult ret = TRUE; //osync_xmlformat_compare(xmlformat_left, xmlformat_right, xmlscore);
	
		
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static char *print_contact(OSyncChange *change)
{
//	char *buffer;
//	osync_xmlformat_assemble()
//	xmlDoc *doc = (xmlDoc *)osync_change_get_data(change);
	
//	return (char *)osxml_write_to_string(doc);
	return NULL;
}

static void destroy_xmlformat(char *data, size_t size)
{
	osync_xmlformat_unref((OSyncXMLFormat *)data);
}

static void *init_vcard_to_xmlformat(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	GHashTable *table = g_hash_table_new(g_str_hash, g_str_equal);
	
	g_hash_table_insert(table, "FN", handle_formatted_name_attribute);
	g_hash_table_insert(table, "N", handle_name_attribute);
	g_hash_table_insert(table, "PHOTO", handle_photo_attribute);
	g_hash_table_insert(table, "BDAY", handle_birthday_attribute);
	g_hash_table_insert(table, "ADR", handle_address_attribute);
	g_hash_table_insert(table, "LABEL", handle_label_attribute);
	g_hash_table_insert(table, "TEL", handle_telephone_attribute);
	g_hash_table_insert(table, "EMAIL", handle_email_attribute);
	g_hash_table_insert(table, "MAILER", handle_mailer_attribute);
	g_hash_table_insert(table, "TZ", handle_timezone_attribute);
	g_hash_table_insert(table, "GEO", handle_location_attribute);
	g_hash_table_insert(table, "TITLE", handle_title_attribute);
	g_hash_table_insert(table, "ROLE", handle_role_attribute);
	g_hash_table_insert(table, "LOGO", handle_logo_attribute);
	g_hash_table_insert(table, "ORG", handle_organization_attribute);
	g_hash_table_insert(table, "NOTE", handle_note_attribute);
	g_hash_table_insert(table, "REV", handle_revision_attribute);
	g_hash_table_insert(table, "SOUND", handle_sound_attribute);
	g_hash_table_insert(table, "URL", handle_url_attribute);
	g_hash_table_insert(table, "UID", handle_uid_attribute);
	g_hash_table_insert(table, "KEY", handle_key_attribute);
	g_hash_table_insert(table, "NICKNAME", handle_nickname_attribute);
	g_hash_table_insert(table, "CLASS", handle_class_attribute);
	g_hash_table_insert(table, "CATEGORIES", handle_categories_attribute);
	
	g_hash_table_insert(table, "VERSION", HANDLE_IGNORE);
	g_hash_table_insert(table, "BEGIN", HANDLE_IGNORE);
	g_hash_table_insert(table, "END", HANDLE_IGNORE);
	
	g_hash_table_insert(table, "ENCODING", HANDLE_IGNORE);
	g_hash_table_insert(table, "CHARSET", HANDLE_IGNORE);
	
//	g_hash_table_insert(table, "TYPE", handle_type_parameter);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, table);
	return (void *)table;
}

static void fin_vcard_to_xmlformat(void *data)
{
	g_hash_table_destroy((GHashTable *)data);
}

static void *init_xmlformat_to_vcard(void)
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
	
	g_hash_table_insert(hooks->parameters, "Type", handle_xml_type_parameter);
	g_hash_table_insert(hooks->parameters, "Category", handle_xml_category_parameter);
	g_hash_table_insert(hooks->parameters, "Unit", handle_xml_unit_parameter);
	
	g_hash_table_insert(hooks->parameters, "UnknownParameter", xml_handle_unknown_parameter);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static void fin_xmlformat_to_vcard(void *data)
{
	OSyncHookTables *hooks = (OSyncHookTables *)hooks;
	g_hash_table_destroy(hooks->attributes);
	g_hash_table_destroy(hooks->parameters);
	g_free(hooks);
}

//static time_t get_revision(OSyncChange *change, OSyncError **error)
//{	
//	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, change, error);
//	
//	xmlDoc *doc = (xmlDoc *)osync_change_get_data(change);
//	
//	xmlXPathObject *xobj = osxml_get_nodeset(doc, "/contact/Revision");
//		
//	xmlNodeSet *nodes = xobj->nodesetval;
//		
//	int size = (nodes) ? nodes->nodeNr : 0;
//	if (size != 1) {
//		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find the revision");
//		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
//		return -1;
//	}
//	
//	char *revision = (char*)osxml_find_node(nodes->nodeTab[0], "Content");
//	
//	osync_trace(TRACE_INTERNAL, "About to convert string %s", revision);
//	time_t time = vformat_time_to_unix(revision);
//	g_free(revision);
//	xmlXPathFreeObject(xobj);
//	osync_trace(TRACE_EXIT, "%s: %i", __func__, time);
//	return time;
//}

//void get_info(OSyncEnv *env)
//{
//	osync_env_register_objtype(env, "contact");
//	osync_env_register_objformat(env, "contact", "xmlformat-contact");
//	osync_env_format_set_compare_func(env, "xmlformat-contact", compare_contact);
//	osync_env_format_set_destroy_func(env, "xmlformat-contact", destroy_xmlformat);
//	osync_env_format_set_print_func(env, "xmlformat-contact", print_contact);
////	osync_env_format_set_copy_func(env, "xmlformat-contact", osxml_copy);
//	osync_env_format_set_revision_func(env, "xmlformat-contact", get_revision);
//	
//	osync_env_register_converter(env, CONVERTER_CONV, "vcard21", "xmlformat-contact", conv_vcard_to_xmlformat);
//	osync_env_converter_set_init(env, "vcard21", "xmlformat-contact", init_vcard_to_xmlformat, fin_vcard_to_xmlformat);
//	osync_env_register_converter(env, CONVERTER_CONV, "xmlformat-contact", "vcard21", conv_xmlformat_to_vcard21);
//	osync_env_converter_set_init(env, "xmlformat-contact", "vcard21", init_xmlformat_to_vcard, fin_xmlformat_to_vcard);
//	
//	osync_env_register_converter(env, CONVERTER_CONV, "vcard30", "xmlformat-contact", conv_vcard_to_xmlformat);
//	osync_env_converter_set_init(env, "vcard30", "xmlformat-contact", init_vcard_to_xmlformat, fin_vcard_to_xmlformat);
//	osync_env_register_converter(env, CONVERTER_CONV, "xmlformat-contact", "vcard30", conv_xmlformat_to_vcard30);
//	osync_env_converter_set_init(env, "xmlformat-contact", "vcard30", init_xmlformat_to_vcard, fin_xmlformat_to_vcard);	
//}
