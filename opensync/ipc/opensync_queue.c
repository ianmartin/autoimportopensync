/*
 * libosengine - A synchronization engine for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007  Daniel Friedrich <daniel.friedrich@opensync.org>
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

#include "opensync_message.h"
#include "opensync_queue.h"

#include "opensync_queue_internals.h"
#include "opensync_queue_private.h"

/**
 * @ingroup OSyncQueue
 * @brief A Queue used for asynchronous communication between thread
 * 
 */

/*@{*/

static
gboolean _timeout_prepare(GSource *source, gint *timeout_)
{
  /* TODO adapt *timeout_ value to shortest message timeout value...
     GTimeVal current_time;
     g_source_get_current_time(source, &current_time);
  */

  *timeout_ = 1;
  return FALSE;
}

static
gboolean _timeout_check(GSource *source)
{
  GList *p;
  GTimeVal current_time;
  OSyncTimeoutInfo *toinfo;
  OSyncPendingMessage *pending;

  OSyncQueue *queue = *((OSyncQueue **)(source + 1));

  g_source_get_current_time(source, &current_time);

  /* Search for the pending reply. We have to lock the
   * list since another thread might be duing the updates */
  g_mutex_lock(queue->pendingLock);

  for (p = queue->pendingReplies; p; p = p->next) {
    pending = p->data;

    if (!pending->timeout_info)
      continue;

    toinfo = pending->timeout_info;

    if (current_time.tv_sec >= toinfo->expiration.tv_sec 
        || (current_time.tv_sec == toinfo->expiration.tv_sec 
            && current_time.tv_usec >= toinfo->expiration.tv_usec)) {
      /* Unlock the pending lock since the messages might be sent during the callback */
      g_mutex_unlock(queue->pendingLock);

      return TRUE;
    }
  }
  /* Unlock the pending lock since the messages might be sent during the callback */
  g_mutex_unlock(queue->pendingLock);

  return FALSE;
}

static
gboolean _timeout_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
  GList *p;
  OSyncPendingMessage *pending;
  OSyncTimeoutInfo *toinfo;
  OSyncQueue *queue = NULL;
  GTimeVal current_time;

  osync_trace(TRACE_INTERNAL, "%s(%p)", __func__, user_data);

  queue = *((OSyncQueue **)(source + 1));

  g_source_get_current_time (source, &current_time);

  /* Search for the pending reply. We have to lock the
   * list since another thread might be duing the updates */
  g_mutex_lock(queue->pendingLock);

  for (p = queue->pendingReplies; p; p = p->next) {
    pending = p->data;

    if (!pending->timeout_info)
      continue;

    toinfo = pending->timeout_info;

    if (current_time.tv_sec == toinfo->expiration.tv_sec ||
        (current_time.tv_sec >= toinfo->expiration.tv_sec 
         && current_time.tv_usec >= toinfo->expiration.tv_usec)) {
      OSyncError *error = NULL;
      OSyncError *timeouterr = NULL;
      OSyncMessage *errormsg = NULL;

      /* Call the callback of the pending message */
      osync_assert(pending->callback);
      osync_error_set(&timeouterr, OSYNC_ERROR_IO_ERROR, "Timeout.");
      errormsg = osync_message_new_errorreply(NULL, timeouterr, &error);
      osync_error_unref(&timeouterr);

      /* Remove first the pending message!
         To avoid that _incoming_dispatch catchs this message
         when we're releasing the lock. If _incoming_dispatch
         would catch this message, the pending callback
         gets called twice! */

      queue->pendingReplies = g_list_remove(queue->pendingReplies, pending);
      /* Unlock the pending lock since the messages might be sent during the callback */
      g_mutex_unlock(queue->pendingLock);

      pending->callback(errormsg, pending->user_data);
      if (errormsg != NULL)
        osync_message_unref(errormsg);

      // TODO: Refcounting for OSyncPendingMessage
      g_free(pending->timeout_info);
      g_free(pending);

      /* Lock again, to keep the iteration of the pendingReplies list atomic. */
      g_mutex_lock(queue->pendingLock);

      break;
    }
  }
	
  g_mutex_unlock(queue->pendingLock);

  return TRUE;
}

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

