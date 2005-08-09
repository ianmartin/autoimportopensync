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
 
#include "opensync-xml.h"
#include "vformat.h"
#include "xml-vcard.h"
#include <glib.h>

static void handle_unknown_parameter(xmlNode *current, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown parameter %s", vformat_attribute_param_get_name(param));
	xmlNode *property = xmlNewChild(current, NULL, (xmlChar*)"UnknownParam",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
	osxml_node_add(property, "ParamName", vformat_attribute_param_get_name(param));
}

static void handle_type_parameter(xmlNode *current, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling type parameter %s", vformat_attribute_param_get_name(param));
	xmlNewChild(current, NULL, (xmlChar*)"Type",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
}

static xmlNode *handle_fullname_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling fullname attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"FullName", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_name_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling name attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Name", NULL);
	osxml_node_add(current, "LastName", vformat_attribute_get_nth_value(attr, 0));
	osxml_node_add(current, "FirstName", vformat_attribute_get_nth_value(attr, 1));
	osxml_node_add(current, "Additional", vformat_attribute_get_nth_value(attr, 2));
	osxml_node_add(current, "Prefix", vformat_attribute_get_nth_value(attr, 3));
	osxml_node_add(current, "Suffix", vformat_attribute_get_nth_value(attr, 4));
	return current;
}

static xmlNode *handle_photo_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling photo attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Photo", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_birthday_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling birthday attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Birthday", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_address_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling address attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Address", NULL);
	osxml_node_add(current, "PostalBox", vformat_attribute_get_nth_value(attr, 0));
	osxml_node_add(current, "ExtendedAddress", vformat_attribute_get_nth_value(attr, 1));
	osxml_node_add(current, "Street", vformat_attribute_get_nth_value(attr, 2));
	osxml_node_add(current, "City", vformat_attribute_get_nth_value(attr, 3));
	osxml_node_add(current, "Region", vformat_attribute_get_nth_value(attr, 4));
	osxml_node_add(current, "PostalCode", vformat_attribute_get_nth_value(attr, 5));
	osxml_node_add(current, "Country", vformat_attribute_get_nth_value(attr, 6));
	return current;
}

static xmlNode *handle_label_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling AddressLabel attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"AddressLabel", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_telephone_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Telephone attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Telephone", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_email_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling EMail attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"EMail", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_mailer_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Mailer attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Mailer", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_timezone_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Timezone attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Timezone", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_location_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Location attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Location", NULL);
	osxml_node_add(current, "Latitude", vformat_attribute_get_nth_value(attr, 0));
	osxml_node_add(current, "Longitude", vformat_attribute_get_nth_value(attr, 1));
	return current;
}

static xmlNode *handle_title_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Title attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Title", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_role_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Role attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Role", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_logo_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Logo attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Logo", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_organization_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Organization attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Organization", NULL);
	osxml_node_add(current, "Name", vformat_attribute_get_nth_value(attr, 0));
	osxml_node_add(current, "Department", vformat_attribute_get_nth_value(attr, 1));
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	values = g_list_nth(values, 2);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osxml_node_add(current, "Unit", retstr->str);
	}
	return current;
}

static xmlNode *handle_note_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Note attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Note", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_revision_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Revision attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Revision", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_sound_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Sound attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Sound", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_url_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Url attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Url", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_uid_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Uid attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Uid", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_key_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Key attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Key", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_nickname_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Nickname attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Nickname", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_class_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Class attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Class", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_categories_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Categories attribute");
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"Categories", NULL);
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osxml_node_add(current, "Category", retstr->str);
	}
	
	return current;
}

static xmlNode *handle_unknown_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown attribute %s", vformat_attribute_get_name(attr));
	xmlNode *current = xmlNewChild(root, NULL, (xmlChar*)"UnknownNode", NULL);
	osxml_node_add(current, "NodeName", vformat_attribute_get_name(attr));
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osxml_node_add(current, "Content", retstr->str);
	}
	return current;
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

