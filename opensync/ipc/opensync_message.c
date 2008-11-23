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

#include "opensync_serializer_internals.h"
#include "opensync_message.h"

#include "opensync_message_internals.h"

/**
 * @ingroup OSyncMessage
 * @brief A Message used by the inter thread messaging library
 * 
 */
 
/*@{*/

/*! @brief Creates a new message of the given command
 * 
 * @param cmd The message command 
 * @param size The size of the message
 * @param error Pointer to a error-struct
 * @returns Pointer to a newly allocated message
 * 
 */
OSyncMessage *osync_message_new(OSyncMessageCommand cmd, unsigned int size, OSyncError **error)
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

/** @brief Increase the reference count of the message 
 * 
 * @param message The message
 * @returns The referenced message pointer
 * 
 */
OSyncMessage *osync_message_ref(OSyncMessage *message)
{
	g_atomic_int_inc(&(message->refCount));

	return message;
}

/** @brief Decrease the reference count of the message 
 * 
 * @param message The message 
 * 
 */
void osync_message_unref(OSyncMessage *message)
{
	if (g_atomic_int_dec_and_test(&(message->refCount))) {
		
		g_byte_array_free(message->buffer, TRUE);
		
		g_free(message);
	}
}

/** @brief Set new message command for the message object
 * 
 * @param message The message to modify 
 * @param cmd The new message command
 * 
 */
void osync_message_set_cmd(OSyncMessage *message, OSyncMessageCommand cmd)
{
	osync_assert(message);
	message->cmd = cmd;
}

/** @brief Get message command for the message object
 * 
 * @param message The message
 * 
 */
OSyncMessageCommand osync_message_get_cmd(OSyncMessage *message)
{
	osync_assert(message);
	return message->cmd;
}

/** @brief Set an ID for the message 
 * 
 * @param message The message
 * @param id The ID which get set for supplied message object 
 * 
 */
void osync_message_set_id(OSyncMessage *message, long long int id)
{
	osync_assert(message);
	message->id = id;
}

/** @brief Get message ID of supplied message object
 * 
 * @param message The message
 * @returns The message ID of supplied message
 * 
 */
long long int osync_message_get_id(OSyncMessage *message)
{
	osync_assert(message);
	return message->id;
}

/** @brief Get message size of supplied message object
 * 
 * @param message The message
 * @returns The message size of supplied message
 * 
 */
unsigned int osync_message_get_message_size(OSyncMessage *message)
{
	osync_assert(message);
	return message->buffer->len;
}

/** @brief Set message size for supplied message object
 * 
 * @param message The message
 * @param size The size of the message to set
 * 
 */
void osync_message_set_message_size(OSyncMessage *message, unsigned int size)
{
	osync_assert(message);
	message->buffer->len = size;
}

/** @brief Get the buffer/content of the message object
 * 
 * @param message The message
 * @param data Pointer to data 
 * @param size Size of the data
 * 
 */
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
 * @param handler Which handler should be called when the reply is received
 * @param user_data Which user data should be passed to the handler
 * 
 */
void osync_message_set_handler(OSyncMessage *message, OSyncMessageHandler handler, void *user_data)
{
	message->user_data = user_data;
	message->callback = handler;
}


/*! @brief Get the message handler of the message
 * 
 * @param message The message to work on
 * @returns The message handler of the message
 * 
 */
OSyncMessageHandler osync_message_get_handler(OSyncMessage *message)
{
	osync_assert(message);
	return message->callback;
}

/*! @brief Get the data which gets passed to the handler function 
 * 
 * @param message The message to work on
 * @returns Pointer of the supplied handler data
 * 
 */
void *osync_message_get_handler_data(OSyncMessage *message)
{
	osync_assert(message);
	return message->user_data;
}

/*! @brief Creates a new reply
 * 
 * @param message The message to which you wish to reply
 * @param error Pointer to error-struct 
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
 * @param message The message to which you wish to reply
 * @param error Pointer to error object for the error reply
 * @param loc_error Pointer to a error-struct for errors which appear while creating message 
 * @returns Pointer to a newly allocated error-reply message
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

/*! @brief Creates a new error message 
 * 
 * @param error Pointer to error object to send error
 * @param loc_error Pointer to a error-struct for errors which appear while creating message 
 * @returns Pointer to a newly allocated error message
 */
OSyncMessage *osync_message_new_error(OSyncError *error, OSyncError **loc_error)
{
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_ERROR, 0, loc_error);
	if (!message)
		return NULL;

	osync_marshal_error(message, error);
	
	return message;
}

/*! @brief Creates a new queue error message 
 * 
 * @param error Pointer to error object to send error
 * @param loc_error Pointer to a error-struct for errors which appear while creating message 
 * @returns Pointer to a newly allocated queue error message
 */
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
 * @return TRUE if the message is a error, FLASE otherwise
 * 
 */
osync_bool osync_message_is_error(OSyncMessage *message)
{
	if (message->cmd == OSYNC_MESSAGE_ERRORREPLY)
		return TRUE;
	return FALSE;
}

/*! @brief Checks if the message got answered 
 * 
 * @param message The message to check
 * @return TRUE if the message got answered, FLASE otherwise
 * 
 */
osync_bool osync_message_is_answered(OSyncMessage *message)
{
	return message->is_answered;
}

/*! @brief Set message as answered 
 * 
 * @param message The message to work on 
 * 
 */
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

/*! @brief Appends an integer value to serialized message buffer
 * 
 * @param message The message
 * @param value The integer value to append
 */
void osync_message_write_int(OSyncMessage *message, int value)
{
	g_byte_array_append( message->buffer, (unsigned char*)&value, sizeof( int ) );
}

/*! @brief Appends an unsigned integer value to serialized message buffer
 * 
 * @param message The message
 * @param value The integer value to append
 */
