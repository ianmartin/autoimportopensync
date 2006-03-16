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

/* Set connection types
 */
void parse_connection_type(char *str, gn_config *config) {

	if (!strcasecmp(str, "bluetooth"))
		config->connection_type = GN_CT_Bluetooth;
	else if (!strcasecmp(str, "irda"))
		config->connection_type = GN_CT_Irda;
	else if (!strcasecmp(str, "dku2"))
		config->connection_type = GN_CT_DKU2;
	else if (!strcasecmp(str, "dau9p"))
		config->connection_type = GN_CT_DAU9P;
	else if (!strcasecmp(str, "serial"))
		config->connection_type = GN_CT_Serial;
	else if (!strcasecmp(str, "infrared"))
		config->connection_type = GN_CT_Infrared;
	else if (!strcasecmp(str, "tekram"))
		config->connection_type = GN_CT_Tekram;
	else if (!strcasecmp(str, "tcp"))
		config->connection_type = GN_CT_TCP;
	else if (!strcasecmp(str, "m2bus"))
		config->connection_type = GN_CT_M2BUS;
}

/* Parse config file of gnokii plugin 
 *
 * Returns: bool
 * ReturnVal: true	on success
 * ReturnVal: false	on error
 */
osync_bool gnokii_config_parse(gn_config *config, char *data, int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p)", __func__, config, data, size, error);
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseMemory(data, size);

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

	while (cur != NULL) {
		char *str = (char*) xmlNodeGetContent(cur);
		
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *) "model"))
				strncpy(config->model, str, strlen(str));
	
			if (!xmlStrcmp(cur->name, (const xmlChar *) "port"))
				strncpy(config->port_device, str, strlen(str));

			if (!xmlStrcmp(cur->name, (const xmlChar *) "connection")) {
				// check for connection types which are supported by gnokii.
				parse_connection_type(str, config);
			}

			// rfcomm channel
			if (!xmlStrcmp(cur->name, (const xmlChar *) "rfcomm_channel")) {
				config->rfcomm_cn = atoi(str);
			}

			// check for debug option of libgnokii
			if (!xmlStrcmp(cur->name, (const xmlChar *) "debug")) {
				if (!strcasecmp(str, "on"))
					gn_log_debug_mask = GN_LOG_T_STDERR;	// debug output to stderr
			}


			xmlFree(str);
		}
		cur = cur->next;
	}

	if (!strlen(config->model)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Model is not set in configuration");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (!config->connection_type) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Connection type is not (correctly) set in configuration");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (!strlen(config->port_device)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Port (MAC address) is not set in configuration");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}


	xmlFreeDoc(doc);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/* Fill the statemachine struct for libgnokii with:
 * model, port (serial connection) or MAC address (bluetooth) and connection type.
 * This is required for the cellphone connection. 
 *
 */
void gnokii_config_state(struct gn_statemachine *state, gn_config *config) {

	/* model */
        strncpy(state->config.model, config->model, GN_MODEL_MAX_LENGTH);

	/* port (bluetooth: destination mac address) */
        strncpy(state->config.port_device, config->port_device, GN_DEVICE_NAME_MAX_LENGTH);

	/* connection type - gn_connection_type */
        state->config.connection_type = config->connection_type; 

	/* rfcomm channel */
	state->config.rfcomm_cn = config->rfcomm_cn;
	
}

