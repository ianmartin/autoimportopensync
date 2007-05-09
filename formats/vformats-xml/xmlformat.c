/*
 * xmlformat - Common code base for xmlformat-* plugins. 
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2006  Daniel Friedrich <daniel.friedrich@opensync.org>
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
 * 
 */

#include "xmlformat.h"

/*** PARAMETER ***/

void handle_value_parameter(OSyncXMLField *xmlfield, VFormatParam *param)
{
	
	osync_trace(TRACE_INTERNAL, "Handling Value parameter %s\n", vformat_attribute_param_get_name(param));
	osync_xmlfield_set_attr(xmlfield, "Value", vformat_attribute_param_get_nth_value(param, 0));
}

/**** ATTRIBUTE ****/
OSyncXMLField *handle_attribute_simple_content(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, const char *name, OSyncError **error) 
{ 
	osync_trace(TRACE_INTERNAL, "Handling %s attribute", name);
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, name, error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	} 
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0)); 
	return xmlfield; 
}

OSyncXMLField *handle_categories_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Categories attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Categories", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	GList *values = vformat_attribute_get_values_decoded(attr);
	for (; values; values = values->next) {
		GString *retstr = (GString *)values->data;
		g_assert(retstr);
		osync_xmlfield_add_key_value(xmlfield, "Category", retstr->str);
	}
	
	return xmlfield;
}

OSyncXMLField *handle_class_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "Handling Class attribute");
	OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Class", error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	osync_xmlfield_set_key_value(xmlfield, "Content", vformat_attribute_get_nth_value(attr, 0));
	return xmlfield;
}

OSyncXMLField *handle_uid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Uid", error);
}

OSyncXMLField *handle_url_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error) 
{ 
	return handle_attribute_simple_content(xmlformat, attr, "Url", error);
}

/**** XML Attributes ****/
VFormatAttribute *handle_xml_attribute_simple_content(VFormat *vformat, OSyncXMLField *xmlfield, const char *name, const char *encoding)
{
	osync_assert(vformat);
	osync_assert(xmlfield);
	osync_assert(name);

	osync_trace(TRACE_INTERNAL, "Handling \"%s\" xml attribute", name);
	VFormatAttribute *attr = vformat_attribute_new(NULL, name);
	add_values(attr, xmlfield, encoding);
	vformat_add_attribute(vformat, attr);
	return attr;
}

VFormatAttribute *handle_xml_categories_attribute(VFormat *vformat, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vformat, xmlfield, "CATEGORIES", encoding);
}

VFormatAttribute *handle_xml_class_attribute(VFormat *vformat, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vformat, xmlfield, "CLASS", encoding);
}

VFormatAttribute *handle_xml_uid_attribute(VFormat *vformat, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vformat, xmlfield, "UID", encoding);
}

VFormatAttribute *handle_xml_url_attribute(VFormat *vformat, OSyncXMLField *xmlfield, const char *encoding)
{
	return handle_xml_attribute_simple_content(vformat, xmlfield, "URL", encoding);
}

/*** Encoding helpers ************/
osync_bool needs_encoding(const unsigned char *tmp, const char *encoding)
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

osync_bool needs_charset(const unsigned char *tmp)
{
	int i = 0;
	while (tmp[i] != 0) {
		if (tmp[i] > 127)
			return TRUE;
		i++;
	}
	return FALSE;
}

/* Attribute helpers */
void add_value(VFormatAttribute *attr, OSyncXMLField *xmlfield, const char *name, const char *encoding)
{
	osync_assert(xmlfield);
	osync_assert(name);

	const char *tmp = osync_xmlfield_get_key_value(xmlfield, name);
	
	if (!tmp) {
		/* If there is no node with the given name, add an empty value to the list.
		 * This is necessary because some fields (N and ADR, for example) need
		 * a specific order of the values
		 */
		tmp = "";
	}
	
	if (needs_charset((unsigned char*)tmp))
		if (!vformat_attribute_has_param (attr, "CHARSET"))
			vformat_attribute_add_param_with_value(attr, "CHARSET", "UTF-8");
	
	/* XXX: This one breaks unit test case: conv_vcard_evolution2_special
	   TODO: Combine this with converter extension/config ... e.g. if a mobile needs QP!
	        
	if (needs_encoding((unsigned char*)tmp, encoding)) {
		if (!vformat_attribute_has_param (attr, "ENCODING"))
			vformat_attribute_add_param_with_value(attr, "ENCODING", encoding);
		vformat_attribute_add_value_decoded(attr, tmp, strlen(tmp) + 1);
	} else*/
	vformat_attribute_add_value(attr, tmp);
}

