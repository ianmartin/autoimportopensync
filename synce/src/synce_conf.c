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
osync_bool synce_parse_settings (SyncePluginPtr *env, xmlDocPtr doc, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, doc);
	xmlNodePtr cur;
	
	// set defaults
	env->config_contacts = FALSE;
	env->config_calendar = FALSE;
	env->config_todos = FALSE;
	env->config_file = NULL;
	
	cur = xmlDocGetRootElement (doc);
	
	if (!cur) {
		osync_error_set (error, OSYNC_ERROR_GENERIC, "Unable to get root element of the settings");
		osync_trace (TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print (error));
		return FALSE;
	}
	
	if (strcmp (cur->name, "opensync-plugin-config") != 0) {
		osync_error_set (error, OSYNC_ERROR_GENERIC, "Config valid is not valid");
		osync_trace (TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print (error));
		return FALSE;
	}
	
	cur = cur->children;
	
	while (cur != NULL) {
		if (!strcmp (cur->name, "option")) {
			char *name, *type, *value;
			
			if ((name = xmlGetProp (cur, "name"))) {
				type = xmlGetProp (node, "type");
				if (type && !strcmp (type, "bool"))
					value = xmlGetProp (node, "value");
				else
					value = NULL;
				xmlFree (type);
				
				if (!strcmp (name, "addressbook")) {
					env->config_contacts = value && !strcmp (value, "true");
				} else if (!strcmp (name, "calendar")) {
					env->config_calendar = value && !strcmp (value, "true");
				} else if (!strcmp (name, "tasks")) {
					env->config_todos = value && !strcmp (value, "true");
				} else if (!strcmp (name, "file")) {
					env->config_file = xmlNodeGetContent (cur);
				}
				
				xmlFree (value);
				xmlFree (name);
			}
		}
		cur = cur->next;
	}
	
	if (env->config_contacts == 0 && env->config_calendar == 0
			&& env->config_todos == 0 && env->config_file == NULL) {
		osync_error_set (error, OSYNC_ERROR_GENERIC, "Nothing was configured");
		osync_trace (TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_trace (TRACE_EXIT, "%s", __func__);
	
	return TRUE;
}
