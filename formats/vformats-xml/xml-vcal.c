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
	xmlNode *property = xmlNewChild(current, NULL, "UnknownParam", vformat_attribute_param_get_nth_value(param, 0));
	osxml_node_add(property, "ParamName", vformat_attribute_param_get_name(param));
}

static xmlNode *handle_unknown_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown attribute %s", vformat_attribute_get_name(attr));
	xmlNode *current = xmlNewChild(root, NULL, "UnknownNode", NULL);
	osxml_node_add(current, "NodeName", vformat_attribute_get_name(attr));
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osxml_node_add(current, "Content", retstr->str);
	}
	return current;
}

static xmlNode *handle_prodid_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling prodid attribute");
	xmlNode *current = xmlNewChild(root, NULL, "ProductID", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_method_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling method attribute");
	xmlNode *current = xmlNewChild(root, NULL, "Method", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_tzid_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzid attribute");
	return xmlNewChild(root, NULL, "TimezoneID", vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_tz_location_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tz location attribute");
	return xmlNewChild(root, NULL, "Location", vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_tzoffsetfrom_location_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzoffsetfrom attribute");
	return xmlNewChild(root, NULL, "TZOffsetFrom", vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_tzoffsetto_location_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzoffsetto attribute");
	return xmlNewChild(root, NULL, "TZOffsetTo", vformat_attribute_get_nth_value(attr, 0));
}

static xmlNode *handle_tzname_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling tzname attribute");
	return xmlNewChild(root, NULL, "TZName", vformat_attribute_get_nth_value(attr, 0));
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

static void vcal_handle_attribute(GHashTable *hooks, xmlNode *root, VFormatAttribute *attr)
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

static void vcal_parse_attributes(GHashTable *hooks, GList **attributes, xmlNode *root)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, attributes, root);
	
	GList *a = NULL;
	for (a = *attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;
		
		osync_trace(TRACE_INTERNAL, "attribute %s:%s", vformat_attribute_get_name(attr), vformat_attribute_get_nth_value(attr, 0));
		
		if (!strcmp(vformat_attribute_get_name(attr), "BEGIN")) {
			//Handling supcomponent
			a = a->next;
			if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "VTIMEZONE")) {
				xmlNode *current = xmlNewChild(root, NULL, "Timezone", NULL);
				vcal_parse_attributes(hooks, &a, current);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "DAYLIGHT")) {
				xmlNode *current = xmlNewChild(root, NULL, "DaylightSavings", NULL);
				vcal_parse_attributes(hooks, &a, current);
			} else if (!strcmp(vformat_attribute_get_nth_value(attr, 0), "STANDARD")) {
				vcal_parse_attributes(hooks, &a, root);
			}
		} else if (!strcmp(vformat_attribute_get_name(attr), "END")) {
			osync_trace(TRACE_EXIT, "%s: Found END", __func__);
			*attributes = a;
			return;
		} else
			vcal_handle_attribute(hooks, root, attr);
	}
	osync_trace(TRACE_EXIT, "%s: Done", __func__);
}

static osync_bool conv_vcal_to_xml(void *conv_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, conv_data, input, inpsize, output, outpsize, free_input, error);
	
	GHashTable *hooks = (GHashTable *)conv_data;
	
	osync_trace(TRACE_INTERNAL, "Input vcal is:\n%s", input);
	
	//Parse the vcard
	VFormat *vcal = vformat_new_from_string(input);
	
	osync_trace(TRACE_INTERNAL, "Creating xml doc");
	
	//Create a new xml document
	xmlDoc *doc = xmlNewDoc("1.0");
	xmlNode *root = osxml_node_add_root(doc, "contact");
	
	osync_trace(TRACE_INTERNAL, "parsing attributes");
	
	//For every attribute we have call the handling hook
	GList *attributes = vformat_get_attributes(vcal);
	vcal_parse_attributes(hooks, &attributes, root);
	
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

static void xml_handle_unknown_parameter(VFormatAttribute *attr, xmlNode *current)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown xml parameter %s", current->name);
	char *content = xmlNodeGetContent(current);
	vformat_attribute_add_param_with_value(attr, current->name, content);
	g_free(content);
}

