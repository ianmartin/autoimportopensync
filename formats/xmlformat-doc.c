/*
 * xmlformat-doc - serialised textual versions of the XML event formats, for external plugins
 * Copyright (C) 2007  Andrew Baumann <andrewb@cse.unsw.edu.au>
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

#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <libxml/tree.h>

#include <opensync/opensync.h>
#include <opensync/opensync-xmlformat.h>
#include <opensync/opensync-format.h>

osync_bool convert_func(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
        *free_input = TRUE;
        *output = g_strdup(input);
        *outpsize = inpsize;
        return TRUE;
}

static osync_bool detect_plain_as_xmlformat(const char *objtype, const char *data, int size)
{
        GString *string = NULL;
        xmlDocPtr doc = NULL;
        xmlNodePtr cur = NULL;
        int ret = 0;

	osync_assert(objtype);

	string = g_string_new("<?xml version=\"*\"?>*");
	g_string_append(string, "<");
	g_string_append(string, objtype);
	g_string_append(string, ">*");
	if (!g_pattern_match_simple(g_string_free(string, FALSE), data))
		return FALSE;


	doc = xmlReadMemory(data, size, NULL, NULL, XML_PARSE_NOBLANKS);
	if (!doc)
		return FALSE;
		
	cur = xmlDocGetRootElement(doc);
	if (!cur)
		return FALSE;

	xmlStrcmp(BAD_CAST objtype, cur->name);

	if (ret)
		return FALSE;


	return TRUE;
}

static osync_bool detect_plain_as_xmlformat_contact(const char *data, int size, void *userdata)
{
	return detect_plain_as_xmlformat("contact", data, size);
}

static osync_bool detect_plain_as_xmlformat_event(const char *data, int size, void *userdata)
{
	return detect_plain_as_xmlformat("event", data, size);
}

static osync_bool detect_plain_as_xmlformat_todo(const char *data, int size, void *userdata)
{
	return detect_plain_as_xmlformat("todo", data, size);
}

static osync_bool detect_plain_as_xmlformat_note(const char *data, int size, void *userdata)
{
	return detect_plain_as_xmlformat("note", data, size);
}

static osync_bool from_xml(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	*free_input = TRUE;
	return osync_xmlformat_assemble((OSyncXMLFormat *)input, output, outpsize);
}

static osync_bool to_xml(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	OSyncXMLFormat *ret = osync_xmlformat_parse(input, inpsize, error);
	*free_input = TRUE;
	if (!ret)
		return FALSE;
	*output = (char *)ret;
	return TRUE;
}

static void destroy(char *input, unsigned int inpsize)
{
	free(input);
}

static osync_bool register_format(OSyncFormatEnv *env, const char *name, const char *objtype)
{
	OSyncError *error = NULL;

	OSyncObjFormat *format = osync_objformat_new(name, objtype, &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}

	osync_objformat_set_destroy_func(format, destroy);
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);

	return TRUE;
}

void get_format_info(OSyncFormatEnv *env)
{
	if (!register_format(env, "xmlformat-contact-doc", "contact"))
		return;

	if (!register_format(env, "xmlformat-event-doc", "event"))
		return;

	if (!register_format(env, "xmlformat-todo-doc", "todo"))
		return;

	if (!register_format(env, "xmlformat-note-doc", "note"))
		return;
}

static osync_bool register_converter(OSyncFormatEnv *env, const char *fromname, const char *toname, OSyncFormatDetectFunc detect_func)
{
	OSyncFormatConverter *conv;
	OSyncError *error = NULL;

	OSyncObjFormat *fromformat = osync_format_env_find_objformat(env, fromname);
	OSyncObjFormat *toformat = osync_format_env_find_objformat(env, toname);
	OSyncObjFormat *plain = osync_format_env_find_objformat(env, "plain");

	if (!fromformat || !toformat) {
		osync_trace(TRACE_ERROR, "Unable to register converter for %s->%s, format not found\n", fromname, toname);
		return FALSE;
	}

	/* Converters */
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, fromformat, toformat, to_xml, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, toformat, fromformat, from_xml, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);


	/* plain as xmlformat */
	conv = osync_converter_new(OSYNC_CONVERTER_DECAP, plain, fromformat, convert_func, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	return TRUE;
}

osync_bool get_conversion_info(OSyncFormatEnv *env)
{
	if (!register_converter(env, "xmlformat-contact-doc", "xmlformat-contact", detect_plain_as_xmlformat_contact))
		return FALSE;

	if (!register_converter(env, "xmlformat-event-doc", "xmlformat-event", detect_plain_as_xmlformat_event))
		return FALSE;

	if (!register_converter(env, "xmlformat-todo-doc", "xmlformat-todo", detect_plain_as_xmlformat_todo))
		return FALSE;

	if (!register_converter(env, "xmlformat-note-doc", "xmlformat-note", detect_plain_as_xmlformat_note))
		return FALSE;

	return TRUE;
}

int get_version(void)
{
	return 1;
}
