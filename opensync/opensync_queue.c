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

#include <fcntl.h>

#include "easyipc.h"

#include "opensync.h"
#include "opensync_internals.h"

/**
 * @ingroup OSEngineQueue
 * @brief A Queue used for asynchronous communication between thread
 * 
 */

/*@{*/

/*! @brief Creates a new asynchronous queue
 * 
 * This function return the pointer to a newly created OSyncQueue
 * 
 */
OSyncQueue *osync_queue_new(const char *name, OSyncError **error)
{
	OSyncQueue *queue = osync_try_malloc0(sizeof(OSyncQueue), error);
	if (!queue)
		return NULL;

	queue->name = g_strdup(name);
	return queue;
}

void osync_queue_free(OSyncQueue *queue)
{
	if (queue->source)
		g_source_destroy(queue->source);

	if (queue->name)
		g_free(queue->name);
	g_free(queue);
}

osync_bool osync_queue_exists(OSyncQueue *queue)
{
	return g_file_test(queue->name, G_FILE_TEST_EXISTS) ? TRUE : FALSE;
}

osync_bool osync_queue_create(OSyncQueue *queue, OSyncError **error)
{
	if (mkfifo(queue->name, 0600) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to create fifo");
		return FALSE;
	}
	return TRUE;
}

gboolean source_callback(gpointer);

osync_bool osync_queue_connect(OSyncQueue *queue, int flags, OSyncError **error)
{
	int fd = open(queue->name, flags);
	if (fd == -1) {
                osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open fifo");
                return FALSE;
        }
	queue->fd = fd;

	if (queue->context) {
		queue->channel = g_io_channel_unix_new(queue->fd);
		GSource *source = g_io_create_watch(queue->channel, G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL);
		g_source_set_callback(source, source_callback, queue, NULL);
		queue->source = source;
		g_source_attach(source, queue->context);
	}

	return TRUE;
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
void osync_queue_set_message_handler(OSyncQueue *queue, OSyncMessageHandler handler, gpointer user_data)
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
osync_bool osync_queue_send_int(OSyncQueue *queue, int data, OSyncError **error)
{
	if (eipc_write_int(queue->fd, data) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error occured while sending");
		return FALSE;
	}
	return TRUE;
}

osync_bool osync_queue_send_string(OSyncQueue *queue, const char *string, OSyncError **error)
{
	if (eipc_write_string(queue->fd, string) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error occured while sending");
		return FALSE;
	}
	return TRUE;
}

osync_bool osync_queue_send_data(OSyncQueue *queue, void *data, unsigned int size, OSyncError **error)
{
	if (eipc_writen(queue->fd, data, (int)size) != size) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error occured while sending");
		return FALSE;
	}
	return TRUE;
}

osync_bool osync_queue_send_long_long_int(OSyncQueue *queue, long long int data, OSyncError **error)
{
	if (eipc_write_long_long_int(queue->fd, data) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error occured while sending");
		return FALSE;
	}
	return TRUE;
}

osync_bool osync_queue_read_int(OSyncQueue *queue, int *data, OSyncError **error)
{
	if (eipc_read_int(queue->fd, data) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error occured while reading");
		return FALSE;
	}
	return TRUE;
}

osync_bool osync_queue_read_string(OSyncQueue *queue, char **string, OSyncError **error)
{
	if (eipc_read_string_alloc(queue->fd, string) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error occured while reading");
		return FALSE;
	}
	return TRUE;
}

osync_bool osync_queue_read_data(OSyncQueue *queue, void *data, unsigned int size, OSyncError **error)
{
	if (eipc_readn(queue->fd, data, (int)size) != size) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error occured while reading");
		return FALSE;
	}
	return TRUE;
}

osync_bool osync_queue_read_long_long_int(OSyncQueue*queue, long long int *data, OSyncError **error)
{
	if (eipc_read_long_long_int(queue->fd, data) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error occured while reading");
		return FALSE;
	}
	return TRUE;
}

gboolean source_callback(gpointer user_data)
{
  OSyncError *error = NULL;
	OSyncQueue *queue = user_data;

  if ( g_io_channel_get_buffer_condition( queue->channel ) && G_IO_IN ) {
    OSyncMessage *message = osync_message_new( 0, &error );
    if ( !message ) {
      osync_error_free( &error );
      return FALSE;
    }
    if ( !osync_demarshal_message( queue, message, &error ) ) {
      osync_error_free( &error );
      return FALSE;
    }

		if (osync_message_get_command(message) == OSYNC_MESSAGE_REPLY ||
        osync_message_get_command(message) == OSYNC_MESSAGE_ERRORREPLY) {

		} else {
			if (!queue->message_handler) {
				printf("no messagehandler for queue %p\n", queue);
				printf("ERROR! You need to set a queue message handler before receiving messages\n");
			} else {
				queue->message_handler( message, queue->user_data);
			}
		}
    
  } else {
    printf( "maybe an error ;)" );
  }

	return TRUE;
}

/*! @brief Sets the queue to use the gmainloop with the given context
 * 
 * This function will attach the OSyncQueue as a source to the given context.
 * The queue will then be check for new messages and the messages will be
 * handled.
 * 
 * @param queue The queue to set up
 * @param context The context to use. NULL for default loop
 * 
 */
void osync_queue_setup_with_gmainloop(OSyncQueue *queue, GMainContext *context)
{
/*	GSourceFuncs *functions = g_malloc0(sizeof(GSourceFuncs));
	functions->prepare = _queue_prepare;
	functions->check = _queue_check;
	functions->dispatch = _queue_dispatch;
	functions->finalize = NULL;

	GSource *source = g_source_new(functions, sizeof(GSource) + sizeof(OSyncQueue *));
	OSyncQueue **queueptr = (OSyncQueue **)(source + 1);
	*queueptr = queue;
	g_source_set_callback(source, NULL, queue, NULL);
	queue->source = source;
	g_source_attach(source, context);
	queue->context = context;*/

	queue->context = context;
}

osync_bool osync_queue_dispatch(OSyncQueue *queue, OSyncError **error)
{
  if ( !source_callback(queue) ) {
    osync_error_set( error, OSYNC_ERROR_GENERIC, "Recieved invalid message in source callback." );
    return FALSE;
  }

  return TRUE;
}