static void vcard_handle_attribute(GHashTable *hooks, xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, hooks, root, attr, attr ? vformat_attribute_get_name(attr) : "None");
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
	xmlNode *(* attr_handler)(xmlNode *, VFormatAttribute *) = g_hash_table_lookup(hooks, vformat_attribute_get_name(attr));
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
		vcard_handle_parameter(hooks, current, param);
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static osync_bool conv_vcard_to_xml(void *conv_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, conv_data, input, inpsize, output, outpsize, free_input, error);
	
	GHashTable *hooks = (GHashTable *)conv_data;
	
	osync_trace(TRACE_INTERNAL, "Input Vcard is:\n%s", input);
	
	//Parse the vcard
	VFormat *vcard = vformat_new_from_string(input);
	
	osync_trace(TRACE_INTERNAL, "Creating xml doc");
	
	//Create a new xml document
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "contact");
	
	osync_trace(TRACE_INTERNAL, "parsing attributes");
	
	//For every attribute we have call the handling hook
	GList *attributes = vformat_get_attributes(vcard);
	GList *a = NULL;
	for (a = attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;
		vcard_handle_attribute(hooks, root, attr);
	}
	
	osync_trace(TRACE_INTERNAL, "Output XML is:\n%s", osxml_write_to_string(doc));
	
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
	char *tmp = osxml_find_node(parent, name);
	if (!tmp)
		return;
	
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
	vformat_attribute_add_param_with_value(attr, "TYPE", content);
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

static void xml_vcard_handle_parameter(OSyncHookTables *hooks, VFormatAttribute *attr, xmlNode *current)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, hooks, attr, current, current ? (char *)current->name : "None");
	
	//Find the handler for this parameter
	void (* xml_param_handler)(VFormatAttribute *attr, xmlNode *);
	char *content = (char*)xmlNodeGetContent(current);
	char *paramname = g_strdup_printf("%s=%s", current->name, content);
	g_free(content);
	xml_param_handler = g_hash_table_lookup(hooks->parameters, paramname);
	g_free(paramname);
	if (!xml_param_handler)
		xml_param_handler = g_hash_table_lookup(hooks->parameters, current->name);
	
	if (xml_param_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	
	if (xml_param_handler)
		xml_param_handler(attr, current);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
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

static VFormatAttribute *handle_xml_fullname_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling fullname xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "FN");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_name_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling fullname xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "N");
	add_value(attr, root, "LastName", encoding);
	add_value(attr, root, "FirstName", encoding);
	add_value(attr, root, "Additional", encoding);
	add_value(attr, root, "Prefix", encoding);
	add_value(attr, root, "Suffix", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static void xml_vcard_handle_attribute(OSyncHookTables *hooks, VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, hooks, vcard, root, root ? (char *)root->name : "None");
	VFormatAttribute *attr = NULL;
	
	//We need to find the handler for this attribute
	VFormatAttribute *(* xml_attr_handler)(VFormat *vcard, xmlNode *root, const char *) = g_hash_table_lookup(hooks->attributes, root->name);
	osync_trace(TRACE_INTERNAL, "xml hook is: %p", xml_attr_handler);
	if (xml_attr_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	if (xml_attr_handler)
		attr = xml_attr_handler(vcard, root, encoding);
	else {
		osync_trace(TRACE_EXIT, "%s: Ignored2", __func__);
		return;
	}
	
	//Handle all parameters of this attribute
	xmlNode *child = root->xmlChildrenNode;
	while (child) {
		xml_vcard_handle_parameter(hooks, attr, child);
		child = child->next;
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static VFormatAttribute *handle_xml_photo_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling photo xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "PHOTO");
	add_value(attr, root, "Content", encoding);
	vformat_attribute_add_param_with_value(attr, "ENCODING", "b");
	vformat_attribute_add_param_with_value(attr, "TYPE", osxml_find_node(root, "Type"));
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_birthday_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling birthday xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "BDAY");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_address_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling address xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ADR");
	add_value(attr, root, "PostalBox", encoding);
	add_value(attr, root, "ExtendedAddress", encoding);
	add_value(attr, root, "Street", encoding);
	add_value(attr, root, "City", encoding);
	add_value(attr, root, "Region", encoding);
	add_value(attr, root, "PostalCode", encoding);
	add_value(attr, root, "Country", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_label_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling label xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LABEL");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_telephone_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling telephone xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TEL");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_email_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling email xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "EMAIL");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_mailer_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling mailer xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "MAILER");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_timezone_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling timezone xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TZ");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_location_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling location xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "GEO");
	add_value(attr, root, "Latitude", encoding);
	add_value(attr, root, "Longitude", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_title_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling title xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "TITLE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_role_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling role xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ROLE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_logo_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling logo xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LOGO");
	add_value(attr, root, "Content", encoding);
	vformat_attribute_add_param_with_value(attr, "ENCODING", "b");
	vformat_attribute_add_param_with_value(attr, "TYPE", osxml_find_node(root, "Type"));
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_organization_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling organization xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "ORG");
	add_value(attr, root, "Name", encoding);
	add_value(attr, root, "Department", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_note_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling note xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "NOTE");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_revision_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling revision xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "REV");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_sound_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling sound xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "SOUND");
	add_value(attr, root, "Content", encoding);
	vformat_attribute_add_param_with_value(attr, "ENCODING", "b");
	vformat_attribute_add_param_with_value(attr, "TYPE", osxml_find_node(root, "Type"));
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

