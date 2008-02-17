/*
 * libopensync - A synchronization framework
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

#ifndef _OPENSYNC_MESSAGES_H
#define _OPENSYNC_MESSAGES_H

/**
 * @defgroup OSyncMessage OpenSync Message
 * @brief A Message used by the inter thread messaging library
 * 
 */

/*@{*/

/*! @brief The Type of the message
 * 
 */
typedef enum {
	OSYNC_MESSAGE_NOOP,
	OSYNC_MESSAGE_CONNECT,
	OSYNC_MESSAGE_DISCONNECT,
	OSYNC_MESSAGE_GET_CHANGES,
	OSYNC_MESSAGE_READ_CHANGE,
	OSYNC_MESSAGE_COMMIT_CHANGE,
	OSYNC_MESSAGE_COMMITTED_ALL,
	OSYNC_MESSAGE_SYNC_DONE,
	OSYNC_MESSAGE_CALL_PLUGIN,
	OSYNC_MESSAGE_NEW_CHANGE,
	OSYNC_MESSAGE_REPLY,
	OSYNC_MESSAGE_ERRORREPLY,
	OSYNC_MESSAGE_INITIALIZE,
	OSYNC_MESSAGE_FINALIZE,
	OSYNC_MESSAGE_DISCOVER,
	OSYNC_MESSAGE_SYNCHRONIZE,
	OSYNC_MESSAGE_ENGINE_CHANGED,
	OSYNC_MESSAGE_MAPPING_CHANGED,
	OSYNC_MESSAGE_MAPPINGENTRY_CHANGED,
	OSYNC_MESSAGE_ERROR,
	OSYNC_MESSAGE_QUEUE_ERROR,
	OSYNC_MESSAGE_QUEUE_HUP
} OSyncMessageCommand;

/*! @brief Function which can receive messages
 * 
 * @param message The reply that is being received.
 * @param user_data The userdata which was set previously
 * 
 */
typedef void (*OSyncMessageHandler)(OSyncMessage *message, void *user_data);

/*@}*/

OSYNC_EXPORT OSyncMessage *osync_message_new(OSyncMessageCommand cmd, int size, OSyncError **error);
OSYNC_EXPORT OSyncMessage *osync_message_new_reply(OSyncMessage *message, OSyncError **error);
OSYNC_EXPORT OSyncMessage *osync_message_new_errorreply(OSyncMessage *message, OSyncError *error, OSyncError **loc_error);
OSYNC_EXPORT OSyncMessage *osync_message_new_error(OSyncError *error, OSyncError **loc_error);
OSYNC_EXPORT OSyncMessage *osync_message_new_queue_error(OSyncError *error, OSyncError **loc_error);
OSYNC_EXPORT OSyncMessage *osync_message_ref(OSyncMessage *message);
OSYNC_EXPORT void osync_message_unref(OSyncMessage *message);

OSYNC_EXPORT void osync_message_set_cmd(OSyncMessage *message, OSyncMessageCommand cmd);
OSYNC_EXPORT OSyncMessageCommand osync_message_get_cmd(OSyncMessage *message);
OSYNC_EXPORT void osync_message_set_id(OSyncMessage *message, long long int id);
OSYNC_EXPORT long long int osync_message_get_id(OSyncMessage *message);
OSYNC_EXPORT unsigned int osync_message_get_message_size(OSyncMessage *message);
OSYNC_EXPORT void osync_message_set_message_size(OSyncMessage *message, unsigned int size);
OSYNC_EXPORT void osync_message_get_buffer(OSyncMessage *message, char **data, unsigned int *size);

OSYNC_EXPORT void osync_message_set_handler(OSyncMessage *message, OSyncMessageHandler handler, void *user_data);
OSYNC_EXPORT OSyncMessageHandler osync_message_get_handler(OSyncMessage *message);
OSYNC_EXPORT void *osync_message_get_handler_data(OSyncMessage *message);

OSYNC_EXPORT osync_bool osync_message_is_error(OSyncMessage *message);
OSYNC_EXPORT OSyncMessageCommand osync_message_get_command(OSyncMessage *message);
OSYNC_EXPORT char* osync_message_get_commandstr(OSyncMessage *message);
OSYNC_EXPORT osync_bool osync_message_is_answered(OSyncMessage *message);
OSYNC_EXPORT void osync_message_set_answered(OSyncMessage *message);

OSYNC_EXPORT void osync_message_write_int(OSyncMessage *message, int value);
OSYNC_EXPORT void osync_message_write_long_long_int(OSyncMessage *message, long long int value);
OSYNC_EXPORT void osync_message_write_string(OSyncMessage *message, const char *value);
OSYNC_EXPORT void osync_message_write_data(OSyncMessage *message, const void *value, int size);
OSYNC_EXPORT void osync_message_write_buffer(OSyncMessage *message, const void *value, int size);

OSYNC_EXPORT void osync_message_read_int(OSyncMessage *message, int *value);
OSYNC_EXPORT void osync_message_read_long_long_int(OSyncMessage *message, long long int *value);
OSYNC_EXPORT void osync_message_read_string(OSyncMessage *message, char **value);
OSYNC_EXPORT void osync_message_read_data(OSyncMessage *message, void *value, int size);
OSYNC_EXPORT void osync_message_read_const_data(OSyncMessage *message, void **value, int size);
OSYNC_EXPORT void osync_message_read_const_string(OSyncMessage *message, char **value);
OSYNC_EXPORT void osync_message_read_buffer(OSyncMessage *message, void **value, int *size);

#endif /*_OPENSYNC_MESSAGES_H*/
