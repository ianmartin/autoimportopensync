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

#include "opensync_message_internals.h"

/**
 * @ingroup OSEngineMessage
 * @brief A Message used by the inter thread messaging library
 * 
 */
 
/*@{*/

/*! @brief Creates a new message of the given type
 * 
 * This function will create a new message of the given type, with
 * the given parent and signal name. The parent will be passed to the OSyncMessageHandler
 * 
 * @param parent Who send this message. Can be any pointer.
 * @param msgname The name of the message
 * @param type The type of this message
 * @returns Pointer to a newly allocated message
 * 
 */
OSyncMessage *osync_message_new(OSyncMessageCommand cmd, int size, OSyncError **error)
{
	OSyncMessage *message = osync_try_malloc0(sizeof(OSyncMessage), error);
	if (!message)
		return NULL;

	message->cmd = cmd;
	message->refCount = 1;
	if (size > 0)
		message->buffer = g_byte_array_sized_new( size );
	else
		message->buffer = g_byte_array_new();
	message->buffer_read_pos = 0;
	return message;
}

void osync_message_ref(OSyncMessage *message)
{
	g_atomic_int_inc(&(message->refCount));
}

void osync_message_unref(OSyncMessage *message)
{
	if (g_atomic_int_dec_and_test(&(message->refCount))) {
		
		g_byte_array_free(message->buffer, TRUE);
		
		g_free(message);
	}
}

void osync_message_set_cmd(OSyncMessage *message, OSyncMessageCommand cmd)
{
	osync_assert(message);
	message->cmd = cmd;
}

OSyncMessageCommand osync_message_get_cmd(OSyncMessage *message)
{
	osync_assert(message);
	return message->cmd;
}

void osync_message_set_id(OSyncMessage *message, long long int id)
{
	osync_assert(message);
	message->id = id;
}

long long int osync_message_get_id(OSyncMessage *message)
{
	osync_assert(message);
	return message->id;
}

unsigned int osync_message_get_message_size(OSyncMessage *message)
{
	osync_assert(message);
	return message->buffer->len;
}

void osync_message_set_message_size(OSyncMessage *message, unsigned int size)
{
	osync_assert(message);
	message->buffer->len = size;
}

void osync_message_get_buffer(OSyncMessage *message, char **data, unsigned int *size)
{
	osync_assert(message);
	
	if (data)
		*data = (char *)message->buffer->data;
	
	if (size)
		*size = message->buffer->len;
}

/*! @brief Sets the handler that will receive the reply
 * 
 * @param message The message to work on
 * @param replyqueue Which queue should receive the reply
 * @param handler Which handler should be called when the reply is received
 * @param user_data Which user data should be passed to the handler
 * 
 */
void osync_message_set_handler(OSyncMessage *message, OSyncMessageHandler handler, void *user_data)
{
	message->user_data = user_data;
	message->callback = handler;
}

OSyncMessageHandler osync_message_get_handler(OSyncMessage *message)
{
	osync_assert(message);
	return message->callback;
}

void *osync_message_get_handler_data(OSyncMessage *message)
{
	osync_assert(message);
	return message->user_data;
}

/*! @brief Creates a new reply
 * 
 * @param parent Who send this message. Can be any pointer.
 * @param message The message to which you wish to reply
 * @returns Pointer to a newly allocated message
 * 
 */
OSyncMessage *osync_message_new_reply(OSyncMessage *message, OSyncError **error)
{
	OSyncMessage *reply = osync_message_new(OSYNC_MESSAGE_REPLY, 0, error);
 	if (!reply)
		return NULL;

	reply->id = message->id;
	return reply;
}

/*! @brief Creates a new error reply
 * 
 * @param parent Who send this message. Can be any pointer.
 * @param message The message to which you wish to reply
 * @returns Pointer to a newly allocated message
 */
OSyncMessage *osync_message_new_errorreply(OSyncMessage *message, OSyncError *error, OSyncError **loc_error)
{
	OSyncMessage *reply = osync_message_new(OSYNC_MESSAGE_ERRORREPLY, 0, loc_error);
	if (!reply)
		return NULL;

	osync_marshal_error(reply, error);
	
	if (message)
		reply->id = message->id;
	return reply;
}

