/*
 * xmlformat - registration of xml object formats 
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2006  Daniel Friedrich <daniel.friedrich@opensync.org>
 * Copyright (C) 2007  Daniel Gollub <dgollub@suse.de>
 * Copyright (C) 2007  Jerry Yu <jijun.yu@sun.com>
 * Copyright (C) 2007  Christopher Stender <cstender@suse.de>
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

#include "xmlformat.h"

void destroy_xmlformat(char *input, unsigned int inpsize)
{
	osync_xmlformat_unref((OSyncXMLFormat *)input);
}

static osync_bool duplicate_xmlformat(const char *uid, const char *input, unsigned int insize, char **newuid, char **output, unsigned int *outsize, osync_bool *dirty, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %p, %i, %p, %p, %p, %p, %p)", __func__, uid, input, insize, newuid, output, outsize, dirty, error);
	
	char *buffer = NULL;
	unsigned int size;
	
	osync_xmlformat_assemble((OSyncXMLFormat *) input, &buffer, &size);

	OSyncXMLFormat *xmlformat = osync_xmlformat_parse(buffer, size, error);
	if (!xmlformat) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	*dirty = TRUE;
	*newuid = g_strdup_printf ("%s-dupe", uid);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}


osync_bool copy_xmlformat(const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error)
{
	/* TODO: we can do that faster with a osync_xmlformat_copy() function */
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p)", __func__, input, inpsize, output, outpsize, error);
	OSyncXMLFormat *xmlformat = NULL;

	char *buffer = NULL;
	unsigned int size;
	
	osync_xmlformat_assemble((OSyncXMLFormat *) input, &buffer, &size);
	xmlformat = osync_xmlformat_parse(buffer, size, error);
	if (!xmlformat) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	*output = (char *) xmlformat;
	*outpsize = size;

	g_free(buffer);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}
char *print_xmlformat(const char *data, unsigned int size)
{
	char *buffer;
	unsigned int i;

	if (!data)
		return NULL;

	if(!osync_xmlformat_assemble((OSyncXMLFormat *)data, &buffer, &i))
		return NULL;

	return buffer;
}

osync_bool marshal_xmlformat(const char *input, unsigned int inpsize, OSyncMessage *message, OSyncError **error)
{
	char *buffer;
	unsigned int size;
	
	if(!osync_xmlformat_assemble((OSyncXMLFormat *)input, &buffer, &size))
		return FALSE;

	osync_message_write_buffer(message, buffer, (int)size);
	
	return TRUE;
}