void add_values(VFormatAttribute *attr, OSyncXMLField *xmlfield, const char *encoding)
{
	int i, c = osync_xmlfield_get_key_count(xmlfield);
	for(i=0; i<c; i++)
	{
		const char *tmp = osync_xmlfield_get_nth_key_value(xmlfield, i);

		if (!tmp) {
			/* If there is no node with the given name, add an empty value to the list.
			 * This is necessary because some fields (N and ADR, for example) need
			 * a specific order of the values
			 */
			tmp = "";
		}
	
		if (needs_charset((unsigned char*)tmp))
			if (!vformat_attribute_has_param (attr, "CHARSET"))
				vformat_attribute_add_param_with_value(attr, "CHARSET", "UTF-8");
	
		if (needs_encoding((unsigned char*)tmp, encoding)) {
			if (!vformat_attribute_has_param (attr, "ENCODING"))
				vformat_attribute_add_param_with_value(attr, "ENCODING", encoding);
			vformat_attribute_add_value_decoded(attr, tmp, strlen(tmp) + 1);
		} else
			vformat_attribute_add_value(attr, tmp);
	}
}

void add_values_from_nth_field_on(VFormatAttribute *attr, OSyncXMLField *xmlfield, const char *encoding, int nth)
{
	int i, c = osync_xmlfield_get_key_count(xmlfield);
	for(i=nth; i<c; i++)
	{
		const char *tmp = osync_xmlfield_get_nth_key_value(xmlfield, i);

		if (!tmp) {
			/* If there is no node with the given name, add an empty value to the list.
			 * This is necessary because some fields (N and ADR, for example) need
			 * a specific order of the values
			 */
			tmp = "";
		}
	
		if (needs_charset((unsigned char*)tmp))
			if (!vformat_attribute_has_param (attr, "CHARSET"))
				vformat_attribute_add_param_with_value(attr, "CHARSET", "UTF-8");
	
		if (needs_encoding((unsigned char*)tmp, encoding)) {
			if (!vformat_attribute_has_param (attr, "ENCODING"))
				vformat_attribute_add_param_with_value(attr, "ENCODING", encoding);
			vformat_attribute_add_value_decoded(attr, tmp, strlen(tmp) + 1);
		} else
			vformat_attribute_add_value(attr, tmp);
	}
}