/* This function is called from the master thread. The function dispatched incoming data from
 * the remote end */
static
gboolean _incoming_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
  OSyncPendingMessage *pending = NULL;
  OSyncQueue *queue = user_data;
  GList *p = NULL;
  OSyncMessage *message = NULL;
	
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, user_data);

  while ((message = g_async_queue_try_pop(queue->incoming))) {
    /* We check if the message is a reply to something */
    osync_trace(TRACE_INTERNAL, "Dispatching %p:%i(%s)", message, osync_message_get_cmd(message), osync_message_get_commandstr(message));
		
    if (osync_message_get_cmd(message) == OSYNC_MESSAGE_REPLY || osync_message_get_cmd(message) == OSYNC_MESSAGE_ERRORREPLY) {
			
      /* Search for the pending reply. We have to lock the
       * list since another thread might be duing the updates */
      g_mutex_lock(queue->pendingLock);
			
      for (p = queue->pendingReplies; p; p = p->next) {
        pending = p->data;
			
        if (pending->id == osync_message_get_id(message)) {

          /* Remove first the pending message!
             To avoid that _timeout_dispatch catchs this message
             when we're releasing the lock. If _timeout_dispatch
             would catch this message, the pending callback
             gets called twice! */

          queue->pendingReplies = g_list_remove(queue->pendingReplies, pending);

          /* Unlock the pending lock since the messages might be sent during the callback */
          g_mutex_unlock(queue->pendingLock);
			
          /* Call the callback of the pending message */
          osync_assert(pending->callback);
          pending->callback(message, pending->user_data);

          // TODO: Refcounting for OSyncPendingMessage
          if (pending->timeout_info)
            g_free(pending->timeout_info);
          g_free(pending);

          /* Lock again, to keep the iteration of the pendingReplies list atomic. */
          g_mutex_lock(queue->pendingLock);
          break;
        }
      }
			
      g_mutex_unlock(queue->pendingLock);
    } else 
      queue->message_handler(message, queue->user_data);
		
    osync_message_unref(message);
  }
	
  osync_trace(TRACE_EXIT, "%s: Done dispatching", __func__);
  return TRUE;
}

