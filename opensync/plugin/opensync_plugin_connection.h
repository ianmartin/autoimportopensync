/*
 * libopensync - A synchronization framework
 * Copyright (C) 2008  Daniel Gollub <dgollub@suse.de>
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

#ifndef _OPENSYNC_PLUGIN_CONNECTION_H_
#define _OPENSYNC_PLUGIN_CONNECTION_H_

/*! @brief Gives information about the current connection type 
 * 
 * @ingroup OSyncPluginConnectionAPI 
 **/
typedef enum {
	/** Unknown */
	OSYNC_PLUGIN_CONNECTION_UNKNOWN = 0,
	/** Bluetooth */
	OSYNC_PLUGIN_CONNECTION_BLUETOOTH,
	/** USB */
	OSYNC_PLUGIN_CONNECTION_USB,
	/** Network */ 
	OSYNC_PLUGIN_CONNECTION_NETWORK, 
	/** Serial */
	OSYNC_PLUGIN_CONNECTION_SERIAL,
	/** IrDA */
	OSYNC_PLUGIN_CONNECTION_IRDA
} OSyncPluginConnectionType;

OSYNC_EXPORT OSyncPluginConnection *osync_plugin_connection_new(OSyncError **error);
OSYNC_EXPORT void osync_plugin_connection_unref(OSyncPluginConnection *connection);
OSYNC_EXPORT OSyncPluginConnection *osync_plugin_connection_ref(OSyncPluginConnection *connection);

/* Bluetooth */
OSYNC_EXPORT const char *osync_plugin_connection_bt_get_addr(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_bt_set_addr(OSyncPluginConnection *connection, const char *address);

OSYNC_EXPORT unsigned int osync_plugin_connection_bt_get_channel(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_bt_set_channel(OSyncPluginConnection *connection, unsigned int channel);

OSYNC_EXPORT const char *osync_plugin_connection_bt_get_sdpuuid(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_bt_set_sdpuuid(OSyncPluginConnection *connection, const char *sdpuuid);

/* USB */
OSYNC_EXPORT unsigned int osync_plugin_connection_usb_get_vendorid(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_usb_set_vendorid(OSyncPluginConnection *connection, unsigned int vendorid);

OSYNC_EXPORT unsigned int osync_plugin_connection_usb_get_productid(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_usb_set_productid(OSyncPluginConnection *connection, unsigned int productid);

/* Network */
OSYNC_EXPORT const char *osync_plugin_connection_net_get_address(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_net_set_address(OSyncPluginConnection *connection, const char *address);

OSYNC_EXPORT unsigned int osync_plugin_connection_net_get_port(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_net_set_port(OSyncPluginConnection *connection, unsigned int port);

OSYNC_EXPORT const char *osync_plugin_connection_net_get_protocol(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_net_set_protocol(OSyncPluginConnection *connection, const char *protocol);

OSYNC_EXPORT const char *osync_plugin_connection_net_get_dnssd(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_net_set_dnssd(OSyncPluginConnection *connection, const char *dnssd);

/* Serial */
OSYNC_EXPORT unsigned int osync_plugin_connection_serial_get_speed(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_serial_set_speed(OSyncPluginConnection *connection, unsigned int speed);

OSYNC_EXPORT const char *osync_plugin_connection_serial_get_devicenode(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_serial_set_devicenode(OSyncPluginConnection *connection, const char *devicenode);

/* IrDA */
OSYNC_EXPORT const char *osync_plugin_connection_irda_get_service(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_irda_set_service(OSyncPluginConnection *connection, const char *irdaservice);

#endif /*_OPENSYNC_PLUGIN_CONNECTION_H_*/

