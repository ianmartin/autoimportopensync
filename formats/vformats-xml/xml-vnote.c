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
#include "e-vcard.h"
#include <glib.h>

static const char *property_get_nth_value(EVCardAttributeParam *param, int nth)
{
	const char *ret = NULL;
	GList *values = e_vcard_attribute_param_get_values(param);
	if (!values)
		return NULL;
	ret = g_list_nth_data(values, nth);
	//g_list_free(values);
	return ret;
}

static const char *attribute_get_nth_value(EVCardAttribute *attr, int nth)
{
	GList *values = e_vcard_attribute_get_values_decoded(attr);
	if (!values)
		return NULL;
	GString *retstr = (GString *)g_list_nth_data(values, nth);
	if (!retstr)
		return NULL;
	
	if (!g_utf8_validate(retstr->str, -1, NULL)) {
		values = e_vcard_attribute_get_values(attr);
		if (!values)
			return NULL;
		return g_list_nth_data(values, nth);
	}
	
	return retstr->str;
}

static OSyncXMLEncoding property_to_xml_encoding(EVCardAttribute *attr)
{
	OSyncXMLEncoding encoding;
	memset(&encoding, 0, sizeof(encoding));
	
	encoding.charset = OSXML_UTF8;
	encoding.encoding = OSXML_8BIT;
	
	GList *params = e_vcard_attribute_get_params(attr);
	GList *p;
	for (p = params; p; p = p->next) {
		EVCardAttributeParam *param = p->data;
		if (!strcmp("ENCODING", e_vcard_attribute_param_get_name(param))) {
			if (!g_ascii_strcasecmp(property_get_nth_value(param, 0), "b"))
				encoding.encoding = OSXML_BASE64;
		}
	}
	return encoding;
}