static void _osync_queue_stop_incoming(OSyncQueue *queue)
{
  if (queue->incoming_source) {
    g_source_destroy(queue->incoming_source);
    queue->incoming_source = NULL;
  }
	
  if (queue->incomingContext) {
    g_main_context_unref(queue->incomingContext);
    queue->incomingContext = NULL;
  }
	
  if (queue->incoming_functions) {
    g_free(queue->incoming_functions);
    queue->incoming_functions = NULL;
  }
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

static int _osync_queue_write_data(OSyncQueue *queue, const void *vptr, size_t n, OSyncError **error)
{
#ifdef _WIN32
  return FALSE;
#else //_WIN32

  ssize_t nwritten = 0;

  while (n > 0) {
    if ((nwritten = write(queue->fd, vptr, n)) <= 0) {
      if (errno == EINTR)
        nwritten = 0;  /* and call write() again */
      else {
        osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to write IPC data: %i: %s", errno, g_strerror(errno));
        return (-1);  /* error */
      }
    }
		
    n -= nwritten;
    vptr += nwritten;
  }
  return (nwritten);
#endif //_WIN32
}

static osync_bool _osync_queue_write_long_long_int(OSyncQueue *queue, const long long int message, OSyncError **error)
{
  if (_osync_queue_write_data(queue, &message, sizeof(long long int), error) < 0)
    return FALSE;

  return TRUE;
}

static osync_bool _osync_queue_write_int(OSyncQueue *queue, const int message, OSyncError **error)
{
  if (_osync_queue_write_data(queue, &message, sizeof(int), error) < 0)
    return FALSE;

  return TRUE;
}

/* This function sends the data to the remote side. If there is an error, it sends an error
 * message to the incoming queue */
static
gboolean _queue_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
  OSyncQueue *queue = user_data;
  OSyncError *error = NULL;
	
  OSyncMessage *message = NULL;
	
  while ((message = g_async_queue_try_pop(queue->outgoing))) {
    char *data = NULL;
    unsigned int length = 0;

    /* Check if the queue is connected */
    if (!queue->connected) {
      osync_error_set(&error, OSYNC_ERROR_GENERIC, "Trying to send to a queue thats not connected");
      goto error;
    }
		
    /* The size of the message */
    if (!_osync_queue_write_int(queue, osync_message_get_message_size(message), &error))
      goto error;
		
    /* The command type (integer) */
    if (!_osync_queue_write_int(queue, osync_message_get_cmd(message), &error))
      goto error;

    /* The id (long long int) */
    if (!_osync_queue_write_long_long_int(queue, osync_message_get_id(message), &error))
      goto error;
		
    osync_message_get_buffer(message, &data, &length);
		
    if (length) {
      int sent = 0;
      do {
        int written = _osync_queue_write_data(queue, data + sent, length - sent, &error);
        if (written < 0)
          goto error;
				
        sent += written;
      } while (sent < length);
    }
		
    osync_message_unref(message);
  }
	
  return TRUE;
	
 error:
  if (message)
    osync_message_unref(message);
	
  if (error) {
    message = osync_message_new_queue_error(error, NULL);
    if (message)
      g_async_queue_push(queue->incoming, message);
		
    osync_error_unref(&error);
  }
  return FALSE;
}

static
gboolean _source_prepare(GSource *source, gint *timeout_)
{
  *timeout_ = 1;
  return FALSE;
}

static
int _osync_queue_read_data(OSyncQueue *queue, void *vptr, size_t n, OSyncError **error)
{
#ifdef _WIN32
  return 0;
#else //_WIN32
  size_t nleft;
  ssize_t nread = 0;

  nleft = n;
  while (n > 0) {
    if ((nread = read(queue->fd, vptr, nleft)) < 0) {
      if (errno == EINTR)
        nread = 0;  /* and call read() again */
      else {
        osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to read IPC data: %i: %s", errno, g_strerror(errno));
        return (-1);
      }
    } else if (nread == 0)
      break;  /* EOF */
		
    nleft -= nread;
    vptr += nread;
  }
  return (n - nleft);  /* return >= 0 */
#endif //_WIN32
}

static
osync_bool _osync_queue_read_int(OSyncQueue *queue, int *message, OSyncError **error)
{
  int read = _osync_queue_read_data(queue, message, sizeof(int), error);
	
  if (read < 0)
    return FALSE;

  if (read != sizeof(int)) {
    osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to read int. EOF");
    return FALSE;
  }
	
  return TRUE;
}

static
osync_bool _osync_queue_read_long_long_int(OSyncQueue *queue, long long int *message, OSyncError **error)
{
  int read = _osync_queue_read_data(queue, message, sizeof(long long int), error);

  if (read < 0)
    return FALSE;
	
  if (read != sizeof(long long int)) {
    osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to read int. EOF");
    return FALSE;
  }

  return TRUE;
}