static VFormatAttribute *xml_handle_unknown_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling unknown xml attribute %s", root->name);
	char *name = osxml_find_node(root, "Name");
	VFormatAttribute *attr = vformat_attribute_new(NULL, name);
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_prodid_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling prodid xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "PRODID");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_method_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling method xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "METHOD");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static void xml_vcard_handle_parameter(OSyncHookTables *hooks, VFormatAttribute *attr, xmlNode *current)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, hooks, attr, current, current ? (char *)current->name : "None");
	
	//Find the handler for this parameter
	void (* xml_param_handler)(VFormatAttribute *attr, xmlNode *);
	char *content = xmlNodeGetContent(current);
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

static void xml_vcal_handle_attribute(OSyncHookTables *hooks, VFormat *vcard, xmlNode *root)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, hooks, vcard, root, root ? (char *)root->name : "None");
	VFormatAttribute *attr = NULL;
	
	//We need to find the handler for this attribute
	VFormatAttribute *(* xml_attr_handler)(VFormat *vcard, xmlNode *root) = g_hash_table_lookup(hooks->attributes, root->name);
	osync_trace(TRACE_INTERNAL, "xml hook is: %p", xml_attr_handler);
	if (xml_attr_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	if (xml_attr_handler)
		attr = xml_attr_handler(vcard, root);
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

static osync_bool conv_xml_to_vcal(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error, VFormatType target)
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
	VFormat *vcal = vformat_new();
	
	osync_trace(TRACE_INTERNAL, "parsing cml attributes");
	while (root) {
		xml_vcal_handle_attribute((OSyncHookTables *)user_data, vcal, root);
		root = root->next;
	}
	
	*free_input = TRUE;
	*output = vformat_to_string(vcal, target);
	osync_trace(TRACE_INTERNAL, "vevent output is: \n%s", *output);
	*outpsize = strlen(*output) + 1;
	osync_trace(TRACE_EXIT, "%s", __func__);
	
	return TRUE;
}

static osync_bool conv_xml_to_vevent10(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	return conv_xml_to_vcal(user_data, input, inpsize, output, outpsize, free_input, error, VFORMAT_EVENT_10);
}

static osync_bool conv_xml_to_vevent20(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	return conv_xml_to_vcal(user_data, input, inpsize, output, outpsize, free_input, error, VFORMAT_EVENT_20);
}

static osync_bool conv_xml_to_vtodo10(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	return conv_xml_to_vcal(user_data, input, inpsize, output, outpsize, free_input, error, VFORMAT_TODO_10);
}

static osync_bool conv_xml_to_vtodo20(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	return conv_xml_to_vcal(user_data, input, inpsize, output, outpsize, free_input, error, VFORMAT_TODO_20);
}

static OSyncConvCmpResult compare_vcal(OSyncChange *leftchange, OSyncChange *rightchange)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftchange, rightchange);
	
	OSyncXMLScore score[] =
	{
	{100, "/calendar/StartTime"},
	{100, "/calendar/EndTime"},
	{100, "/calendar/Summary"},
	{0, "/contact/Uid"},
	{0, "/contact/Revision"},
	{0, NULL}
	};
	
	OSyncConvCmpResult ret = osxml_compare((xmlDoc*)osync_change_get_data(leftchange), (xmlDoc*)osync_change_get_data(rightchange), score, 0, 299);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static char *print_vcal(OSyncChange *change)
{
	xmlDoc *doc = (xmlDoc *)osync_change_get_data(change);
	
	return osxml_write_to_string(doc);
}

static void destroy_xml(char *data, size_t size)
{
	xmlFreeDoc((xmlDoc *)data);
}