osync_bool demarshal_xmlformat(OSyncMessage *message, char **output, unsigned int *outpsize, OSyncError **error)
{
	void *buffer = NULL;
	unsigned int size = 0;
	osync_message_read_buffer(message, &buffer, (int *)&size);
	
	OSyncXMLFormat *xmlformat = osync_xmlformat_parse((char *)buffer, size, error);
	if (!xmlformat) {
		osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	*output = (char*)xmlformat;
	*outpsize = 0; /* TODO: the compiler do not know the size of OSyncXMLFormat : 0 is ok? */
	return TRUE;
}


static OSyncConvCmpResult compare_contact(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %i)", __func__, leftdata, leftsize, rightdata, rightsize);
	
	char* keys_content[] =  {"Content", NULL};
	char* keys_name[] = {"FirstName", "LastName", NULL};
	OSyncXMLPoints points[] = {
		{"EMail", 		10, 	keys_content},
		{"Name", 		90, 	keys_name},
		{"Telephone", 	10, 	keys_content},
		{NULL}
	};

	OSyncConvCmpResult ret = osync_xmlformat_compare((OSyncXMLFormat *)leftdata, (OSyncXMLFormat *)rightdata, points, 0, 100);
		
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static void create_contact(char **data, unsigned int *size)
{
	OSyncError *error = NULL;
	*data = (char *)osync_xmlformat_new("contact", &error);
	if (!*data)
		osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

static OSyncConvCmpResult compare_event(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftdata, rightdata);
	
	char* keys_content[] =  {"Content", NULL};
	OSyncXMLPoints points[] = {
		{"DateEnd", 	10, 	keys_content},
		{"DateStarted", 	10, 	keys_content},
		{"Summary", 		90, 	keys_content},
		{NULL}
	};
	
	OSyncConvCmpResult ret = osync_xmlformat_compare((OSyncXMLFormat *)leftdata, (OSyncXMLFormat *)rightdata, points, 0, 100);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

void create_event(char **data, unsigned int *size)
{
	OSyncError *error = NULL;
	*data = (char *)osync_xmlformat_new("event", &error);
	if (!*data)
		osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

static OSyncConvCmpResult compare_todo(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftdata, rightdata);
	
	char* keys_content[] =  {"Content", NULL};
	OSyncXMLPoints points[] = {
		{"DateDue", 	10, 	keys_content},
		{"DateStarted", 	10, 	keys_content},
		{"Summary", 		90, 	keys_content},
		{NULL}
	};
	
	OSyncConvCmpResult ret = osync_xmlformat_compare((OSyncXMLFormat *)leftdata, (OSyncXMLFormat *)rightdata, points, 0, 100);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static void create_todo(char **data, unsigned int *size)
{
	OSyncError *error = NULL;
	*data = (char *)osync_xmlformat_new("todo", &error);
	if (!*data)
		osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

static OSyncConvCmpResult compare_note(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftdata, rightdata);
	
	char* keys_content[] =  {"Content", NULL};
	OSyncXMLPoints points[] = {
		{"Body", 	90, 	keys_content},
		{"DateCreated", 	10, 	keys_content},
		{"Summary", 		90, 	keys_content},
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


static time_t get_revision(const char *data, unsigned int size, OSyncError **error)
{	
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, data, size, error);
	
	OSyncXMLFieldList *fieldlist = osync_xmlformat_search_field((OSyncXMLFormat *)data, "Revision", error, NULL);
	if (!fieldlist)
		goto error;

	int length = osync_xmlfieldlist_get_length(fieldlist);
	if (length != 1) {
		osync_xmlfieldlist_free(fieldlist);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find the revision.");
		goto error;
	}

	OSyncXMLField *xmlfield = osync_xmlfieldlist_item(fieldlist, 0);
	osync_xmlfieldlist_free(fieldlist);
	
	const char *revision = osync_xmlfield_get_nth_key_value(xmlfield, 0);
	osync_trace(TRACE_INTERNAL, "About to convert string %s", revision);
	//time_t time = vformat_time_to_unix(revision);
	time_t time = osync_time_vtime2unix(revision, 0);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, time);
	return time;

error:	
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return -1;
}


osync_bool get_format_info(OSyncFormatEnv *env)
{
	OSyncError *error = NULL;
	OSyncObjFormat *format = NULL;

	/* register xmlformat-contact */
	format = osync_objformat_new("xmlformat-contact", "contact", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format xmlformat: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	
	osync_objformat_set_compare_func(format, compare_contact);
	osync_objformat_set_destroy_func(format, destroy_xmlformat);
	osync_objformat_set_duplicate_func(format, duplicate_xmlformat);
	osync_objformat_set_print_func(format, print_xmlformat);
	osync_objformat_set_copy_func(format, copy_xmlformat);
	osync_objformat_set_create_func(format, create_contact);
	
	osync_objformat_set_revision_func(format, get_revision);
	
	osync_objformat_must_marshal(format);
	osync_objformat_set_marshal_func(format, marshal_xmlformat);
	osync_objformat_set_demarshal_func(format, demarshal_xmlformat);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);


	/* register xmlformat-event */
	format = osync_objformat_new("xmlformat-event", "event", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format xmlformat: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	
	osync_objformat_set_compare_func(format, compare_event);
	osync_objformat_set_destroy_func(format, destroy_xmlformat);
	osync_objformat_set_duplicate_func(format, duplicate_xmlformat);
	osync_objformat_set_print_func(format, print_xmlformat);
	osync_objformat_set_copy_func(format, copy_xmlformat);
	osync_objformat_set_create_func(format, create_event);
	
	osync_objformat_set_revision_func(format, get_revision);
	
	osync_objformat_must_marshal(format);
	osync_objformat_set_marshal_func(format, marshal_xmlformat);
	osync_objformat_set_demarshal_func(format, demarshal_xmlformat);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);


	/* register xmlformat-todo */
	format = osync_objformat_new("xmlformat-todo", "todo", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format xmlfomat: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}

	osync_objformat_set_compare_func(format, compare_todo);
	osync_objformat_set_destroy_func(format, destroy_xmlformat);
	osync_objformat_set_duplicate_func(format, duplicate_xmlformat);
	osync_objformat_set_print_func(format, print_xmlformat);
	osync_objformat_set_copy_func(format, copy_xmlformat);
	osync_objformat_set_create_func(format, create_todo);

	osync_objformat_set_revision_func(format, get_revision);

//	osync_objformat_must_marshal(format);
	osync_objformat_set_marshal_func(format, marshal_xmlformat);
	osync_objformat_set_demarshal_func(format, demarshal_xmlformat);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);


	/* register xmlformat-note */
	format = osync_objformat_new("xmlformat-note", "note", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format xmlfomat: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}

	osync_objformat_set_compare_func(format, compare_note);
	osync_objformat_set_destroy_func(format, destroy_xmlformat);
	osync_objformat_set_duplicate_func(format, duplicate_xmlformat);
	osync_objformat_set_print_func(format, print_xmlformat);
	osync_objformat_set_copy_func(format, copy_xmlformat);
	osync_objformat_set_create_func(format, create_note);

	osync_objformat_set_revision_func(format, get_revision);

//	osync_objformat_must_marshal(format);
	osync_objformat_set_marshal_func(format, marshal_xmlformat);
	osync_objformat_set_demarshal_func(format, demarshal_xmlformat);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);

	return TRUE;
}

int get_version(void)
{
	return 1;
}

