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
 
#include "engine.h"
#include "engine_internals.h"

/**
 * @defgroup OSEngineQueue OpenSync Message Queues Internals
 * @ingroup OSEnginePrivate
 * @brief A Queue used for asynchronous communication between thread
 * 
 */

/*@{*/

/*! @brief Creates a new asynchronous queue
 * 
 * This function return the pointer to a newly created ITMQueue
 * 
 */
ITMQueue *itm_queue_new(void)
{
	ITMQueue *queue = g_malloc0(sizeof(ITMQueue));
	queue->queue = g_async_queue_new();
	return queue;
}

void itm_queue_free(ITMQueue *queue)
{
	if (queue->source)
		g_source_destroy(queue->source);
	g_async_queue_unref(queue->queue);
	g_free(queue);
}

void itm_queue_flush(ITMQueue *queue)
{
	while (g_async_queue_try_pop(queue->queue));
}

/*! @brief Sets the message handler for a queue
 * 
 * Sets the function that will receive all messages, except the methodcall replies
 * 
 * @param queue The queue to set the handler on
 * @param handler The message handler function
 * @param user_data The userdata that the message handler should receive
 * 
 */
void itm_queue_set_message_handler(ITMQueue *queue, ITMessageHandler handler, gpointer user_data)
{
	queue->message_handler = handler;
	queue->user_data = user_data;
}

/*! @brief Sends a message down a queue
 * 
 * @param queue The queue to send the message to
 * @param message The message to send
 * 
 */
void itm_queue_send(ITMQueue *queue, ITMessage *message)
{
	g_async_queue_push(queue->queue, message);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
gboolean timeoutfunc(gpointer data)
{
	timeout_info *to_info = data;
	ITMessage *reply = itm_message_new_errorreply(to_info->replysender, to_info->message);
	itm_message_set_error(reply, "TIMEOUT", 0);
	itm_message_send_reply(reply);
	return FALSE;
}
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/*! @brief Sends a message down a queue and returns a timeout if necessary
 * 
 * This send an method call to a queue. If no answer is received until timeout,
 * the message callback will receive a timeout error. If the other side still answers
 * , the answer is send to the queue, too
 * 
 * @param queue The queue to send the message to
 * @param message The message to send
 * @param timeout How long to wait for an answer
 * 
 */
void itm_queue_send_with_timeout(ITMQueue *queue, ITMessage *message, int timeout, void *replysender)
{
	timeout_info *to_info = g_malloc0(sizeof(timeout_info));
	to_info->message = message;
	to_info->sendingqueue = queue;
	to_info->replysender = replysender;
	message->timeout_id = g_timeout_add(timeout * 1000, timeoutfunc, to_info);
	itm_queue_send(queue, message);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

gboolean _queue_prepare(GSource *source, gint *timeout_)
{
	*timeout_ = 1;
	return FALSE;
}

gboolean _queue_check(GSource *source)
{
	ITMQueue *queue = *((ITMQueue **)(source + 1));
	if (g_async_queue_length(queue->queue) > 0)
		return TRUE;
	return FALSE;
}

gboolean _queue_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
	ITMQueue *queue = user_data;

	ITMessage *message = g_async_queue_try_pop(queue->queue);
	if (message) {
		if (itm_message_is_type(message, ITMESSAGE_METHODREPLY) || itm_message_is_type(message, ITMESSAGE_ERRORREPLY)) {
			message->callback(message->parent, message, message->user_data);
		} else {
			if (!queue->message_handler) {
				printf("no messagehandler for queue %p\n", queue);
				printf("ERROR! You need to set a queue message handler before receiving messages\n");
			} else {
				queue->message_handler(message->parent, message, queue->user_data);
			}
		}
	}
	return TRUE;
}
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/*! @brief Sets the queue to use the gmainloop with the given context
 * 
 * This function will attach the ITMQueue as a source to the given context.
 * The queue will then be check for new messages and the messages will be
 * handled.
 * 
 * @param queue The queue to set up
 * @param context The context to use. NULL for default loop
 * 
 */
void itm_queue_setup_with_gmainloop(ITMQueue *queue, GMainContext *context)
{
	GSourceFuncs *functions = g_malloc0(sizeof(GSourceFuncs));
	functions->prepare = _queue_prepare;
	functions->check = _queue_check;
	functions->dispatch = _queue_dispatch;
	functions->finalize = NULL;

	GSource *source = g_source_new(functions, sizeof(GSource) + sizeof(ITMQueue *));
	ITMQueue **queueptr = (ITMQueue **)(source + 1);
	*queueptr = queue;
	g_source_set_callback(source, NULL, queue, NULL);
	queue->source = source;
	g_source_attach(source, context);
}

void itm_queue_dispatch(ITMQueue *queue)
{
	_queue_dispatch(NULL, NULL, queue);
}

/*@}*/
