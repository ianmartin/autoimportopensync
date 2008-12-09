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

#include "opensync-archive.h"
#include "opensync-group.h"
#include "opensync-engine.h"
#include "opensync-data.h"
#include "opensync-mapping.h"

#include "opensync_obj_engine.h"
#include "opensync_obj_engine_internals.h"
#include "opensync_sink_engine_internals.h"

#include "opensync_mapping_entry_engine_internals.h"

OSyncSinkEngine *osync_sink_engine_new(int position, OSyncClientProxy *proxy, OSyncObjEngine *objengine, OSyncError **error)
{
	OSyncSinkEngine *sinkengine = NULL;
	osync_trace(TRACE_ENTRY, "%s(%i, %p, %p, %p)", __func__, position, proxy, objengine, error);
	osync_assert(proxy);
	osync_assert(objengine);
	
	sinkengine = osync_try_malloc0(sizeof(OSyncSinkEngine), error);
	if (!sinkengine)
		goto error;
	sinkengine->ref_count = 1;
	sinkengine->position = position;
	
	/* we dont reference the proxy to avoid circular dependencies. This object is completely
	 * dependent on the proxy anyways */
	sinkengine->proxy = proxy;
	
	sinkengine->engine = objengine;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, sinkengine);
	return sinkengine;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

OSyncSinkEngine *osync_sink_engine_ref(OSyncSinkEngine *engine)
{
	osync_assert(engine);
	
	g_atomic_int_inc(&(engine->ref_count));

	return engine;
}

void osync_sink_engine_unref(OSyncSinkEngine *engine)
{
	osync_assert(engine);
		
	if (g_atomic_int_dec_and_test(&(engine->ref_count))) {
		while (engine->unmapped) {
			OSyncChange *change = engine->unmapped->data;
			osync_change_unref(change);
			
			engine->unmapped = g_list_remove(engine->unmapped, engine->unmapped->data);
		}
		
		while (engine->entries) {
			OSyncMappingEntryEngine *entry = engine->entries->data;
			osync_entry_engine_unref(entry);
			
			engine->entries = g_list_remove(engine->entries, engine->entries->data);
		}
		
		g_free(engine);
	}
}

osync_bool osync_sink_engine_is_connected(OSyncSinkEngine *engine)
{
	OSyncObjEngine *objengine = NULL;
	osync_assert(engine);

	objengine = engine->engine;

	if (!objengine)
		return FALSE;

	return !!(objengine->sink_connects & (1 << engine->position));
}