static
gboolean _source_check(GSource *source)
{
  OSyncQueue *queue = *((OSyncQueue **)(source + 1));
  OSyncMessage *message = NULL;
  OSyncError *error = NULL;
	
  if (queue->connected == FALSE) {
    /* Ok. so we arent connected. lets check if there are pending replies. We cannot
     * receive any data on the pipe, therefore, any pending replies will never
     * be answered. So we return error messages for all of them. */
    if (queue->pendingReplies) {
      GList *p = NULL;
      g_mutex_lock(queue->pendingLock);
      osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Broken Pipe");
      for (p = queue->pendingReplies; p; p = p->next) {
        OSyncPendingMessage *pending = p->data;
				
        message = osync_message_new_errorreply(NULL, error, NULL);
        if (message) {
          osync_message_set_id(message, pending->id);
					
          g_async_queue_push(queue->incoming, message);
        }
      }
			
      osync_error_unref(&error);
      g_mutex_unlock(queue->pendingLock);
    }
		
    return FALSE;
  }
	
  switch (osync_queue_poll(queue)) {
  case OSYNC_QUEUE_EVENT_NONE:
    return FALSE;
  case OSYNC_QUEUE_EVENT_READ:
    return TRUE;
  case OSYNC_QUEUE_EVENT_HUP:
  case OSYNC_QUEUE_EVENT_ERROR:
    queue->connected = FALSE;
			
    /* Now we can send the hup message, and wake up the consumer thread so
     * it can pickup the messages in the incoming queue */
    message = osync_message_new(OSYNC_MESSAGE_QUEUE_HUP, 0, &error);
    if (!message)
      goto error;
			
    g_async_queue_push(queue->incoming, message);
			
    if (queue->incomingContext)
      g_main_context_wakeup(queue->incomingContext);
    return FALSE;
  }
	
  return FALSE;

 error:
  message = osync_message_new_queue_error(error, NULL);
  if (message) 
    g_async_queue_push(queue->incoming, message);
	
  osync_error_unref(&error);
  return FALSE;
}

/* This function reads from the file descriptor and inserts incoming data into the
 * incoming queue */
static
gboolean _source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
  OSyncQueue *queue = user_data;
  OSyncMessage *message = NULL;
  OSyncError *error = NULL;
	
  do {
    int size = 0;
    int cmd = 0;
    long long int id = 0;
    char *buffer = NULL;
		
    /* The size of the buffer */
    if (!_osync_queue_read_int(queue, &size, &error))
      goto error;
		
    /* The command */
    if (!_osync_queue_read_int(queue, &cmd, &error))
      goto error;
		
    /* The id */
    if (!_osync_queue_read_long_long_int(queue, &id, &error))
      goto error;
		
    message = osync_message_new(cmd, size, &error);
    if (!message)
      goto error;
	
    osync_message_set_id(message, id);
		
    /* We now get the buffer from the message which will already
     * have the correct size for the read */
    osync_message_get_buffer(message, &buffer, NULL);
    if (size) {
      unsigned int read = 0;
      do {
        int inc = _osync_queue_read_data(queue, buffer + read, size - read, &error);
				
        if (inc < 0)
          goto error_free_message;
				
        if (inc == 0) {
          osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "Encountered EOF while data was missing");
          goto error_free_message;
        }
				
        read += inc;
      } while (read < size);
    }
    osync_message_set_message_size(message, size);
		
    g_async_queue_push(queue->incoming, message);
		
    if (queue->incomingContext)
      g_main_context_wakeup(queue->incomingContext);
  } while (_source_check(queue->read_source));
	
  return TRUE;

 error_free_message:
  osync_message_unref(message);
 error:
  if (error) {
    message = osync_message_new_queue_error(error, NULL);
    if (message)
      g_async_queue_push(queue->incoming, message);
		
		
    osync_error_unref(&error);
  }
	
  return FALSE;
}


/*! @brief Flush all message of the Queue 
 * 
 * Flush all message inside the Queue and dereference the messages.
 *
 * @param queue The queue to flush 
 * 
 */

static void _osync_queue_flush_messages(GAsyncQueue *queue)
{
  OSyncMessage *message;
  osync_assert(queue);

  g_async_queue_lock(queue);

  while ((message = g_async_queue_try_pop_unlocked(queue)))
    osync_message_unref(message);

  g_async_queue_unlock(queue);
}

/*! @brief Creates a new asynchronous queue
 * 
 * This function return the pointer to a newly created OSyncQueue
 * 
 */

