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

static osync_bool init_x_evo_to_xml(void *input)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, input);
	
	GHashTable *table = (GHashTable *)input;
	
	g_hash_table_insert(table, "X-AIM", handle_x_aim_attribute);
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
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

static osync_bool init_xml_to_x_evo(void *input)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, input);
	
	/*EVCardAttribute *attr = NULL;
	xmlNode *root = osxml_node_get_root((xmlDoc *)input, "contact", error);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get root element of xml-contact");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	const char *std_encoding = NULL;
	if (target == EVC_FORMAT_VCARD_21)
		std_encoding = "QUOTED-PRINTABLE";
	else
		std_encoding = "B";
		
		
	osxml_node_mark_unknown(current);
	GList *values = e_vcard_attribute_get_values(attr);
	GString *string = g_string_new(attribute_get_nth_value(attr, 0));
	for (p = values->next; p; p = p->next) {
		g_string_sprintfa(string, ";%s", (char *)p->data);
	}
	osxml_node_add(current, "NodeName", name);
	osxml_node_set(current, "UnknownNode", string->str, encoding);
	g_string_free(string, 1);
		
		
		
	while (root) {
		if (!strcmp(root->name, "IM-AIM")) {
			current = xmlNewChild(root, NULL, "", NULL);
			osxml_node_set(current, "", content, encoding);
		}
		
		if (!strcmp(name, "X-ICQ-ICQ")) {
			current = xmlNewChild(root, NULL, "", NULL);
			osxml_node_set(current, "IM-ICQ", content, encoding);
		}
		
		if (!strcmp(name, "X-YAHOO-YAHOO")) {
			current = xmlNewChild(root, NULL, "", NULL);
			osxml_node_set(current, "IM-Yahoo", content, encoding);
		}
		
		if (!strcmp(name, "X-EVOLUTION-FILE-AS")) {
			current = xmlNewChild(root, NULL, "", NULL);
			osxml_node_set(current, "FileAs", content, encoding);
		}
		
		if (!strcmp(name, "X-GROUPWISE-GROUPWISE")) {
			current = xmlNewChild(root, NULL, "", NULL);
			osxml_node_set(current, "GroupwiseDirectory", content, encoding);
		}
		
		if (!strcmp(name, "X-EVOLUTION-MANAGER")) {
			current = xmlNewChild(root, NULL, "", NULL);
			osxml_node_set(current, "Manager", content, encoding);
		}
		
		if (!strcmp(name, "X-MOZILLA-HTML")) {
			current = xmlNewChild(root, NULL, "", NULL);
			osxml_node_set(current, "WantsHtml", content, encoding);
		}
		
		if (!strcmp(name, "X-EVOLUTION-ASSISTANT")) {
			current = xmlNewChild(root, NULL, "", NULL);
			osxml_node_set(current, "Assistant", content, encoding);
		}
		
		if (!strcmp(name, "X-EVOLUTION-SPOUSE")) {
			current = xmlNewChild(root, NULL, "", NULL);
			osxml_node_set(current, "Spouse", content, encoding);
		}
		
		if (!strcmp(name, "X-EVOLUTION-BLOG-URL")) {
			current = xmlNewChild(root, NULL, "", NULL);
			osxml_node_set(current, "BlogUrl", content, encoding);
		}
		
		if (!strcmp(name, "CALURI")) {
			current = xmlNewChild(root, NULL, "", NULL);
			osxml_node_set(current, "CalendarUrl", content, encoding);
		}
		
		if (!strcmp(name, "FBURL")) {
			current = xmlNewChild(root, NULL, "", NULL);
			osxml_node_set(current, "FreeBusyUrl", content, encoding);
		}
		
		if (!strcmp(name, "X-EVOLUTION-VIDEO-URL")) {
			current = xmlNewChild(root, NULL, "", NULL);
			osxml_node_set(current, "VideoUrl", content, encoding);
		}
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		if (!strcmp(root->name, "FullName")) {
			//FullName
			attr = e_vcard_attribute_new(NULL, EVC_FN);
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Name")) {
			//Name
			attr = e_vcard_attribute_new(NULL, EVC_N);
			add_value(attr, root, "LastName", std_encoding);
			add_value(attr, root, "FirstName", std_encoding);
			add_value(attr, root, "Additional", std_encoding);
			add_value(attr, root, "Prefix", std_encoding);
			add_value(attr, root, "Suffix", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Photo")) {
			//Photo
			attr = e_vcard_attribute_new(NULL, EVC_PHOTO);
			add_value(attr, root, "Content", std_encoding);
			add_parameter(attr, "ENCODING", "b");
			add_parameter(attr, "TYPE", osxml_find_node(root, "Type"));
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Birthday")) {
			//Birthday
			attr = e_vcard_attribute_new(NULL, EVC_BDAY);
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Address")) {
			//Address
			attr = e_vcard_attribute_new(NULL, EVC_ADR);
			add_value(attr, root, "PostalBox", std_encoding);
			add_value(attr, root, "ExtendedAddress", std_encoding);
			add_value(attr, root, "Street", std_encoding);
			add_value(attr, root, "City", std_encoding);
			add_value(attr, root, "Region", std_encoding);
			add_value(attr, root, "PostalCode", std_encoding);
			add_value(attr, root, "Country", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "AddressLabel")) {
			//Address Labeling
			attr = e_vcard_attribute_new(NULL, EVC_LABEL);
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Telephone")) {
			//Telephone
			attr = e_vcard_attribute_new(NULL, EVC_TEL);
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "EMail")) {
			//EMail
			attr = e_vcard_attribute_new(NULL, EVC_EMAIL);
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Mailer")) {
			//Mailer
			attr = e_vcard_attribute_new(NULL, EVC_MAILER);
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Timezone")) {
			//Timezone
			attr = e_vcard_attribute_new(NULL, "TZ");
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Location")) {
			//Location
			attr = e_vcard_attribute_new(NULL, "GEO");
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Title")) {
			//Title
			attr = e_vcard_attribute_new(NULL, EVC_TITLE);
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Role")) {
			//Role
			attr = e_vcard_attribute_new(NULL, EVC_ROLE);
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Logo")) {
			//Logo
			printf("Logo is not supported yet\n");
		} else if (!strcmp(root->name, "Organization")) {
			//Company
			attr = e_vcard_attribute_new(NULL, EVC_ORG);
			add_value(attr, root, "Name", std_encoding);
			add_value(attr, root, "Unit", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Note")) {
			//Note
			attr = e_vcard_attribute_new(NULL, EVC_NOTE);
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Revision")) {
			//Revision
			attr = e_vcard_attribute_new(NULL, EVC_REV);
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Sound")) {
			//Sound
			printf("Sound is not supported yet\n");
		} else if (!strcmp(root->name, "Url")) {
			//Url
			attr = e_vcard_attribute_new(NULL, EVC_URL);
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Uid")) {
			//Uid
			attr = e_vcard_attribute_new(NULL, EVC_UID);
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Key")) {
			//Public Key
			attr = e_vcard_attribute_new(NULL, EVC_KEY);
			add_value(attr, root, "Content", std_encoding);
			e_vcard_add_attribute(vcard, attr);
		} else if (!strcmp(root->name, "Nickname")) {
			//Nickname
			attr = e_vcard_attribute_new(NULL, "NICKNAME");
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
		
		xmlNode *child = root->xmlChildrenNode;;
		while (child) {
			if (!strcmp(child->name, "Content")) {
				child = child->next;
				continue;
			}
			
			char *content = xmlNodeGetContent(child);
			if (!content) {
				child = child->next;
				continue;
			}
			
			if (!strcmp(content, "Work"))
				add_parameter(attr, "TYPE", "WORK");
			else if (!strcmp(content, "Home"))
				add_parameter(attr, "TYPE", "HOME");
			else if (!strcmp(content, "Postal"))
				add_parameter(attr, "TYPE", "POSTAL");
			else if (!strcmp(content, "Parcel"))
				add_parameter(attr, "TYPE", "PARCEL");
			else if (!strcmp(content, "Domestic"))
				add_parameter(attr, "TYPE", "DOM");
			else if (!strcmp(content, "International"))
				add_parameter(attr, "TYPE", "INTL");
			else if (!strcmp(content, "1"))
				add_parameter(attr, "TYPE", "PREF");
			else if (!strcmp(content, "Voice"))
				add_parameter(attr, "TYPE", "VOICE");
			else if (!strcmp(content, "Fax"))
				add_parameter(attr, "TYPE", "FAX");
			else if (!strcmp(content, "Message"))
				add_parameter(attr, "TYPE", "MSG");
			else if (!strcmp(content, "Cellular"))
				add_parameter(attr, "TYPE", "CELL");
			else if (!strcmp(content, "Pager"))
				add_parameter(attr, "TYPE", "PAGER");
			else if (!strcmp(content, "BulletinBoard"))
				add_parameter(attr, "TYPE", "BBS");
			else if (!strcmp(content, "Modem"))
				add_parameter(attr, "TYPE", "MODEM");
			else if (!strcmp(content, "Car"))
				add_parameter(attr, "TYPE", "CAR");
			else if (!strcmp(content, "ISDN"))
				add_parameter(attr, "TYPE", "ISDN");
			else if (!strcmp(content, "Video"))
				add_parameter(attr, "TYPE", "VIDEO");
			else if (!strcmp(content, "Internet"))
				add_parameter(attr, "TYPE", "INTERNET");
			else if (!strcmp(content, "X509"))
				add_parameter(attr, "TYPE", "X509");
			else if (!strcmp(content, "PGP"))
				add_parameter(attr, "TYPE", "PGP");
			else if (!strcmp(child->name, "UnknownParam"))
				add_parameter(attr, osxml_find_node(child, "ParamName"), osxml_find_node(child, "Content"));

			g_free(content);
			child = child->next;
		}
		
		root = root->next;
	}*/
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "xml-contact");
	
	osync_env_register_extension(env, "xml-contact", "evolution", init_x_evo_to_xml, init_xml_to_x_evo);
}