void osync_message_write_uint(OSyncMessage *message, unsigned int value)
{
	g_byte_array_append( message->buffer, (unsigned char*)&value, sizeof( unsigned int ) );
}

/*! @brief Appends a long long integer value to serialized message buffer
 * 
 * @param message The message
 * @param value The long long integer value to append
 */
void osync_message_write_long_long_int(OSyncMessage *message, long long int value)
{
	g_byte_array_append( message->buffer, (unsigned char*)&value, sizeof( long long int ) );
}

/*! @brief Appends a string to serialized message buffer
 * 
 * @param message The message
 * @param value The string to append
 */
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

/*! @brief Appends data with a specific length to the serialized message buffer
 *
 * This data should be completely serialized. This is only for internal use,
 * since this function doesn't append the size/end of the appended data.
 * 
 * @param message The message
 * @param value The data to append
 * @param size Size of corresponding data parameter
 */
void osync_message_write_data(OSyncMessage *message, const void *value, int size)
{
	/* TODO move this to PRIVATE API */
	g_byte_array_append( message->buffer, value, size );
}

/*! @brief Appends data with a specific length to the serialized message buffer,
 * plus the length of the data to determine the end.
 *
 * @param message The message
 * @param value The data to append
 * @param size Size of corresponding data parameter
 */
void osync_message_write_buffer(OSyncMessage *message, const void *value, int size)
{
	/* serialize the length of the data to make it possible to determine the end
	   of this data blob in the serialized blob. This makes demarshaling possible! */
	osync_message_write_int(message, size);
	if (size > 0)
		osync_message_write_data(message, value, size);
}

/*! @brief Read serialized integer from message buffer. This increments the read
 * position of the message buffer.
 *
 * @param message The message
 * @param value Reference to store the integer value 
 */
void osync_message_read_int(OSyncMessage *message, int *value)
{
	osync_assert(message->buffer->len >= message->buffer_read_pos + sizeof(int));
	
	memcpy(value, &(message->buffer->data[ message->buffer_read_pos ]), sizeof(int));
	message->buffer_read_pos += sizeof(int);
}

/*! @brief Read serialized unsigned integer from message buffer. This increments the read
 * position of the message buffer.
 *
 * @param message The message
 * @param value Reference to store the integer value 
 */
void osync_message_read_uint(OSyncMessage *message, unsigned int *value)
{
	osync_assert(message->buffer->len >= message->buffer_read_pos + sizeof(unsigned int));
	
	memcpy(value, &(message->buffer->data[ message->buffer_read_pos ]), sizeof(unsigned int));
	message->buffer_read_pos += sizeof(unsigned int);
}

/*! @brief Read serialized long long integer from message buffer. This increments the read
 * position of the message buffer.
 *
 * @param message The message
 * @param value Reference to store the long long integer value 
 */
void osync_message_read_long_long_int(OSyncMessage *message, long long int *value)
{
	osync_assert(message->buffer->len >= message->buffer_read_pos + sizeof(long long int));
	
	memcpy(value, &(message->buffer->data[ message->buffer_read_pos ]), sizeof(long long int));
	message->buffer_read_pos += sizeof(long long int);
}

/*! @brief Read serialized const string from message buffer. This increments the read
 * position of the message buffer.
 *
 * @param message The message
 * @param value Reference to store the string pointer 
 */
/* TODO Change char** to const char ** */
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

/*! @brief Read serialized string from message buffer. This increments the read
 * position of the message buffer. Caller is responsible for freeing the duplicated
 * string.
 *
 * @param message The message
 * @param value Reference to store the pointer to the newly allocated string 
 */
void osync_message_read_string(OSyncMessage *message, char **value)
{
	int length = 0;
	osync_message_read_int(message, &length);

	if (length == -1) {
		*value = NULL;
		return;
	}
	
	osync_assert(message->buffer->len >= message->buffer_read_pos + length);
	
	/* TODO: Error handling? */
	*value = (char*) osync_try_malloc0(length, NULL);
	if (!*value)
		return;

	memcpy(*value, &(message->buffer->data[ message->buffer_read_pos ]), length );
	message->buffer_read_pos += length;
}

/*! @brief Read serialized const data from message buffer. This increments the read
 * position of the message buffer.
 *
 * @param message The message
 * @param value Reference to store the data pointer 
 * @param size The size of data
 */
void osync_message_read_const_data(OSyncMessage *message, void **value, int size)
{
	osync_assert(message->buffer->len >= message->buffer_read_pos + size);
	
	*value = &(message->buffer->data[message->buffer_read_pos]);
	message->buffer_read_pos += size;
}

/*! @brief Read specific size of serialized data from message buffer. This increments 
 * the read position of the message buffer. Caller is responsible for freeing the 
 * duplicate data.
 *
 * @param message The message
 * @param value Reference to store the pointer to the newly allocated data 
 * @param size Size of data
 */
void osync_message_read_data(OSyncMessage *message, void *value, int size)
{
	osync_assert(message->buffer->len >= message->buffer_read_pos + size);
	
	memcpy(value, &(message->buffer->data[ message->buffer_read_pos ]), size );
	message->buffer_read_pos += size;
}

/*! @brief Read serialized data from message buffer. This increments the read
 * position of the message buffer. Caller is responsible for freeing the duplicated
 * data.
 *
 * @param message The message
 * @param value Reference to store the pointer to the newly allocated data 
 * @param size Size of data
 */
void osync_message_read_buffer(OSyncMessage *message, void **value, int *size)
{
	/* Now, read the data from the message */
	osync_message_read_int(message, size);
	
	if (*size > 0) {
		*value = g_malloc0(*size);
		osync_message_read_data(message, *value, *size);
	}
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