OSyncQueue *osync_queue_new(const char *name, OSyncError **error)
{
  OSyncQueue *queue = NULL;
  osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, name, error);
	
  queue = osync_try_malloc0(sizeof(OSyncQueue), error);
  if (!queue)
    goto error;
	
  if (name)
    queue->name = g_strdup(name);
  queue->fd = -1;
	
  if (!g_thread_supported ())
    g_thread_init (NULL);
	
  queue->pendingLock = g_mutex_new();
	
  queue->context = g_main_context_new();
	
  queue->outgoing = g_async_queue_new();
  queue->incoming = g_async_queue_new();

  queue->disconnectLock = g_mutex_new();

  osync_trace(TRACE_EXIT, "%s: %p", __func__, queue);
  return queue;

 error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return NULL;
}

/*! @brief Creates a new asynchronous queue
 * 
 * This function return the pointer to a newly created OSyncQueue
 * 
 */
OSyncQueue *osync_queue_new_from_fd(int fd, OSyncError **error)
{
  OSyncQueue *queue = NULL;
  osync_trace(TRACE_ENTRY, "%s(%i, %p)", __func__, fd, error);
	
  queue = osync_queue_new(NULL, error);
  if (!queue)
    goto error;
	
  queue->fd = fd;

  osync_trace(TRACE_EXIT, "%s: %p", __func__, queue);
  return queue;

 error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return NULL;
}

/* Creates anonymous pipes which dont have to be created and are automatically connected.
 * 
 * Lets assume parent wants to send, child wants to receive
 * 
 * osync_queue_new_pipes()
 * fork()
 * 
 * Parent:
 * connect(write_queue)
 * disconnect(read_queue)
 * 
 * Child:
 * connect(read_queue)
 * close(write_queue)
 * 
 * 
 *  */
osync_bool osync_queue_new_pipes(OSyncQueue **read_queue, OSyncQueue **write_queue, OSyncError **error)
{
#ifdef _WIN32
  return FALSE;
#else
  int filedes[2];
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, read_queue, write_queue, error);
	
  if (pipe(filedes) < 0) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to create pipes");
    goto error;
  }
	
  *read_queue = osync_queue_new_from_fd(filedes[0], error);
  if (!*read_queue)
    goto error_close_pipes;
	
  *write_queue = osync_queue_new_from_fd(filedes[1], error);
  if (!*write_queue)
    goto error_free_read_queue;
	
  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;

 error_free_read_queue:
  osync_queue_free(*read_queue);
 error_close_pipes:
  close(filedes[0]);
  close(filedes[1]);
 error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return FALSE;
#endif
}

void osync_queue_free(OSyncQueue *queue)
{
  OSyncPendingMessage *pending = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, queue);
	
  g_mutex_free(queue->pendingLock);
	
  g_mutex_free(queue->disconnectLock);

  g_main_context_unref(queue->context);

  _osync_queue_stop_incoming(queue);

  _osync_queue_flush_messages(queue->incoming);
  g_async_queue_unref(queue->incoming);
	
  _osync_queue_flush_messages(queue->outgoing);
  g_async_queue_unref(queue->outgoing);

  while (queue->pendingReplies) {
    pending = queue->pendingReplies->data;

    queue->pendingReplies = g_list_remove(queue->pendingReplies, pending);

    // TODO: Refcounting for OSyncPendingMessage
    if (pending->timeout_info)
      g_free(pending->timeout_info);

    g_free(pending);
  }

  if (queue->name)
    g_free(queue->name);
		
  g_free(queue);
  queue = NULL;
	
  osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool osync_queue_exists(OSyncQueue *queue)
{
  osync_assert(queue);
  return g_file_test(queue->name, G_FILE_TEST_EXISTS) ? TRUE : FALSE;
}

osync_bool osync_queue_create(OSyncQueue *queue, OSyncError **error)
{
#ifdef _WIN32
  return FALSE;
#else //_WIN32
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, queue, error);
	
  if (mkfifo(queue->name, 0600) != 0) {
    if (errno != EEXIST) {
      osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to create fifo");
      osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
      return FALSE;
    }
  }
	
  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;
