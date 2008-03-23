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

#ifndef _OPENSYNC_PLUGIN_CONNECTION_INTERANLS_H_
#define _OPENSYNC_PLUGIN_CONNECTION_INTERNALS_H_

/*! @brief Gives information about the current connection type 
 * 
 * @ingroup OSyncPluginConnectionPrivateAPI 
 **/
struct OSyncPluginConnection {
	/** Connection type */
	OSyncPluginConnectionType type;

	/** Bluetooth (MAC) Address */
	char *bt_address;
	/** Bluetooth SDP UUID */
	char *bt_sdpuuid;
	/** Bluetooth RFComm Channel */
	unsigned int bt_channel;

	/** USB Vendor ID */
	unsigned int usb_vendorid;
	/** USB Product ID */
	unsigned int usb_productid;
	/** USB Interface */
	unsigned int usb_interface;
	
	/** Network IP Address or Hostname */
	char *net_address;
	/** Network Port */
	unsigned int net_port;
	/** Network Protocol Suffix (e.g. http:// ssh://) */
	char *net_protocol;
	/** Network DNS-SD service type (e.g.: _syncml-http._tcp) */
	char *net_dnssd;

	/** Serial Port speed (e.g. 115200) */
	unsigned int serial_speed;
	/** Serial Device Node (e.g. /dev/ttyS0, /dev/ttyUSB0, ...) */
	char *serial_devicenode;

	/** IrDA Identifier (Service) String */
	char *irda_service;

	/** Object reference counting */
	int ref_count;
};

#endif /*_OPENSYNC_PLUGIN_CONNECTION_INTERNALS_H_*/

