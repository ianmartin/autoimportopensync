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
		g_free(engine);
	}
}

static void connect_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncSinkEngine *engine = userdata;
	if (!error)
		osync_sink_engine_event(engine, OSYNC_SINK_EVENT_CONNECTED);
	else
		osync_sink_engine_event(engine, OSYNC_SINK_EVENT_ERROR);
}

void osync_sink_engine_raise(OSyncSinkEngine *engine, OSyncSinkEvent event, OSyncError *error)
{
	
}

void osync_sink_engine_event(OSyncSinkEngine *engine, OSyncSinkEvent event)
{
	OSyncError *error = NULL;
	osync_assert(engine);
	
	switch (event) {
		case OSYNC_SINK_EVENT_START:
			if (engine->state == OSYNC_ENGINE_STATE_WAITING) {
				if (!osync_client_proxy_connect(engine->proxy, connect_callback, engine, engine->objtype, &error))
					goto error;
			} else {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Invalid state transistation: %i %i", engine->state, event);
				goto error;
			}
			break;
		case OSYNC_SINK_EVENT_CONNECTED:
		case OSYNC_SINK_EVENT_ERROR:
			break;
	}
	

error:
	osync_sink_engine_raise(engine, OSYNC_SINK_EVENT_ERROR, error);
	osync_error_unref(&error);
}