#endif //_WIN32
}

osync_bool osync_queue_remove(OSyncQueue *queue, OSyncError **error)
{
#ifdef _WIN32
  return FALSE;
#else //_WIN32
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, queue, error);

  if (queue->name && unlink(queue->name) != 0) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to remove queue");
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
    return FALSE;
  }
	
  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;
#endif
}

osync_bool osync_queue_connect(OSyncQueue *queue, OSyncQueueType type, OSyncError **error)
{
#ifdef _WIN32
  return FALSE;
#else //_WIN32
  OSyncQueue **queueptr = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, queue, type, error);
  osync_assert(queue);
  osync_assert(queue->connected == FALSE);
	
  queue->type = type;
	
  if (queue->fd == -1) {
    /* First, open the queue with the flags provided by the user */
    int fd = open(queue->name, type == OSYNC_QUEUE_SENDER ? O_WRONLY : O_RDONLY);
    if (fd == -1) {
      osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open fifo");
      goto error;
    }
    queue->fd = fd;
  }

  int oldflags = fcntl(queue->fd, F_GETFD);
  if (oldflags == -1) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get fifo flags");
    goto error_close;
  }
  if (fcntl(queue->fd, F_SETFD, oldflags|FD_CLOEXEC) == -1) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to set fifo flags");
    goto error_close;
  }

  queue->connected = TRUE;
  signal(SIGPIPE, SIG_IGN);
	
  /* now we start a thread which handles reading/writing of the queue */
  queue->thread = osync_thread_new(queue->context, error);

  if (!queue->thread)
    goto error;
	
  queue->write_functions = g_malloc0(sizeof(GSourceFuncs));
  queue->write_functions->prepare = _queue_prepare;
  queue->write_functions->check = _queue_check;
  queue->write_functions->dispatch = _queue_dispatch;
  queue->write_functions->finalize = NULL;

  queue->write_source = g_source_new(queue->write_functions, sizeof(GSource) + sizeof(OSyncQueue *));
  queueptr = (OSyncQueue **)(queue->write_source + 1);
  *queueptr = queue;
  g_source_set_callback(queue->write_source, NULL, queue, NULL);
  g_source_attach(queue->write_source, queue->context);
  if (queue->context)
    g_main_context_ref(queue->context);

  queue->read_functions = g_malloc0(sizeof(GSourceFuncs));
  queue->read_functions->prepare = _source_prepare;
  queue->read_functions->check = _source_check;
  queue->read_functions->dispatch = _source_dispatch;
  queue->read_functions->finalize = NULL;

  queue->read_source = g_source_new(queue->read_functions, sizeof(GSource) + sizeof(OSyncQueue *));
  queueptr = (OSyncQueue **)(queue->read_source + 1);
  *queueptr = queue;
  g_source_set_callback(queue->read_source, NULL, queue, NULL);
  g_source_attach(queue->read_source, queue->context);
  if (queue->context)
    g_main_context_ref(queue->context);

  queue->timeout_functions = g_malloc0(sizeof(GSourceFuncs));
  queue->timeout_functions->prepare = _timeout_prepare;
  queue->timeout_functions->check = _timeout_check;
  queue->timeout_functions->dispatch = _timeout_dispatch;
  queue->timeout_functions->finalize = NULL;

  queue->timeout_source = g_source_new(queue->timeout_functions, sizeof(GSource) + sizeof(OSyncQueue *));
  queueptr = (OSyncQueue **)(queue->timeout_source + 1);
  *queueptr = queue;
  g_source_set_callback(queue->timeout_source, NULL, queue, NULL);
  g_source_attach(queue->timeout_source, queue->context);
  if (queue->context)
    g_main_context_ref(queue->context);
	
  osync_thread_start(queue->thread);
	
  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;

 error_close:
  close(queue->fd);
 error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return FALSE;
#endif //_WIN32
}

