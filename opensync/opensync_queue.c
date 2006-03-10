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

#include <sys/time.h>
#include <signal.h>

/**
 * @ingroup OSEngineQueue
 * @brief A Queue used for asynchronous communication between thread
 * 
 */

/*@{*/

static
gboolean _incoming_prepare(GSource *source, gint *timeout_)
{
	*timeout_ = 1;
	return FALSE;
}

static
gboolean _incoming_check(GSource *source)
{
	OSyncQueue *queue = *((OSyncQueue **)(source + 1));
	if (g_async_queue_length(queue->incoming) > 0)
		return TRUE;
	return FALSE;
}

static
gboolean _incoming_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, user_data);
	OSyncQueue *queue = user_data;

	OSyncMessage *message = osync_queue_get_message(queue);
	osync_trace(TRACE_INTERNAL, "message %p refcount is %i", message, message->refCount);
	if (message) {
		osync_trace(TRACE_INTERNAL, "message cmd %i id %lli %i", message->cmd, message->id1, message->id2);
		if (message->cmd == OSYNC_MESSAGE_REPLY || message->cmd == OSYNC_MESSAGE_ERRORREPLY) {
			GList *p = NULL;
			for (p = queue->pendingReplies; p; p = p->next) {
				OSyncMessage *pending = p->data;
				osync_trace(TRACE_INTERNAL, "Still pending is %lli %i", pending->id1, pending->id2);
				if (pending->id1 == message->id1 && pending->id2 == message->id2) {
					/* Found the pending reply */
					if (!pending->callback) {
						osync_message_unref(message);
						osync_message_unref(pending);
						osync_trace(TRACE_EXIT_ERROR, "%s: Pending message does not have a callback", __func__);
						return TRUE;
					}
					
					osync_trace(TRACE_INTERNAL, "%p handler to2 %p", pending, pending->user_data);
					osync_trace(TRACE_INTERNAL, "calling reply callback %p %p", message, pending->user_data);
					pending->callback(message, pending->user_data);
					osync_trace(TRACE_INTERNAL, "done calling reply callback");
					
					queue->pendingReplies = g_list_remove(queue->pendingReplies, pending);
					osync_message_unref(message);
					osync_message_unref(pending);
					osync_trace(TRACE_EXIT, "%s: Done dispatching reply", __func__);
					return TRUE;
				}
			}
			osync_trace(TRACE_INTERNAL, "Unable to find pending message for id %lli %i\n", message->id1, message->id2);
		} else {
			if (!queue->message_handler) {
				osync_trace(TRACE_INTERNAL, "you have to setup a message handler for the queue!");
				osync_message_unref(message);
				osync_trace(TRACE_EXIT_ERROR, "%s: you have to setup a message handler for the queue", __func__);
				return FALSE;
			}
			queue->message_handler(message, queue->user_data);
			osync_trace(TRACE_INTERNAL, "message %p refcount2 is %i", message, message->refCount);
			osync_message_unref(message);
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: Done dispatching", __func__);
	return TRUE;
}

static
gboolean _queue_prepare(GSource *source, gint *timeout_)
{
	*timeout_ = 1;
	return FALSE;
}

static
gboolean _queue_check(GSource *source)
{
	OSyncQueue *queue = *((OSyncQueue **)(source + 1));
	if (g_async_queue_length(queue->outgoing) > 0)
		return TRUE;
	return FALSE;
}

static
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

static
osync_bool _osync_queue_write_long_long_int(OSyncQueue *queue, const long long int message)
{
	if (_osync_queue_write_data(queue, &message, sizeof(long long int)) < 0)
		return FALSE;

	return TRUE;
}

static
osync_bool _osync_queue_write_int(OSyncQueue *queue, const int message)
{
	if (_osync_queue_write_data(queue, &message, sizeof(int)) < 0)
		return FALSE;

	return TRUE;
}

static
gboolean _queue_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
	OSyncQueue *queue = user_data;

	OSyncMessage *message = g_async_queue_try_pop(queue->outgoing);
	if (message) {
		if (!_osync_queue_write_int(queue, message->buffer->len + osync_marshal_get_size_message(message)))
			goto error;
		
		if (!_osync_queue_write_int(queue, message->cmd))
			goto error;
		
		if (!_osync_queue_write_long_long_int(queue, message->id1))
			goto error;
			
		if (!_osync_queue_write_int(queue, message->id2))
			goto error;
			
		int sent = 0;
		do {
			sent += _osync_queue_write_data(queue, message->buffer->data + sent, message->buffer->len - sent);
		} while (sent < message->buffer->len);
		
		osync_message_unref(message);
	}
	return TRUE;
error:
	/*FIXME: how to handle errors here? */
	osync_trace(TRACE_INTERNAL, "%s: error sending message", __func__);
	return TRUE;
}

static
gboolean _source_prepare(GSource *source, gint *timeout_)
{
	*timeout_ = 1;
	return FALSE;
}

/* Read "n" bytes from a descriptor. */
static
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

static
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

static
osync_bool _osync_queue_read_long_long_int(OSyncQueue *queue, long long int *message)
{
  int status;

  if ( (status = _osync_queue_read_data(queue, message, sizeof(long long int))) < 0) {
    return FALSE;
  }

  if (status < sizeof(long long int)) {
    return FALSE;
  }

  return TRUE;
}

static
gboolean _source_check(GSource *source)
{
	OSyncQueue *queue = *((OSyncQueue **)(source + 1));
	if (osync_queue_data_available(queue))
		return TRUE;
	return FALSE;
}

