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
#include <sys/poll.h>

#include "opensync.h"
#include "opensync_internals.h"

/**
 * @ingroup OSEngineQueue
 * @brief A Queue used for asynchronous communication between thread
 * 
 */

/*@{*/

gboolean _incoming_prepare(GSource *source, gint *timeout_)
{
	*timeout_ = 1;
	return FALSE;
}

gboolean _incoming_check(GSource *source)
{
	OSyncQueue *queue = *((OSyncQueue **)(source + 1));
	
	if (g_async_queue_length(queue->incoming) > 0)
		return TRUE;
	return FALSE;
}

gboolean _incoming_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
	OSyncQueue *queue = user_data;

	OSyncMessage *message = osync_queue_get_message(queue);
	if (message) {
		if (!queue->message_handler) {
			printf("you have to setup a message handler for the queue!\n");
			return FALSE;
		}
		queue->message_handler(message, queue->user_data);
	}
	return TRUE;
}

gboolean _queue_prepare(GSource *source, gint *timeout_)
{
	*timeout_ = 1;
	return FALSE;
}

gboolean _queue_check(GSource *source)
{
	OSyncQueue *queue = *((OSyncQueue **)(source + 1));
	if (g_async_queue_length(queue->outgoing) > 0)
		return TRUE;
	return FALSE;
}

int _osync_queue_write_data(OSyncQueue *queue, const void *vptr, size_t n)
{

  size_t nleft;
  ssize_t nwritten;
  const char *ptr;

  ptr = vptr;
  nleft = n;
  while (nleft > 0) {
    if ((nwritten = write(queue->fd, ptr, nleft)) <= 0) {
      if (errno == EINTR)
        nwritten = 0;  /* and call write() again */
      else
        return (-1);  /* error */
    }

    nleft -= nwritten;
    ptr += nwritten;
  }
  return (n);
}

osync_bool _osync_queue_write_int(OSyncQueue *queue, const int message)
{
	if (_osync_queue_write_data(queue, &message, sizeof(int)) < 0)
		return FALSE;

	return TRUE;
}

gboolean _queue_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
	OSyncQueue *queue = user_data;

	OSyncMessage *message = g_async_queue_try_pop(queue->outgoing);
	if (message) {
		if (!_osync_queue_write_int(queue, message->buffer->len + osync_marshal_get_size_message(message)))
			return FALSE;
		
		if (!_osync_queue_write_int(queue, message->cmd))
			return FALSE;
			
		int sent = 0;
		do {
			sent += _osync_queue_write_data(queue, message->buffer->data + sent, message->buffer->len - sent);
		} while (sent < message->buffer->len);
		
		osync_message_unref(message);
	}
	return TRUE;
}

gboolean _source_prepare(GSource *source, gint *timeout_)
{
	*timeout_ = 1;
	return FALSE;
}

/* Read "n" bytes from a descriptor. */
int _osync_queue_read_data(OSyncQueue *queue, void *vptr, size_t n)
{

  size_t nleft;
  ssize_t nread;
  char *ptr;

  ptr = vptr;
  nleft = n;
  while (nleft > 0) {
    if ((nread = read(queue->fd, ptr, nleft)) < 0) {
      if (errno == EINTR)
        nread = 0;  /* and call read() again */
      else
        return (-1);
    } else if (nread == 0)
      break;  /* EOF */

    nleft -= nread;
    ptr += nread;
  }
  return (n - nleft);  /* return >= 0 */
}

osync_bool _osync_queue_read_int(OSyncQueue *queue, int *message)
{
  int status;

  if ( (status = _osync_queue_read_data(queue, message, sizeof(int))) < 0) {
    return FALSE;
  }

  if (status < sizeof(int)) {
    return FALSE;
  }

  return TRUE;
}

gboolean _source_check(GSource *source)
{
	OSyncQueue *queue = *((OSyncQueue **)(source + 1));
	if (osync_queue_data_available(queue))
		return TRUE;
	return FALSE;
}