osync_bool osync_queue_disconnect(OSyncQueue *queue, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, queue, error);
  osync_assert(queue);

  g_mutex_lock(queue->disconnectLock);
  if (queue->thread) {
    osync_thread_stop(queue->thread);
    osync_thread_free(queue->thread);
    queue->thread = NULL;
  }
	
  //g_source_unref(queue->write_source);
	
  if (queue->write_functions) {
    g_free(queue->write_functions);
    queue->write_functions = NULL;
  }
		
  //g_source_unref(queue->read_source);
	
  if (queue->timeout_functions) {
    g_free(queue->timeout_functions);
    queue->timeout_functions = NULL;
  }
		
  _osync_queue_stop_incoming(queue);
	
  /* We have to empty the incoming queue if we disconnect the queue. Otherwise, the
   * consumer threads might try to pick up messages even after we are done. */
  _osync_queue_flush_messages(queue->incoming);
	
  if (queue->fd != -1 && close(queue->fd) != 0) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to close queue");
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
    return FALSE;
  }
	
  queue->fd = -1;
  queue->connected = FALSE;
  g_mutex_unlock(queue->disconnectLock);
	
  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;
}

osync_bool osync_queue_is_connected(OSyncQueue *queue)
{
  osync_assert(queue);
  return queue->connected;
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
  OSyncQueue **queueptr = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, queue, context);
	
  queue->incoming_functions = g_malloc0(sizeof(GSourceFuncs));
  queue->incoming_functions->prepare = _incoming_prepare;
  queue->incoming_functions->check = _incoming_check;
  queue->incoming_functions->dispatch = _incoming_dispatch;
  queue->incoming_functions->finalize = NULL;

  queue->incoming_source = g_source_new(queue->incoming_functions, sizeof(GSource) + sizeof(OSyncQueue *));
  queueptr = (OSyncQueue **)(queue->incoming_source + 1);
  *queueptr = queue;
  g_source_set_callback(queue->incoming_source, NULL, queue, NULL);
  g_source_attach(queue->incoming_source, context);
  queue->incomingContext = context;
  // For the source
  if (context)
    g_main_context_ref(context);
	
  //To unref it later
  if (context)
    g_main_context_ref(context);
	
  osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool osync_queue_dispatch(OSyncQueue *queue, OSyncError **error)
{
  _incoming_dispatch(NULL, NULL, queue);
  return TRUE;
}

OSyncQueueEvent osync_queue_poll(OSyncQueue *queue)
{
#ifdef _WIN32
  return OSYNC_QUEUE_EVENT_ERROR;
#else //_WIN32
  struct pollfd pfd;
  pfd.fd = queue->fd;
  pfd.events = POLLIN;
	
  /* Here we poll on the queue. If we read on the queue, we either receive a 
   * POLLIN or POLLHUP. Since we cannot write to the queue, we can block pretty long here.
   * 
   * If we are sending, we can only receive a POLLERR which means that the remote side has
   * disconnected. Since we mainly dispatch the write IO, we dont want to block here. */
  int ret = poll(&pfd, 1, queue->type == OSYNC_QUEUE_SENDER ? 0 : 100);

  if (ret == 0) 
    return OSYNC_QUEUE_EVENT_NONE;	

  /* Ignore interrupts. */
  if  (ret < 0 && errno == EINTR) 
    return OSYNC_QUEUE_EVENT_NONE;

  if (ret < 0 )
    osync_trace(TRACE_ERROR, "queue poll failed - system error :%i %s", errno, strerror(errno));


  if (pfd.revents & POLLERR)
    return OSYNC_QUEUE_EVENT_ERROR;
  else if (pfd.revents & POLLHUP)
    return OSYNC_QUEUE_EVENT_HUP;
  else if (pfd.revents & POLLIN)
    return OSYNC_QUEUE_EVENT_READ;
		
  return OSYNC_QUEUE_EVENT_ERROR;
#endif //_WIN32
}

