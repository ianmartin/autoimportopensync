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
 
#ifndef OPENSYNC_SINK_ENGINE_H_
#define OPENSYNC_SINK_ENGINE_H_

typedef struct OSyncSinkEngine OSyncSinkEngine;

typedef void (* OSyncSinkEngineEventCallback) (OSyncSinkEngine *engine, OSyncEngineEvent event, OSyncError *error, void *userdata);

OSyncSinkEngine *osync_sink_engine_new(OSyncClientProxy *proxy, const char *objtype, OSyncError **error);
void osync_sink_engine_ref(OSyncSinkEngine *engine);
void osync_sink_engine_unref(OSyncSinkEngine *engine);

void osync_sink_engine_event(OSyncSinkEngine *engine, OSyncEngineEvent event);
void osync_sink_engine_set_callback(OSyncSinkEngine *engine, OSyncSinkEngineEventCallback callback, void *userdata);

#endif /*OPENSYNC_SINK_ENGINE_H_*/
