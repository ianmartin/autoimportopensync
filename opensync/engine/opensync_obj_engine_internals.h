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
 
#ifndef OPENSYNC_OBJ_ENGINE_INTERNALS_H_
#define OPENSYNC_OBJ_ENGINE_INTERNALS_H_

struct OSyncObjEngine {
	int ref_count;
	OSyncEngine *parent;
	char *objtype;
	OSyncArchive *archive;
	
	OSyncMappingTable *mapping_table;
	GList *mapping_engines;
	
	GList *sink_engines;
	OSyncError *error;
	OSyncFormatEnv *formatenv;
	
	int sink_errors;
	int sink_connects;
	int sink_disconnects;
	int sink_get_changes;
	int sink_sync_done;
	int sink_written;
	
	OSyncObjEngineEventCallback callback;
	void *callback_userdata;
	
	GList *conflicts;
	osync_bool written;
};

typedef struct OSyncSinkEngine {
	int ref_count;
	int position;
	OSyncClientProxy *proxy;
	OSyncObjEngine *engine;
	GList *entries;
	GList *unmapped;
} OSyncSinkEngine;

typedef struct OSyncMappingEntryEngine {
	int ref_count;
	OSyncSinkEngine *sink_engine;
	osync_bool dirty;
	OSyncChange *change;
	OSyncObjEngine *objengine;
	OSyncMappingEngine *mapping_engine;
	OSyncMappingEntry *entry;
} OSyncMappingEntryEngine;

struct OSyncMappingEngine {
	int ref_count;
	OSyncMapping *mapping;
	OSyncMappingEntryEngine *master;
	GList *entries;
	OSyncObjEngine *parent;
	osync_bool conflict;
	osync_bool synced;
};

OSyncMappingEntryEngine *osync_entry_engine_new(OSyncMappingEntry *entry, OSyncMappingEngine *mapping_engine, OSyncSinkEngine *sink_engine, OSyncObjEngine *objengine, OSyncError **error);
void osync_entry_engine_ref(OSyncMappingEntryEngine *engine);
void osync_entry_engine_unref(OSyncMappingEntryEngine *engine);
osync_bool osync_entry_engine_matches(OSyncMappingEntryEngine *engine, OSyncChange *change);
void osync_entry_engine_update(OSyncMappingEntryEngine *engine, OSyncChange *change);
OSyncChange *osync_entry_engine_get_change(OSyncMappingEntryEngine *engine);
osync_bool osync_entry_engine_is_dirty(OSyncMappingEntryEngine *engine);
void osync_entry_engine_set_dirty(OSyncMappingEntryEngine *engine, osync_bool dirty);

OSyncMappingEngine *osync_mapping_engine_new(OSyncObjEngine *parent, OSyncMapping *mapping, OSyncError **error);
void osync_mapping_engine_ref(OSyncMappingEngine *engine);
void osync_mapping_engine_unref(OSyncMappingEngine *engine);
osync_bool osync_mapping_engine_multiply(OSyncMappingEngine *engine, OSyncError **error);
void osync_mapping_engine_check_conflict(OSyncMappingEngine *engine);
OSyncMappingEntryEngine *osync_mapping_engine_get_entry(OSyncMappingEngine *engine, OSyncSinkEngine *sinkengine);

#endif /*OPENSYNC_OBJ_ENGINE_INTERNALS_H_*/
