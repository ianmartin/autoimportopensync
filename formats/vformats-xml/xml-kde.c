/*
 * x-kde - A plugin for kde extensions for the opensync framework
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
 
#include "xml-support.h"
#include "vformat.h"
#include <glib.h>

#include "xml-vcard.h"

static xmlNode *handle_aim_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling x-aim attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"IM-AIM", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_manager_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Manager attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Manager", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_assistant_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Assistant attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Assistant", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_anniversary_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Anniversary attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Anniversary", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_spouse_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Spouse attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Spouse", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_blog_url_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Blog Url attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"BlogUrl", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_yahoo_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling IM-Yahoo attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"IM-Yahoo", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_icq_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling IM-ICQ attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"IM-ICQ", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_jabber_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Jabber attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"IM-Jabber", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_msn_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling MSN attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"IM-MSN", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_department_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling department attribute");
	xmlNode *current = NULL;
	
	//We need to check first if the node already exists.
	if (!(current = osxml_get_node(root, "Organization")))
		current = xmlNewTextChild(root, NULL, (xmlChar*)"Organization", NULL);
	
	osxml_node_add(current, "Department", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_office_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling office attribute");
	xmlNode *current = NULL;
	
	//We need to check first if the node already exists.
	if (!(current = osxml_get_node(root, "Organization")))
		current = xmlNewTextChild(root, NULL, (xmlChar*)"Organization", NULL);
	
	osxml_node_add(current, "Unit", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_profession_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling profession attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"Profession", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_gadu_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling gadu attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"IM-GaduGadu", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_irc_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling IRC attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"IRC", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_sms_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling SMS attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"SMS", NULL);
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static xmlNode *handle_organization_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling Organization attribute");
	xmlNode *current = NULL;
	
	//We need to check first if the node already exists.
	if (!(current = osxml_get_node(root, "Organization")))
		current = xmlNewTextChild(root, NULL, (xmlChar*)"Organization", NULL);
	
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

static xmlNode *handle_x_kde_attribute(xmlNode *root, VFormatAttribute *attr)
{
	osync_trace(TRACE_INTERNAL, "Handling X-KDE attribute");
	xmlNode *current = xmlNewTextChild(root, NULL, (xmlChar*)"KDE-Extension", NULL);
	osxml_node_add(current, "ExtName", vformat_attribute_get_name(attr));
	osxml_node_add(current, "Content", vformat_attribute_get_nth_value(attr, 0));
	return current;
}

static osync_bool init_kde_to_xml(void *input)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, input);
	
	GHashTable *table = (GHashTable *)input;
	
	g_hash_table_insert(table, "X-KADDRESSBOOK-CRYPTOENCRYPTPREF", handle_x_kde_attribute);
	g_hash_table_insert(table, "X-KADDRESSBOOK-CRYPTOPROTOPREF", handle_x_kde_attribute);
	g_hash_table_insert(table, "X-KADDRESSBOOK-CRYPTOSIGNPREF", handle_x_kde_attribute);
	g_hash_table_insert(table, "X-KADDRESSBOOK-OPENPGPFP", handle_x_kde_attribute);
	g_hash_table_insert(table, "X-KADDRESSBOOK-X-IMAddress", handle_x_kde_attribute);
	
	g_hash_table_insert(table, "X-KADDRESSBOOK-X-ManagersName", handle_manager_attribute);
	g_hash_table_insert(table, "X-KADDRESSBOOK-X-AssistantsName", handle_assistant_attribute);
	g_hash_table_insert(table, "X-KADDRESSBOOK-X-Anniversary", handle_anniversary_attribute);
	g_hash_table_insert(table, "X-KADDRESSBOOK-X-Department", handle_department_attribute);
	g_hash_table_insert(table, "X-KADDRESSBOOK-X-Office", handle_office_attribute);
	g_hash_table_insert(table, "X-KADDRESSBOOK-X-Profession", handle_profession_attribute);
	g_hash_table_insert(table, "X-KADDRESSBOOK-X-SpousesName", handle_spouse_attribute);
	g_hash_table_insert(table, "X-KADDRESSBOOK-BlogFeed", handle_blog_url_attribute);
	g_hash_table_insert(table, "X-messaging/yahoo-All", handle_yahoo_attribute);
	g_hash_table_insert(table, "X-messaging/icq-All", handle_icq_attribute);
	g_hash_table_insert(table, "X-messaging/aim-All", handle_aim_attribute);
	g_hash_table_insert(table, "X-messaging/xmpp-All", handle_jabber_attribute);
	g_hash_table_insert(table, "X-messaging/msn-All", handle_msn_attribute);
	g_hash_table_insert(table, "X-messaging/gadu-All", handle_gadu_attribute);
	g_hash_table_insert(table, "X-messaging/irc-All", handle_irc_attribute);
	g_hash_table_insert(table, "X-messaging/sms-All", handle_sms_attribute);
	
	//Overwrite the organization hook
	g_hash_table_insert(table, "ORG", handle_organization_attribute);
	
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

static VFormatAttribute *handle_xml_manager_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling manager xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-ManagersName");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_assistant_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling assistant xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-AssistantsName");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_anniversary_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling anniversary xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-Anniversary");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_spouse_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling spouse xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-SpousesName");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_blog_url_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling blog_url xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-BlogFeed");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_yahoo_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling yahoo xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/yahoo-All");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_icq_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling icq xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/icq-All");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_aim_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling aim xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/aim-All");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_jabber_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling jabber xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/xmpp-All");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_msn_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling msn xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/msn-All");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_organization_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling organization kde xml attribute");
	VFormatAttribute *org = NULL;
	VFormatAttribute *attr = NULL;
	
	root = root->children;
	
	int i = 0;
	while (root) {
		char *content = (char*)xmlNodeGetContent(root);
		if (!strcmp((char*)root->name, "Name")) {
			org = vformat_attribute_new(NULL, "ORG");
			vformat_attribute_add_value(org, content);
			vformat_add_attribute(vcard, org);
		}
		
		if (!strcmp((char*)root->name, "Department")) {
			attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-Department");
			vformat_attribute_add_value(attr, content);
			vformat_add_attribute(vcard, attr);
		}
		if (!strcmp((char*)root->name, "Unit")) {
			switch (i) {
				case 0:
					attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-Office");
					vformat_attribute_add_value(attr, content);
					vformat_add_attribute(vcard, attr);
					break;
				default:
					vformat_attribute_add_value(org, content);
			}
			i++;
		}
		
		g_free(content);
		root = root->next;
	}
	
	return attr;
}

static VFormatAttribute *handle_xml_profession_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling profession xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-KADDRESSBOOK-X-Profession");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_gadu_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling msn xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/gadu-All");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_irc_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling msn xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/irc-All");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_sms_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling msn xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, "X-messaging/sms-All");
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static VFormatAttribute *handle_xml_x_kde_attribute(VFormat *vcard, xmlNode *root, const char *encoding)
{
	osync_trace(TRACE_INTERNAL, "Handling msn xml attribute");
	VFormatAttribute *attr = vformat_attribute_new(NULL, osxml_find_node(root, "ExtName"));
	add_value(attr, root, "Content", encoding);
	vformat_add_attribute(vcard, attr);
	return attr;
}

static osync_bool init_xml_to_kde(void *input)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, input);
	
	OSyncHookTables *hooks = (OSyncHookTables *)input;
	
	g_hash_table_insert(hooks->attributes, "Manager", handle_xml_manager_attribute);
	g_hash_table_insert(hooks->attributes, "Assistant", handle_xml_assistant_attribute);
	g_hash_table_insert(hooks->attributes, "Anniversary", handle_xml_anniversary_attribute);
	g_hash_table_insert(hooks->attributes, "Organization", handle_xml_organization_attribute);
	g_hash_table_insert(hooks->attributes, "Profession", handle_xml_profession_attribute);
	g_hash_table_insert(hooks->attributes, "Spouse", handle_xml_spouse_attribute);
	g_hash_table_insert(hooks->attributes, "BlogUrl", handle_xml_blog_url_attribute);
	g_hash_table_insert(hooks->attributes, "IM-Yahoo", handle_xml_yahoo_attribute);
	g_hash_table_insert(hooks->attributes, "IM-ICQ", handle_xml_icq_attribute);
	g_hash_table_insert(hooks->attributes, "IM-AIM", handle_xml_aim_attribute);
	g_hash_table_insert(hooks->attributes, "IM-Jabber", handle_xml_jabber_attribute);
	g_hash_table_insert(hooks->attributes, "IM-MSN", handle_xml_msn_attribute);
	g_hash_table_insert(hooks->attributes, "IM-GaduGadu", handle_xml_gadu_attribute);
	g_hash_table_insert(hooks->attributes, "IRC", handle_xml_irc_attribute);
	g_hash_table_insert(hooks->attributes, "SMS", handle_xml_sms_attribute);

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

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "xml-contact");
	
	osync_env_register_extension(env, "vcard21", "xml-contact", "kde", init_kde_to_xml);
	osync_env_register_extension(env, "xml-contact", "vcard21", "kde", init_xml_to_kde);
	
	osync_env_register_extension(env, "vcard30", "xml-contact", "kde", init_kde_to_xml);
	osync_env_register_extension(env, "xml-contact", "vcard30", "kde", init_xml_to_kde);
}
