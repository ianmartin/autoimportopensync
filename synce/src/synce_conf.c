/*
 * file-sync - A plugin for the opensync framework
 *
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright © 2005 Danny Backx <dannybackx@users.sourceforge.net>
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
 
#include <opensync/opensync.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <glib.h>

#include "synce_plugin.h"

/*Load the state from a xml file and return it in the conn struct*/
osync_bool synce_parse_settings(plugin_environment *env, char *data, int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i)", __func__, env, data, size);
	xmlDocPtr doc;
	xmlNodePtr cur;

	//set defaults
	env->config_contacts = NULL;
	env->config_cal = NULL;
	env->config_todos = NULL;
	env->config_files = NULL;

	doc = xmlParseMemory(data, size);

	if (!doc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to parse settings");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	cur = xmlDocGetRootElement(doc);

	if (!cur) {
		xmlFreeDoc(doc);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get root element of the settings");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (xmlStrcmp(cur->name, (xmlChar*)"config")) {
		xmlFreeDoc(doc);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Config valid is not valid");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		char *str = (char*)xmlNodeGetContent(cur);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"contacts")) {
				env->config_contacts = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"files")) {
				env->config_files = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"cal")) {
				env->config_cal = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"todos")) {
				env->config_todos = g_strdup(str);
			}
			xmlFree(str);
		}
		cur = cur->next;
	}
	
	if (env->config_contacts == NULL
			&& env->config_cal == NULL
			&& env->config_todos == NULL
			&& env->config_files == NULL) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Nothing was configured");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	xmlFreeDoc(doc);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}
