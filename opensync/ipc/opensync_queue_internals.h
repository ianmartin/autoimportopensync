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

#ifndef _OPENSYNC_QUEUE_INTERNALS_H
#define _OPENSYNC_QUEUE_INTERNALS_H

OSYNC_TEST_EXPORT osync_bool osync_queue_new_pipes(OSyncQueue **read_queue, OSyncQueue **write_queue, OSyncError **error);
OSYNC_TEST_EXPORT osync_bool osync_queue_remove(OSyncQueue *queue, OSyncError **error);

osync_bool osync_queue_exists(OSyncQueue *queue);

OSYNC_TEST_EXPORT osync_bool osync_queue_is_connected(OSyncQueue *queue);

OSYNC_TEST_EXPORT void osync_queue_set_message_handler(OSyncQueue *queue, OSyncMessageHandler handler, gpointer user_data);
OSYNC_TEST_EXPORT osync_bool osync_queue_send_message(OSyncQueue *queue, OSyncQueue *replyqueue, OSyncMessage *message, OSyncError **error);
OSYNC_TEST_EXPORT osync_bool osync_queue_send_message_with_timeout(OSyncQueue *queue, OSyncQueue *replyqueue, OSyncMessage *message, unsigned int timeout, OSyncError **error);

OSYNC_TEST_EXPORT void osync_queue_setup_with_gmainloop(OSyncQueue *queue, GMainContext *context);
osync_bool osync_queue_dispatch(OSyncQueue *queue, OSyncError **error);

OSyncQueueEvent osync_queue_poll(OSyncQueue *queue);

OSYNC_TEST_EXPORT OSyncMessage *osync_queue_get_message(OSyncQueue *queue);
const char *osync_queue_get_path(OSyncQueue *queue);
int osync_queue_get_fd(OSyncQueue *queue);

osync_bool osync_queue_is_alive(OSyncQueue *queue);

#endif /* _OPENSYNC_QUEUE_INTERNALS_H */