static VFormatAttribute *handle_xml_key_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling key xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "KEY");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_nickname_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling nickname xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "NICKNAME");
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

static osync_bool conv_xml_to_vcard(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error, int target)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
	
	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", osxml_write_to_string((xmlDoc *)input));
	
	//Get the root node of the input document
	xmlNode *root = osxml_node_get_root((xmlDoc *)input, "contact", error);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get root element of xml-contact");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	//Make the new vcard
	VFormat *vcard = vformat_new();
	
	osync_trace(TRACE_INTERNAL, "parsing cml attributes");
	const char *std_encoding = NULL;
	if (target == VFORMAT_CARD_21)
		std_encoding = "QUOTED-PRINTABLE";
	else
		std_encoding = "B";
	
	while (root) {
		xml_vcard_handle_attribute((OSyncHookTables *)user_data, vcard, root, std_encoding);
		root = root->next;
	}
	
	*free_input = TRUE;
	*output = vformat_to_string(vcard, target);
	osync_trace(TRACE_INTERNAL, "vcard output is: \n%s", *output);
	*outpsize = strlen(*output) + 1;
	osync_trace(TRACE_EXIT, "%s", __func__);
	
	return TRUE;
}

static osync_bool conv_xml_to_vcard30(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	return conv_xml_to_vcard(user_data, input, inpsize, output, outpsize, free_input, error, VFORMAT_CARD_30);
}

static osync_bool conv_xml_to_vcard21(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	return conv_xml_to_vcard(user_data, input, inpsize, output, outpsize, free_input, error, VFORMAT_CARD_21);
}

static OSyncConvCmpResult compare_contact(OSyncChange *leftchange, OSyncChange *rightchange)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftchange, rightchange);
	
	OSyncXMLScore score[] =
	{
	//{100, "/contact/FullName"},
	{100, "/contact/Name"},
	//{20, "/contact/Telephone"},
	//{20, "/contact/Address"},
	//{1, "/contact/UnknownNode"},
	{0, "/contact/*/Slot"},
	{0, "/contact/*/Type"},
	{0, "/contact/WantsHtml"},
	{0, "/contact/Class"},
	{0, "/contact/FileAs"},
	{0, "/contact/Uid"},
	{0, "/contact/Revision"},
	{0, NULL}
	};
	
	OSyncConvCmpResult ret = osxml_compare((xmlDoc*)osync_change_get_data(leftchange), (xmlDoc*)osync_change_get_data(rightchange), score, 0, 99);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static char *print_contact(OSyncChange *change)
{
	xmlDoc *doc = (xmlDoc *)osync_change_get_data(change);
	
	return osxml_write_to_string(doc);
}

