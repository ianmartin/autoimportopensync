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
	OSYNC_PLUGIN_CONNECTION_UNKNOWN		= 0,
	/** Bluetooth */
	OSYNC_PLUGIN_CONNECTION_BLUETOOTH	= (1 << 0),
	/** USB */
	OSYNC_PLUGIN_CONNECTION_USB		= (1 << 1),
	/** Network */ 
	OSYNC_PLUGIN_CONNECTION_NETWORK		= (1 << 2), 
	/** Serial */
	OSYNC_PLUGIN_CONNECTION_SERIAL		= (1 << 3),
	/** IrDA */
	OSYNC_PLUGIN_CONNECTION_IRDA		= (1 << 4)
} OSyncPluginConnectionType;

typedef OSyncPluginConnectionType OSyncPluginConnectionSupportedFlag;
typedef unsigned int OSyncPluginConnectionSupportedFlags;

typedef enum {
	/* Bluetooth */
	OSYNC_PLUGIN_CONNECTION_BLUETOOTH_ADDRESS	= (1 << 0),
	OSYNC_PLUGIN_CONNECTION_BLUETOOTH_RFCOMM	= (1 << 1),
	OSYNC_PLUGIN_CONNECTION_BLUETOOTH_SDPUUID	= (1 << 2),
	/* USB */
	OSYNC_PLUGIN_CONNECTION_USB_VENDORID		= (1 << 3),
	OSYNC_PLUGIN_CONNECTION_USB_PRODUCTID		= (1 << 4),
	OSYNC_PLUGIN_CONNECTION_USB_INTERFACE		= (1 << 5),
	/* Network */
	OSYNC_PLUGIN_CONNECTION_NETWORK_ADDRESS		= (1 << 6),
	OSYNC_PLUGIN_CONNECTION_NETWORK_PORT		= (1 << 7),
	OSYNC_PLUGIN_CONNECTION_NETWORK_PROTOCOL	= (1 << 8),
	OSYNC_PLUGIN_CONNECTION_NETWORK_DNSSD		= (1 << 9),
	/* Serial */
	OSYNC_PLUGIN_CONNECTION_SERIAL_SPEED		= (1 << 10),
	OSYNC_PLUGIN_CONNECTION_SERIAL_DEVICENODE	= (1 << 11),
	/* IrDA */
	OSYNC_PLUGIN_CONNECTION_IRDA_SERVICE		= (1 << 12)
} OSyncPluginConnectionOptionSupportedFlag;

typedef unsigned int OSyncPluginConnectionOptionSupportedFlags;

OSYNC_EXPORT OSyncPluginConnection *osync_plugin_connection_new(OSyncError **error);
OSYNC_EXPORT void osync_plugin_connection_unref(OSyncPluginConnection *connection);
OSYNC_EXPORT OSyncPluginConnection *osync_plugin_connection_ref(OSyncPluginConnection *connection);

OSYNC_EXPORT OSyncPluginConnectionType osync_plugin_connection_get_type(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_set_type(OSyncPluginConnection *connection, OSyncPluginConnectionType type);

OSYNC_EXPORT osync_bool osync_plugin_connection_is_supported(OSyncPluginConnection *connection, OSyncPluginConnectionSupportedFlag flag);
OSYNC_EXPORT void osync_plugin_connection_set_supported(OSyncPluginConnection *connection, OSyncPluginConnectionSupportedFlags flags);

OSYNC_EXPORT osync_bool osync_plugin_connection_option_is_supported(OSyncPluginConnection *connection, OSyncPluginConnectionOptionSupportedFlag flag);
OSYNC_EXPORT void osync_plugin_connection_option_set_supported(OSyncPluginConnection *connection, OSyncPluginConnectionOptionSupportedFlags flags);

/* Bluetooth */
OSYNC_EXPORT const char *osync_plugin_connection_bt_get_addr(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_bt_set_addr(OSyncPluginConnection *connection, const char *address);

OSYNC_EXPORT unsigned int osync_plugin_connection_bt_get_channel(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_bt_set_channel(OSyncPluginConnection *connection, unsigned int channel);

OSYNC_EXPORT const char *osync_plugin_connection_bt_get_sdpuuid(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_bt_set_sdpuuid(OSyncPluginConnection *connection, const char *sdpuuid);

/* USB */
OSYNC_EXPORT const char *osync_plugin_connection_usb_get_vendorid(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_usb_set_vendorid(OSyncPluginConnection *connection, const char *vendorid);

OSYNC_EXPORT const char *osync_plugin_connection_usb_get_productid(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_usb_set_productid(OSyncPluginConnection *connection, const char *productid);

OSYNC_EXPORT unsigned int osync_plugin_connection_usb_get_interface(OSyncPluginConnection *connection);
OSYNC_EXPORT void osync_plugin_connection_usb_set_interface(OSyncPluginConnection *connection, unsigned int interf);

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

