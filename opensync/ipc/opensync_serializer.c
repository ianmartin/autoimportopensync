/*
 * libosengine - A synchronization engine for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
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

#include "opensync_serializer.h"
#include "opensync_message.h"

#include "opensync-data.h"
#include "opensync-format.h"
#include "opensync-plugin.h"

osync_bool osync_marshal_data(OSyncMessage *message, OSyncData *data, OSyncError **error)
{
	osync_assert(message);
	osync_assert(data);

	/* Order:
	 * 
	 * format
	 * objtype
	 * size
	 * data */

	osync_assert(message);
	osync_assert(data);
	
	/* Find the format */
	OSyncObjFormat *objformat = osync_data_get_objformat(data);
	
	/* Write the format and objtype first */
	osync_message_write_string(message, osync_objformat_get_name(objformat));
	osync_message_write_string(message, osync_data_get_objtype(data));

	/* Now we get the pointer to the data */
	char *input_data = NULL;
	unsigned int input_size = 0;
	osync_data_get_data(data, &input_data, &input_size);
	
	if (input_size > 0) {
		osync_message_write_int(message, 1);
		
		/* If the format must be marshalled, we call the marshal function
		 * and the send the marshalled data. Otherwise we send the unmarshalled data */
		if (osync_objformat_must_marshal(objformat) == TRUE) {
			if (!osync_objformat_marshal(objformat, input_data, input_size, message, error))
				goto error;
		} else {
			/* If the format is a plain format, then we have to add
			 * one byte for \0 to the input_size. This extra byte will
			 * be removed by the osync_demarshal_data funciton.
			 */
			input_size++;
			osync_message_write_buffer(message, input_data, input_size);
		}
	} else {
		osync_message_write_int(message, 0);
	}
	
	return TRUE;

error:
	return FALSE;
}

osync_bool osync_demarshal_data(OSyncMessage *message, OSyncData **data, OSyncFormatEnv *env, OSyncError **error)
{
	osync_assert(message);
	osync_assert(env);

	/* Order:
	 * 
	 * format
	 * objtype
	 * size
	 * data */
	
	/* Get the objtype and format */
	char *objformat = NULL;
	char *objtype = NULL;
	osync_message_read_string(message, &objformat);
	osync_message_read_string(message, &objtype);
	
	/* Search for the format */
	OSyncObjFormat *format = osync_format_env_find_objformat(env, objformat);
	if (!format) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find objformat %s", objformat);
		goto error;
	}

	unsigned int input_size = 0;
	char *input_data = NULL;
	
	int has_data = 0;
	osync_message_read_int(message, &has_data);
	
	if (has_data) {
		if (osync_objformat_must_marshal(format) == TRUE) {
			if (!osync_objformat_demarshal(format, message, &input_data, &input_size, error))
				goto error;
		} else {
			osync_message_read_buffer(message, (void *)&input_data, (int *)&input_size);

                        /* If the format is a plain, then we have to remove
                         * one from the input_size, since once one was added by 
                         * osync_marshall_data() for trailing newline.
                         */
			input_size--;
		}
	}
	
	osync_trace(TRACE_INTERNAL, "Data is: %p, %i", input_data, input_size);
	
	*data = osync_data_new(input_data, input_size, format, error);
	if (!*data)
		goto error;
	
	osync_data_set_objtype(*data, objtype);
	g_free(objtype);
	g_free(objformat);
	
	return TRUE;

error:
	g_free(objformat);
	g_free(objtype);
	return FALSE;
}

osync_bool osync_marshal_change(OSyncMessage *message, OSyncChange *change, OSyncError **error)
{
	osync_assert(message);
	osync_assert(change);

	/* Order:
	 * 
	 * uid
	 * hash
	 * changetype
	 * data */
	
	osync_message_write_string(message, osync_change_get_uid(change));
	osync_message_write_string(message, osync_change_get_hash(change));
	osync_message_write_int(message, osync_change_get_changetype(change));
	
	OSyncData *data = osync_change_get_data(change);
	if (!osync_marshal_data(message, data, error))
		goto error;
	
	return TRUE;

error:
	return FALSE;
}