static void destroy_xml(char *data, size_t size)
{
	xmlFreeDoc((xmlDoc *)data);
}

static void *init_vcard_to_xml(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	GHashTable *table = g_hash_table_new(g_str_hash, g_str_equal);
	
	g_hash_table_insert(table, "FN", handle_fullname_attribute);
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
	
	g_hash_table_insert(table, "TYPE", handle_type_parameter);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, table);
	return (void *)table;
}

static void fin_vcard_to_xml(void *data)
{
	g_hash_table_destroy((GHashTable *)data);
}

static void *init_xml_to_vcard(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	
	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));
	
	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);
	
	g_hash_table_insert(hooks->attributes, "FullName", handle_xml_fullname_attribute);
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
	g_hash_table_insert(hooks->attributes, "UnknownNode", xml_handle_unknown_attribute);
	
	g_hash_table_insert(hooks->parameters, "Type", handle_xml_type_parameter);
	g_hash_table_insert(hooks->parameters, "Category", handle_xml_category_parameter);
	g_hash_table_insert(hooks->parameters, "Unit", handle_xml_unit_parameter);
	
	g_hash_table_insert(hooks->parameters, "UnknownParameter", xml_handle_unknown_parameter);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static void fin_xml_to_vcard(void *data)
{
	OSyncHookTables *hooks = (OSyncHookTables *)hooks;
	g_hash_table_destroy(hooks->attributes);
	g_hash_table_destroy(hooks->parameters);
	g_free(hooks);
}

static time_t get_revision(OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, change, error);
	
	xmlDoc *doc = (xmlDoc *)osync_change_get_data(change);
	
	xmlXPathObject *xobj = osxml_get_nodeset(doc, "/contact/Revision");
		
	xmlNodeSet *nodes = xobj->nodesetval;
		
	int size = (nodes) ? nodes->nodeNr : 0;
	if (size != 1) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find the revision");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return -1;
	}
	
	char *revision = (char*)osxml_find_node(nodes->nodeTab[0], "Content");
	
	osync_trace(TRACE_INTERNAL, "About to convert string %s", revision);
	time_t time = vformat_time_to_unix(revision);
	g_free(revision);
	xmlXPathFreeObject(xobj);
	osync_trace(TRACE_EXIT, "%s: %i", __func__, time);
	return time;
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "xml-contact");
	osync_env_format_set_compare_func(env, "xml-contact", compare_contact);
	osync_env_format_set_destroy_func(env, "xml-contact", destroy_xml);
	osync_env_format_set_print_func(env, "xml-contact", print_contact);
	osync_env_format_set_copy_func(env, "xml-contact", osxml_copy);
	osync_env_format_set_revision_func(env, "xml-contact", get_revision);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vcard21", "xml-contact", conv_vcard_to_xml);
	osync_env_converter_set_init(env, "vcard21", "xml-contact", init_vcard_to_xml, fin_vcard_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-contact", "vcard21", conv_xml_to_vcard21);
	osync_env_converter_set_init(env, "xml-contact", "vcard21", init_xml_to_vcard, fin_xml_to_vcard);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vcard30", "xml-contact", conv_vcard_to_xml);
	osync_env_converter_set_init(env, "vcard30", "xml-contact", init_vcard_to_xml, fin_vcard_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-contact", "vcard30", conv_xml_to_vcard30);
	osync_env_converter_set_init(env, "xml-contact", "vcard30", init_xml_to_vcard, fin_xml_to_vcard);
}
