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
#include <glib.h>

static osync_bool conv_vnote_to_xml(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_debug("VNOTE", 4, "start: %s", __func__);
	printf("input is %i\n%s\n", inpsize, input);
	
	/*GList *p = NULL;
	GList *a = NULL;
	
	VFormat *vcard = vformat_new_from_string(input);
	vformat_dump_structure (vcard);
	GList *attributes = vformat_get_attributes(vcard);
	xmlDoc *doc = xmlNewDoc("1.0");
	xmlNode *root = osxml_node_add_root(doc, "note");
	
	for (a = attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;
		const char *name = vformat_attribute_get_name(attr);

		if (!strcmp(name, "VERSION"))
			continue;
		
		if (!strcmp(name, "BEGIN")) {
			root = osxml_node_add_root(doc, "note");
			continue;
		}
		
		xmlNode *current = xmlNewChild(root, NULL, "", NULL);
		
		//Created
		if (!strcmp(name, "DCREATED")) {
			osxml_node_set(current, "Created", vformat_attribute_get_nth_value(attr, 0), encoding);
			continue;
		}

		//LastModified
		if (!strcmp(name, "LAST_MODIFIED")) {
			osxml_node_set(current, "LastModified", vformat_attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Summary
		if (!strcmp(name, "SUMMARY")) {
			osxml_node_set(current, "Summary", vformat_attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Body
		if (!strcmp(name, "BODY")) {
			osxml_node_set(current, "Body", vformat_attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Categories
		if (!strcmp(name, "CATEGORIES")) {
			osxml_node_set(current, "Categories", vformat_attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Class
		if (!strcmp(name, "CLASS")) {
			osxml_node_set(current, "Class", vformat_attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Unknown tag.
		osxml_node_mark_unknown(current);
		GList *values = vformat_attribute_get_values(attr);
		GString *string = g_string_new(vformat_attribute_get_nth_value(attr, 0));
		for (p = values->next; p; p = p->next) {
			g_string_sprintfa(string, ";%s", (char *)p->data);
		}
		osxml_node_add(current, "NodeName", name);
		osxml_node_set(current, "UnknownNode", string->str, encoding);
		g_string_free(string, 1);
	}*/
	
	*free_input = TRUE;
	//*output = (char *)doc;
	/*FIXME: this is not really the size of the data pointed by doc.
	 * But this shouldn't cause problems, anyway, because this
	 * size field should never be used for xml docs. Actually,
	 * what needs fixing is the code that uses the size
	 * field for changes with xml data.
	 */
	//*outpsize = sizeof(doc);
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

static void add_value(VFormatAttribute *attr, xmlNode *parent, const char *name, const char *encoding)
{
	char *tmp = osxml_find_node(parent, name);
	if (!tmp)
		return;
	
	if (needs_charset(tmp))
		if (!vformat_attribute_has_param (attr, "CHARSET"))
			vformat_attribute_add_param_with_value(attr, "CHARSET", "UTF-8");
	
	if (needs_encoding(tmp, encoding)) {
		if (!vformat_attribute_has_param (attr, "ENCODING"))
			vformat_attribute_add_param_with_value(attr, "ENCODING", encoding);
		vformat_attribute_add_value_decoded(attr, tmp, strlen(tmp) + 1);
	} else
		vformat_attribute_add_value(attr, tmp);
	g_free(tmp);
}

static osync_bool conv_xml_to_vnote(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_debug("VNOTE", 4, "start: %s", __func__);
	xmlDocDump(stdout, (xmlDoc *)input);
	VFormatAttribute *attr = NULL;
	xmlNode *root = osxml_node_get_root((xmlDoc *)input, "note", error);
	if (!root)
		return FALSE;
	
	VFormat *vcard = vformat_new();
	
	const char *std_encoding = "QUOTED-PRINTABLE";
		
	while (root) {
		if (!strcmp(root->name, "Created")) {
			//Created
			attr = vformat_attribute_new(NULL, "DCREATED");
			add_value(attr, root, "Content", std_encoding);
			vformat_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "LastModifed")) {
			//LastModifed
			attr = vformat_attribute_new(NULL, "LAST-MODIFIED");
			add_value(attr, root, "Content", std_encoding);
			vformat_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Summary")) {
			//Summary
			attr = vformat_attribute_new(NULL, "SUMMARY");
			add_value(attr, root, "Content", std_encoding);
			vformat_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Body")) {
			//Body
			attr = vformat_attribute_new(NULL, "BODY");
			add_value(attr, root, "Content", std_encoding);
			vformat_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Categories")) {
			//Categories
			attr = vformat_attribute_new(NULL, "CATEGORIES");
			add_value(attr, root, "Content", std_encoding);
			vformat_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Class")) {
			//Class
			attr = vformat_attribute_new(NULL, "CLASS");
			add_value(attr, root, "Content", std_encoding);
			vformat_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "UnknownNode")) {
			//Unknown Node
			attr = vformat_attribute_new(NULL, osxml_find_node(root, "NodeName"));
			add_value(attr, root, "Content", std_encoding);
			vformat_add_attribute(vcard, attr);
		}
		
		root = root->next;
	}
	
	*free_input = TRUE;
	*output = vformat_to_string(vcard, VFORMAT_NOTE);
	*outpsize = strlen(*output);
	return TRUE;
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
	
	return osxml_compare((xmlDoc*)osync_change_get_data(leftchange), (xmlDoc*)osync_change_get_data(rightchange), score, 10, 50);
}

static char *print_note(OSyncChange *change)
{
	osync_debug("VNOTE", 4, "start: %s", __func__);
	xmlDoc *doc = (xmlDoc *)osync_change_get_data(change);
	char *result;
	int size;
	osync_bool free;
	if (!conv_xml_to_vnote(NULL, (char*)doc, 0, &result, &size, &free, NULL))
		return NULL;
	return result;
}

static void destroy_xml(char *data, size_t size)
{
	xmlFreeDoc((xmlDoc *)data);
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "note");
	osync_env_register_objformat(env, "note", "xml-note");
	
	osync_env_format_set_compare_func(env, "xml-note", compare_notes);
	osync_env_format_set_destroy_func(env, "xml-note", destroy_xml);
	osync_env_format_set_print_func(env, "xml-note", print_note);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vnote11", "xml-note", conv_vnote_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-note", "vnote11", conv_xml_to_vnote);
}