osync_bool osync_demarshal_change(OSyncMessage *message, OSyncChange **change, OSyncFormatEnv *env, OSyncError **error)
{
	osync_assert(message);
	osync_assert(env);

	/* Order:
	 * 
	 * uid
	 * hash
	 * changetype
	 * data */
	
	*change = osync_change_new(error);
	if (!*change)
		goto error;

	char *uid = NULL;
	char *hash = NULL;
	int change_type = OSYNC_CHANGE_TYPE_UNKNOWN;
	
 	osync_message_read_string(message, &uid);
	osync_message_read_string(message, &hash);
	osync_message_read_int(message, &change_type);

	OSyncData *data = NULL;
	if (!osync_demarshal_data(message, &data, env, error))
		goto error_free_change;
	
	osync_change_set_uid(*change, uid);
	g_free(uid);
	
	osync_change_set_hash(*change, hash);
	g_free(hash);
	
	osync_change_set_changetype(*change, change_type);
	osync_change_set_data(*change, data);
	osync_data_unref(data);

	return TRUE;

error_free_change:
	g_free(uid);
	g_free(hash);
	osync_change_unref(*change);
error:
	return FALSE;
}

osync_bool osync_marshal_objformat_sink(OSyncMessage *message, OSyncObjFormatSink *sink, OSyncError **error)
{
	/* Order:
	 * 
	 * objformat name
	 * objformat sink config
	 */

	const char *objformat_name = osync_objformat_sink_get_objformat(sink);
	const char *objformat_sink_config = osync_objformat_sink_get_config(sink);

	osync_message_write_string(message, objformat_name); 
	osync_message_write_string(message, objformat_sink_config); 
	
	return TRUE;
}