static osync_bool conv_vnote_to_xml(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_debug("VNOTE", 4, "start: %s", __func__);
	printf("input is %i\n%s\n", inpsize, input);
	GList *p = NULL;
	GList *a = NULL;
	OSyncXMLEncoding encoding;
	
	g_type_init();
	EVCard *vcard = e_vcard_new_from_string(input);
	e_vcard_dump_structure (vcard);
	GList *attributes = e_vcard_get_attributes(vcard);
	xmlDoc *doc = xmlNewDoc("1.0");
	xmlNode *root = osxml_node_add_root(doc, "note");
	
	for (a = attributes; a; a = a->next) {
		EVCardAttribute *attr = a->data;
		encoding = property_to_xml_encoding(attr);
		const char *name = e_vcard_attribute_get_name(attr);

		if (!strcmp(name, "VERSION"))
			continue;
		
		if (!strcmp(name, "BEGIN")) {
			root = osxml_node_add_root(doc, "note");
			continue;
		}
		
		xmlNode *current = xmlNewChild(root, NULL, "", NULL);
		
		//Created
		if (!strcmp(name, "DCREATED")) {
			osxml_node_set(current, "Created", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}

		//LastModified
		if (!strcmp(name, "LAST_MODIFIED")) {
			osxml_node_set(current, "LastModified", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Summary
		if (!strcmp(name, "SUMMARY")) {
			osxml_node_set(current, "Summary", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Body
		if (!strcmp(name, "BODY")) {
			osxml_node_set(current, "Body", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Categories
		if (!strcmp(name, "CATEGORIES")) {
			osxml_node_set(current, "Categories", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Class
		if (!strcmp(name, "CLASS")) {
			osxml_node_set(current, "Class", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Unknown tag.
		osxml_node_mark_unknown(current);
		GList *values = e_vcard_attribute_get_values(attr);
		GString *string = g_string_new(attribute_get_nth_value(attr, 0));
		for (p = values->next; p; p = p->next) {
			g_string_sprintfa(string, ";%s", (char *)p->data);
		}
		osxml_node_add(current, "NodeName", name);
		osxml_node_set(current, "UnknownNode", string->str, encoding);
		g_string_free(string, 1);
	}
	
	*output = (char *)doc;
	/*FIXME: this is not really the size of the data pointed by doc.
	 * But this shouldn't cause problems, anyway, because this
	 * size field should never be used for xml docs. Actually,
	 * what needs fixing is the code that uses the size
	 * field for changes with xml data.
	 */
	*outpsize = sizeof(doc);
	return TRUE;
}

static void add_parameter(EVCardAttribute *attr, const char *name, const char *data)
{
	EVCardAttributeParam *param = e_vcard_attribute_param_new(name);
	if (data)
		e_vcard_attribute_add_param_with_value(attr, param, data);
	else
		e_vcard_attribute_add_param(attr, param);
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
		return !g_utf8_validate(tmp, -1, NULL);
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

static osync_bool has_param(EVCardAttribute *attr, const char *name)
{
	GList *params = e_vcard_attribute_get_params(attr);
	GList *p;
	for (p = params; p; p = p->next) {
		EVCardAttributeParam *param = p->data;
		if (!strcmp(name, e_vcard_attribute_param_get_name(param)))
			return TRUE;
	}
	return FALSE;
}

static void add_value(EVCardAttribute *attr, xmlNode *parent, const char *name, const char *encoding)
{
	char *tmp = osxml_find_node(parent, name);
	if (!tmp)
		return;
	
	if (needs_charset(tmp))
		if (!has_param (attr, "CHARSET"))
			add_parameter(attr, "CHARSET", "UTF-8");
	
	if (needs_encoding(tmp, encoding)) {
		if (!has_param (attr, "ENCODING"))
			add_parameter(attr, "ENCODING", encoding);
		e_vcard_attribute_add_value_decoded(attr, tmp, strlen(tmp) + 1);
	} else
		e_vcard_attribute_add_value(attr, tmp);
	g_free(tmp);
}

static osync_bool conv_xml_to_vnote(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_debug("VNOTE", 4, "start: %s", __func__);
	xmlDocDump(stdout, (xmlDoc *)input);
	EVCardAttribute *attr = NULL;
	xmlNode *root = osxml_node_get_root((xmlDoc *)input, "note", error);
	if (!root)
		return FALSE;
	
	g_type_init();
	EVCard *vcard = e_vcard_new();
	
	const char *std_encoding = "QUOTED-PRINTABLE";
		
	while (root) {
		if (!strcmp(root->name, "Created")) {
			//Created
			attr = e_vcard_attribute_new(NULL, "DCREATED");
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "LastModifed")) {
			//LastModifed
			attr = e_vcard_attribute_new(NULL, "LAST-MODIFIED");
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Summary")) {
			//Summary
			attr = e_vcard_attribute_new(NULL, "SUMMARY");
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Body")) {
			//Body
			attr = e_vcard_attribute_new(NULL, "BODY");
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Categories")) {
			//Categories
			attr = e_vcard_attribute_new(NULL, "CATEGORIES");
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Class")) {
			//Class
			attr = e_vcard_attribute_new(NULL, "CLASS");
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "UnknownNode")) {
			//Unknown Node
			attr = e_vcard_attribute_new(NULL, osxml_find_node(root, "NodeName"));
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		}
		
		root = root->next;
	}
	
	//*output = e_vnote_to_string(vcard);
	//*outpsize = strlen(*output);
	*output = NULL;
	*outpsize = 0;
	return FALSE;
}

static OSyncConvCmpResult compare_notes(OSyncChange *leftchange, OSyncChange *rightchange)
{
	OSyncXMLScore score[] =
	{
	{50, "/note/Summary"},
	{50, "/note/Class"},
	{50, "/note/Body"},
	{0, NULL}
	};
	
	return osxml_compare((xmlDoc*)osync_change_get_data(leftchange), (xmlDoc*)osync_change_get_data(rightchange), score);
}

static char *print_note(OSyncChange *change)
{
	osync_debug("VNOTE", 4, "start: %s", __func__);
	xmlDoc *doc = (xmlDoc *)osync_change_get_data(change);
	char *result;
	int size;
	if (!conv_xml_to_vnote((char*)doc, 0, &result, &size, NULL))
		return NULL;
	return result;
}

static void destroy_xml(char *data, size_t size)
{
	xmlFreeDoc((xmlDoc *)data);
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "note");
	OSyncObjFormat *mxml = osync_conv_register_objformat(env, "note", "xml-note");
	
	osync_conv_format_set_compare_func(mxml, compare_notes);
	osync_conv_format_set_destroy_func(mxml, destroy_xml);
	osync_conv_format_set_print_func(mxml, print_note);
	
	osync_conv_register_converter(env, CONVERTER_CONV, "vnote11", "xml-note", conv_vnote_to_xml, 0);
	osync_conv_register_converter(env, CONVERTER_CONV, "xml-note", "vnote11", conv_xml_to_vnote, 0);
}