void destroy_xmlformat(char *input, size_t inpsize)
{
	osync_xmlformat_unref((OSyncXMLFormat *)input);
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
	if(osync_xmlformat_assemble((OSyncXMLFormat *)data, &buffer, &i) == TRUE)
		return buffer;
	else
		return NULL;
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


/* Paramter and Attribute Handler */ 
void handle_parameter(OSyncHookTables *hooks, OSyncXMLField *xmlfield, VFormatParam *param)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, hooks, xmlfield, param);
	
	//Find the handler for this parameter
	void (* param_handler)(OSyncXMLField *, VFormatParam *);
	char *paramname = g_strdup_printf("%s=%s", vformat_attribute_param_get_name(param), vformat_attribute_param_get_nth_value(param, 0));
	param_handler = g_hash_table_lookup(hooks->parameters, paramname);
	g_free(paramname);

	if (!param_handler)
		param_handler = g_hash_table_lookup(hooks->parameters, vformat_attribute_param_get_name(param));
	
	if (param_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	
	if (param_handler)
		param_handler(xmlfield, param);
//	else 
//		handle_unknown_parameter(current, param);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void handle_attribute(OSyncHookTables *hooks, OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s, %p)", __func__, hooks, xmlformat, attr, attr ? vformat_attribute_get_name(attr) : "None", error);
	OSyncXMLField *xmlfield = NULL;

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
	OSyncXMLField *(* attr_handler)(OSyncXMLFormat *, VFormatAttribute *, OSyncError **) = g_hash_table_lookup(hooks->attributes, vformat_attribute_get_name(attr));
	osync_trace(TRACE_INTERNAL, "Hook is: %p", attr_handler);
	if (attr_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	if (attr_handler)
		xmlfield = attr_handler(xmlformat, attr, error);
//	else
//		xmlfield = handle_unknown_attribute(xmlformat, attr, error);

	//Handle all parameters of this attribute
	GList *params = vformat_attribute_get_params(attr);
	GList *p = NULL;
	for (p = params; p; p = p->next) {
		VFormatParam *param = p->data;
		handle_parameter(hooks, xmlfield, param);
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void xml_handle_parameter(OSyncHookTables *hooks, VFormatAttribute *attr, OSyncXMLField *xmlfield, int attr_nr)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s, %i)", __func__, hooks, attr, xmlfield, xmlfield ? osync_xmlfield_get_name(xmlfield) : "None", attr_nr);
	
	//Find the handler for this parameter
	const char* par_name = osync_xmlfield_get_nth_attr_name(xmlfield, attr_nr);
	const char* par_value = osync_xmlfield_get_nth_attr_value(xmlfield, attr_nr);
	
	void (* xml_param_handler)(VFormatAttribute *attr, OSyncXMLField *);
	char *paramname = g_strdup_printf("%s=%s", par_name, par_value);
	xml_param_handler = g_hash_table_lookup(hooks->parameters, paramname);
	g_free(paramname);
	
	if (!xml_param_handler)
		xml_param_handler = g_hash_table_lookup(hooks->parameters, par_name);
	
	if (xml_param_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}

	if (xml_param_handler)
		xml_param_handler(attr, xmlfield);
	else 
		osync_trace(TRACE_INTERNAL, "No handler found!!!");
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void xml_handle_attribute(OSyncHookTables *hooks, VFormat *vformat, OSyncXMLField *xmlfield, const char *encoding)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s)", __func__, hooks, vformat, xmlfield, xmlfield ? osync_xmlfield_get_name(xmlfield) : "None");
	
	VFormatAttribute *attr = NULL;
	
	//We need to find the handler for this attribute
	VFormatAttribute *(* xml_attr_handler)(VFormat *vformat, OSyncXMLField *xmlfield, const char *) = g_hash_table_lookup(hooks->attributes, osync_xmlfield_get_name(xmlfield));
	osync_trace(TRACE_INTERNAL, "xml hook is: %p", xml_attr_handler);
	if (xml_attr_handler == HANDLE_IGNORE) {
		osync_trace(TRACE_EXIT, "%s: Ignored", __func__);
		return;
	}
	if (xml_attr_handler)
		attr = xml_attr_handler(vformat, xmlfield, encoding);
	else {
		osync_trace(TRACE_EXIT, "%s: Ignored2", __func__);
		return;
	}
	
	//Handle all parameters of this attribute
	int i, c = osync_xmlfield_get_attr_count(xmlfield);
	for(i=0; i<c; i++) {
		xml_handle_parameter(hooks, attr, xmlfield, i);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);	
}

static OSyncConvCmpResult compare_contact(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %i)", __func__, leftdata, leftsize, rightdata, rightsize);
	
	char* keys_content[] =  {"Content", NULL};
	char* keys_name[] = {"FirstName", "LastName", NULL};
	OSyncXMLPoints points[] = {
		{"Name", 		90, 	keys_name},
		{"Telephone", 	10, 	keys_content},
		{"EMail", 		10, 	keys_content},
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
		{"Summary", 		90, 	keys_content},
		{"DateTimeStart", 	10, 	keys_content},
		{"DateTimeEnd", 	10, 	keys_content},
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
		{"Summary", 		90, 	keys_content},
		{"DateStarted", 	10, 	keys_content},
		{"DateDue", 	10, 	keys_content},
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
	time_t time = vformat_time_to_unix(revision);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, time);
	return time;

error:	
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return -1;
}


void get_format_info(OSyncFormatEnv *env)
{
	OSyncError *error = NULL;
	OSyncObjFormat *format = NULL;

	/* register xmlformat-contact */
	format = osync_objformat_new("xmlformat-contact", "contact", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format xmlformat: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	
	osync_objformat_set_compare_func(format, compare_contact);
	osync_objformat_set_destroy_func(format, destroy_xmlformat);
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
		return;
	}
	
	osync_objformat_set_compare_func(format, compare_event);
	osync_objformat_set_destroy_func(format, destroy_xmlformat);
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
		return;
	}

	osync_objformat_set_compare_func(format, compare_todo);
	osync_objformat_set_destroy_func(format, destroy_xmlformat);
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

