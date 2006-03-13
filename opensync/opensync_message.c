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
	if (g_atomic_int_dec_and_test(&(message->refCount)))
		g_free(message);
}

/*! @brief Sets the handler that will receive the reply
 * 
 * @param message The message to work on
 * @param replyqueue Which queue should receive the reply
 * @param handler Which handler should be called when the reply is received
 * @param user_data Which user data should be passed to the handler
 * 
 */
void osync_message_set_handler(OSyncMessage *message, OSyncMessageHandler handler, gpointer user_data)
{
	osync_trace(TRACE_INTERNAL, "%p handler to %p", message, user_data);
	message->user_data = user_data;
	message->callback = handler;
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

	reply->id1 = message->id1;
	reply->id2 = message->id2;
	return reply;
}

/*! @brief Creates a new error reply
 * 
 * @param parent Who send this message. Can be any pointer.
 * @param message The message to which you wish to reply
 * @returns Pointer to a newly allocated message
 */
OSyncMessage *osync_message_new_errorreply(OSyncMessage *message, OSyncError **error)
{
	OSyncMessage *reply = osync_message_new(OSYNC_MESSAGE_ERRORREPLY, 0, error);
	if (!reply)
		return NULL;

	reply->id1 = message->id1;
	reply->id2 = message->id2;
	return reply;
}

void osync_message_set_error(OSyncMessage *message, OSyncError **error)
{
	osync_error_duplicate(&message->error, error);
}

OSyncError *osync_message_get_error(OSyncMessage *message)
{
	return message->error;
}

/*! @brief Checks if the message is a error
 * 
 * @param message The message to check
 * @return #TRUE if the message is a error, #FALSE otherwise
 * 
 */
gboolean osync_message_is_error(OSyncMessage *message)
{
	if (message->cmd == OSYNC_MESSAGE_ERRORREPLY)
		return TRUE;
	return FALSE;
}

/*! @brief Reset the timeout for a given message
 * 
 * This function will reset the timeout for a message. The timeout will
 * restart with its original value.
 * 
 * @param message The message to reset
 * 
 */
void osync_message_reset_timeout(OSyncMessage *message)
{
	if (!message->to_info)
		return;

	GSource *source = message->to_info->source;

	GMainContext *context = g_source_get_context(source);
	g_source_destroy(source);
	message->to_info->source = g_timeout_source_new(message->to_info->timeout * 1000);
	g_source_set_callback(message->to_info->source, message->to_info->timeoutfunc, message->to_info, NULL);
	g_source_attach(message->to_info->source, context);
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

void osync_message_read_int(OSyncMessage *message, int *value)
{
	memcpy(value, &(message->buffer->data[ message->buffer_read_pos ]), sizeof(int));
	message->buffer_read_pos += sizeof(int);
}

void osync_message_read_long_long_int(OSyncMessage *message, long long int *value)
{
	memcpy(value, &(message->buffer->data[ message->buffer_read_pos ]), sizeof(long long int));
	message->buffer_read_pos += sizeof(long long int);
}

void osync_message_read_string(OSyncMessage *message, char **value)
{
	int length = 0;
	memcpy(&length, &(message->buffer->data[ message->buffer_read_pos ]), sizeof(int));
	message->buffer_read_pos += sizeof(int);

	if (length == -1) {
		*value = NULL;
		return;
	}
	*value = (char*)malloc(length);
	memcpy(*value, &(message->buffer->data[ message->buffer_read_pos ]), length );
	message->buffer_read_pos += length;
}

void osync_message_read_data(OSyncMessage *message, void *value, int size)
{
	osync_trace(TRACE_INTERNAL, "memcpy now");
	memcpy(value, &(message->buffer->data[ message->buffer_read_pos ]), size );
	osync_trace(TRACE_INTERNAL, "memcpy done");
	message->buffer_read_pos += size;
}

