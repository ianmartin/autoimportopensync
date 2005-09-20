/*
 * Configuration file reading module for the SynCE plugin to OpenSync.
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
osync_bool synce_parse_settings(SyncePluginPtr *env, char *data, int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i)", __func__, env, data, size);
	xmlDocPtr doc;
	xmlNodePtr cur;

	//set defaults
	env->config_contacts = FALSE;
	env->config_calendar = FALSE;
	env->config_todos = FALSE;
	env->config_file = NULL;

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
			if (!xmlStrcmp(cur->name, (const xmlChar *)"contact")) {
				/* Disable by mentioning NO or FALSE, otherwise enable. */
				env->config_contacts = TRUE;
				if (g_ascii_strcasecmp(str, "FALSE") == 0)
					env->config_contacts = FALSE;
				if (g_ascii_strcasecmp(str, "NO") == 0)
					env->config_contacts = FALSE;
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"file")) {
				env->config_file = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"calendar")) {
				/* Disable by mentioning NO or FALSE, otherwise enable. */
				env->config_calendar = TRUE;
				if (g_ascii_strcasecmp(str, "FALSE") == 0)
					env->config_calendar = FALSE;
				if (g_ascii_strcasecmp(str, "NO") == 0)
					env->config_calendar = FALSE;
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"todos")) {
				/* Disable by mentioning NO or FALSE, otherwise enable. */
				env->config_todos = TRUE;
				if (g_ascii_strcasecmp(str, "FALSE") == 0)
					env->config_todos = FALSE;
				if (g_ascii_strcasecmp(str, "NO") == 0)
					env->config_todos = FALSE;
			}
			xmlFree(str);
		}
		cur = cur->next;
	}

	/* This belongs in XXX_connect()
	 * if (!osync_member_objtype_enabled(env->member, "contact"))
	 * 	env->config_contacts = FALSE;
	 * if (!osync_member_objtype_enabled(env->member, "todos"))
	 * 	env->config_todos = FALSE;
	 * if (!osync_member_objtype_enabled(env->member, "calendar"))
	 * 	env->config_calendar = FALSE;
	 */

	if (env->config_contacts == 0 && env->config_calendar == 0
			&& env->config_todos == 0 && env->config_file == NULL) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Nothing was configured");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	xmlFreeDoc(doc);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}
