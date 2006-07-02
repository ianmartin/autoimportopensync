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
 
#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-client.h"
#include "opensync-engine.h"
#include "opensync-group.h"
#include "opensync-data.h"

#include "opensync_sink_engine_internals.h"

OSyncSinkEngine *osync_sink_engine_new(OSyncClientProxy *proxy, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, proxy, objtype, error);
	
	OSyncSinkEngine *engine = osync_try_malloc0(sizeof(OSyncSinkEngine), error);
	if (!engine)
		goto error;
	engine->state = OSYNC_ENGINE_STATE_WAITING;
	engine->ref_count = 1;
	
	engine->proxy = proxy;
	osync_client_proxy_ref(proxy);
	
	engine->objtype = g_strdup(objtype);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, engine);
	return engine;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void osync_sink_engine_ref(OSyncSinkEngine *engine)
{
	osync_assert(engine);
	
	g_atomic_int_inc(&(engine->ref_count));
}

void osync_sink_engine_unref(OSyncSinkEngine *engine)
{
	osync_assert(engine);
		
	if (g_atomic_int_dec_and_test(&(engine->ref_count))) {
		if (engine->proxy)
			osync_client_proxy_unref(engine->proxy);
		
		if (engine->objtype)
			g_free(engine->objtype);
			
		g_free(engine);
	}
}

static void _sink_connect_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncSinkEngine *engine = userdata;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	if (!error) {
		osync_sink_engine_event(engine, OSYNC_ENGINE_EVENT_CONNECTED);
		engine->callback(engine, OSYNC_ENGINE_EVENT_CONNECTED, NULL, engine->callback_userdata);
	} else {
		osync_sink_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		engine->callback(engine, OSYNC_ENGINE_EVENT_ERROR, error, engine->callback_userdata);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _sink_disconnect_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncSinkEngine *engine = userdata;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	if (!error) {
		engine->callback(engine, OSYNC_ENGINE_EVENT_DISCONNECT, NULL, engine->callback_userdata);
	} else {
		osync_sink_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		engine->callback(engine, OSYNC_ENGINE_EVENT_ERROR, error, engine->callback_userdata);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _sink_read_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncSinkEngine *engine = userdata;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	if (!error) {
		engine->callback(engine, OSYNC_ENGINE_EVENT_READ, NULL, engine->callback_userdata);
	} else {
		osync_sink_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		engine->callback(engine, OSYNC_ENGINE_EVENT_ERROR, error, engine->callback_userdata);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _sink_change_callback(OSyncClientProxy *proxy, void *userdata, OSyncChange *change)
{
	OSyncSinkEngine *engine = userdata;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, change);
	
	if (!error) {
		engine->callback(engine, OSYNC_ENGINE_EVENT_READ, NULL, engine->callback_userdata);
	} else {
		osync_sink_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		engine->callback(engine, OSYNC_ENGINE_EVENT_ERROR, error, engine->callback_userdata);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _sink_committed_all_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncSinkEngine *engine = userdata;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	if (!error) {
		engine->callback(engine, OSYNC_ENGINE_EVENT_WRITE, NULL, engine->callback_userdata);
	} else {
		osync_sink_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		engine->callback(engine, OSYNC_ENGINE_EVENT_ERROR, error, engine->callback_userdata);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_sink_engine_set_callback(OSyncSinkEngine *engine, OSyncSinkEngineEventCallback callback, void *userdata)
{
	osync_assert(engine);
	engine->callback = callback;
	engine->callback_userdata = userdata;
}

void osync_sink_engine_event(OSyncSinkEngine *engine, OSyncEngineEvent event)
{
	OSyncError *error = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, engine, event);
	osync_assert(engine);
	
	switch (event) {
		case OSYNC_ENGINE_EVENT_CONNECT:
			if (!osync_client_proxy_connect(engine->proxy, _sink_connect_callback, engine, engine->objtype, &error))
				goto error;
			break;
		case OSYNC_ENGINE_EVENT_CONNECTED:
		case OSYNC_ENGINE_EVENT_ERROR:
			break;
		case OSYNC_ENGINE_EVENT_READ:
			if (!osync_client_proxy_get_changes(engine->proxy, _sink_read_callback, _sink_change_callback, engine, engine->objtype, &error))
				goto error;
			break;
		case OSYNC_ENGINE_EVENT_WRITE:
			if (!osync_client_proxy_committed_all(engine->proxy, _sink_committed_all_callback, engine, engine->objtype, &error))
				goto error;
			break;
		case OSYNC_ENGINE_EVENT_DISCONNECT:
			if (!osync_client_proxy_disconnect(engine->proxy, _sink_disconnect_callback, engine, engine->objtype, &error))
				goto error;
			break;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	engine->callback(engine, OSYNC_ENGINE_EVENT_ERROR, error, engine->callback_userdata);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}