static void *init_vcal_to_xml(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	GHashTable *table = g_hash_table_new(g_str_hash, g_str_equal);
	
	g_hash_table_insert(table, "BEGIN", HANDLE_IGNORE);
	g_hash_table_insert(table, "END", HANDLE_IGNORE);
	
	//vcal attributes
	g_hash_table_insert(table, "PRODID", handle_prodid_attribute);
	g_hash_table_insert(table, "METHOD", handle_method_attribute);
	
	g_hash_table_insert(table, "CALSCALE", HANDLE_IGNORE);
	g_hash_table_insert(table, "GEO", HANDLE_IGNORE);
	g_hash_table_insert(table, "TZ", HANDLE_IGNORE);
	g_hash_table_insert(table, "CATEGORIES", HANDLE_IGNORE);
	g_hash_table_insert(table, "CLASS", HANDLE_IGNORE);
	g_hash_table_insert(table, "URL", HANDLE_IGNORE);
	g_hash_table_insert(table, "UID", HANDLE_IGNORE);	
	g_hash_table_insert(table, "DAYLIGHT", HANDLE_IGNORE);
	g_hash_table_insert(table, "ATTACH", HANDLE_IGNORE);
	g_hash_table_insert(table, "ATTENDEE", HANDLE_IGNORE);
	g_hash_table_insert(table, "AALARM", HANDLE_IGNORE);
	g_hash_table_insert(table, "DCREATED", HANDLE_IGNORE);
	g_hash_table_insert(table, "COMPLETED", HANDLE_IGNORE);
	g_hash_table_insert(table, "DESCRIPTION", HANDLE_IGNORE);
	g_hash_table_insert(table, "DALARM", HANDLE_IGNORE);
	g_hash_table_insert(table, "DUE", HANDLE_IGNORE);
	g_hash_table_insert(table, "DTEND", HANDLE_IGNORE);
	g_hash_table_insert(table, "EXDATE", HANDLE_IGNORE);
	g_hash_table_insert(table, "EXRULE", HANDLE_IGNORE);
	g_hash_table_insert(table, "LAST-MODIFIED", HANDLE_IGNORE);
	g_hash_table_insert(table, "LOCATION", HANDLE_IGNORE);
	g_hash_table_insert(table, "MALARM", HANDLE_IGNORE);
	g_hash_table_insert(table, "RNUM", HANDLE_IGNORE);
	g_hash_table_insert(table, "PRIORITY", HANDLE_IGNORE);
	g_hash_table_insert(table, "PALARM", HANDLE_IGNORE);
	g_hash_table_insert(table, "RELATED-TO", HANDLE_IGNORE);
	g_hash_table_insert(table, "RDATE", HANDLE_IGNORE);
	g_hash_table_insert(table, "RRULE", HANDLE_IGNORE);
	g_hash_table_insert(table, "RESOURCES", HANDLE_IGNORE);
	g_hash_table_insert(table, "SEQUENCE", HANDLE_IGNORE);
	g_hash_table_insert(table, "DTSTART", HANDLE_IGNORE);
	g_hash_table_insert(table, "STATUS", HANDLE_IGNORE);
	g_hash_table_insert(table, "SUMMARY", HANDLE_IGNORE);
	g_hash_table_insert(table, "TRANSP", HANDLE_IGNORE);
	g_hash_table_insert(table, "URL", HANDLE_IGNORE);
	g_hash_table_insert(table, "DTSTART", HANDLE_IGNORE);

	//Timezone
	g_hash_table_insert(table, "TZID", handle_tzid_attribute);
	g_hash_table_insert(table, "X-LIC-LOCATION", handle_tz_location_attribute);
	g_hash_table_insert(table, "TZOFFSETFROM", handle_tzoffsetfrom_location_attribute);
	g_hash_table_insert(table, "TZOFFSETTO", handle_tzoffsetto_location_attribute);
	g_hash_table_insert(table, "TZNAME", handle_tzname_attribute);
        
	//Event component
	
	g_hash_table_insert(table, "TRIGGER", HANDLE_IGNORE);
	g_hash_table_insert(table, "REPEAT", HANDLE_IGNORE);
	g_hash_table_insert(table, "DURATION", HANDLE_IGNORE);
	g_hash_table_insert(table, "ACTION", HANDLE_IGNORE);
	g_hash_table_insert(table, "ATTACH", HANDLE_IGNORE);
	g_hash_table_insert(table, "DESCRIPTION", HANDLE_IGNORE);
	g_hash_table_insert(table, "ATTENDEE", HANDLE_IGNORE);
	g_hash_table_insert(table, "SUMMARY", HANDLE_IGNORE);

	//VAlarm component
	g_hash_table_insert(table, "TRIGGER", HANDLE_IGNORE);
	g_hash_table_insert(table, "REPEAT", HANDLE_IGNORE);
	g_hash_table_insert(table, "DURATION", HANDLE_IGNORE);
	g_hash_table_insert(table, "ACTION", HANDLE_IGNORE);
	g_hash_table_insert(table, "ATTACH", HANDLE_IGNORE);
	g_hash_table_insert(table, "DESCRIPTION", HANDLE_IGNORE);
	g_hash_table_insert(table, "ATTENDEE", HANDLE_IGNORE);
	g_hash_table_insert(table, "SUMMARY", HANDLE_IGNORE);
	
	

	
	g_hash_table_insert(table, "VERSION", HANDLE_IGNORE);
	g_hash_table_insert(table, "ENCODING", HANDLE_IGNORE);
	g_hash_table_insert(table, "CHARSET", HANDLE_IGNORE);
	
	g_hash_table_insert(table, "TYPE", HANDLE_IGNORE);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, table);
	return (void *)table;
}

