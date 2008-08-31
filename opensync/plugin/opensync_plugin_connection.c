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

#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-plugin.h"
#include "opensync_plugin_connection_internals.h"

OSyncPluginConnection *osync_plugin_connection_new(OSyncError **error)
{
	OSyncPluginConnection *connection = osync_try_malloc0(sizeof(OSyncPluginConnection), error);
	if (!connection)
		return NULL;

	connection->ref_count = 1;
	connection->type = OSYNC_PLUGIN_CONNECTION_UNKNOWN;

	return connection;
}

OSyncPluginConnection *osync_plugin_connection_ref(OSyncPluginConnection *connection)
{
	osync_assert(connection);
	
	g_atomic_int_inc(&(connection->ref_count));

	return connection;
}

void osync_plugin_connection_unref(OSyncPluginConnection *connection)
{
	osync_assert(connection);
	
	if (g_atomic_int_dec_and_test(&(connection->ref_count))) {
		if (connection->bt_address)
			g_free(connection->bt_address);

		if (connection->bt_sdpuuid)
			g_free(connection->bt_sdpuuid);

		if (connection->net_address)
			g_free(connection->net_address);

		if (connection->net_protocol)
			g_free(connection->net_protocol);

		if (connection->net_dnssd)
			g_free(connection->net_dnssd);

		if (connection->serial_devicenode)
			g_free(connection->serial_devicenode);

		if (connection->irda_service)
			g_free(connection->irda_service);

		g_free(connection);
	}
}

OSyncPluginConnectionType osync_plugin_connection_get_type(OSyncPluginConnection *connection)
{
	osync_assert(connection);

	return connection->type;
}

void osync_plugin_connection_set_type(OSyncPluginConnection *connection, OSyncPluginConnectionType type)
{
	osync_assert(connection);

	connection->type = type;
}

osync_bool osync_plugin_connection_is_supported(OSyncPluginConnection *connection, OSyncPluginConnectionSupportedFlag flag)
{
	osync_assert(connection);
	if (connection->supported & flag)
		return TRUE;

	return FALSE;
}

void osync_plugin_connection_set_supported(OSyncPluginConnection *connection, OSyncPluginConnectionSupportedFlags flags)
{
	osync_assert(connection);
	connection->supported = flags;
}

osync_bool osync_plugin_connection_option_is_supported(OSyncPluginConnection *connection, OSyncPluginConnectionOptionSupportedFlag flag)
{
	osync_assert(connection);
	if (connection->supported_options & flag)
		return TRUE;

	return FALSE;
}

void osync_plugin_connection_option_set_supported(OSyncPluginConnection *connection, OSyncPluginConnectionSupportedFlags flags)
{
	osync_assert(connection);
	connection->supported_options = flags;
}


const char *osync_plugin_connection_bt_get_addr(OSyncPluginConnection *connection)
{
	osync_assert(connection);

	return connection->bt_address;
}

void osync_plugin_connection_bt_set_addr(OSyncPluginConnection *connection, const char *address)
{
	osync_assert(connection);
	
	if (connection->bt_address)
		g_free(connection->bt_address);

	connection->bt_address = g_strdup(address);
}


unsigned int osync_plugin_connection_bt_get_channel(OSyncPluginConnection *connection)
{
	osync_assert(connection);

	return connection->bt_channel;
}

void osync_plugin_connection_bt_set_channel(OSyncPluginConnection *connection, unsigned int channel)
{
	osync_assert(connection);
	connection->bt_channel = channel;
}


const char *osync_plugin_connection_bt_get_sdpuuid(OSyncPluginConnection *connection)
{
	osync_assert(connection);
	
	return connection->bt_sdpuuid;
}

void osync_plugin_connection_bt_set_sdpuuid(OSyncPluginConnection *connection, const char *sdpuuid)
{
	osync_assert(connection);

	if (connection->bt_sdpuuid)
		g_free(connection->bt_sdpuuid);

	connection->bt_sdpuuid = g_strdup(sdpuuid);
}


