/*
 * xmlformat-vtodo10 - convert vtodo10 to xmlformat-todo and backwards
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
 *
 */

#include "xmlformat-vtodo.h"
#include "xmlformat-vtodo10.h"

osync_bool conv_vtodo10_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);
	
	OSyncHookTables *hooks = init_vtodo_to_xmlformat(VFORMAT_TODO_10);
	
	osync_trace(TRACE_INTERNAL, "Input vcal is:\n%s", input);
	//Parse the vtodo
	VFormat *vtodo = vformat_new_from_string(input);

	OSyncXMLFormat *xmlformat = osync_xmlformat_new("todo", error);
	
	osync_trace(TRACE_INTERNAL, "parsing attributes");
	
	GList *attributes = vformat_get_attributes(vtodo);
	vcalendar_parse_attributes(xmlformat, &attributes, hooks, hooks->attributes, hooks->parameters);
	
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
		osync_trace(TRACE_INTERNAL, "XMLFORMAT TODO: Not valid!");
	else
		osync_trace(TRACE_INTERNAL, "XMLFORMAT TODO: Valid!");

	vformat_free(vtodo);
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

osync_bool conv_xmlformat_to_vtodo10(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	return conv_xmlformat_to_vtodo(input, inpsize, output, outpsize, free_input, config, error, VFORMAT_TODO_10);
}


