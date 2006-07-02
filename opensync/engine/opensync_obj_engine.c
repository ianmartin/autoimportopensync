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

#include "opensync-engine.h"
#include "opensync-client.h"
#include "opensync-data.h"

#include "opensync_obj_engine_internals.h"

static int BitCount(unsigned int u)                          
{
	unsigned int uCount = u - ((u >> 1) & 033333333333) - ((u >> 2) & 011111111111);
	return ((uCount + (uCount >> 3)) & 030707070707) % 63;
}

static int _get_proxy(OSyncObjEngine *engine, OSyncClientProxy *proxy)
{
	int i = 0;
	GList *p = NULL;
	for (p = engine->proxies; p; p = p->next) {
		if (p->data == proxy)
			break;
		i++;
	}
	return i;
}

static void _obj_engine_connect_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncObjEngine *engine = userdata;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	int i = _get_proxy(engine, proxy);
	
	if (error) {
		osync_obj_engine_set_error(engine, error);
		engine->sink_errors = engine->sink_errors | (0x1 << i);
	} else
		engine->sink_connects = engine->sink_connects | (0x1 << i);
			
	if (BitCount(engine->sink_errors | engine->sink_connects) == g_list_length(engine->proxies)) {
		if (BitCount(engine->sink_connects) < 2) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Less than 2 proxies are connected");
			osync_obj_engine_set_error(engine, locerror);
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_CONNECTED);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->sink_errors | engine->sink_connects));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _obj_engine_disconnect_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncObjEngine *engine = userdata;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	int i = _get_proxy(engine, proxy);
	
	if (error) {
		osync_obj_engine_set_error(engine, error);
		engine->sink_errors = engine->sink_errors | (0x1 << i);
	} else
		engine->sink_disconnects = engine->sink_disconnects | (0x1 << i);
			
	if (BitCount(engine->sink_errors | engine->sink_disconnects) == g_list_length(engine->proxies)) {
		if (BitCount(engine->sink_disconnects) < BitCount(engine->sink_connects)) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Less proxies disconnected than connected");
			osync_obj_engine_set_error(engine, locerror);
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_DISCONNECTED);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->sink_errors | engine->sink_disconnects));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _obj_engine_read_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncObjEngine *engine = userdata;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	int i = _get_proxy(engine, proxy);
	
	if (error) {
		osync_obj_engine_set_error(engine, error);
		engine->sink_errors = engine->sink_errors | (0x1 << i);
	} else
		engine->sink_get_changes = engine->sink_get_changes | (0x1 << i);
			
	if (BitCount(engine->sink_errors | engine->sink_get_changes) == g_list_length(engine->proxies)) {
		if (BitCount(engine->sink_get_changes) < BitCount(engine->sink_connects)) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Less proxies reported get_changes than connected");
			osync_obj_engine_set_error(engine, locerror);
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_READ);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->sink_errors | engine->sink_get_changes));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _obj_engine_change_callback(OSyncClientProxy *proxy, void *userdata, OSyncChange *change)
{
	//OSyncSinkEngine *engine = userdata;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, change);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _obj_engine_written_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncObjEngine *engine = userdata;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	int i = _get_proxy(engine, proxy);
	
	if (error) {
		osync_obj_engine_set_error(engine, error);
		engine->sink_errors = engine->sink_errors | (0x1 << i);
	} else
		engine->sink_written = engine->sink_written | (0x1 << i);
			
	if (BitCount(engine->sink_errors | engine->sink_written) == g_list_length(engine->proxies)) {
		if (BitCount(engine->sink_written) < BitCount(engine->sink_connects)) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Less proxies reported committed all than connected");
			osync_obj_engine_set_error(engine, locerror);
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_WRITTEN);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->sink_errors | engine->sink_written));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _obj_engine_sync_done_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncObjEngine *engine = userdata;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	int i = _get_proxy(engine, proxy);
	
	if (error) {
		osync_obj_engine_set_error(engine, error);
		engine->sink_errors = engine->sink_errors | (0x1 << i);
	} else
		engine->sink_sync_done = engine->sink_sync_done | (0x1 << i);
			
	if (BitCount(engine->sink_errors | engine->sink_sync_done) == g_list_length(engine->proxies)) {
		if (BitCount(engine->sink_sync_done) < BitCount(engine->sink_connects)) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Less proxies reported sync_done than connected");
			osync_obj_engine_set_error(engine, locerror);
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_SYNC_DONE);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->sink_errors | engine->sink_sync_done));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

