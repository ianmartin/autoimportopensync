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

#include "opensync_mapping_engine.h"
#include "opensync_sink_engine_internals.h"

#include "opensync_mapping_entry_engine_internals.h"
#include "opensync_mapping_engine_internals.h"


OSyncMappingEntryEngine *osync_entry_engine_new(OSyncMappingEntry *entry, OSyncMappingEngine *mapping_engine, OSyncSinkEngine *sink_engine, OSyncObjEngine *objengine, OSyncError **error)
{
  OSyncMappingEntryEngine *engine = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, entry, mapping_engine, sink_engine, objengine, error);
  osync_assert(sink_engine);
  osync_assert(entry);
	
  engine = osync_try_malloc0(sizeof(OSyncMappingEntryEngine), error);
  if (!engine)
    goto error;
  engine->ref_count = 1;
	
  engine->sink_engine = sink_engine;
	
  engine->objengine = objengine;
	
  engine->mapping_engine = osync_mapping_engine_ref(mapping_engine);
  engine->entry = osync_mapping_entry_ref(entry);
	
  sink_engine->entries = g_list_append(sink_engine->entries, engine);
  osync_entry_engine_ref(engine);
	
  osync_trace(TRACE_EXIT, "%s: %p", __func__, engine);
  return engine;

 error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return NULL;
}

OSyncMappingEntryEngine *osync_entry_engine_ref(OSyncMappingEntryEngine *engine)
{
  osync_assert(engine);
	
  g_atomic_int_inc(&(engine->ref_count));

  return engine;
}

void osync_entry_engine_unref(OSyncMappingEntryEngine *engine)
{
  osync_assert(engine);
		
  if (g_atomic_int_dec_and_test(&(engine->ref_count))) {
	
    if (engine->change)
      osync_change_unref(engine->change);

    if (engine->mapping_engine)
      osync_mapping_engine_unref(engine->mapping_engine);

    if (engine->entry)
      osync_mapping_entry_unref(engine->entry);
		
    g_free(engine);
  }
}

osync_bool osync_entry_engine_matches(OSyncMappingEntryEngine *engine, OSyncChange *change)
{
  OSyncMappingEntry *entry = NULL;
  const char *mapping_entry_uid = NULL; 
  osync_assert(engine);
  osync_assert(engine->entry);
  osync_assert(change);
	
  entry = engine->entry;
  mapping_entry_uid = osync_mapping_entry_get_uid(entry); 
  osync_assert(mapping_entry_uid);
	
  if (!strcmp(mapping_entry_uid, osync_change_get_uid(change)))
    return TRUE;
	
  return FALSE;
}

void osync_entry_engine_update(OSyncMappingEntryEngine *engine, OSyncChange *change)
{
  osync_assert(engine);
	
  if (engine->change)
    osync_change_unref(engine->change);
	
  engine->change = change;
  engine->mapping_engine->synced = FALSE;
	
  if (change)
    osync_change_ref(change);
}

OSyncChange *osync_entry_engine_get_change(OSyncMappingEntryEngine *engine)
{
  osync_assert(engine);
  return engine->change;
}

osync_bool osync_entry_engine_is_dirty(OSyncMappingEntryEngine *engine)
{
  osync_assert(engine);
  return engine->dirty;
}

void osync_entry_engine_set_dirty(OSyncMappingEntryEngine *engine, osync_bool dirty)
{
  osync_assert(engine);
  engine->dirty = dirty;
}