static void fin_vcal_to_xml(void *data)
{
	g_hash_table_destroy((GHashTable *)data);
}

static void *init_xml_to_vcal(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	
	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));
	
	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);
	
	g_hash_table_insert(hooks->attributes, "ProductID", handle_xml_prodid_attribute);
	g_hash_table_insert(hooks->attributes, "Method", handle_xml_method_attribute);
	g_hash_table_insert(hooks->attributes, "UnknownNode", xml_handle_unknown_attribute);
	
	//g_hash_table_insert(hooks->parameters, "Type", handle_xml_type_parameter);
	
	g_hash_table_insert(hooks->parameters, "UnknownParameter", xml_handle_unknown_parameter);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static void fin_xml_to_vcal(void *data)
{
	OSyncHookTables *hooks = (OSyncHookTables *)hooks;
	g_hash_table_destroy(hooks->attributes);
	g_hash_table_destroy(hooks->parameters);
	g_free(hooks);
}

void get_info(OSyncEnv *env)
{
	//Calendar
	osync_env_register_objtype(env, "calendar");
	osync_env_register_objformat(env, "calendar", "xml-event");
	osync_env_format_set_compare_func(env, "xml-event", compare_vcal);
	osync_env_format_set_destroy_func(env, "xml-event", destroy_xml);
	osync_env_format_set_print_func(env, "xml-event", print_vcal);
	osync_env_format_set_copy_func(env, "xml-event", osxml_copy);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vevent10", "xml-event", conv_vcal_to_xml);
	osync_env_converter_set_init(env, "vevent10", "xml-event", init_vcal_to_xml, fin_vcal_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-event", "vevent10", conv_xml_to_vevent10);
	osync_env_converter_set_init(env, "xml-event", "vevent10", init_xml_to_vcal, fin_xml_to_vcal);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vevent20", "xml-event", conv_vcal_to_xml);
	osync_env_converter_set_init(env, "vevent20", "xml-event", init_vcal_to_xml, fin_vcal_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-event", "vevent20", conv_xml_to_vevent20);
	osync_env_converter_set_init(env, "xml-event", "vevent20", init_xml_to_vcal, fin_xml_to_vcal);
	
	//Todo
	osync_env_register_objtype(env, "calendar");
	osync_env_register_objformat(env, "calendar", "xml-todo");
	osync_env_format_set_compare_func(env, "xml-todo", compare_vcal);
	osync_env_format_set_destroy_func(env, "xml-todo", destroy_xml);
	osync_env_format_set_print_func(env, "xml-todo", print_vcal);
	osync_env_format_set_copy_func(env, "xml-todo", osxml_copy);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vtodo10", "xml-todo", conv_vcal_to_xml);
	osync_env_converter_set_init(env, "vtodo10", "xml-todo", init_vcal_to_xml, fin_vcal_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-todo", "vtodo10", conv_xml_to_vtodo10);
	osync_env_converter_set_init(env, "xml-todo", "vtodo10", init_xml_to_vcal, fin_xml_to_vcal);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vtodo20", "xml-todo", conv_vcal_to_xml);
	osync_env_converter_set_init(env, "vtodo20", "xml-todo", init_vcal_to_xml, fin_vcal_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-todo", "vtodo20", conv_xml_to_vtodo20);
	osync_env_converter_set_init(env, "xml-todo", "vtodo20", init_xml_to_vcal, fin_xml_to_vcal);
}