static
gboolean _source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
	OSyncQueue *queue = user_data;
	OSyncMessage *message = NULL;
	OSyncError *error = NULL;
	
	if (osync_queue_data_available(queue)) {
		int size = 0;
		int cmd = 0;
		long long int id1 = 0;
		int id2 = 0;
		if (!_osync_queue_read_int(queue, &size))
			return FALSE;
		
		if (!_osync_queue_read_int(queue, &cmd))
			return FALSE;
		
		if (!_osync_queue_read_long_long_int(queue, &id1))
			return FALSE;
		if (!_osync_queue_read_int(queue, &id2))
			return FALSE;
		
		message = osync_message_new(cmd, size, &error);
		if (!message) {
			osync_error_free(&error);
			return FALSE;
		}
		message->id1 = id1;
		message->id2 = id2;
		
		int read = 0;
		do {
			read += _osync_queue_read_data(queue, message->buffer->data + read, size - read);
		} while (read < size);
		
		g_async_queue_push(queue->incoming, message);
	}
	
	return TRUE;
}

osync_bool osync_queue_start_thread(OSyncQueue *queue, OSyncError **error)
{
	signal(SIGPIPE, SIG_IGN);
	
	queue->thread = osync_thread_new(queue->context, error);

	if (!queue->thread)
		goto error;
	
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
	
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/*! @brief Creates a new asynchronous queue
 * 
 * This function return the pointer to a newly created OSyncQueue
 * 
 */
OSyncQueue *osync_queue_new(const char *name, osync_bool run, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, name, error);
	
	OSyncQueue *queue = osync_try_malloc0(sizeof(OSyncQueue), error);
	if (!queue)
		goto error;
	
	queue->name = g_strdup(name);
	queue->fd = -1;
	
	if (!g_thread_supported ())
		g_thread_init (NULL);
	
	queue->context = g_main_context_new();
	
	queue->incoming = g_async_queue_new();
	g_async_queue_ref(queue->incoming);
	queue->outgoing = g_async_queue_new();
	g_async_queue_ref(queue->outgoing);
	
	if (run) {
		if (!osync_queue_start_thread(queue, error))
			goto error_free_queue;
	}

	osync_trace(TRACE_EXIT, "%s: %p", __func__, queue);
	return queue;

error_free_queue:
	osync_queue_free(queue);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void osync_queue_stop_thread(OSyncQueue *queue)
{
	if (queue->thread) {
		osync_thread_stop(queue->thread);
		osync_thread_free(queue->thread);
		queue->thread = NULL;
	}
}

void osync_queue_free(OSyncQueue *queue)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, queue);
	
	osync_queue_stop_thread(queue);

	if (queue->source)
		g_source_destroy(queue->source);

	if (queue->name)
		g_free(queue->name);
		
	g_free(queue);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool osync_queue_exists(OSyncQueue *queue)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, queue);
	
	osync_bool ret = g_file_test(queue->name, G_FILE_TEST_EXISTS) ? TRUE : FALSE;
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

osync_bool osync_queue_create(OSyncQueue *queue, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, queue, error);
	
	if (mkfifo(queue->name, 0600) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to create fifo");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool osync_queue_connect(OSyncQueue *queue, int flags, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, queue, error);
	
	int fd = open(queue->name, flags);
	if (fd == -1) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open fifo");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	queue->fd = fd;

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

	osync_trace(TRACE_EXIT, "%s", __func__);
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
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, queue, handler, user_data);
	
	queue->message_handler = handler;
	queue->user_data = user_data;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
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
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, queue, context);
	
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
	
	osync_trace(TRACE_EXIT, "%s", __func__);
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
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, queue, error);
	
	if (unlink(queue->name) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to remove queue");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool osync_queue_disconnect(OSyncQueue *queue, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, queue, error);
	
	if (close(queue->fd) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to close queue");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

void gen_id(long long int *part1, int *part2)
{
	struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);

    long long int now = tv.tv_sec * 1000000 + tv.tv_usec;
    
    int rnd = (int)random();
    rnd = rnd << 16 | getpid();
    
    *part1 = now;
    *part2 = rnd;
}

osync_bool osync_queue_send_message(OSyncQueue *queue, OSyncQueue *replyqueue, OSyncMessage *message, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, queue, replyqueue, message, error);
	
	if (queue->error) {
		osync_error_duplicate(error, &(queue->error));
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	if (message->callback) {
		gen_id(&(message->id1), &(message->id2));
		
		osync_message_ref(message);
		replyqueue->pendingReplies = g_list_append(replyqueue->pendingReplies, message);
	}
	
	osync_message_ref(message);
	g_async_queue_push(queue->outgoing, message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool osync_queue_send_message_with_timeout(OSyncQueue *queue, OSyncQueue *replyqueue, OSyncMessage *message, int timeout, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, queue, message, error);
	
	if (queue->error) {
		osync_error_duplicate(error, &(queue->error));
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_bool ret = osync_queue_send_message(queue, replyqueue, message, error);
	osync_trace(ret ? TRACE_EXIT : TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return ret;
}

osync_bool osync_queue_is_alive(OSyncQueue *queue)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, queue);
	
	if (!osync_queue_connect(queue, O_WRONLY | O_NONBLOCK, NULL)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to connect", __func__);
		return FALSE;
	}
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_NOOP, 0, NULL);
	if (!message) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to create new message", __func__);
		return FALSE;
	}
	
	if (!osync_queue_send_message(queue, NULL, message, NULL)) {
		osync_trace(TRACE_EXIT, "%s: Not alive", __func__);
		return FALSE;
	}
	
	osync_queue_disconnect(queue, NULL);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}
