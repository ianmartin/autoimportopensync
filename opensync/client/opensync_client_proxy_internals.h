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
 
#ifndef OSYNC_CLIENT_PROXY_INTERNALS_H_
#define OSYNC_CLIENT_PROXY_INTERNALS_H_

struct OSyncClientProxy {
	int ref_count;
	
	char *path;
	OSyncQueue *incoming;
	OSyncQueue *outgoing;
	pid_t child_pid;
	OSyncClient *client;
	OSyncStartType type;
	OSyncFormatEnv *formatenv;
	
	osync_bool has_main_sink;
	GList *objtypes;
	
	GMainContext *context;
	
	change_cb change_callback;
	void *change_callback_data;
};

#endif /*OSYNC_CLIENT_PROXY_INTERNALS_H_*/
