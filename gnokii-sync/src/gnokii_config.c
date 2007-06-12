/***************************************************************************
 *   Copyright (C) 2006 by Daniel Gollub                                   *
 *                            <dgollub@suse.de>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "gnokii_sync.h"

/* Parse config file of gnokii plugin 
 *
 * Returns: bool
 * ReturnVal: true	on success
 * ReturnVal: false	on error
 */
osync_bool gnokii_config_parse(struct gn_statemachine *state, const char *data, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, state, data, error);
	int i = 0;
	char *str = NULL;
	char **lines = malloc(sizeof(char*) * 10);
	memset(lines, 0, sizeof(char*) * 10);
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseMemory(data, strlen(data) + 1);

	if (!doc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to parse settings");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	cur = xmlDocGetRootElement(doc);
	
	if (!cur) {
		xmlFreeDoc(doc);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get the xml root element of the config file");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (xmlStrcmp(cur->name, (xmlChar*) "config")) {
		xmlFreeDoc(doc);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Config is not valid");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	cur = cur->xmlChildrenNode;

	lines[i] = g_strdup("[global]");

	while (cur != NULL) {
		str = (char *) xmlNodeGetContent(cur);
		
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *) "model"))
				lines[++i] = g_strdup_printf("model = %s", str);
	
			if (!xmlStrcmp(cur->name, (const xmlChar *) "port"))
				lines[++i] = g_strdup_printf("port = %s", str);

			if (!xmlStrcmp(cur->name, (const xmlChar *) "connection"))
				lines[++i] = g_strdup_printf("connection = %s", str);

			// rfcomm channel
			if (!xmlStrcmp(cur->name, (const xmlChar *) "rfcomm_channel"))
				lines[++i] = g_strdup_printf("rfcomm_channel = %s", str);

			// check for debug option of libgnokii
			if (!xmlStrcmp(cur->name, (const xmlChar *) "debug")) {
				lines[++i] = g_strdup("[logging]");
				lines[++i] = g_strdup_printf("debug = %s", str);
			}
			g_free(str);
		}

		cur = cur->next;
	}

	/* TODO: adapt error handling to return value of gn_cfg_phone_load()
	if (!strlen(config->model)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Model is not set in configuration");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (GN_CT_NONE == config->connection_type) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Connection type is not (correctly) set in configuration");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (!strlen(config->port_device)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Port (MAC address) is not set in configuration");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	*/

	gn_cfg_memory_read(lines);
	gn_cfg_phone_load(NULL, state); 

	for (i=0; lines[i] != NULL; i++)
		g_free(lines[i]);

	g_free(lines);

	xmlFreeDoc(doc);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

