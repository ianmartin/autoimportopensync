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
 
#ifndef _OPENSYNC_SUPPORT_INTERNALS_H
#define _OPENSYNC_SUPPORT_INTERNALS_H

typedef struct OSyncThread {
	GThread *thread;
	GCond *started;
	GMutex *started_mutex;
	GMainContext *context;
	GMainLoop *loop;
} OSyncThread;

OSyncThread *osync_thread_new(GMainContext *context, OSyncError **error);
void osync_thread_free(OSyncThread *thread);
void osync_thread_start(OSyncThread *thread);
void osync_thread_stop(OSyncThread *thread);
void osync_thread_exit(OSyncThread *thread, int retval);

OSyncThread *osync_thread_create(GThreadFunc func, void *userdata, OSyncError **error);

int osync_bitcount(unsigned int u);

#endif //_OPENSYNC_SUPPORT_INTERNALS_H