unsigned int osync_plugin_connection_usb_get_vendorid(OSyncPluginConnection *connection)
{
	osync_assert(connection);

	return connection->usb_vendorid;
}

void osync_plugin_connection_usb_set_vendorid(OSyncPluginConnection *connection, unsigned int vendorid)
{
	osync_assert(connection);

	connection->usb_vendorid = vendorid;
}

unsigned int osync_plugin_connection_usb_get_productid(OSyncPluginConnection *connection)
{
	osync_assert(connection);

	return connection->usb_productid;
}

void osync_plugin_connection_usb_set_productid(OSyncPluginConnection *connection, unsigned int productid)
{
	osync_assert(connection);

	connection->usb_productid = productid;
}

unsigned int osync_plugin_connection_usb_get_interface(OSyncPluginConnection *connection)
{
	osync_assert(connection);

	return connection->usb_interface;
}

void osync_plugin_connection_usb_set_interface(OSyncPluginConnection *connection, unsigned int interface)
{
	osync_assert(connection);

	connection->usb_interface = interface;
}

const char *osync_plugin_connection_net_get_address(OSyncPluginConnection *connection)
{
	osync_assert(connection);

	return connection->net_address;
}

void osync_plugin_connection_net_set_address(OSyncPluginConnection *connection, const char *address)
{
	osync_assert(connection);

	if (connection->net_address)
		g_free(connection->net_address);

	connection->net_address = g_strdup(address);
}


unsigned int osync_plugin_connection_net_get_port(OSyncPluginConnection *connection)
{
	osync_assert(connection);

	return connection->net_port;
}

void osync_plugin_connection_net_set_port(OSyncPluginConnection *connection, unsigned int port)
{
	osync_assert(connection);

	connection->net_port = port;
}


const char *osync_plugin_connection_net_get_protocol(OSyncPluginConnection *connection)
{
	osync_assert(connection);

	return connection->net_protocol;
}

void osync_plugin_connection_net_set_protocol(OSyncPluginConnection *connection, const char *protocol)
{
	osync_assert(connection);

	if (connection->net_protocol)
		g_free(connection->net_protocol);

	connection->net_protocol = g_strdup(protocol);
}


const char *osync_plugin_connection_net_get_dnssd(OSyncPluginConnection *connection)
{
	osync_assert(connection);

	return connection->net_dnssd;
}

void osync_plugin_connection_net_set_dnssd(OSyncPluginConnection *connection, const char *dnssd)
{
	osync_assert(connection);

	if (connection->net_dnssd)
		g_free(connection->net_dnssd);

	connection->net_dnssd = g_strdup(dnssd);
}


unsigned int osync_plugin_connection_serial_get_speed(OSyncPluginConnection *connection)
{
	osync_assert(connection);

	return connection->serial_speed;
}

void osync_plugin_connection_serial_set_speed(OSyncPluginConnection *connection, unsigned int speed)
{
	osync_assert(connection);

	connection->serial_speed = speed;
}


const char *osync_plugin_connection_serial_get_devicenode(OSyncPluginConnection *connection)
{
	osync_assert(connection);

	return connection->serial_devicenode;
}

void osync_plugin_connection_serial_set_devicenode(OSyncPluginConnection *connection, const char *devicenode)
{
	osync_assert(connection);

	if (connection->serial_devicenode)
		g_free(connection->serial_devicenode);

	connection->serial_devicenode = g_strdup(devicenode);
}


const char *osync_plugin_connection_irda_get_service(OSyncPluginConnection *connection)
{
	osync_assert(connection);

	return connection->irda_service;
}

void osync_plugin_connection_irda_set_service(OSyncPluginConnection *connection, const char *irdaservice)
{
	osync_assert(connection);

	if (connection->irda_service)
		g_free(connection->irda_service);

	connection->irda_service = g_strdup(irdaservice);
}

