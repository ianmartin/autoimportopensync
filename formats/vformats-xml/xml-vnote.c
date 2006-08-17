
/*
 * xml-vnote - A plugin for parsing vnote objects for the opensync framework
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
#include "xml-vnote.h"
#include <glib.h>

static void handle_unknown_parameter(xmlNode *current, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown parameter %s", vformat_attribute_param_get_name(param));
	xmlNode *property = xmlNewTextChild(current, NULL, (xmlChar*)"UnknownParam",
		(xmlChar*)vformat_attribute_param_get_nth_value(param, 0));
	osxml_node_add(property, "ParamName", vformat_attribute_param_get_name(param));
}

static xmlNode *handle_created_attribute(xmlNode *root, VFormatAttribute *attr)
{
	char *timestamp;
	const char *tmp;

	osync_trace(TRACE_INTERNAL, "Handling created attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"DateCreated", NULL);
	tmp = vformat_attribute_get_nth_value(attr, 0); 
	timestamp = osync_time_timestamp(tmp);
	osxml_node_add(current, "Content", timestamp);
	g_free(timestamp);
	return current;
}

static xmlNode *handle_last_modified_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling last_modified attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"LastModified", NULL);
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

static xmlNode *handle_body_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling body attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Body", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_class_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Class attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Class", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static void handle_type_parameter(xmlNode *current, VFormatParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling type parameter %s", vformat_attribute_param_get_name(param));
	xmlNewTextChild(current, NULL, (xmlChar*)"Type",
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

static void vnote_handle_parameter(GHashTable *hooks, xmlNode *current, VFormatParam *param)
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

static void vnote_handle_attribute(GHashTable *hooks, xmlNode *root, VFormatAttribute *attr)
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
		vnote_handle_parameter(hooks, current, param);
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static osync_bool conv_vnote_to_xml(void *conv_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, conv_data, input, inpsize, output, outpsize, free_input, error);
	
	GHashTable *hooks = (GHashTable *)conv_data;
	
	osync_trace(TRACE_SENSITIVE, "Input vnote is:\n%s", input);
	
	/* The input is not null-terminated, but vformat_new_from_string() expects a null-terminated string */
	char *input_str = g_malloc(inpsize + 1);
	memcpy(input_str, input, inpsize);
	input_str[inpsize] = '\0';

	//Parse the vnote
	VFormat *vnote = vformat_new_from_string(input_str);

	g_free(input_str);
	
	osync_trace(TRACE_INTERNAL, "Creating xml doc");
	
	//Create a new xml document
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "Note");
	
	osync_trace(TRACE_INTERNAL, "parsing attributes");
	
	//For every attribute we have call the handling hook
	GList *attributes = vformat_get_attributes(vnote);
	GList *a = NULL;
	for (a = attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;
		vnote_handle_attribute(hooks, root, attr);
	}
	
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

static void xml_handle_unknown_parameter(VFormatAttribute *attr, xmlNode *current)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown xml parameter %s", current->name);
	char *content = (char*)xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, (char*)current->name, content);
	g_free(content);
}

static VFormatAttribute *handle_xml_categories_attribute(VFormat *vnote, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling categories xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CATEGORIES");
	vformat_add_attribute(vnote, attr);
	return attr;
}

static VFormatAttribute *handle_xml_class_attribute(VFormat *vnote, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling class xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "CLASS");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vnote, attr);
	return attr;
}

static VFormatAttribute *handle_xml_summary_attribute(VFormat *vnote, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "SUMMARY");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vnote, attr);
	return attr;
}

static VFormatAttribute *handle_xml_body_attribute(VFormat *vnote, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "BODY");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vnote, attr);
	return attr;
}

static VFormatAttribute *handle_xml_created_attribute(VFormat *vnote, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "DCREATED");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vnote, attr);
	return attr;
}

static VFormatAttribute *handle_xml_last_modified_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	VFormatAttribute *attr = vformat_attribute_new(NULL, "LAST-MODIFIED");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static void xml_vnote_handle_parameter(OSyncHookTables *hooks, VFormatAttribute *attr, xmlNode *current)
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

static VFormatAttribute *xml_handle_unknown_attribute(VFormat *vnote, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown xml attribute %s", root->name);
	char *name = osxml_find_node(root, "NodeName");
	VFormatAttribute *attr = vformat_attribute_new(NULL, name);
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vnote, attr);
	return attr;
}

