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

static osync_bool conv_vcard_to_xml(char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_debug("VCARD", 4, "start: %s", __func__);
	//printf("input is %i\n%s\n", inpsize, input);
	GList *p = NULL;
	GList *a = NULL;
	OSyncXMLEncoding encoding;
	
	g_type_init();
	EVCard *vcard = e_vcard_new_from_string(input);
	//e_vcard_dump_structure (vcard);
	GList *attributes = e_vcard_get_attributes(vcard);
	xmlDoc *doc = xmlNewDoc("1.0");
	xmlNode *root = osxml_node_add_root(doc, "contact");
	
	for (a = attributes; a; a = a->next) {
		EVCardAttribute *attr = a->data;
		encoding = property_to_xml_encoding(attr);
		const char *name = e_vcard_attribute_get_name(attr);

		if (!strcmp(name, "VERSION"))
			continue;
		
		if (!strcmp(name, "BEGIN")) {
			root = osxml_node_add_root(doc, "contact");
			continue;
		}
		
		xmlNode *current = xmlNewChild(root, NULL, "", NULL);

		GList *params = e_vcard_attribute_get_params(attr);
		for (p = params; p; p = p->next) {
			EVCardAttributeParam *param = p->data;

			const char *propname = property_get_nth_value(param, 0);
			if (propname) {
				if (!strcmp(e_vcard_attribute_param_get_name(param), "ENCODING"))
					continue;
				if (!strcmp(e_vcard_attribute_param_get_name(param), "CHARSET"))
					continue;
				
				if (!strcmp(propname, "WORK"))
					osxml_node_add(current, "Location", "Work");
				else if (!strcmp(propname, "HOME"))
					osxml_node_add(current, "Location", "Home");
				else if (!strcmp(propname, "POSTAL"))
					osxml_node_add(current, "Type", "Postal");
				else if (!strcmp(propname, "PARCEL"))
					osxml_node_add(current, "Type", "Parcel");
				else if (!strcmp(propname, "DOM"))
					osxml_node_add(current, "Range", "Domestic");
				else if (!strcmp(propname, "INTL"))
					osxml_node_add(current, "Range", "International");
				else if (!strcmp(propname, "PREF"))
					osxml_node_add(current, "Order", "1");
				else if (!strcmp(propname, "VOICE"))
					osxml_node_add(current, "Type", "Voice");
				else if (!strcmp(propname, "FAX"))
					osxml_node_add(current, "Type", "Fax");
				else if (!strcmp(propname, "MSG"))
					osxml_node_add(current, "Type", "Message");
				else if (!strcmp(propname, "CELL"))
					osxml_node_add(current, "Type", "Cellular");
				else if (!strcmp(propname, "PAGER"))
					osxml_node_add(current, "Type", "Pager");
				else if (!strcmp(propname, "BBS"))
					osxml_node_add(current, "Type", "BulletinBoard");
				else if (!strcmp(propname, "MODEM"))
					osxml_node_add(current, "Type", "Modem");
				else if (!strcmp(propname, "CAR"))
					osxml_node_add(current, "Location", "Car");
				else if (!strcmp(propname, "ISDN"))
					osxml_node_add(current, "Type", "ISDN");
				else if (!strcmp(propname, "VIDEO"))
					osxml_node_add(current, "Type", "Video");
				else if (!strcmp(propname, "INTERNET"))
					osxml_node_add(current, "Type", "Internet");
				else if (!strcmp(propname, "X509"))
					osxml_node_add(current, "Type", "X509");
				else if (!strcmp(propname, "PGP"))
					osxml_node_add(current, "Type", "PGP");
				else if (!strcmp(name, "PHOTO") && !strcmp(e_vcard_attribute_param_get_name(param), "TYPE"))
					osxml_node_add(current, "Type", g_strdup(propname));
				else {
					xmlNode *property = xmlNewChild(current, NULL, "", NULL);
					osxml_node_set(property, "UnknownParam", property_get_nth_value(param, 0), encoding);
					osxml_node_add(property, "ParamName", e_vcard_attribute_param_get_name(param));
					osxml_node_mark_unknown(current);
				}
				/*if (content)
					g_free(content);
				g_free(propname);*/
			}
		}
		
		//FullName
		if (!strcmp(name, "FN")) {
			osxml_node_set(current, "FullName", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}

		//Name
		if (!strcmp(name, "N")) {
			osxml_node_set(current, "Name", NULL, encoding);
			osxml_node_add(current, "LastName", attribute_get_nth_value(attr, 0));
			osxml_node_add(current, "FirstName", attribute_get_nth_value(attr, 1));
			osxml_node_add(current, "Additional", attribute_get_nth_value(attr, 2));
			osxml_node_add(current, "Prefix", attribute_get_nth_value(attr, 3));
			osxml_node_add(current, "Suffix", attribute_get_nth_value(attr, 4));
			continue;
		}
		
		//Photo
		if (!strcmp(name, "PHOTO")) {
			osxml_node_set(current, "Photo", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Birthday
		if (!strcmp(name, "BDAY")) {
			osxml_node_set(current, "Birthday", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Address
		if (!strcmp(name, "ADR")) {
			osxml_node_set(current, "Address", NULL, encoding);
			osxml_node_add(current, "PostalBox", attribute_get_nth_value(attr, 0));
			osxml_node_add(current, "ExtendedAddress", attribute_get_nth_value(attr, 1));
			osxml_node_add(current, "Street", attribute_get_nth_value(attr, 2));
			osxml_node_add(current, "City", attribute_get_nth_value(attr, 3));
			osxml_node_add(current, "Region", attribute_get_nth_value(attr, 4));
			osxml_node_add(current, "PostalCode", attribute_get_nth_value(attr, 5));
			osxml_node_add(current, "Country", attribute_get_nth_value(attr, 6));
			continue;
		}
		
		//Address Labeling
		if (!strcmp(name, "LABEL")) {
			osxml_node_set(current, "AddressLabel", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Telephone
		if (!strcmp(name, "TEL")) {
			osxml_node_set(current, "Telephone", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//EMail
		if (!strcmp(name, "EMAIL")) {
			osxml_node_set(current, "EMail", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Mailer
		if (!strcmp(name, "MAILER")) {
			osxml_node_set(current, "Mailer", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Timezone
		if (!strcmp(name, "TZ")) {
			osxml_node_set(current, "Timezone", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Location
		if (!strcmp(name, "GEO")) {
			osxml_node_set(current, "Location", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Title
		if (!strcmp(name, "TITLE")) {
			osxml_node_set(current, "Title", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Role
		if (!strcmp(name, "ROLE")) {
			osxml_node_set(current, "Role", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Logo
		if (!strcmp(name, "LOGO")) {
			printf("Logo is not supported yet\n");
			continue;
		}
		
		//Company
		if (!strcmp(name, "ORG")) {
			osxml_node_set(current, "Organization", NULL, encoding);
			osxml_node_add(current, "Name", attribute_get_nth_value(attr, 0));
			osxml_node_add(current, "Unit", attribute_get_nth_value(attr, 1));
			continue;
		}
		
		//Note
		if (!strcmp(name, "NOTE")) {
			osxml_node_set(current, "Note", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Revision
		if (!strcmp(name, "REV")) {
			osxml_node_set(current, "Revision", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Sound
		if (!strcmp(name, "SOUND")) {
			printf("Sound is not supported yet\n");
			continue;
		}
		
		//Url
		if (!strcmp(name, "URL")) {
			osxml_node_set(current, "Url", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Uid
		if (!strcmp(name, "UID")) {
			osxml_node_set(current, "Uid", attribute_get_nth_value(attr, 0), encoding);
			continue;
		}
		
		//Public Key
		if (!strcmp(name, "KEY")) {
			osxml_node_set(current, "Key", attribute_get_nth_value(attr, 0), encoding);
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
	
	*free_input = TRUE;
	*output = (char *)doc;
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

static osync_bool conv_xml_to_vcard(char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error, int target)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	//xmlDocDump(stdout, (xmlDoc *)input);
	EVCardAttribute *attr = NULL;
	xmlNode *root = osxml_node_get_root((xmlDoc *)input, "contact", error);
	if (!root)
		return FALSE;
	
	g_type_init();
	EVCard *vcard = e_vcard_new();
	
	const char *std_encoding = NULL;
	if (target == EVC_FORMAT_VCARD_21)
		std_encoding = "QUOTED-PRINTABLE";
	else
		std_encoding = "B";
		
	while (root) {
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
	}
	
	*free_input = TRUE;
	*output = e_vcard_to_string(vcard, target);
	*outpsize = strlen(*output);
	return TRUE;
}

static osync_bool conv_xml_to_vcard30(char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	return conv_xml_to_vcard(input, inpsize, output, outpsize, free_input, error, EVC_FORMAT_VCARD_30);
}

static osync_bool conv_xml_to_vcard21(char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	return conv_xml_to_vcard(input, inpsize, output, outpsize, free_input, error, EVC_FORMAT_VCARD_21);
}

static OSyncConvCmpResult compare_contact(OSyncChange *leftchange, OSyncChange *rightchange)
{
	OSyncXMLScore score[] =
	{
	{50, "/contact/FullName"},
	{50, "/contact/Name"},
	{50, "/contact/Telephone"},
	{50, "/contact/Address"},
	{0, NULL}
	};
	
	return osxml_compare((xmlDoc*)osync_change_get_data(leftchange), (xmlDoc*)osync_change_get_data(rightchange), score);
}

static char *print_contact(OSyncChange *change)
{
	osync_debug("XCONT", 4, "start: %s", __func__);
	xmlDoc *doc = (xmlDoc *)osync_change_get_data(change);
	char *result;
	int size;
	osync_bool free_input;
	if (!conv_xml_to_vcard30((char*)doc, 0, &result, &size, &free_input, NULL))
		return NULL;
	return result;
}

static void destroy_xml(char *data, size_t size)
{
	xmlFreeDoc((xmlDoc *)data);
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "xml-contact");
	osync_env_format_set_compare_func(env, "xml-contact", compare_contact);
	osync_env_format_set_destroy_func(env, "xml-contact", destroy_xml);
	osync_env_format_set_print_func(env, "xml-contact", print_contact);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vcard21", "xml-contact", conv_vcard_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-contact", "vcard21", conv_xml_to_vcard21);
	
	osync_env_register_converter(env, CONVERTER_CONV, "vcard30", "xml-contact", conv_vcard_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-contact", "vcard30", conv_xml_to_vcard30);
}
