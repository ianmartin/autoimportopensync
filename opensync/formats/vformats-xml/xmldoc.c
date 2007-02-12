/*
 * xmldoc - serialised textual versions of the XML event formats, for external plugins
 * Copyright (C) 2006  Andrew Baumann <andrewb@cse.unsw.edu.au>
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

static osync_bool from_xml(void *conv_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	*free_input = TRUE;
	return osxml_marshall(input, inpsize, output, outpsize, error);
}

static osync_bool to_xml(void *conv_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	*free_input = TRUE;
	return osxml_demarshall(input, inpsize, output, outpsize, error);
}

static osync_bool copy_string(const char *input, int inpsize, char **output, int *outpsize)
{
	*output = strdup(input);
	*outpsize = inpsize;
	return TRUE;
}

static void destroy(char *input, size_t inpsize)
{
	free(input);
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "event");
	osync_env_register_objformat(env, "event", "xml-event-doc");
	osync_env_format_set_destroy_func(env, "xml-event-doc", destroy);
	osync_env_format_set_copy_func(env, "xml-event-doc", copy_string);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-event", "xml-event-doc", from_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-event-doc", "xml-event", to_xml);

	osync_env_register_objtype(env, "todo");
	osync_env_register_objformat(env, "todo", "xml-todo-doc");
	osync_env_format_set_destroy_func(env, "xml-todo-doc", destroy);
	osync_env_format_set_copy_func(env, "xml-todo-doc", copy_string);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-todo", "xml-todo-doc", from_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-todo-doc", "xml-todo", to_xml);

	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "xml-contact-doc");
	osync_env_format_set_destroy_func(env, "xml-contact-doc", destroy);
	osync_env_format_set_copy_func(env, "xml-contact-doc", copy_string);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-contact", "xml-contact-doc", from_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-contact-doc", "xml-contact", to_xml);

	osync_env_register_objtype(env, "note");
	osync_env_register_objformat(env, "note", "xml-note-doc");
	osync_env_format_set_destroy_func(env, "xml-note-doc", destroy);
	osync_env_format_set_copy_func(env, "xml-note-doc", copy_string);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-note", "xml-note-doc", from_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-note-doc", "xml-note", to_xml);
}