static void xml_vnote_handle_attribute(OSyncHookTables *hooks, VFormat *vnote, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, hooks, vnote, root, root ? (char *)root->name : "None");
	VFormatAttribute *attr = NULL;
	
	//We need to find the handler for this attribute
	VFormatAttribute *(* xml_attr_handler)(VFormat *vnote, xmlNode *root, const char *) = g_hash_table_lookup(hooks->attributes, root->name);
	osync_trace(TRACE_INTERNAL, "xml hook is: %p", xml_attr_handler);
	if (xml_attr_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	if (xml_attr_handler)
		attr = xml_attr_handler(vnote, root, encoding);
	else {
		osync_trace(TRACE_EXIT, "%s: Ignored2", __func__);
		return;
	}
	
	//Handle all parameters of this attribute
	xmlNode *child = root->xmlChildrenNode;
	while (child) {
		xml_vnote_handle_parameter(hooks, attr, child);
		child = child->next;
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static osync_bool conv_xml_to_vnote(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
	
	xmlChar *str = osxml_write_to_string((xmlDoc *)input);
	osync_trace(TRACE_SENSITIVE, "Input XML is:\n%s", str);
	xmlFree(str);
	
	//Get the root node of the input document
	xmlNode *root = osxml_node_get_root((xmlDoc *)input, "Note", error);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get root element of xml-note");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	//Make the new vnote
	VFormat *vnote = vformat_new();
	
	osync_trace(TRACE_INTERNAL, "parsing xml attributes");
	while (root) {
		xml_vnote_handle_attribute((OSyncHookTables *)user_data, vnote, root, "QUOTED-PRINTABLE");
		root = root->next;
	}
	
	*free_input = TRUE;
	*output = vformat_to_string(vnote, VFORMAT_NOTE);
	osync_trace(TRACE_SENSITIVE, "vnote output is: \n%s", *output);
	*outpsize = strlen(*output);
	osync_trace(TRACE_EXIT, "%s", __func__);
	
	return TRUE;
}

static OSyncConvCmpResult compare_notes(OSyncChange *leftchange, OSyncChange *rightchange)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftchange, rightchange);
	
	OSyncXMLScore score[] =
	{
	{100, "/Note/Summary"},
	{100, "/Note/Body"},
	{0, "/Note/*/Type"},
	{0, "/Note/Uid"},
	{0, "/Note/LastModified"},
	{0, "/Note/DateCreated"},
	{0, NULL}
	};
	
	OSyncConvCmpResult ret = osxml_compare((xmlDoc*)osync_change_get_data(leftchange), (xmlDoc*)osync_change_get_data(rightchange), score, 0, 199);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static char *print_note(OSyncChange *change)
{
	xmlDoc *doc = (xmlDoc *)osync_change_get_data(change);
	
	return (char *)osxml_write_to_string(doc);
}

static void destroy_xml(char *data, size_t size)
{
	xmlFreeDoc((xmlDoc *)data);
}

static void *init_vnote_to_xml(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	GHashTable *table = g_hash_table_new(g_str_hash, g_str_equal);
	
	g_hash_table_insert(table, "X-IRMC-LUID", HANDLE_IGNORE);
	g_hash_table_insert(table, "DCREATED", handle_created_attribute);
	g_hash_table_insert(table, "LAST-MODIFIED", handle_last_modified_attribute);
	g_hash_table_insert(table, "SUMMARY", handle_summary_attribute);
	g_hash_table_insert(table, "BODY", handle_body_attribute);
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

static void fin_vnote_to_xml(void *data)
{
	g_hash_table_destroy((GHashTable *)data);
}

static void *init_xml_to_vnote(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	
	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));
	
	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);
	
	g_hash_table_insert(hooks->attributes, "Summary", handle_xml_summary_attribute);
	g_hash_table_insert(hooks->attributes, "Body", handle_xml_body_attribute);
	g_hash_table_insert(hooks->attributes, "Class", handle_xml_class_attribute);
	g_hash_table_insert(hooks->attributes, "Categories", handle_xml_categories_attribute);
	g_hash_table_insert(hooks->attributes, "UnknownNode", xml_handle_unknown_attribute);
	g_hash_table_insert(hooks->attributes, "DateCreated", handle_xml_created_attribute);
	g_hash_table_insert(hooks->attributes, "LastModified", handle_xml_last_modified_attribute);
	
	g_hash_table_insert(hooks->parameters, "Type", handle_xml_type_parameter);
	g_hash_table_insert(hooks->parameters, "Category", handle_xml_category_parameter);
	
	g_hash_table_insert(hooks->parameters, "UnknownParameter", xml_handle_unknown_parameter);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static void fin_xml_to_vnote(void *data)
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
	
	xmlXPathObject *xobj = osxml_get_nodeset(doc, "/Note/LastModified");
		
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
	osync_env_register_objtype(env, "note");
	osync_env_register_objformat(env, "note", "xml-note");
	osync_env_format_set_compare_func(env, "xml-note", compare_notes);
	osync_env_format_set_destroy_func(env, "xml-note", destroy_xml);
	osync_env_format_set_print_func(env, "xml-note", print_note);
	osync_env_format_set_copy_func(env, "xml-note", osxml_copy);
	osync_env_format_set_revision_func(env, "xml-note", get_revision);
	osync_env_format_set_marshall_func(env, "xml-note", osxml_marshall);
	osync_env_format_set_demarshall_func(env, "xml-note", osxml_demarshall);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vnote11", "xml-note", conv_vnote_to_xml);
	osync_env_converter_set_init(env, "vnote11", "xml-note", init_vnote_to_xml, fin_vnote_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-note", "vnote11", conv_xml_to_vnote);
	osync_env_converter_set_init(env, "xml-note", "vnote11", init_xml_to_vnote, fin_xml_to_vnote);
}