OSyncMessage *osync_message_new_error(OSyncError *error, OSyncError **loc_error)
{
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_ERROR, 0, loc_error);
	if (!message)
		return NULL;

	osync_marshal_error(message, error);
	
	return message;
}

OSyncMessage *osync_message_new_queue_error(OSyncError *error, OSyncError **loc_error)
{
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_QUEUE_ERROR, 0, loc_error);
	if (!message)
		return NULL;

	osync_marshal_error(message, error);
	
	return message;
}

/*! @brief Checks if the message is a error
 * 
 * @param message The message to check
 * @return #TRUE if the message is a error, #FALSE otherwise
 * 
 */
osync_bool osync_message_is_error(OSyncMessage *message)
{
	if (message->cmd == OSYNC_MESSAGE_ERRORREPLY)
		return TRUE;
	return FALSE;
}

osync_bool osync_message_is_answered(OSyncMessage *message)
{
	return message->is_answered;
}

void osync_message_set_answered(OSyncMessage *message)
{
	message->is_answered = TRUE;
}

/*! @brief Gets the command from a message
 * 
 * This function will return the command of a message
 * 
 * @param message The message
 * @returns the command
 */
OSyncMessageCommand osync_message_get_command(OSyncMessage *message)
{
	g_assert(message);
	return message->cmd;
}

/*@}*/

char* osync_message_get_commandstr(OSyncMessage *message)
{
	char* cmdstr = "UNKNOWN";
	
	switch(message->cmd)
	{
		case OSYNC_MESSAGE_NOOP:
			cmdstr = "OSYNC_MESSAGE_NOOP"; break;
		case OSYNC_MESSAGE_CONNECT:
			cmdstr = "OSYNC_MESSAGE_CONNECT"; break;
		case OSYNC_MESSAGE_DISCONNECT:
			cmdstr = "OSYNC_MESSAGE_DISCONNECT"; break;
		case OSYNC_MESSAGE_GET_CHANGES:
			cmdstr = "OSYNC_MESSAGE_GET_CHANGES"; break;
		case OSYNC_MESSAGE_READ_CHANGE:
			cmdstr = "OSYNC_MESSAGE_READ_CHANGE"; break;
		case OSYNC_MESSAGE_COMMIT_CHANGE:
			cmdstr = "OSYNC_MESSAGE_COMMIT_CHANGE"; break;
		case OSYNC_MESSAGE_COMMITTED_ALL:
			cmdstr = "OSYNC_MESSAGE_COMMITTED_ALL"; break;
		case OSYNC_MESSAGE_SYNC_DONE:
			cmdstr = "OSYNC_MESSAGE_SYNC_DONE"; break;
		case OSYNC_MESSAGE_CALL_PLUGIN:
			cmdstr = "OSYNC_MESSAGE_CALL_PLUGIN"; break;
		case OSYNC_MESSAGE_NEW_CHANGE:
			cmdstr = "OSYNC_MESSAGE_NEW_CHANGE"; break;
		case OSYNC_MESSAGE_REPLY:
			cmdstr = "OSYNC_MESSAGE_REPLY"; break;
		case OSYNC_MESSAGE_ERRORREPLY:
			cmdstr = "OSYNC_MESSAGE_ERRORREPLY"; break;
		case OSYNC_MESSAGE_INITIALIZE:
			cmdstr = "OSYNC_MESSAGE_INITIALIZE"; break;
		case OSYNC_MESSAGE_FINALIZE:
			cmdstr = "OSYNC_MESSAGE_FINALIZE"; break;
		case OSYNC_MESSAGE_DISCOVER:
			cmdstr = "OSYNC_MESSAGE_DISCOVER"; break;
		case OSYNC_MESSAGE_SYNCHRONIZE:
			cmdstr = "OSYNC_MESSAGE_SYNCHRONIZE"; break;
		case OSYNC_MESSAGE_ENGINE_CHANGED:
			cmdstr = "OSYNC_MESSAGE_ENGINE_CHANGED"; break;
		case OSYNC_MESSAGE_MAPPING_CHANGED:
			cmdstr = "OSYNC_MESSAGE_MAPPING_CHANGED"; break;
		case OSYNC_MESSAGE_MAPPINGENTRY_CHANGED:
			cmdstr = "OSYNC_MESSAGE_MAPPINGENTRY_CHANGED"; break;
		case OSYNC_MESSAGE_ERROR:
			cmdstr = "OSYNC_MESSAGE_ERROR"; break;
		case OSYNC_MESSAGE_QUEUE_ERROR:
			cmdstr = "OSYNC_MESSAGE_QUEUE_ERROR"; break;
		case OSYNC_MESSAGE_QUEUE_HUP:
			cmdstr = "OSYNC_MESSAGE_QUEUE_HUP"; break;
	}
	
	return cmdstr;	
}