/** note that this function is blocking */
OSyncMessage *osync_queue_get_message(OSyncQueue *queue)
{
  return g_async_queue_pop(queue->incoming);
}

/* Ids are generated as follows:
 * 
 * the upper 6 bytes are the time in seconds and microseconds
 * 
 * the lower 2 bytes are a random number
 * 
 * */
static long long int gen_id(const GTimeVal *tv)
{
  long long int now = (tv->tv_sec * 1000000 + tv->tv_usec) << 16;
  long long int rnd = ((long long int)g_random_int()) & 0xFFFF;
    
  return now | rnd;
}

osync_bool osync_queue_send_message(OSyncQueue *queue, OSyncQueue *replyqueue, OSyncMessage *message, OSyncError **error)
{
  return osync_queue_send_message_with_timeout(queue, replyqueue, message, 0, error);
}

osync_bool osync_queue_send_message_with_timeout(OSyncQueue *queue, OSyncQueue *replyqueue, OSyncMessage *message, unsigned int timeout, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %u, %p)", __func__, queue, replyqueue, message, timeout, error);

  if (osync_message_get_handler(message)) {
    OSyncPendingMessage *pending = NULL;
    GTimeVal current_time;
    long long int id = 0;
    osync_assert(replyqueue);
    pending = osync_try_malloc0(sizeof(OSyncPendingMessage), error);
    if (!pending)
      goto error;

    /* g_sourcet_get_current_time used cached time ... hopefully faster then g_get_.._time() */
    g_source_get_current_time(queue->timeout_source, &current_time);

    id = gen_id(&current_time);
    osync_message_set_id(message, id);
    pending->id = id;
    osync_trace(TRACE_INTERNAL, "Setting id %lli for pending reply", id);

    if (timeout) {
      OSyncTimeoutInfo *toinfo = osync_try_malloc0(sizeof(OSyncTimeoutInfo), error);
      if (!toinfo)
        goto error;

      toinfo->expiration = current_time;
      toinfo->expiration.tv_sec += timeout;

      pending->timeout_info = toinfo;
    } else {
      osync_trace(TRACE_INTERNAL, "handler message got sent without timeout!: %s", osync_message_get_commandstr(message));
    }
		
    pending->callback = osync_message_get_handler(message);
    pending->user_data = osync_message_get_handler_data(message);
		
    g_mutex_lock(replyqueue->pendingLock);
    replyqueue->pendingReplies = g_list_append(replyqueue->pendingReplies, pending);
    g_mutex_unlock(replyqueue->pendingLock);
  }
	
  osync_message_ref(message);
  g_async_queue_push(queue->outgoing, message);

  g_main_context_wakeup(queue->context);

  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;

 error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return FALSE;
}

osync_bool osync_queue_is_alive(OSyncQueue *queue)
{
  OSyncMessage *message = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, queue);
	
  // FIXME
  /*if (!osync_queue_connect(queue, O_WRONLY | O_NONBLOCK, NULL)) {
    osync_trace(TRACE_EXIT_ERROR, "%s: Unable to connect", __func__);
    return FALSE;
    }*/
	
  message = osync_message_new(OSYNC_MESSAGE_NOOP, 0, NULL);
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

/*! @brief Get the path of the fifo for the Queue
 * 
 * Get the full path of the fifo for this Queue if fifos used.
 *
 * @param queue The queue to get the file descriptor
 * @returns The full path of the fifo or NULL if no fifos used 
 * 
 */
const char *osync_queue_get_path(OSyncQueue *queue)
{
  osync_assert(queue);
  return queue->name;
}

/*! @brief Get the pipe file descriptor of the Queue
 * 
 * Get the pipe file descriptor of this Queue if pipes are used.
 *
 * @param queue The queue to get the file descriptor
 * @returns The pipe file descriptor of the queue or -1 if no pipes used or set
 * 
 */
int osync_queue_get_fd(OSyncQueue *queue)
{
  osync_assert(queue);
  return queue->fd;
}

