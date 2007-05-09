/*
 * xmlformat-note - A plugin for parsing vnote objects for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007  Daniel Gollub <dgollub@suse.de>
 * Copyright (C) 2007  Jerry Yu <jijun.yu@sun.com>
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
 */

#include "xmlformat-vnote.h"

/* ******* Paramter ****** */
static OSyncXMLField *handle_body_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Body", error);
}

static void *init_vnote_to_xmlformat(VFormatType target)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);

	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));
	
	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);
	
	//VNOTE components
	insert_attr_handler(hooks->attributes, "BEGIN", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "END", HANDLE_IGNORE);
	insert_attr_handler(hooks->attributes, "BODY", (void *)handle_body_attribute);
	insert_attr_handler(hooks->attributes, "SUMMARY", (void *)handle_summary_attribute);
	insert_attr_handler(hooks->attributes, "CLASS", (void *)handle_class_attribute);
	insert_attr_handler(hooks->attributes, "CATEGORIES", (void *)handle_categories_attribute);
	insert_attr_handler(hooks->attributes, "UID", (void *)handle_uid_attribute);
	insert_attr_handler(hooks->attributes, "LAST-MODIFIED", (void *)handle_last_modified_attribute);
	insert_attr_handler(hooks->attributes, "CREATED", (void *)handle_created_attribute);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static osync_bool conv_vnote_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);
	
	OSyncHookTables *hooks = init_vnote_to_xmlformat(VFORMAT_NOTE);
	
	osync_trace(TRACE_INTERNAL, "Input vcal is:\n%s", input);
	//Parse the vnote
	VFormat *vnote = vformat_new_from_string(input);

	OSyncXMLFormat *xmlformat = osync_xmlformat_new("note", error);
	
	osync_trace(TRACE_INTERNAL, "parsing attributes");
	
	GList *attributes = vformat_get_attributes(vnote);
	vcalendar_parse_attributes(hooks, hooks->attributes, xmlformat, hooks->parameters, &attributes);
	
	g_hash_table_destroy(hooks->attributes);
	g_hash_table_destroy(hooks->parameters);
	g_free(hooks);
	
	*free_input = TRUE;
	*output = (char *)xmlformat;
	*outpsize = sizeof(xmlformat);

	osync_xmlformat_sort(xmlformat);
	
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "... Output XMLFormat is: \n%s", str);
	g_free(str);
	
	if (osync_xmlformat_validate(xmlformat) == FALSE)
		osync_trace(TRACE_INTERNAL, "XMLFORMAT NOTE: Not valid!");
	else
		osync_trace(TRACE_INTERNAL, "XMLFORMAT NOTE: Valid!");

	vformat_free(vnote);
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

static VFormatAttribute *handle_xml_body_attribute(VFormat *vnote, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vnote, xmlfield, "BODY", encoding);
}

static OSyncHookTables *init_xmlformat_to_vnote(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	
	OSyncHookTables *hooks = g_malloc0(sizeof(OSyncHookTables));

	hooks->attributes = g_hash_table_new(g_str_hash, g_str_equal);
	hooks->parameters = g_hash_table_new(g_str_hash, g_str_equal);

	//VNOTE component
	insert_xml_attr_handler(hooks->attributes, "XIrmcLuid", (void *)handle_xml_uid_attribute);
	insert_xml_attr_handler(hooks->attributes, "Summary", (void *)handle_xml_summary_attribute);
	insert_xml_attr_handler(hooks->attributes, "Class", (void *)handle_xml_class_attribute);
	insert_xml_attr_handler(hooks->attributes, "Categories", (void *)handle_xml_categories_attribute);
	insert_xml_attr_handler(hooks->attributes, "LastModified", (void *)handle_xml_last_modified_attribute);
	insert_xml_attr_handler(hooks->attributes, "DateCreated", (void *)handle_xml_created_attribute);
	insert_xml_attr_handler(hooks->attributes, "Body", (void *)handle_xml_body_attribute);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, hooks);
	return (void *)hooks;
}

static osync_bool conv_xmlformat_to_vnotememo(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error, int target)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);
	
	OSyncHookTables *hooks = init_xmlformat_to_vnote();
	
	int i = 0;
	if (config) {
		gchar ** config_array = g_strsplit_set(config, "=;", 0);
		for (i=0; config_array[i]; i+=2)
		{
			/*TODO: what's the meaning of config? */
		}
		g_strfreev(config_array);
	}

	// Print input XMLFormat into terminal
	OSyncXMLFormat *xmlformat = (OSyncXMLFormat *)input;
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "Input XMLFormat is:\n%s", str);
	g_free(str);

	//Parsing xml attributes
	VFormat *vnote = vformat_new();
	osync_trace(TRACE_INTERNAL, "parsing xml attributes");
	const char *std_encoding = NULL;
	if (target == VFORMAT_NOTE)
		std_encoding = "QUOTED-PRINTABLE";
	else
		std_encoding = "B";
	
	OSyncXMLField *xmlfield = osync_xmlformat_get_first_field(xmlformat);
	for(; xmlfield != NULL; xmlfield = osync_xmlfield_get_next(xmlfield)) {
		xml_handle_attribute(hooks, vnote, xmlfield, std_encoding);
	}
	
	g_hash_table_destroy(hooks->attributes);
	g_hash_table_destroy(hooks->parameters);
	g_free(hooks);

	*free_input = TRUE;
	*output = vformat_to_string (vnote, target);
	*outpsize = strlen(*output) + 1;
	
	vformat_free(vnote);	
	
	osync_trace(TRACE_INTERNAL, "Output vnote is : \n%s", *output);
	osync_trace(TRACE_EXIT, "%s", __func__);
	
	return TRUE;
}

static osync_bool conv_xmlformat_to_vnote(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	return conv_xmlformat_to_vnotememo(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_NOTE);
}

void get_conversion_info(OSyncFormatEnv *env)
{
	OSyncFormatConverter *conv = NULL;
	OSyncError *error = NULL;

	OSyncObjFormat *xmlformat = osync_format_env_find_objformat(env, "xmlformat-note");
	OSyncObjFormat *vnote = osync_format_env_find_objformat(env, "vnote11");

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vnote, conv_xmlformat_to_vnote, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vnote, xmlformat, conv_vnote_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
}

int get_version(void)
{
	return 1;
}
