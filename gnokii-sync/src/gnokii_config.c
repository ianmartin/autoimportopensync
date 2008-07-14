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

#include "gnokii_config.h"

/* Parse config file of gnokii plugin 
 *
 * Returns: bool
 * ReturnVal: true	on success
 * ReturnVal: false	on error
 */
osync_bool gnokii_config_parse(gnokii_environment *env, OSyncPluginConfig *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, config, error);
	int i = 0;
	OSyncPluginConnection *plugin_connection = NULL; 
	char **lines = malloc(sizeof(char*) * 10);
	memset(lines, 0, sizeof(char*) * 10);

	const char *port = NULL;
	const char *connection = NULL;

	/* By default everything is enabled. */
	env->contact_memory_sm = TRUE;
	env->contact_memory_me = TRUE;

	lines[i] = g_strdup("[global]");

	/* TODO: Advanced config reimplemtation: memory type */
	/*
	if (!xmlStrcmp(cur->name, (const xmlChar *) "contact_memory")) {
		xmlNodePtr memnode = cur->children;

		if (!memnode)
			break;

		// If contact_memory is used we disable everything first
		env->contact_memory_sm = FALSE;
		env->contact_memory_me = FALSE;

		for (;memnode; memnode = memnode->next) {

			if (xmlStrcmp(memnode->name, BAD_CAST "memory")) {
				continue;
			}

			// xmlNodeGetContent(cur->children-> next->next->next->children)
			str = xmlNodeGetContent(memnode->children);

			if (!xmlStrcmp(str, BAD_CAST "SM"))
				env->contact_memory_sm = TRUE;
			else if (!xmlStrcmp(str, BAD_CAST "ME"))
				env->contact_memory_me = TRUE;

			xmlFree(str);
		}
	}
	*/

	/* Connection Type */
	plugin_connection = osync_plugin_config_get_connection(config);
	if (!plugin_connection) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "No connection configuration for Plugin.");
		goto error;
	}

	switch (osync_plugin_connection_get_type(plugin_connection)) {
		case OSYNC_PLUGIN_CONNECTION_BLUETOOTH:
			port = osync_plugin_connection_bt_get_addr(plugin_connection);
			connection = "bluetooth";
			if (osync_plugin_connection_bt_get_channel(plugin_connection))
				lines[++i] = g_strdup_printf("rfcomm_channel = %u", 
					osync_plugin_connection_bt_get_channel(plugin_connection));
			break;
		case OSYNC_PLUGIN_CONNECTION_USB:
			connection = "dku2libusb";
			/* Dummy value for dku2libsusb - TODO: check if this is still needed?! */
			port = "dummy";
			break;
		case OSYNC_PLUGIN_CONNECTION_SERIAL:
			connection = "serial";
			port = osync_plugin_connection_serial_get_devicenode(plugin_connection);
			if (!osync_plugin_connection_serial_get_speed(plugin_connection)) {
				osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "No serial baudrate/speed set.");
				goto error;
			}

			lines[++i] = g_strdup_printf("serial_baudrate = %u", 
					osync_plugin_connection_serial_get_speed(plugin_connection));


			break;
		case OSYNC_PLUGIN_CONNECTION_IRDA:
			connection = "irda";
			if (!osync_plugin_connection_irda_get_service(plugin_connection)) {
				osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "No irda service string configured.");
				goto error;
			}

			lines[++i] = g_strdup_printf("irda_string = %s", 
					osync_plugin_connection_irda_get_service(plugin_connection));

			break;
		case OSYNC_PLUGIN_CONNECTION_NETWORK:
			connection = "tcp";
			lines[++i] = g_strdup_printf("port = %s:%u",
					osync_plugin_connection_net_get_address(plugin_connection),
					osync_plugin_connection_net_get_port(plugin_connection));
			break;
		case OSYNC_PLUGIN_CONNECTION_UNKNOWN:
		default:
			osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unknown/Unsupported connection type is configured.");
			goto error;
			break;
	}

	if (port)
		lines[++i] = g_strdup_printf("port = %s", port);

	if (connection)
		lines[++i] = g_strdup_printf("connection = %s", connection);

	/* Get MODEL information from Version/Discovery and map to GNOKII type */
	lines[++i] = g_strdup_printf("model = %s", "6230i");
	


	/* TODO: reimplement advanaced config: check for debug option of libgnokii */
	/*
		if (!xmlStrcmp(cur->name, (const xmlChar *) "debug")) {
			lines[++i] = g_strdup("[logging]");
			lines[++i] = g_strdup_printf("debug = %s", str);
		}
	*/

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
	gn_cfg_phone_load(NULL, env->state); 

	for (i=0; lines[i] != NULL; i++)
		g_free(lines[i]);

	g_free(lines);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	for (i=0; lines[i] != NULL; i++)
		g_free(lines[i]);

	g_free(lines);


	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

