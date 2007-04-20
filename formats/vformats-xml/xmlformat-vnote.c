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
#include <opensync/opensync.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync_xml.h>

#include "xmlformat.h"
#include "xmlformat-vnote.h"

/* ******* Paramter ****** */
static OSyncXMLField *handle_created_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "DateCreated", error);
}


static OSyncXMLField *handle_body_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Body", error);
}

static OSyncXMLField *handle_summary_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Summary", error);
}

static OSyncXMLField *handle_last_modified_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "LastModified", error);
}

static void vnote_parse_attributes(OSyncHookTables *hooks, GHashTable *table, OSyncXMLFormat *xmlformat, GHashTable *paramtable, GList **attributes)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, attributes);
	
	GList *a = NULL;
	for (a = *attributes; a; a = a->next) {
		VFormatAttribute *attr = a->data;
		
		osync_trace(TRACE_INTERNAL, "attributes:\"%s\"", vformat_attribute_get_name(attr));
		if (!strcmp(vformat_attribute_get_name(attr), "BEGIN")) {

			osync_trace(TRACE_INTERNAL, "%s: FOUND BEGIN", __func__);
		} else if (!strcmp(vformat_attribute_get_name(attr), "END")) {
			osync_trace(TRACE_EXIT, "%s: FOUND END", __func__);
			*attributes = a;
			return;
		} else
			handle_attribute(hooks, xmlformat, attr, NULL);
	}
	osync_trace(TRACE_EXIT, "%s: DONE", __func__);
}

static OSyncConvCmpResult compare_note(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftdata, rightdata);
	
	char* keys_content[] =  {"Content", NULL};
	OSyncXMLPoints points[] = {
		{"Summary", 		90, 	keys_content},
		{"Body", 	90, 	keys_content},
		{"DateCreated", 	10, 	keys_content},
		{NULL}
	};
	
	OSyncConvCmpResult ret = osync_xmlformat_compare((OSyncXMLFormat *)leftdata, (OSyncXMLFormat *)rightdata, points, 0, 100);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static void create_note(char **data, unsigned int *size)
{
	OSyncError *error = NULL;
	*data = (char *)osync_xmlformat_new("note", &error);
	if (!*data)
		osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

static void insert_attr_handler(GHashTable *table, const char *attrname, void* handler)
{
	g_hash_table_insert(table, (gpointer)attrname, handler);
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
	vnote_parse_attributes(hooks, hooks->attributes, xmlformat, hooks->parameters, &attributes);
	
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

static VFormatAttribute *handle_xml_summary_attribute(VFormat *vnote, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vnote, xmlfield, "SUMMARY", encoding);
}

static VFormatAttribute *handle_xml_last_modified_attribute(VFormat *vnote, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vnote, xmlfield, "LAST-MODIFIED", encoding);
}

static VFormatAttribute *handle_xml_created_attribute(VFormat *vnote, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vnote, xmlfield, "CREATED", encoding);
}

static void insert_xml_attr_handler(GHashTable *table, const char *name, void *handler)
{
	g_hash_table_insert(table, (gpointer)name, handler);
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


static time_t get_revision(const char *data, unsigned int size, OSyncError **error)
{	
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, data, size, error);
	
	OSyncXMLFieldList *fieldlist = osync_xmlformat_search_field((OSyncXMLFormat *)data, "LastModified", NULL);

	int length = osync_xmlfieldlist_get_length(fieldlist);
	if (length != 1) {
		osync_xmlfieldlist_free(fieldlist);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find the revision.");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return -1;
	}

	OSyncXMLField *xmlfield = osync_xmlfieldlist_item(fieldlist, 0);
	osync_xmlfieldlist_free(fieldlist);
	
	const char *revision = osync_xmlfield_get_nth_key_value(xmlfield, 0);
	osync_trace(TRACE_INTERNAL, "About to convert string %s", revision);
	time_t time = vformat_time_to_unix(revision);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, time);
	return time;
}

void get_format_info(OSyncFormatEnv *env)
{
	OSyncError *error = NULL;
	OSyncObjFormat *format = osync_objformat_new("xmlformat-note", "note", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format xmlfomat: %s", osync_error_print(&error));
		return;
	}

	osync_objformat_set_compare_func(format, compare_note);
	osync_objformat_set_destroy_func(format, destroy_xmlformat);
	osync_objformat_set_print_func(format, print_xmlformat);
	osync_objformat_set_copy_func(format, copy_xmlformat);
	osync_objformat_set_create_func(format, create_note);

	osync_objformat_set_revision_func(format, get_revision);

//	osync_objformat_must_marshal(format);
	osync_objformat_set_marshal_func(format, marshal_xmlformat);
	osync_objformat_set_demarshal_func(format, demarshal_xmlformat);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);

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