OSyncObjEngine *osync_obj_engine_new(OSyncEngine *parent, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, parent, objtype, error);
	g_assert(parent);
	g_assert(objtype);
	
	OSyncObjEngine *engine = osync_try_malloc0(sizeof(OSyncObjEngine), error);
	if (!engine)
		goto error;
	engine->ref_count = 1;
	
	engine->parent = parent;
	osync_engine_ref(parent);
	
	engine->objtype = g_strdup(objtype);
	
	int num = osync_engine_num_proxies(engine->parent);
	int i = 0;
	for (i = 0; i < num; i++) {
		OSyncClientProxy *proxy = osync_engine_nth_proxy(engine->parent, i);
		
		engine->proxies = g_list_append(engine->proxies, proxy);
		osync_client_proxy_ref(proxy);
	}
	
	//engine->mapping_table = osync_mappingtable_new(error);
	//if (!engine->mapping_table)
	//	goto error_free_engine;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, engine);
	return engine;

//error_free_engine:
//	osync_obj_engine_unref(engine);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void osync_obj_engine_ref(OSyncObjEngine *engine)
{
	osync_assert(engine);
	
	g_atomic_int_inc(&(engine->ref_count));
}

void osync_obj_engine_unref(OSyncObjEngine *engine)
{
	osync_assert(engine);
		
	if (g_atomic_int_dec_and_test(&(engine->ref_count))) {
		while (engine->proxies) {
			OSyncClientProxy *proxy = engine->proxies->data;
			osync_client_proxy_unref(proxy);
			engine->proxies = g_list_remove(engine->proxies, proxy);
		}
		
		if (engine->parent)
			osync_engine_unref(engine->parent);
			
		if (engine->objtype)
			g_free(engine->objtype);
		
		//if (engine->mapping_table)
		//	osync_mapping_table_unref(engine->mapping_table);
			
		g_free(engine);
	}
}

osync_bool osync_obj_engine_command(OSyncObjEngine *engine, OSyncEngineCommand cmd, OSyncError **error)
{
	GList *p = NULL;
	OSyncClientProxy *proxy =  NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, engine, cmd, error);
	osync_assert(engine);
	
	switch (cmd) {
		case OSYNC_ENGINE_COMMAND_CONNECT:
			for (p = engine->proxies; p; p = p->next) {
				proxy = p->data;
				if (!osync_client_proxy_connect(proxy, _obj_engine_connect_callback, engine, engine->objtype, error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_COMMAND_READ:
			for (p = engine->proxies; p; p = p->next) {
				proxy = p->data;
				if (!osync_client_proxy_get_changes(proxy, _obj_engine_read_callback, _obj_engine_change_callback, engine, engine->objtype, error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_COMMAND_WRITE:
			for (p = engine->proxies; p; p = p->next) {
				proxy = p->data;
				/* Write the changes */
				
				if (!osync_client_proxy_committed_all(proxy, _obj_engine_written_callback, engine, engine->objtype, error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_COMMAND_SYNC_DONE:
			for (p = engine->proxies; p; p = p->next) {
				proxy = p->data;
				if (!osync_client_proxy_sync_done(proxy, _obj_engine_sync_done_callback, engine, engine->objtype, error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_COMMAND_DISCONNECT:
			for (p = engine->proxies; p; p = p->next) {
				proxy = p->data;
				if (!osync_client_proxy_disconnect(proxy, _obj_engine_disconnect_callback, engine, engine->objtype, error))
					goto error;
			}
			break;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	osync_error_unref(error);
	return FALSE;
}


void osync_obj_engine_event(OSyncObjEngine *engine, OSyncEngineEvent event)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, engine, event);
	osync_assert(engine);
	
	engine->callback(engine, event, engine->error, engine->callback_userdata);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

void osync_obj_engine_set_callback(OSyncObjEngine *engine, OSyncObjEngineEventCallback callback, void *userdata)
{
	osync_assert(engine);
	engine->callback = callback;
	engine->callback_userdata = userdata;
}

void osync_obj_engine_set_error(OSyncObjEngine *engine, OSyncError *error)
{
	osync_assert(engine);
	if (engine->error)
		osync_error_unref(&engine->error);
	engine->error = error;
	osync_error_ref(&error);
}