void osync_message_write_int(OSyncMessage *message, int value)
{
	g_byte_array_append( message->buffer, (unsigned char*)&value, sizeof( int ) );
}

void osync_message_write_long_long_int(OSyncMessage *message, long long int value)
{
	g_byte_array_append( message->buffer, (unsigned char*)&value, sizeof( long long int ) );
}

void osync_message_write_string(OSyncMessage *message, const char *value)
{
	int length = 0;
	if (value == NULL) {
		length = -1;
		g_byte_array_append( message->buffer, (unsigned char*)&length, sizeof( int ) );
	} else {
		int length = strlen( value ) + 1;
		g_byte_array_append( message->buffer, (unsigned char*)&length, sizeof( int ) );
		g_byte_array_append( message->buffer, (unsigned char*)value, length );
	}
}

void osync_message_write_data(OSyncMessage *message, const void *value, int size)
{
	g_byte_array_append( message->buffer, value, size );
}

void osync_message_write_buffer(OSyncMessage *message, const void *value, int size)
{
	osync_message_write_int(message, size);
	if (size > 0)
		osync_message_write_data(message, value, size);
}

void osync_message_read_int(OSyncMessage *message, int *value)
{
	osync_assert(message->buffer->len >= message->buffer_read_pos + sizeof(int));
	
	memcpy(value, &(message->buffer->data[ message->buffer_read_pos ]), sizeof(int));
	message->buffer_read_pos += sizeof(int);
}

void osync_message_read_long_long_int(OSyncMessage *message, long long int *value)
{
	osync_assert(message->buffer->len >= message->buffer_read_pos + sizeof(long long int));
	
	memcpy(value, &(message->buffer->data[ message->buffer_read_pos ]), sizeof(long long int));
	message->buffer_read_pos += sizeof(long long int);
}

void osync_message_read_const_string(OSyncMessage *message, char **value)
{
	int length = 0;
	osync_message_read_int(message, &length);

	if (length == -1) {
		*value = NULL;
		return;
	}
	
	osync_assert(message->buffer->len >= message->buffer_read_pos + length);
	*value = (char *)&(message->buffer->data[message->buffer_read_pos]);
	message->buffer_read_pos += length;
}

void osync_message_read_string(OSyncMessage *message, char **value)
{
	int length = 0;
	osync_message_read_int(message, &length);

	if (length == -1) {
		*value = NULL;
		return;
	}
	
	osync_assert(message->buffer->len >= message->buffer_read_pos + length);
	
	*value = (char*)malloc(length);
	memcpy(*value, &(message->buffer->data[ message->buffer_read_pos ]), length );
	message->buffer_read_pos += length;
}

void osync_message_read_const_data(OSyncMessage *message, void **value, int size)
{
	osync_assert(message->buffer->len >= message->buffer_read_pos + size);
	
	*value = &(message->buffer->data[message->buffer_read_pos]);
	message->buffer_read_pos += size;
}

void osync_message_read_data(OSyncMessage *message, void *value, int size)
{
	osync_assert(message->buffer->len >= message->buffer_read_pos + size);
	
	memcpy(value, &(message->buffer->data[ message->buffer_read_pos ]), size );
	message->buffer_read_pos += size;
}

void osync_message_read_buffer(OSyncMessage *message, void **value, int *size)
{
	/* Now, read the data from the message */
	osync_message_read_int(message, size);
	
	if (*size > 0) {
		*value = g_malloc0(*size);
		osync_message_read_data(message, *value, *size);
	}
}
