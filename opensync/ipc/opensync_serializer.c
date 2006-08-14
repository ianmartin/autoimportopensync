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
	/* Order:
	 * 
	 * format
	 * objtype
	 * size
	 * data */
	
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

osync_bool osync_marshal_objtype_sink(OSyncMessage *message, OSyncObjTypeSink *sink, OSyncError **error)
{
	/* Order:
	 * 
	 * name
	 * number of formats
	 * format list (string)
	 * enabled */
	
	int i = 0;
	int num = osync_objtype_sink_num_objformats(sink);
	osync_message_write_string(message, osync_objtype_sink_get_name(sink));
	osync_message_write_int(message, num);
	
	for (i = 0; i < num; i++) {
		const char *format = osync_objtype_sink_nth_objformat(sink, i);
		osync_message_write_string(message, format);
	}
	
	osync_message_write_int(message, osync_objtype_sink_is_enabled(sink));
	
	return TRUE;
}

osync_bool osync_demarshal_objtype_sink(OSyncMessage *message, OSyncObjTypeSink **sink, OSyncError **error)
{
	/* Order:
	 * 
	 * name
	 * number of formats
	 * format list (string)
	 * enabled */
	
	*sink = osync_objtype_sink_new(NULL, error);
	if (!*sink)
		goto error;

	char *name = NULL;
	int num_formats = 0;
	int enabled = 0;
	char *format = NULL;
	
 	osync_message_read_string(message, &name);
 	osync_objtype_sink_set_name(*sink, name);
 	g_free(name);
 	
	osync_message_read_int(message, &num_formats);
	int i = 0;
	for (i = 0; i < num_formats; i++) {
 		osync_message_read_string(message, &format);
		osync_objtype_sink_add_objformat(*sink, format);
 		g_free(format);
	}

	osync_message_read_int(message, &enabled);
	osync_objtype_sink_set_enabled(*sink, enabled);

	return TRUE;

error:
	return FALSE;
}

void osync_marshal_error(OSyncMessage *message, OSyncError *error)
{
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
	int hasError = 0;

	osync_message_read_int(message, &hasError);
	
	if (hasError) {
		char *msg = NULL;
		int error_type = OSYNC_NO_ERROR;
		
		osync_message_read_int(message, &error_type);
		osync_message_read_string(message, &msg);
		
		osync_error_set(error, (OSyncErrorType)error_type, msg);
		g_free(msg);
	} else
		osync_error_unref(error);
}
