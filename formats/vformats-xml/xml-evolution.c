/*
 * x-evo - A plugin for evolution extensions for the opensync framework
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

#include "xml-vcard.h"

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

static const char *property_get_nth_value(EVCardAttributeParam *param, int nth)
{
	const char *ret = NULL;
	GList *values = e_vcard_attribute_param_get_values(param);
	if (!values)
		return NULL;
	ret = g_list_nth_data(values, nth);
	return ret;
}

static xmlNode *handle_x_aim_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling x-aim attribute");
	xmlNode *current = xmlNewChild(root, NULL, "IM-AIM", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_file_as_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling FileAs attribute");
	xmlNode *current = xmlNewChild(root, NULL, "FileAs", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_manager_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Manager attribute");
	xmlNode *current = xmlNewChild(root, NULL, "Manager", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_assistant_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Assistant attribute");
	xmlNode *current = xmlNewChild(root, NULL, "Assistant", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_anniversary_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Anniversary attribute");
	xmlNode *current = xmlNewChild(root, NULL, "Anniversary", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_spouse_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Spouse attribute");
	xmlNode *current = xmlNewChild(root, NULL, "Spouse", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_blog_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling BlogUrl attribute");
	xmlNode *current = xmlNewChild(root, NULL, "BlogUrl", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_calendar_url_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling CalendarUrl attribute");
	xmlNode *current = xmlNewChild(root, NULL, "CalendarUrl", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_free_busy_url_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling FreeBusyUrl attribute");
	xmlNode *current = xmlNewChild(root, NULL, "FreeBusyUrl", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_video_chat_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling VideoUrl attribute");
	xmlNode *current = xmlNewChild(root, NULL, "VideoUrl", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_wants_html_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling WantsHtml attribute");
	xmlNode *current = xmlNewChild(root, NULL, "WantsHtml", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_yahoo_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling IM-Yahoo attribute");
	xmlNode *current = xmlNewChild(root, NULL, "IM-Yahoo", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_icq_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling IM-ICQ attribute");
	xmlNode *current = xmlNewChild(root, NULL, "IM-ICQ", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_groupwise_attribute(xmlNode *root, EVCardAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling GroupwiseDirectory attribute");
	xmlNode *current = xmlNewChild(root, NULL, "GroupwiseDirectory", NULL);
	osxml_node_add(current, "Content", attribute_get_nth_value(attr, 0));
	return current;
}

static void handle_slot_parameter(xmlNode *current, EVCardAttributeParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Slot parameter %s", e_vcard_attribute_param_get_name(param));
	xmlNewChild(current, NULL, "Slot", property_get_nth_value(param, 0));
}

static void handle_assistant_parameter(xmlNode *current, EVCardAttributeParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Assistant parameter %s", e_vcard_attribute_param_get_name(param));
	xmlNewChild(current, NULL, "Type", "Assistant");
}

static void handle_callback_parameter(xmlNode *current, EVCardAttributeParam *param)
{
	osync_trace(TRACE_INTERNAL, "Handling Callback parameter %s", e_vcard_attribute_param_get_name(param));
	xmlNewChild(current, NULL, "Type", "Callback");
}

static osync_bool init_x_evo_to_xml(void *input)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, input);
	
	GHashTable *table = (GHashTable *)input;
	
	g_hash_table_insert(table, "X-EVOLUTION-FILE-AS", handle_file_as_attribute);
	g_hash_table_insert(table, "X-EVOLUTION-MANAGER", handle_manager_attribute);
	g_hash_table_insert(table, "X-EVOLUTION-ASSISTANT", handle_assistant_attribute);
	g_hash_table_insert(table, "X-EVOLUTION-ANNIVERSARY", handle_anniversary_attribute);
	g_hash_table_insert(table, "X-EVOLUTION-SPOUSE", handle_spouse_attribute);
	g_hash_table_insert(table, "X-EVOLUTION-BLOG-URL", handle_blog_attribute);
	g_hash_table_insert(table, "CALURI", handle_calendar_url_attribute);
	g_hash_table_insert(table, "FBURL", handle_free_busy_url_attribute);
	g_hash_table_insert(table, "X-EVOLUTION-VIDEO-URL", handle_video_chat_attribute);
	g_hash_table_insert(table, "X-MOZILLA-HTML", handle_wants_html_attribute);
	g_hash_table_insert(table, "X-YAHOO", handle_yahoo_attribute);
	g_hash_table_insert(table, "X-ICQ", handle_icq_attribute);
	g_hash_table_insert(table, "X-GROUPWISE", handle_groupwise_attribute);
	g_hash_table_insert(table, "X-AIM", handle_x_aim_attribute);
	
	g_hash_table_insert(table, "X-EVOLUTION-UI-SLOT", handle_slot_parameter);
	g_hash_table_insert(table, "TYPE=X-EVOLUTION-ASSISTANT", handle_assistant_parameter);
	g_hash_table_insert(table, "TYPE=X-EVOLUTION-CALLBACK", handle_callback_parameter);
	
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
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

static EVCardAttribute *handle_xml_file_as_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling file_as xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "X-EVOLUTION-FILE-AS");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static EVCardAttribute *handle_xml_manager_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling manager xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "X-EVOLUTION-MANAGER");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static EVCardAttribute *handle_xml_assistant_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling assistant xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "X-EVOLUTION-ASSISTANT");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static EVCardAttribute *handle_xml_anniversary_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling anniversary xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "X-EVOLUTION-ANNIVERSARY");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static EVCardAttribute *handle_xml_spouse_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling spouse xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "X-EVOLUTION-SPOUSE");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static EVCardAttribute *handle_xml_blog_url_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling blog_url xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "X-EVOLUTION-BLOG-URL");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static EVCardAttribute *handle_xml_calendar_url_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling calendar_url xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "CALURI");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static EVCardAttribute *handle_xml_free_busy_url_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling free_busy_url xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "FBURL");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static EVCardAttribute *handle_xml_video_url_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling videourl xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "X-EVOLUTION-VIDEO-URL");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static EVCardAttribute *handle_xml_wants_html_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling wants_html xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "X-MOZILLA-HTML");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static EVCardAttribute *handle_xml_yahoo_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling yahoo xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "X-YAHOO");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static EVCardAttribute *handle_xml_icq_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling icq xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "X-ICQ");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static EVCardAttribute *handle_xml_groupwise_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling groupwise xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "X-GROUPWISE");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static EVCardAttribute *handle_xml_aim_attribute(EVCard *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling aim xml attribute");
	EVCardAttribute *attr = e_vcard_attribute_new(NULL, "X-AIM");
	add_value(attr, root, "Content", encoding);
	e_vcard_add_attribute(vcard, attr);
	return attr;
}

static void handle_xml_slot_parameter(EVCardAttribute *attr, xmlNode *current)
{
	osync_trace(TRACE_INTERNAL, "Handling slot xml parameter");
	char *content = xmlNodeGetContent(current);
	add_parameter(attr, "X-EVOLUTION-UI-SLOT", content);
	g_free(content);
}

static void handle_xml_assistant_parameter(EVCardAttribute *attr, xmlNode *current)
{
	osync_trace(TRACE_INTERNAL, "Handling assistant xml parameter");
	add_parameter(attr, "TYPE", "X-EVOLUTION-ASSISTANT");
}

static void handle_xml_callback_parameter(EVCardAttribute *attr, xmlNode *current)
{
	osync_trace(TRACE_INTERNAL, "Handling callback xml parameter");
	add_parameter(attr, "TYPE", "X-EVOLUTION-CALLBACK");
}

static osync_bool init_xml_to_x_evo(void *input)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, input);
	
	OSyncHookTables *hooks = (OSyncHookTables *)input;
	
	g_hash_table_insert(hooks->attributes, "FileAs", handle_xml_file_as_attribute);
	g_hash_table_insert(hooks->attributes, "Manager", handle_xml_manager_attribute);
	g_hash_table_insert(hooks->attributes, "Assistant", handle_xml_assistant_attribute);
	g_hash_table_insert(hooks->attributes, "Anniversary", handle_xml_anniversary_attribute);
	g_hash_table_insert(hooks->attributes, "Spouse", handle_xml_spouse_attribute);
	g_hash_table_insert(hooks->attributes, "BlogUrl", handle_xml_blog_url_attribute);
	g_hash_table_insert(hooks->attributes, "CalendarUrl", handle_xml_calendar_url_attribute);
	g_hash_table_insert(hooks->attributes, "FreeBusyUrl", handle_xml_free_busy_url_attribute);
	g_hash_table_insert(hooks->attributes, "VideoUrl", handle_xml_video_url_attribute);
	g_hash_table_insert(hooks->attributes, "WantsHtml", handle_xml_wants_html_attribute);
	g_hash_table_insert(hooks->attributes, "IM-Yahoo", handle_xml_yahoo_attribute);
	g_hash_table_insert(hooks->attributes, "IM-ICQ", handle_xml_icq_attribute);
	g_hash_table_insert(hooks->attributes, "GroupwiseDirectory", handle_xml_groupwise_attribute);
	g_hash_table_insert(hooks->attributes, "IM-AIM", handle_xml_aim_attribute);
	
	g_hash_table_insert(hooks->parameters, "Slot", handle_xml_slot_parameter);
	g_hash_table_insert(hooks->parameters, "Type=Assistant", handle_xml_assistant_parameter);
	g_hash_table_insert(hooks->parameters, "Type=Callback", handle_xml_callback_parameter);
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "xml-contact");
	
	osync_env_register_extension(env, "xml-contact", "evolution", init_x_evo_to_xml, init_xml_to_x_evo);
}
