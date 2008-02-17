/*
 * libopensync - A synchronization engine for the opensync framework
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

#ifndef _OPENSYNC_QUEUE_H
#define _OPENSYNC_QUEUE_H

/**
 * @ingroup OSyncQueue
 * @brief A Queue used for asynchronous communication between thread
 */

/*@{*/


/*! @brief The type of a queue event 
 * 
 */

typedef enum {
	OSYNC_QUEUE_EVENT_NONE,
	OSYNC_QUEUE_EVENT_READ,
	OSYNC_QUEUE_EVENT_ERROR,
	OSYNC_QUEUE_EVENT_HUP
} OSyncQueueEvent;


/*! @brief The queue type 
 * 
 */

typedef enum {
	OSYNC_QUEUE_SENDER,
	OSYNC_QUEUE_RECEIVER
} OSyncQueueType;

/*@}*/

OSYNC_EXPORT OSyncQueue *osync_queue_new(const char *name, OSyncError **error);
OSYNC_EXPORT OSyncQueue *osync_queue_new_from_fd(int fd, OSyncError **error);
OSYNC_EXPORT osync_bool osync_queue_new_pipes(OSyncQueue **read_queue, OSyncQueue **write_queue, OSyncError **error);
OSYNC_EXPORT osync_bool osync_queue_create(OSyncQueue *queue, OSyncError **error);

OSYNC_EXPORT void osync_queue_free(OSyncQueue *queue);
OSYNC_EXPORT osync_bool osync_queue_remove(OSyncQueue *queue, OSyncError **error);
OSYNC_EXPORT osync_bool osync_queue_exists(OSyncQueue *queue);

OSYNC_EXPORT osync_bool osync_queue_connect(OSyncQueue *queue, OSyncQueueType type, OSyncError **error);
OSYNC_EXPORT osync_bool osync_queue_disconnect(OSyncQueue *queue, OSyncError **error);
OSYNC_EXPORT osync_bool osync_queue_is_connected(OSyncQueue *queue);

OSYNC_EXPORT void osync_queue_set_message_handler(OSyncQueue *queue, OSyncMessageHandler handler, gpointer user_data);
OSYNC_EXPORT osync_bool osync_queue_send_message(OSyncQueue *queue, OSyncQueue *replyqueue, OSyncMessage *message, OSyncError **error);
OSYNC_EXPORT osync_bool osync_queue_send_message_with_timeout(OSyncQueue *queue, OSyncQueue *replyqueue, OSyncMessage *message, unsigned int timeout, OSyncError **error);

OSYNC_EXPORT void osync_queue_setup_with_gmainloop(OSyncQueue *queue, GMainContext *context);
OSYNC_EXPORT osync_bool osync_queue_dispatch(OSyncQueue *queue, OSyncError **error);

OSYNC_EXPORT OSyncQueueEvent osync_queue_poll(OSyncQueue *queue);

OSYNC_EXPORT OSyncMessage *osync_queue_get_message(OSyncQueue *queue);
OSYNC_EXPORT const char *osync_queue_get_path(OSyncQueue *queue);
OSYNC_EXPORT int osync_queue_get_fd(OSyncQueue *queue);

OSYNC_EXPORT osync_bool osync_queue_is_alive(OSyncQueue *queue);

#endif //_OPENSYNC_QUEUE_H