osync_bool osync_demarshal_objformat_sink(OSyncMessage *message, OSyncObjFormatSink **sink, OSyncError **error)
{
	osync_assert(message);

	/* Order:
	 * 
	 * objformat name
	 * objformat sink config
	 */
	
	/* Get the objtype and format */
	char *objformat_name = NULL;
	char *objformat_sink_config = NULL;
	osync_message_read_string(message, &objformat_name);
	
	*sink = osync_objformat_sink_new(objformat_name, error);
	if (!*sink)
		goto error;

	osync_message_read_string(message, &objformat_sink_config);
	osync_objformat_sink_set_config(*sink, objformat_sink_config);
	g_free(objformat_sink_config);

	return TRUE;

error:
	osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_marshal_objtype_sink(OSyncMessage *message, OSyncObjTypeSink *sink, OSyncError **error)
{
	osync_assert(message);
	osync_assert(sink);

	/* Order:
	 * 
	 * name
	 * read function (bool)
	 * get_changes function (bool)
	 * write function (bool)
	 * number of format sinks 
	 * format sink list (format sinks)
	 * enabled (int)
	 * timeout connect (int)
	 * timeout disconnect (int)
	 * timeout get_changes (int)
	 * timeout commit (int)
	 * timeout batch_commit (int)
	 * timeout committed_all (int)
	 * timeout sync_done (int)
	 * timeout read (int)
	 * timeout write (int)
	 * 
	 */
	
	int i = 0;
	unsigned int num = osync_objtype_sink_num_objformat_sinks(sink);
	osync_message_write_string(message, osync_objtype_sink_get_name(sink));

	osync_message_write_int(message, osync_objtype_sink_get_function_read(sink));
	osync_message_write_int(message, osync_objtype_sink_get_function_getchanges(sink));
	osync_message_write_int(message, osync_objtype_sink_get_function_write(sink));

	osync_message_write_int(message, num);
	for (i = 0; i < num; i++) {
		OSyncObjFormatSink *formatsink = osync_objtype_sink_nth_objformat_sink(sink, i);
		if (!osync_marshal_objformat_sink(message, formatsink, error)) 
			goto error;
	}
	
	/* enabled */
	osync_message_write_int(message, osync_objtype_sink_is_enabled(sink));

	/* timeouts */
	osync_message_write_int(message, osync_objtype_sink_get_connect_timeout(sink));
	osync_message_write_int(message, osync_objtype_sink_get_disconnect_timeout(sink));

	osync_message_write_int(message, osync_objtype_sink_get_getchanges_timeout(sink));
	osync_message_write_int(message, osync_objtype_sink_get_commit_timeout(sink));
	osync_message_write_int(message, osync_objtype_sink_get_batchcommit_timeout(sink));
	osync_message_write_int(message, osync_objtype_sink_get_committedall_timeout(sink));
	osync_message_write_int(message, osync_objtype_sink_get_syncdone_timeout(sink));

	osync_message_write_int(message, osync_objtype_sink_get_read_timeout(sink));
	osync_message_write_int(message, osync_objtype_sink_get_write_timeout(sink));

	
	return TRUE;

error:
	return FALSE;
}

osync_bool osync_demarshal_objtype_sink(OSyncMessage *message, OSyncObjTypeSink **sink, OSyncError **error)
{
	osync_assert(message);

	/* Order:
	 * 
	 * name
	 * read function (bool)
	 * get_changes function (bool)
	 * write function (bool)
	 * number of format sinks 
	 * format sink list (format sinks)
	 * enabled (int)
	 * timeout connect (int)
	 * timeout disconnect (int)
	 * timeout get_changes (int)
	 * timeout commit (int)
	 * timeout batch_commit (int)
	 * timeout committed_all (int)
	 * timeout sync_done (int)
	 * timeout read (int)
	 * timeout write (int)
	 * 
	 */
	
	*sink = osync_objtype_sink_new(NULL, error);
	if (!*sink)
		goto error;

	char *name = NULL;
	int num_formats = 0;
	int enabled = 0, timeout = 0;
	int read = 0, get_changes = 0, write = 0;
	
 	osync_message_read_string(message, &name);
 	osync_objtype_sink_set_name(*sink, name);
 	g_free(name);
 	
	osync_message_read_int(message, &read);
	osync_objtype_sink_set_function_read(*sink, read);

	osync_message_read_int(message, &get_changes);
	osync_objtype_sink_set_function_getchanges(*sink, get_changes);

	osync_message_read_int(message, &write);
	osync_objtype_sink_set_function_write(*sink, write);

	osync_message_read_int(message, &num_formats);
	int i = 0;
	for (i = 0; i < num_formats; i++) {
		OSyncObjFormatSink *formatsink;
		if (!osync_demarshal_objformat_sink(message, &formatsink, error)) 
			goto error;

		osync_objtype_sink_add_objformat_sink(*sink, formatsink);
		osync_objformat_sink_unref(formatsink);
	}

	/* enabled */
	osync_message_read_int(message, &enabled);
	osync_objtype_sink_set_enabled(*sink, enabled);

	/* timeouts */
	osync_message_read_int(message, &timeout);
	osync_objtype_sink_set_connect_timeout(*sink, timeout);

	osync_message_read_int(message, &timeout);
	osync_objtype_sink_set_disconnect_timeout(*sink, timeout);

	osync_message_read_int(message, &timeout);
	osync_objtype_sink_set_getchanges_timeout(*sink, timeout);

	osync_message_read_int(message, &timeout);
	osync_objtype_sink_set_commit_timeout(*sink, timeout);

	osync_message_read_int(message, &timeout);
	osync_objtype_sink_set_batchcommit_timeout(*sink, timeout);

	osync_message_read_int(message, &timeout);
	osync_objtype_sink_set_committedall_timeout(*sink, timeout);

	osync_message_read_int(message, &timeout);
	osync_objtype_sink_set_syncdone_timeout(*sink, timeout);

	osync_message_read_int(message, &timeout);
	osync_objtype_sink_set_read_timeout(*sink, timeout);

	osync_message_read_int(message, &timeout);
	osync_objtype_sink_set_write_timeout(*sink, timeout);

	return TRUE;

error:
	return FALSE;
}

void osync_marshal_error(OSyncMessage *message, OSyncError *error)
{
	osync_assert(message);

	if (error) {
		osync_message_write_int(message, 1);
		osync_message_write_int(message, osync_error_get_type(&error));
		const char *msg = osync_error_print(&error);
		osync_message_write_string(message, msg);
	} else {
		osync_message_write_int(message, 0);
	}
}

void osync_demarshal_error(OSyncMessage *message, OSyncError **error)
{
	osync_assert(message);

	int hasError = 0;

	osync_message_read_int(message, &hasError);
	
	if (hasError) {
		char *msg = NULL;
		int error_type = OSYNC_NO_ERROR;
		
		osync_message_read_int(message, &error_type);
		osync_message_read_string(message, &msg);
		
		osync_error_set(error, (OSyncErrorType)error_type, msg);
		g_free(msg);
	}
}

osync_bool osync_marshal_pluginconnection(OSyncMessage *message, OSyncPluginConnection *conn, OSyncError **error)
{
	osync_assert(message);
	osync_assert(conn);

	/* Order:
	 * 
	 * type (int)
	 *
	 * (following are depending on type)
	 *
	 * bt_address (char*)
	 * bt_sdpuuid (char*)
	 * bt_channel (uint)
	 *
	 * usb_vendorid (uint)
	 * usb_productid (uint)
	 * usb_interface (uint)
	 *
	 * net_address (char*)
	 * net_port (uint)
	 * net_protocol (char*)
	 * net_dnssd (char*)
	 *
	 * serial_speed (uint)
	 * serial_devicenode (char*)
	 * 
	 * irda_service (char *)
	 */

	OSyncPluginConnectionType type = osync_plugin_connection_get_type(conn);
	osync_message_write_int(message, type);
	switch(type) {
		case OSYNC_PLUGIN_CONNECTION_BLUETOOTH:
			osync_message_write_string(message, osync_plugin_connection_bt_get_addr(conn));
			osync_message_write_string(message, osync_plugin_connection_bt_get_sdpuuid(conn));
			osync_message_write_uint(message, osync_plugin_connection_bt_get_channel(conn));
			break;
		case OSYNC_PLUGIN_CONNECTION_USB:
			osync_message_write_uint(message, osync_plugin_connection_usb_get_vendorid(conn));
			osync_message_write_uint(message, osync_plugin_connection_usb_get_productid(conn));
			osync_message_write_uint(message, osync_plugin_connection_usb_get_interface(conn));
			break;
		case OSYNC_PLUGIN_CONNECTION_NETWORK:
			osync_message_write_string(message, osync_plugin_connection_net_get_address(conn));
			osync_message_write_uint(message, osync_plugin_connection_net_get_port(conn));
			osync_message_write_string(message, osync_plugin_connection_net_get_protocol(conn));
			osync_message_write_string(message, osync_plugin_connection_net_get_dnssd(conn));
			break;
		case OSYNC_PLUGIN_CONNECTION_SERIAL:
			osync_message_write_uint(message, osync_plugin_connection_serial_get_speed(conn));
			osync_message_write_string(message, osync_plugin_connection_serial_get_devicenode(conn));
			break;
		case OSYNC_PLUGIN_CONNECTION_IRDA:
			osync_message_write_string(message, osync_plugin_connection_irda_get_service(conn));
			break;
		case OSYNC_PLUGIN_CONNECTION_UNKNOWN:
			break;
	}

	
	return TRUE;
}

osync_bool osync_demarshal_pluginconnection(OSyncMessage *message, OSyncPluginConnection **conn, OSyncError **error)
{
	/* Order:
	 *
	 * type (int)
	 * 
	 * (following are depending on type)
	 *
	 * bt_address (char*)
	 * bt_sdpuuid (char*)
	 * bt_channel (uint)
	 *
	 * usb_vendorid (uint)
	 * usb_productid (uint)
	 * usb_interface (uint)
	 *
	 * net_address (char*)
	 * net_port (uint)
	 * net_protocol (char*)
	 * net_dnssd (char*)
	 *
	 * serial_speed (uint)
	 * serial_devicenode (char*)
	 * 
	 * irda_service (char *)
	 */
	
	int type; 

	char *bt_address, *bt_sdpuuid;
	unsigned int bt_channel;

	unsigned int usb_vendorid, usb_productid, usb_interface; 

	char *net_address, *net_protocol, *net_dnssd;

	unsigned int serial_speed;
	char *serial_devicenode;

	char *irda_service;

	osync_message_read_int(message, &type);

	*conn = osync_plugin_connection_new(type, error);
	if (!*conn)
		goto error;

	switch(type) {
		case OSYNC_PLUGIN_CONNECTION_BLUETOOTH:
			osync_message_read_string(message, &bt_address);
			osync_plugin_connection_bt_set_addr(*conn, bt_address);

			osync_message_read_string(message, &bt_sdpuuid);
			osync_plugin_connection_bt_set_sdpuuid(*conn, bt_sdpuuid);

			osync_message_read_uint(message, &bt_channel);
			osync_plugin_connection_bt_set_channel(*conn, bt_channel);

			g_free(bt_address);
			g_free(bt_sdpuuid);
			break;
		case OSYNC_PLUGIN_CONNECTION_USB:
			osync_message_read_uint(message, &usb_vendorid);
			osync_plugin_connection_usb_set_vendorid(*conn, usb_vendorid);

			osync_message_read_uint(message, &usb_productid);
			osync_plugin_connection_usb_set_productid(*conn, usb_productid);

			osync_message_read_uint(message, &usb_interface);
			osync_plugin_connection_usb_set_productid(*conn, usb_interface);
			break;
		case OSYNC_PLUGIN_CONNECTION_NETWORK:
			osync_message_read_string(message, &net_address);
			osync_plugin_connection_net_set_address(*conn, net_address);

			osync_message_read_string(message, &net_protocol);
			osync_plugin_connection_net_set_address(*conn, net_protocol);

			osync_message_read_string(message, &net_dnssd);
			osync_plugin_connection_net_set_address(*conn, net_dnssd);
			
			g_free(net_address);
			g_free(net_protocol);
			g_free(net_dnssd);
			break;
		case OSYNC_PLUGIN_CONNECTION_SERIAL:
			osync_message_read_uint(message, &serial_speed);
			osync_plugin_connection_serial_set_speed(*conn, serial_speed);

			osync_message_read_string(message, &serial_devicenode);
			osync_plugin_connection_serial_set_devicenode(*conn, serial_devicenode);

			g_free(serial_devicenode);
			break;
		case OSYNC_PLUGIN_CONNECTION_IRDA:
			osync_message_read_string(message, &irda_service);
			osync_plugin_connection_serial_set_devicenode(*conn, irda_service);

			g_free(irda_service);
			break;
		case OSYNC_PLUGIN_CONNECTION_UNKNOWN:
			break;
	}

	return TRUE;

error:
	return FALSE;
}

#define MARSHAL_OBJFORMATSINK_CONFIG (0x1 << 1)

osync_bool osync_marshal_objformatsink(OSyncMessage *message, OSyncObjFormatSink *sink, OSyncError **error)
{
	osync_assert(message);
	osync_assert(sink);

	/* Order:
	 *
	 * name (string)
	 *
	 * available_settings (uint)
	 * 
	 * (optional)
	 * config (string)
	 */

	unsigned int available_settings = 0;
	const char *config = osync_objformat_sink_get_config(sink);
	const char *name = osync_objformat_sink_get_objformat(sink);

	osync_assert(name);
	osync_message_write_string(message, name);
	
	if (config)
		available_settings |= MARSHAL_OBJFORMATSINK_CONFIG;

	osync_message_write_uint(message, available_settings);

	if (config)
		osync_message_write_string(message, config); 

	return TRUE;
}

osync_bool osync_demarshal_objformatsink(OSyncMessage *message, OSyncObjFormatSink **sink, OSyncError **error)
{
	osync_assert(message);

	/* Order:
	 *
	 * name (string)
	 *
	 * available_settings (uint)
	 * 
	 * (optional)
	 * config (string)
	 */

	char *name = NULL;
	char *config = NULL;

	unsigned int available_settings = 0;

	osync_message_read_string(message, &name);
	osync_assert(name);

	*sink = osync_objformat_sink_new(name, error);
	if (!*sink)
		goto error;

	g_free(name);

	osync_message_read_uint(message, &available_settings);

	if (available_settings & MARSHAL_OBJFORMATSINK_CONFIG) {
		osync_message_read_string(message, &config);
		osync_objformat_sink_set_config(*sink, config);
		g_free(config);
	}

	return TRUE;

error:
	if (name)
		g_free(name);
	return FALSE;
}

#define MARSHAL_PLUGINRESSOURCE_NAME (0x1 << 1)
#define MARSHAL_PLUGINRESSOURCE_MIME (0x1 << 2)
#define MARSHAL_PLUGINRESSOURCE_PATH (0x1 << 3)
#define MARSHAL_PLUGINRESSOURCE_URL  (0x1 << 4)

osync_bool osync_marshal_pluginressource(OSyncMessage *message, OSyncPluginRessource *res, OSyncError **error)
{
	osync_assert(message);
	osync_assert(res);

	/* Order:
	 *
	 * enabled (int)
	 * objtype (string)
	 * num_sinks (uint)
	 * sinks (OSyncObjFormatSink)
	 *
	 * available_settings (uint)
	 * 
	 * (optional)
	 * name (string)
	 * mime (string)
	 * path (string)
	 * url (string)
	 */

	unsigned int available_settings = 0;

	const char *name = osync_plugin_ressource_get_name(res);
	const char *mime = osync_plugin_ressource_get_mime(res);
	const char *objtype = osync_plugin_ressource_get_objtype(res);
	const char *path = osync_plugin_ressource_get_path(res);
	const char *url = osync_plugin_ressource_get_url(res);

	/* enabled */
	osync_message_write_int(message, osync_plugin_ressource_is_enabled(res));

	/* objtype */
	osync_assert(objtype);
	osync_message_write_string(message, objtype);

	/* num_sinks */
	OSyncList *sinks = osync_plugin_ressource_get_objformat_sinks(res);
	unsigned int num_sinks = osync_list_length(sinks);
	osync_message_write_uint(message, num_sinks);

	/* format sinks */
	OSyncList *s;
	for (s = osync_plugin_ressource_get_objformat_sinks(res); s; s = s->next) {
		OSyncObjFormatSink *sink = s->data;
		if (!osync_marshal_objformatsink(message, sink, error))
			goto error;
	}

	/** optional fields */

	if (name)
		available_settings |= MARSHAL_PLUGINRESSOURCE_NAME;

	if (mime)
		available_settings |= MARSHAL_PLUGINRESSOURCE_MIME;

	if (path)
		available_settings |= MARSHAL_PLUGINRESSOURCE_PATH;

	if (url)
		available_settings |= MARSHAL_PLUGINRESSOURCE_URL;

	osync_message_write_uint(message, available_settings);

	if (name)
		osync_message_write_string(message, name);

	if (mime)
		osync_message_write_string(message, mime);

	if (path)
		osync_message_write_string(message, path);

	if (url)
		osync_message_write_string(message, url);
	
	return TRUE;

error:
	return FALSE;
}

osync_bool osync_demarshal_pluginressource(OSyncMessage *message, OSyncPluginRessource **res, OSyncError **error)
{
	/* Order:
	 *
	 * enabled (int)
	 * objtype (string)
	 * num_sinks (uint)
	 * sinks (OSyncObjFormatSink)
	 *
	 * available_settings (uint)
	 * 
	 * (optional)
	 * name (string)
	 * mime (string)
	 * path (string)
	 * url (string)
	 */

	int enabled;
	char *objtype = NULL;
	unsigned int i, num_sinks;
	unsigned int available_settings;
	char *name = NULL;
	char *mime = NULL;
	char *path = NULL;
	char *url = NULL;

	*res = osync_plugin_ressource_new(error);
	if (!*res)
		goto error;


	osync_message_read_int(message, &enabled);
	osync_plugin_ressource_enable(*res, enabled);

	osync_message_read_string(message, &objtype);
	osync_plugin_ressource_set_objtype(*res, objtype);
	g_free(objtype);

	osync_message_read_uint(message, &num_sinks);
	for (i=0; i < num_sinks; i++) {
		OSyncObjFormatSink *sink = NULL;
		if (!osync_demarshal_objformatsink(message, &sink, error))
			goto error;

		osync_plugin_ressource_add_objformat_sink(*res, sink);
	}

	osync_message_read_uint(message, &available_settings);

	if (available_settings & MARSHAL_PLUGINRESSOURCE_NAME) {
		osync_message_read_string(message, &name);
		osync_plugin_ressource_set_name(*res, name);
		g_free(name);
	}

	if (available_settings & MARSHAL_PLUGINRESSOURCE_MIME) {
		osync_message_read_string(message, &mime);
		osync_plugin_ressource_set_mime(*res, mime);
		g_free(mime);
	}

	if (available_settings & MARSHAL_PLUGINRESSOURCE_PATH) {
		osync_message_read_string(message, &path);
		osync_plugin_ressource_set_path(*res, path);
		g_free(path);
	}

	if (available_settings & MARSHAL_PLUGINRESSOURCE_URL) {
		osync_message_read_string(message, &url);
		osync_plugin_ressource_set_url(*res, url);
		g_free(url);
	}

	return TRUE;

error:
	return FALSE;
}

#define MARSHAL_PLUGINCONFIG_CONNECTION (0x1 << 1)
#define MARSHAL_PLUGINCONFIG_AUTHENTICATON (0x1 << 2)
#define MARSHAL_PLUGINCONFIG_LOCALIZATION (0x1 << 3)

osync_bool osync_marshal_pluginconfig(OSyncMessage *message, OSyncPluginConfig *config, OSyncError **error)
{
	osync_assert(message);
	osync_assert(config);

	/* Order:
	 * 
	 * $available subconfigs
	 *
	 * $connection
	 * $authenticatoin
	 * $localization
	 * $num_ressources
	 * $ressources
	 */

	unsigned int available_subconfigs = 0;

	OSyncPluginConnection *conn = osync_plugin_config_get_connection(config);

	if (conn)
		available_subconfigs |= MARSHAL_PLUGINCONFIG_CONNECTION;
		

	osync_message_write_uint(message, available_subconfigs);

	if (conn && !osync_marshal_pluginconnection(message, conn, error))
		goto error;


	OSyncList *r = osync_plugin_config_get_ressources(config);
	osync_message_write_uint(message, osync_list_length(r));

	for (; r; r = r->next) {
		OSyncPluginRessource *res = r->data;
		if (!osync_marshal_pluginressource(message, res, error))
			goto error;
	}

	
	return TRUE;

error:
	return FALSE;
}

osync_bool osync_demarshal_pluginconfig(OSyncMessage *message, OSyncPluginConfig **config, OSyncError **error)
{
	/* Order:
	 * 
	 * $available subconfigs
	 *
	 * $connection
	 * $authenticatoin
	 * $localization
	 * $num_ressources
	 * $ressources

	 */

	OSyncPluginConnection *conn = NULL;
	unsigned int available_subconfigs = 0;
	unsigned int i, num_ressources = 0;
	
	*config = osync_plugin_config_new(error);
	if (!*config)
		goto error;

	/* available subconfigs */
	osync_message_read_uint(message, &available_subconfigs);

	/* Connection */
	if (available_subconfigs & MARSHAL_PLUGINCONFIG_CONNECTION) {
		if (!osync_demarshal_pluginconnection(message, &conn, error))
			goto error_free_config;

		osync_plugin_config_set_connection(*config, conn);
	}

	osync_message_read_uint(message, &num_ressources);

	/* number of ressources */
	for (i = 0; i < num_ressources; i++) {
		OSyncPluginRessource *res;
		if (!osync_demarshal_pluginressource(message, &res, error))
			goto error_free_config;

		osync_plugin_config_add_ressource(*config, res);

	}

	return TRUE;

error_free_config:
	osync_plugin_config_unref(*config);
error:
	return FALSE;
}

