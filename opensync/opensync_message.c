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

/*! @brief Sends a message down a queue and returns a timeout if necessary
 *
 * This send an method call to a queue. If no answer is received until timeout,
 * the message callback will receive a timeout error. If the other side still answers
 * , the answer is send to the queue, too
 *
 * @param queue The queue to send the message to
 * @param message The message to send
 * @param timeout How long to wait for an answer
 * @param replysender The object that is sending the reply
 *
 */
/*void osync_queue_send_with_timeout(OSyncQueue *queue, OSyncMessage *message, int timeout, void *replysender)
{
        if (timeout) {
                timeout_info *to_info = g_malloc0(sizeof(timeout_info));
                to_info->message = message;
                to_info->sendingqueue = queue;
                to_info->replysender = replysender;
                to_info->timeout = timeout;
                to_info->timeoutfunc = timeoutfunc;
                message->source = g_timeout_source_new(timeout * 1000);
                message->to_info = to_info;
                g_source_set_callback(message->source, timeoutfunc, to_info, NULL);
                g_source_attach(message->source, message->replyqueue->context);
        }
        osync_queue_send(queue, message);
}*/


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
OSyncMessage *osync_message_new(OSyncMessageCommand cmd, OSyncError **error)
{
	OSyncMessage *message = osync_try_malloc0(sizeof(OSyncMessage), error);
	if (!message)
		return NULL;

	message->cmd = cmd;
	message->refCount = 1;
	return message;
}

void osync_message_ref(OSyncMessage *message)
{
	message->refCount++;
}

void osync_message_unref(OSyncMessage *message)
{
	message->refCount--;
	if (message->refCount <= 0) {
		g_free(message);
	}
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
	OSyncMessage *reply = osync_message_new(OSYNC_MESSAGE_REPLY, error);
  if ( !reply )
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
OSyncMessage *osync_message_new_errorreply(OSyncMessage *message, OSyncError **error)
{
	OSyncMessage *reply = osync_message_new(OSYNC_MESSAGE_ERRORREPLY, error);
  if ( !reply )
    return NULL;

	reply->id = message->id;
	return reply;
}

void osync_message_set_error(OSyncMessage *message, OSyncError *error)
{
	message->error = error;
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

/*! @brief Sends a reply to a message
 * 
 * This function will send a reply to a message to the sender of the
 * original message. It also removes any timeout for the original message.
 * 
 * @param reply The reply to send
 * 
 */
void osync_message_send(OSyncMessage *message, OSyncQueue *queue, OSyncError **error)
{
  osync_marshal_message( queue, message, error );
}

osync_bool osync_message_is_answered(OSyncMessage *message)
{
	return message->is_answered;
}

void osync_message_set_answered(OSyncMessage *message)
{
	message->is_answered = TRUE;
}

/*! @brief Gets the msgname from a message
 * 
 * This function will return the name of a message
 * 
 * @param message The message
 * @returns the command
 */
OSyncMessageCommand osync_message_get_command(OSyncMessage *message)
{
	g_assert(message);
	return message->cmd;
}

/*! @brief Gets the msgname from a message
 * 
 * This function will return the name of a message
 * 
 * @param message The message
 * @returns the command
 */
long long osync_message_get_id(OSyncMessage *message)
{
	g_assert(message);
	return message->id;
}

/*@}*/