gboolean _source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
	OSyncQueue *queue = user_data;
	OSyncMessage *message = NULL;
	OSyncError *error = NULL;
	
	if (osync_queue_data_available(queue)) {
		printf("data available\n");
		int size = 0;
		int cmd = 0;
		if (!_osync_queue_read_int(queue, &size))
			return FALSE;
			
		printf("read %i\n", size);
		if (!_osync_queue_read_int(queue, &cmd))
			return FALSE;
		printf("cmd %i\n", cmd);
		
		printf("data %i, %i\n", size, cmd);
		
		message = osync_message_new(cmd, size, &error);
		if (!message) {
			osync_error_free(&error);
			return FALSE;
		}
		
		printf("reading bytebuffer now\n");
		int read = 0;
		do {
			printf("read before %i\n", read);
			read += _osync_queue_read_data(queue, message->buffer->data + read, size - read);
			printf("read after %i\n", read);
		} while (read < size);
		
		printf("done reading bytebuffer: %p\n", message);
		g_async_queue_push(queue->incoming, message);
		printf("%p %i new length %i\n", queue, getpid(), g_async_queue_length(queue->incoming));
	}
	
	return TRUE;
}

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
	queue->fd = -1;
	
	if (!g_thread_supported ())
		g_thread_init (NULL);
	
	queue->context = g_main_context_new();
	
	queue->incoming = g_async_queue_new();
	g_async_queue_ref(queue->incoming);
	queue->outgoing = g_async_queue_new();
	g_async_queue_ref(queue->outgoing);
	
	queue->thread = osync_thread_new(queue->context, error);
	if (!queue->thread)
		return NULL;
	
	GSourceFuncs *functions = g_malloc0(sizeof(GSourceFuncs));
	functions->prepare = _queue_prepare;
	functions->check = _queue_check;
	functions->dispatch = _queue_dispatch;
	functions->finalize = NULL;

	GSource *source = g_source_new(functions, sizeof(GSource) + sizeof(OSyncQueue *));
	OSyncQueue **queueptr = (OSyncQueue **)(source + 1);
	*queueptr = queue;
	g_source_set_callback(source, NULL, queue, NULL);
	g_source_attach(source, queue->context);
	
	osync_thread_start(queue->thread);
	
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

osync_bool osync_queue_connect(OSyncQueue *queue, int flags, OSyncError **error)
{
	int fd = open(queue->name, flags);
	if (fd == -1) {
                osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open fifo");
                return FALSE;
        }
	queue->fd = fd;

	if (queue->context) {
		GSourceFuncs *functions = g_malloc0(sizeof(GSourceFuncs));
		functions->prepare = _source_prepare;
		functions->check = _source_check;
		functions->dispatch = _source_dispatch;
		functions->finalize = NULL;
	
		GSource *source = g_source_new(functions, sizeof(GSource) + sizeof(OSyncQueue *));
		OSyncQueue **queueptr = (OSyncQueue **)(source + 1);
		*queueptr = queue;
		g_source_set_callback(source, NULL, queue, NULL);
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
	GSourceFuncs *functions = g_malloc0(sizeof(GSourceFuncs));
	functions->prepare = _incoming_prepare;
	functions->check = _incoming_check;
	functions->dispatch = _incoming_dispatch;
	functions->finalize = NULL;

	GSource *source = g_source_new(functions, sizeof(GSource) + sizeof(OSyncQueue *));
	OSyncQueue **queueptr = (OSyncQueue **)(source + 1);
	*queueptr = queue;
	g_source_set_callback(source, NULL, queue, NULL);
	queue->source = source;
	g_source_attach(source, context);
	queue->incomingContext = context;
}

osync_bool osync_queue_dispatch(OSyncQueue *queue, OSyncError **error)
{
	_incoming_dispatch(NULL, NULL, queue);
	return TRUE;
}

osync_bool osync_queue_data_available(OSyncQueue *queue)
{
	struct pollfd pfd;
	pfd.fd = queue->fd;
	pfd.events = POLLIN;
	
	if (poll(&pfd, 1, 0) != 1)
		return FALSE;
	return TRUE;
}

OSyncMessage *osync_queue_get_message(OSyncQueue *queue)
{
	return g_async_queue_try_pop(queue->incoming);
}

osync_bool osync_queue_remove(OSyncQueue *queue, OSyncError **error)
{
	if (unlink(queue->name) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to remove queue");
		return FALSE;
	}
	return TRUE;
}

osync_bool osync_queue_disconnect(OSyncQueue *queue, OSyncError **error)
{
	if (close(queue->fd) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to close queue");
		return FALSE;
	}
	return TRUE;
}


osync_bool osync_queue_send_message(OSyncQueue *queue, OSyncMessage *message, OSyncError **error)
{
	if (queue->error) {
		osync_error_duplicate(error, &(queue->error));
		return FALSE;
	}
	
	osync_message_ref(message);
	g_async_queue_push(queue->outgoing, message);
	return TRUE;
}
