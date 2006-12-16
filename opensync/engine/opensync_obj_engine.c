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
#include "opensync-client.h"
#include "opensync-data.h"
#include "opensync-mapping.h"
#include "opensync-format.h"

#include "opensync_obj_engine.h"
#include "opensync_obj_engine_internals.h"

void osync_sink_engine_unref(OSyncSinkEngine *engine);

OSyncSinkEngine *osync_sink_engine_new(int position, OSyncClientProxy *proxy, OSyncObjEngine *objengine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%i, %p, %p, %p)", __func__, position, proxy, objengine, error);
	osync_assert(proxy);
	osync_assert(objengine);
	
	OSyncSinkEngine *sinkengine = osync_try_malloc0(sizeof(OSyncSinkEngine), error);
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

void osync_sink_engine_ref(OSyncSinkEngine *engine)
{
	osync_assert(engine);
	
	g_atomic_int_inc(&(engine->ref_count));
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

OSyncMappingEntryEngine *osync_entry_engine_new(OSyncMappingEntry *entry, OSyncMappingEngine *mapping_engine, OSyncSinkEngine *sink_engine, OSyncObjEngine *objengine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, entry, mapping_engine, sink_engine, objengine, error);
	osync_assert(sink_engine);
	osync_assert(entry);
	
	OSyncMappingEntryEngine *engine = osync_try_malloc0(sizeof(OSyncMappingEntryEngine), error);
	if (!engine)
		goto error;
	engine->ref_count = 1;
	
	engine->sink_engine = sink_engine;
	
	engine->objengine = objengine;
	
	engine->mapping_engine = mapping_engine;
	engine->entry = entry;
	
	sink_engine->entries = g_list_append(sink_engine->entries, engine);
	osync_entry_engine_ref(engine);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, engine);
	return engine;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void osync_entry_engine_ref(OSyncMappingEntryEngine *engine)
{
	osync_assert(engine);
	
	g_atomic_int_inc(&(engine->ref_count));
}

void osync_entry_engine_unref(OSyncMappingEntryEngine *engine)
{
	osync_assert(engine);
		
	if (g_atomic_int_dec_and_test(&(engine->ref_count))) {
	
		if (engine->change)
			osync_change_unref(engine->change);
		
		g_free(engine);
	}
}

osync_bool osync_entry_engine_matches(OSyncMappingEntryEngine *engine, OSyncChange *change)
{
	osync_assert(engine);
	osync_assert(engine->entry);
	osync_assert(change);
	
	OSyncMappingEntry *entry = engine->entry;
	
	if (!strcmp(osync_mapping_entry_get_uid(entry), osync_change_get_uid(change)))
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

OSyncMappingEngine *osync_mapping_engine_new(OSyncObjEngine *parent, OSyncMapping *mapping, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, parent, mapping, error);
	g_assert(mapping);
	
	OSyncMappingEngine *engine = osync_try_malloc0(sizeof(OSyncMappingEngine), error);
	if (!engine)
		goto error;
	engine->ref_count = 1;
	
	engine->mapping = mapping;
	osync_mapping_ref(mapping);
	
	engine->parent = parent;
	engine->synced = TRUE;
	
	GList *s = NULL;
	for (s = parent->sink_engines; s; s = s->next) {
		OSyncSinkEngine *sink_engine = s->data;
		
		OSyncMember *member = osync_client_proxy_get_member(sink_engine->proxy);
		OSyncMappingEntry *mapping_entry = osync_mapping_find_entry_by_member_id(mapping, osync_member_get_id(member));
		osync_assert(mapping_entry);
		
		OSyncMappingEntryEngine *entry_engine = osync_entry_engine_new(mapping_entry, engine, sink_engine, parent, error);
		if (!entry_engine)
			goto error_free_engine;
		
		engine->entries = g_list_append(engine->entries, entry_engine);
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, engine);
	return engine;

error_free_engine:
	osync_mapping_engine_unref(engine);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void osync_mapping_engine_ref(OSyncMappingEngine *engine)
{
	osync_assert(engine);
	
	g_atomic_int_inc(&(engine->ref_count));
}

void osync_mapping_engine_unref(OSyncMappingEngine *engine)
{
	osync_assert(engine);
		
	if (g_atomic_int_dec_and_test(&(engine->ref_count))) {
		if (engine->mapping)
			osync_mapping_unref(engine->mapping);
		
		if (engine->master)
			osync_entry_engine_unref(engine->master);
		
		while (engine->entries) {
			OSyncMappingEntryEngine *entry = engine->entries->data;
			osync_entry_engine_unref(entry);
			
			engine->entries = g_list_remove(engine->entries, engine->entries->data);
		}
		
		g_free(engine);
	}
}

osync_bool osync_mapping_engine_multiply(OSyncMappingEngine *engine, OSyncError **error)
{
	osync_assert(engine);
	osync_assert(engine->mapping);
	
	osync_trace(TRACE_ENTRY, "%s(%p(%lli), %p)", __func__, engine, osync_mapping_get_id(engine->mapping), error);
		
	if (engine->synced) {
		osync_trace(TRACE_EXIT, "%s: No need to multiply. Already synced", __func__);
		return TRUE;
	}
	
	if (!engine->master) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No master set");
		goto error;
	}
	
	GList *e = NULL;
	for (e = engine->entries; e; e = e->next) {
		OSyncMappingEntryEngine *entry_engine = e->data;
		if (entry_engine == engine->master)
			continue;
		
		osync_trace(TRACE_INTERNAL, "Propagating change %s to %p from %p", __func__, osync_mapping_entry_get_uid(entry_engine->entry), entry_engine, engine->master);
		
		/* Input is:
		 * masterChange -> change that solved the mapping
		 * masterData -> data of masterChange
		 * existChange -> change that will be overwritten (if any) */
		
		OSyncChange *existChange = entry_engine->change;
		OSyncChange *masterChange = osync_entry_engine_get_change(engine->master);
		OSyncData *masterData = osync_change_get_data(masterChange);
		
		/* Clone the masterData. This has to be done since the data
		 * might get changed (converted) and we dont want to touch the 
		 * original data */
		OSyncData *newData = osync_data_clone(masterData, error);
		if (!newData)
			goto error;
		
		if (!existChange) {
			existChange = osync_change_new(error);
			if (!existChange)
				goto error;
			
			osync_change_set_changetype(existChange, OSYNC_CHANGE_TYPE_UNKNOWN);
		} else {
			/* Ref the change so that we can unref it later */
			osync_change_ref(existChange);
		}
		
		/* Save the changetypes, so that we can calculate the correct changetype later */
		OSyncChangeType existChangeType = osync_change_get_changetype(existChange);
		OSyncChangeType newChangeType = osync_change_get_changetype(masterChange);
		
		/* Now update the entry with the change */
		osync_entry_engine_update(entry_engine, existChange);
		
		/* We have to use the uid of the entry, so that the member
		 * can correctly identify the entry */
		if (newChangeType == OSYNC_CHANGE_TYPE_ADDED)
			osync_change_set_uid(existChange, osync_change_get_uid(masterChange));
		else
			osync_change_set_uid(existChange, osync_mapping_entry_get_uid(entry_engine->entry));
		osync_change_set_data(existChange, newData);
		osync_change_set_changetype(existChange, osync_change_get_changetype(masterChange));
		
		/* We also have to update the changetype of the new change */
		osync_trace(TRACE_INTERNAL, "Orig change type: %i New change type: %i", existChangeType, newChangeType);
		if (newChangeType == OSYNC_CHANGE_TYPE_ADDED && (existChangeType != OSYNC_CHANGE_TYPE_DELETED && existChangeType != OSYNC_CHANGE_TYPE_UNKNOWN)) {
			osync_trace(TRACE_INTERNAL, "Updating change type to MODIFIED");
			osync_change_set_changetype(existChange, OSYNC_CHANGE_TYPE_MODIFIED);
		} else if (newChangeType == OSYNC_CHANGE_TYPE_MODIFIED && (existChangeType == OSYNC_CHANGE_TYPE_DELETED)) {
			osync_trace(TRACE_INTERNAL, "Updating change type to ADDED");
			osync_change_set_changetype(existChange, OSYNC_CHANGE_TYPE_ADDED);
		}
		
		osync_change_unref(existChange);
		/* Also unref newData. Otherwise this cannot be freed when it is written. */
		osync_data_unref(newData);
			
		osync_entry_engine_set_dirty(entry_engine, TRUE);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

void osync_mapping_engine_set_master(OSyncMappingEngine *engine, OSyncMappingEntryEngine *entry)
{
	if (engine->master)
		osync_entry_engine_unref(engine->master);
	engine->master = entry;
	osync_entry_engine_ref(engine->master);
}

static unsigned int prod(unsigned int n)
{
	if (n == 0)
		return 0;
	int x = ((n + 1) / 2) * n;
	if (!(n & 0x1))
		x += n / 2;
	return x;
}

void osync_mapping_engine_check_conflict(OSyncMappingEngine *engine)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);
	osync_assert(engine != NULL);
	
	if (engine->master != NULL) {
		osync_trace(TRACE_EXIT, "%s: Already has a master", __func__);
		return;
	}
	
	if (engine->conflict) {
		osync_trace(TRACE_INTERNAL, "Detected conflict early");
		goto conflict;
	}
	
	int is_same = 0;
	GList *e = NULL;
	for (e = engine->entries; e; e = e->next) {
		OSyncMappingEntryEngine *leftentry = e->data;
		OSyncMappingEntryEngine *rightentry = NULL;
		
		OSyncChange *leftchange = osync_entry_engine_get_change(leftentry);
		OSyncChange *rightchange = NULL;
		osync_trace(TRACE_INTERNAL, "change: %p: %i", leftchange, leftchange ? osync_change_get_changetype(leftchange) : OSYNC_CHANGE_TYPE_UNKNOWN);
		if (leftchange == NULL)
			continue;
			
		if (osync_change_get_changetype(leftchange) == OSYNC_CHANGE_TYPE_UNKNOWN)
			continue;
		
		osync_mapping_engine_set_master(engine, leftentry);
		GList *n = NULL;
		for (n = e->next; n; n = n->next) {
			rightentry = n->data;
			rightchange = osync_entry_engine_get_change(rightentry);
		
			if (rightchange == NULL)
				continue;
		
			if (osync_change_get_changetype(rightchange) == OSYNC_CHANGE_TYPE_UNKNOWN)
				continue;
			
			if (osync_change_compare(leftchange, rightchange) != OSYNC_CONV_DATA_SAME) {
				engine->conflict = TRUE;
				goto conflict;
			} else {
				is_same++;
			}
		}
	}
	
	conflict:
	if (engine->conflict) {
		//conflict, solve conflict
		osync_trace(TRACE_INTERNAL, "Got conflict for mapping_engine %p", engine);
		engine->parent->conflicts = g_list_append(engine->parent->conflicts, engine);
		osync_status_conflict(engine->parent->parent, engine);
		osync_trace(TRACE_EXIT, "%s: Got conflict", __func__);
		return;
	}
	osync_assert(engine->master);
	osync_status_update_mapping(engine->parent->parent, engine, OSYNC_MAPPING_EVENT_SOLVED, NULL);
	
	if (is_same == prod(g_list_length(engine->entries) - 1)) {
		osync_trace(TRACE_INTERNAL, "No need to sync. All entries are the same");
		GList *e = NULL;
		for (e = engine->entries; e; e = e->next) {
			OSyncMappingEntryEngine *entry = e->data;
			entry->dirty = FALSE;
		}
		engine->synced = TRUE;
	}
	
	osync_trace(TRACE_EXIT, "%s: No conflict", __func__);
}

OSyncMappingEntryEngine *osync_mapping_engine_get_entry(OSyncMappingEngine *engine, OSyncSinkEngine *sinkengine)
{
	GList *e = NULL;
	for (e = engine->entries; e; e = e->next) {
		OSyncMappingEntryEngine *entry_engine = e->data;
		if (sinkengine == entry_engine->sink_engine)
			return entry_engine;
	}
	
	return NULL;
}

int osync_mapping_engine_num_changes(OSyncMappingEngine *engine)
{
	osync_assert(engine);
	
	int num = 0;
	GList *e = NULL;
	for (e = engine->entries; e; e = e->next) {
		OSyncMappingEntryEngine *entry = e->data;
		if (entry->change)
			num++;
	}
	
	return num;
}

OSyncChange *osync_mapping_engine_nth_change(OSyncMappingEngine *engine, int nth)
{
	osync_assert(engine);
	
	int num = 0;
	GList *e = NULL;
	for (e = engine->entries; e; e = e->next) {
		OSyncMappingEntryEngine *entry = e->data;
		if (entry->change) {
			if (num == nth)
				return entry->change;
			num++;
		}
	}
	
	return NULL;
}

OSyncMappingEntryEngine *osync_mapping_engine_find_entry(OSyncMappingEngine *engine, OSyncChange *change)
{
	GList *e;
	for (e = engine->entries; e; e = e->next) {
		OSyncMappingEntryEngine *entry = e->data;
		if (change && entry->change == change)
			return entry;
	}
	
	return NULL;
}

static int BitCount(unsigned int u)                          
{
	unsigned int uCount = u - ((u >> 1) & 033333333333) - ((u >> 2) & 011111111111);
	return ((uCount + (uCount >> 3)) & 030707070707) % 63;
}

osync_bool osync_mapping_engine_solve(OSyncMappingEngine *engine, OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, change);
	
	OSyncMappingEntryEngine *entry = osync_mapping_engine_find_entry(engine, change);
	engine->conflict = FALSE;
	osync_mapping_engine_set_master(engine, entry);
	osync_status_update_mapping(engine->parent->parent, engine, OSYNC_MAPPING_EVENT_SOLVED, NULL);
	engine->parent->conflicts = g_list_remove(engine->parent->conflicts, engine);
	
	if (osync_engine_check_get_changes(engine->parent->parent) && BitCount(engine->parent->sink_errors | engine->parent->sink_get_changes) == g_list_length(engine->parent->sink_engines)) {
		OSyncError *error = NULL;
		if (!osync_obj_engine_command(engine->parent, OSYNC_ENGINE_COMMAND_WRITE, &error))
			goto error;
	} else
		osync_trace(TRACE_INTERNAL, "Not triggering write. didnt receive all reads yet");
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static OSyncMappingEngine *_create_mapping_engine(OSyncObjEngine *engine, OSyncError **error)
{
	/* If there is none, create one */
	OSyncMapping *mapping = osync_mapping_new(error);
	if (!mapping)
		goto error;
	
	osync_mapping_set_id(mapping, osync_mapping_table_get_next_id(engine->mapping_table));
	osync_mapping_table_add_mapping(engine->mapping_table, mapping);
	
	GList *s = NULL;
	for (s = engine->sink_engines; s; s = s->next) {
		OSyncSinkEngine *sink_engine = s->data;
		
		OSyncMember *member = osync_client_proxy_get_member(sink_engine->proxy);
		
		OSyncMappingEntry *mapping_entry = osync_mapping_entry_new(error);
		osync_mapping_entry_set_member_id(mapping_entry, osync_member_get_id(member));
		osync_mapping_add_entry(mapping, mapping_entry);
		osync_mapping_entry_unref(mapping_entry);
	}
	
	OSyncMappingEngine *mapping_engine = osync_mapping_engine_new(engine, mapping, error);
	if (!mapping_engine)
		goto error_free_mapping;
	osync_mapping_unref(mapping);
	
	return mapping_engine;
	
error_free_mapping:
	osync_mapping_unref(mapping);
error:
	return NULL;
}

static osync_bool _osync_change_elevate(OSyncChange *change, int level, osync_bool *dirty, OSyncError **error)
{
	int i = 0;
	for (i = 0; i < level; i++) {
		if (!osync_change_duplicate(change, dirty, error))
			return FALSE;
	}
	return TRUE;
}

/** @brief Solves the conflict by duplicating the conflicting entries
 * 
 * @param engine The engine
 * @param dupe_mapping The conflicting mapping to duplicate
 * 
 */
osync_bool osync_mapping_engine_duplicate(OSyncMappingEngine *existingMapping, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, existingMapping, error);
	g_assert(existingMapping);
	
	int elevation = 0;
	OSyncObjEngine *objengine = existingMapping->parent;
	
	/* Remove all deleted items first and copy the changes to a list */
	GList *entries = NULL;
	GList *e = existingMapping->entries;
	for (; e; e = e->next) {
		OSyncMappingEntryEngine *entry = e->data;
		if (entry->change) {
			if (osync_change_get_changetype(entry->change) == OSYNC_CHANGE_TYPE_MODIFIED || osync_change_get_changetype(entry->change) == OSYNC_CHANGE_TYPE_ADDED) {
				osync_trace(TRACE_INTERNAL, "Appending entry %s, changetype %i from member %lli", osync_change_get_uid(entry->change), osync_change_get_changetype(entry->change), osync_member_get_id(osync_client_proxy_get_member(entry->sink_engine->proxy)));
		
				entries = g_list_append(entries, entry);
			} else {
				osync_trace(TRACE_INTERNAL, "Removing entry %s, changetype %i from member %lli", osync_change_get_uid(entry->change), osync_change_get_changetype(entry->change), osync_member_get_id(osync_client_proxy_get_member(entry->sink_engine->proxy)));
				osync_entry_engine_update(entry, NULL);
			}
		} else {
			osync_trace(TRACE_INTERNAL, "member %lli does not have a entry", osync_member_get_id(osync_client_proxy_get_member(entry->sink_engine->proxy)));
		}
	}
	
	/* Create a list with mappings. In the beginning, only the exisiting mapping is in the list */
	GList *mappings = g_list_append(NULL, existingMapping);
	osync_mapping_engine_ref(existingMapping);
	
	while (entries) {
		OSyncMappingEntryEngine *entry = entries->data
		
		/* Now lets see which mapping is the correct one for the entry */;
		GList *m = NULL;
		OSyncMappingEngine *mapping = NULL;
		elevation = 0;
		for (m = mappings; m; m = m->next) {
			mapping = m->data;
			
			/* Get the first change of the mapping to test. Compare the given change with this change.
			 * If they are the same, we have found a new mapping */
			GList *e = NULL;
			OSyncChange *change = NULL;
			for (e = mapping->entries; e; e = e->next) {
				OSyncMappingEntryEngine *entry = e->data;
				change = entry->change;
				if (change)
					break;
			}
			
			if (!change || osync_change_compare(entry->change, change) == OSYNC_CONV_DATA_SAME)
				break;
			
			mapping = NULL;
			elevation++;
		}
		
		OSyncChange *change = entry->change;
		osync_change_ref(change);
		//osync_entry_engine_update(entry, NULL);
		
		if (!mapping) {
			/* Unable to find a mapping. We have to create a new one */
			mapping = _create_mapping_engine(objengine, error);
			if (!mapping)
				goto error;
			mappings = g_list_append(mappings, mapping);
			objengine->mapping_engines = g_list_append(objengine->mapping_engines, mapping);
			osync_mapping_engine_ref(mapping);
		}
		
		/* update the uid and the content to suit the new level */
		osync_bool dirty = FALSE;
		if (!_osync_change_elevate(change, elevation, &dirty, error))
			goto error;
		
		/* Lets add the entry to the mapping */
		OSyncMappingEntryEngine *newEntry = osync_mapping_engine_get_entry(mapping, entry->sink_engine);
		osync_assert(newEntry);
		osync_entry_engine_update(newEntry, change);
		osync_mapping_entry_set_uid(newEntry->entry, osync_change_get_uid(change));
		osync_change_unref(change);
		
		/* Set the last entry as the master */
		osync_mapping_engine_set_master(mapping, newEntry);
		
		/* Update the dirty status. If the duplicate function said
		 * that the returned item needs to be written, we will set
		 * this information here */
		newEntry->dirty = dirty;
		
		entries = g_list_remove(entries, entry);
	}
	
	
	while (mappings) {
		OSyncMappingEngine *mapping = mappings->data;
		osync_mapping_engine_unref(mapping);
		mappings = g_list_remove(mappings, mapping);
	}
	
	objengine->conflicts = g_list_remove(objengine->conflicts, existingMapping);
		osync_status_update_mapping(objengine->parent, existingMapping, OSYNC_MAPPING_EVENT_SOLVED, NULL);
	
	if (osync_engine_check_get_changes(objengine->parent) && BitCount(objengine->sink_errors | objengine->sink_get_changes) == g_list_length(objengine->sink_engines)) {
		osync_obj_engine_command(objengine, OSYNC_ENGINE_COMMAND_WRITE, error);
	} else
		osync_trace(TRACE_INTERNAL, "Not triggering write. didnt receive all reads yet");

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	while (mappings) {
		OSyncMappingEngine *mapping = mappings->data;
		osync_mapping_engine_unref(mapping);
		mappings = g_list_remove(mappings, mapping);
	}
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static void _obj_engine_connect_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncSinkEngine *sinkengine = userdata;
	OSyncObjEngine *engine = sinkengine->engine;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	if (error) {
		osync_trace(TRACE_INTERNAL, "Obj Engine received connect error: %s", osync_error_print(&error));
		osync_obj_engine_set_error(engine, error);
		engine->sink_errors = engine->sink_errors | (0x1 << sinkengine->position);
		osync_status_update_member(engine->parent, osync_client_proxy_get_member(proxy), OSYNC_CLIENT_EVENT_ERROR, engine->objtype, error);
	} else {
		engine->sink_connects = engine->sink_connects | (0x1 << sinkengine->position);
		osync_status_update_member(engine->parent, osync_client_proxy_get_member(proxy), OSYNC_CLIENT_EVENT_CONNECTED, engine->objtype, NULL);
	}
			
	if (BitCount(engine->sink_errors | engine->sink_connects) == g_list_length(engine->sink_engines)) {
		if (BitCount(engine->sink_connects) < 2) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Less than 2 sink_engines are connected");
			osync_obj_engine_set_error(engine, locerror);
			
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else {
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_CONNECTED);
		}
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->sink_errors | engine->sink_connects));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _obj_engine_disconnect_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncSinkEngine *sinkengine = userdata;
	OSyncObjEngine *engine = sinkengine->engine;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	if (error) {
		osync_obj_engine_set_error(engine, error);
		engine->sink_errors = engine->sink_errors | (0x1 << sinkengine->position);
		osync_status_update_member(engine->parent, osync_client_proxy_get_member(proxy), OSYNC_CLIENT_EVENT_ERROR, engine->objtype, error);
	} else {
		engine->sink_disconnects = engine->sink_disconnects | (0x1 << sinkengine->position);
		osync_status_update_member(engine->parent, osync_client_proxy_get_member(proxy), OSYNC_CLIENT_EVENT_DISCONNECTED, engine->objtype, NULL);
	}
			
	if (BitCount(engine->sink_errors | engine->sink_disconnects) == g_list_length(engine->sink_engines)) {
		if (BitCount(engine->sink_disconnects) < BitCount(engine->sink_connects)) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Less sink_engines disconnected than connected");
			osync_obj_engine_set_error(engine, locerror);
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_DISCONNECTED);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->sink_errors | engine->sink_disconnects));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/* Finds the mapping to which the entry should belong. The
 * return value is MISMATCH if no mapping could be found,
 * SIMILAR if a mapping has been found but its not completely the same
 * SAME if a mapping has been found and is the same */
static OSyncConvCmpResult _obj_engine_mapping_find(OSyncObjEngine *engine, OSyncChange *change, OSyncSinkEngine *sinkengine, OSyncMappingEngine **mapping_engine)
{	
	GList *m = NULL;
	GList *e = NULL;
	OSyncConvCmpResult result = OSYNC_CONV_DATA_MISMATCH;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, engine, change, sinkengine, mapping_engine);
	
	for (m = engine->mapping_engines; m; m = m->next) {
		*mapping_engine = m->data;
		
		/* Go through the already existing mapping entries. We only consider mappings
		 * which dont have a entry on our side and where the data comparsion does not
		 * return MISMATCH */
		for (e = (*mapping_engine)->entries; e; e = e->next) {
			OSyncMappingEntryEngine *entry_engine = e->data;
			/* if the mapping already has a entry on our side, its not worth looking */
			if (entry_engine->sink_engine == sinkengine) {
				*mapping_engine = NULL;
				break;
			}
			
			OSyncChange *mapping_change = osync_entry_engine_get_change(entry_engine);
			if (!mapping_change)
				continue;
			
			result = osync_change_compare(mapping_change, change);
			if (result == OSYNC_CONV_DATA_MISMATCH)
				*mapping_engine = NULL;
			
			break;
		}
		
		if (*mapping_engine) {
			osync_trace(TRACE_EXIT, "%s: Found %p", __func__, *mapping_engine);
			return result;
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: Mismatch", __func__);
	return OSYNC_CONV_DATA_MISMATCH;
}

osync_bool osync_obj_engine_map_changes(OSyncObjEngine *engine, OSyncError **error)
{
	OSyncMappingEngine *mapping_engine = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);
	//osync_trace_disable();
	
	GList *new_mappings = NULL;
	
	GList *v = NULL;
	/* Go through all sink engines that are available */
	for (v = engine->sink_engines; v; v = v->next) {
		OSyncSinkEngine *sinkengine = v->data;
		
		/* We use a temp list to speed things up. We dont have to compare with newly created mappings for
		 * the current sinkengine, since there will be only one entry (for the current sinkengine) so there
		 * is no need to compare */
		new_mappings = NULL;
		
		/* For each sinkengine, go through all unmapped changes */
		while (sinkengine->unmapped) {
			OSyncChange *change = sinkengine->unmapped->data;
			
			osync_trace(TRACE_INTERNAL, "Looking for mapping for change %s, changetype %i from member %lli", osync_change_get_uid(change), osync_change_get_changetype(change), osync_member_get_id(osync_client_proxy_get_member(sinkengine->proxy)));
	
			
			/* See if there is an exisiting mapping, which fits the unmapped change */
			OSyncConvCmpResult result = _obj_engine_mapping_find(engine, change, sinkengine, &mapping_engine);
			if (result == OSYNC_CONV_DATA_MISMATCH) {
				/* If there is none, create one */
				mapping_engine = _create_mapping_engine(engine, error);
				if (!mapping_engine)
					goto error;
				
				osync_trace(TRACE_INTERNAL, "Unable to find mapping. Creating new mapping with id %lli", osync_mapping_get_id(mapping_engine->mapping));
				
				new_mappings = g_list_append(new_mappings, mapping_engine);
			} else if (result == OSYNC_CONV_DATA_SIMILAR) {
				mapping_engine->conflict = TRUE;
			}
			/* Update the entry which belongs to our sinkengine with the the change */
			OSyncMappingEntryEngine *entry_engine = osync_mapping_engine_get_entry(mapping_engine, sinkengine);
			osync_assert(entry_engine);
			
			osync_entry_engine_update(entry_engine, change);
			sinkengine->unmapped = g_list_remove(sinkengine->unmapped, sinkengine->unmapped->data);
			osync_change_unref(change);
		}
		
		engine->mapping_engines = g_list_concat(engine->mapping_engines, new_mappings);
	}
	
	//osync_trace_enable();
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace_enable();
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static void _obj_engine_read_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncSinkEngine *sinkengine = userdata;
	OSyncObjEngine *engine = sinkengine->engine;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	if (error) {
		osync_obj_engine_set_error(engine, error);
		engine->sink_errors = engine->sink_errors | (0x1 << sinkengine->position);
		osync_status_update_member(engine->parent, osync_client_proxy_get_member(proxy), OSYNC_CLIENT_EVENT_ERROR, engine->objtype, error);
	} else {
		engine->sink_get_changes = engine->sink_get_changes | (0x1 << sinkengine->position);
		osync_status_update_member(engine->parent, osync_client_proxy_get_member(proxy), OSYNC_CLIENT_EVENT_READ, engine->objtype, NULL);
	}
	
	if (BitCount(engine->sink_errors | engine->sink_get_changes) == g_list_length(engine->sink_engines)) {
		
		if (BitCount(engine->sink_get_changes) < BitCount(engine->sink_connects)) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Less sink_engines reported get_changes than connected");
			osync_obj_engine_set_error(engine, locerror);
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else {
			/* We are now done reading the changes. so we can now start to create the mappings, conflicts etc */
			if (!osync_obj_engine_map_changes(engine, &locerror)) {
				osync_obj_engine_set_error(engine, locerror);
				osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
			} else {
				GList *m;
				for (m = engine->mapping_engines; m; m = m->next) {
					OSyncMappingEngine *mapping_engine = m->data;
					if (!mapping_engine->synced)
						osync_mapping_engine_check_conflict(mapping_engine);
				}
				
				osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_READ);
			}
		}
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->sink_errors | engine->sink_get_changes));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool osync_obj_engine_receive_change(OSyncObjEngine *objengine, OSyncClientProxy *proxy, OSyncChange *change, OSyncError **error)
{
	OSyncSinkEngine *sinkengine = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, objengine, proxy, change, error);
	
	/* Find the sinkengine for the proxy */
	GList *s = NULL;
	for (s = objengine->sink_engines; s; s = s->next) {
		sinkengine = s->data;
		if (sinkengine->proxy == proxy)
			break;
		sinkengine = NULL;
	}
	
	if (!sinkengine) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find sinkengine");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	/* We now have to see if the change matches one of the already existing mappings */
	GList *e;
	for (e = sinkengine->entries; e; e = e->next) {
		OSyncMappingEntryEngine *mapping_engine = e->data;
		
		if (osync_entry_engine_matches(mapping_engine, change)) {
			osync_entry_engine_update(mapping_engine, change);
			
			osync_status_update_change(sinkengine->engine->parent, change, osync_client_proxy_get_member(proxy), mapping_engine->mapping_engine->mapping, OSYNC_CHANGE_EVENT_READ, NULL);
			
			osync_trace(TRACE_EXIT, "%s: Updated", __func__);
			return TRUE;
		}
	}
	
	osync_status_update_change(sinkengine->engine->parent, change, osync_client_proxy_get_member(proxy), NULL, OSYNC_CHANGE_EVENT_READ, NULL);
			
	/* If we couldnt find a match entry, we will append it the unmapped changes
	 * and take care of it later */
	sinkengine->unmapped = g_list_append(sinkengine->unmapped, change);
	osync_change_ref(change);
	
	osync_trace(TRACE_EXIT, "%s: Unmapped", __func__);
	return TRUE;
}

static void _generate_written_event(OSyncObjEngine *engine)
{
	osync_trace(TRACE_INTERNAL, "%s: %p", __func__, engine);
	/* We need to make sure that all entries are written ... */
	osync_bool dirty = FALSE;
	GList *p = NULL;
	GList *e = NULL;
	OSyncSinkEngine *sinkengine = NULL;
	OSyncError *locerror = NULL;
	
	for (p = engine->sink_engines; p; p = p->next) {
		sinkengine = p->data;
		
		for (e = sinkengine->entries; e; e = e->next) {
			OSyncMappingEntryEngine *entry_engine = e->data;
			if (osync_entry_engine_is_dirty(entry_engine) == TRUE) {
				dirty = TRUE;
				break;
			}
		}
		if (dirty)
			return;
	}
	osync_trace(TRACE_INTERNAL, "%s: Not dirty anymore", __func__);
	
	/* And that we received the written replies from all sinks */
	if (BitCount(engine->sink_errors | engine->sink_written) == g_list_length(engine->sink_engines)) {
		if (BitCount(engine->sink_written) < BitCount(engine->sink_connects)) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Less sink_engines reported committed all than connected");
			osync_obj_engine_set_error(engine, locerror);
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_WRITTEN);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->sink_errors | engine->sink_written));
}

static void _obj_engine_commit_change_callback(OSyncClientProxy *proxy, void *userdata, const char *uid, OSyncError *error)
{
	OSyncMappingEntryEngine *entry_engine = userdata;
	OSyncObjEngine *engine = entry_engine->objengine;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, %p)", __func__, proxy, userdata, uid, error);
	
	osync_entry_engine_set_dirty(entry_engine, FALSE);
	
	
	OSyncMapping *mapping = entry_engine->mapping_engine->mapping;
	OSyncMember *member = osync_client_proxy_get_member(proxy);
	OSyncMappingEntry *entry = entry_engine->entry;
	
	if (uid)
		osync_change_set_uid(entry_engine->change, uid);
	
	if (engine->archive) {
		if (osync_change_get_changetype(entry_engine->change) == OSYNC_CHANGE_TYPE_DELETED) {
			osync_archive_delete_change(engine->archive, osync_mapping_entry_get_id(entry), &locerror);
		} else {
			osync_archive_save_change(engine->archive, osync_mapping_entry_get_id(entry), osync_change_get_uid(entry_engine->change), osync_change_get_objtype(entry_engine->change), osync_mapping_get_id(mapping), osync_member_get_id(member), &locerror);
		}
	}

	osync_assert(entry_engine->mapping_engine);
	osync_status_update_change(engine->parent, entry_engine->change, osync_client_proxy_get_member(proxy), entry_engine->mapping_engine->mapping, OSYNC_CHANGE_EVENT_WRITTEN, NULL);
	osync_entry_engine_update(entry_engine, NULL);
	
	_generate_written_event(engine);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _obj_engine_written_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncSinkEngine *sinkengine = userdata;
	OSyncObjEngine *engine = sinkengine->engine;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	if (error) {
		osync_obj_engine_set_error(engine, error);
		engine->sink_errors = engine->sink_errors | (0x1 << sinkengine->position);
		osync_status_update_member(engine->parent, osync_client_proxy_get_member(proxy), OSYNC_CLIENT_EVENT_ERROR, engine->objtype, error);
	} else {
		engine->sink_written = engine->sink_written | (0x1 << sinkengine->position);
		osync_status_update_member(engine->parent, osync_client_proxy_get_member(proxy), OSYNC_CLIENT_EVENT_WRITTEN, engine->objtype, NULL);
	}
			
	_generate_written_event(engine);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _obj_engine_sync_done_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncSinkEngine *sinkengine = userdata;
	OSyncObjEngine *engine = sinkengine->engine;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	if (error) {
		osync_obj_engine_set_error(engine, error);
		engine->sink_errors = engine->sink_errors | (0x1 << sinkengine->position);
		osync_status_update_member(engine->parent, osync_client_proxy_get_member(proxy), OSYNC_CLIENT_EVENT_ERROR, engine->objtype, error);
	} else {
		engine->sink_sync_done = engine->sink_sync_done | (0x1 << sinkengine->position);
		osync_status_update_member(engine->parent, osync_client_proxy_get_member(proxy), OSYNC_CLIENT_EVENT_SYNC_DONE, engine->objtype, NULL);
	}
			
	if (BitCount(engine->sink_errors | engine->sink_sync_done) == g_list_length(engine->sink_engines)) {
		if (BitCount(engine->sink_sync_done) < BitCount(engine->sink_connects)) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Less sink_engines reported sync_done than connected");
			osync_obj_engine_set_error(engine, locerror);
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_SYNC_DONE);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->sink_errors | engine->sink_sync_done));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static osync_bool _create_mapping_engines(OSyncObjEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, error);
	
	int i = 0;
	for (i = 0; i < osync_mapping_table_num_mappings(engine->mapping_table); i++) {
		OSyncMapping *mapping = osync_mapping_table_nth_mapping(engine->mapping_table, i);
		
		OSyncMappingEngine *mapping_engine = osync_mapping_engine_new(engine, mapping, error);
		if (!mapping_engine)
			goto error;
		
		engine->mapping_engines = g_list_append(engine->mapping_engines, mapping_engine);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

OSyncObjEngine *osync_obj_engine_new(OSyncEngine *parent, const char *objtype, OSyncFormatEnv *formatenv, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p)", __func__, parent, objtype, formatenv, error);
	g_assert(parent);
	g_assert(objtype);
	
	OSyncObjEngine *engine = osync_try_malloc0(sizeof(OSyncObjEngine), error);
	if (!engine)
		goto error;
	engine->ref_count = 1;
	engine->slowsync = FALSE;
	
	/* we dont reference the parent to avoid circular dependencies. This object is completely
	 * dependent on the engine anyways */
	engine->parent = parent;
	
	engine->objtype = g_strdup(objtype);
	engine->formatenv = formatenv;
	
	engine->mapping_table = osync_mapping_table_new(error);
	if (!engine->mapping_table)
		goto error_free_engine;
	
	engine->archive = osync_engine_get_archive(parent);
	
	if (engine->archive) {
		if (!osync_mapping_table_load(engine->mapping_table, engine->archive, objtype, error))
			goto error_free_engine;
	}
	osync_trace(TRACE_INTERNAL, "Loaded %i mappings", osync_mapping_table_num_mappings(engine->mapping_table));
	
	int num = osync_engine_num_proxies(engine->parent);
	int i = 0;
	for (i = 0; i < num; i++) {
		OSyncClientProxy *proxy = osync_engine_nth_proxy(engine->parent, i);
		
		OSyncSinkEngine *sinkengine = osync_sink_engine_new(i, proxy, engine, error);
		if (!sinkengine)
			goto error_free_engine;
		
		engine->sink_engines = g_list_append(engine->sink_engines, sinkengine);
	}
	
	if (!_create_mapping_engines(engine, error))
		goto error_free_engine;
	
	osync_trace(TRACE_INTERNAL, "Created %i mapping engine", g_list_length(engine->mapping_engines));
			
	osync_trace(TRACE_EXIT, "%s: %p", __func__, engine);
	return engine;

error_free_engine:
	osync_obj_engine_unref(engine);
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
		while (engine->sink_engines) {
			OSyncSinkEngine *sinkengine = engine->sink_engines->data;
			osync_sink_engine_unref(sinkengine);
			
			engine->sink_engines = g_list_remove(engine->sink_engines, sinkengine);
		}
		
		while (engine->mapping_engines) {
			OSyncMappingEngine *mapping_engine = engine->mapping_engines->data;
			osync_mapping_engine_unref(mapping_engine);
			
			engine->mapping_engines = g_list_remove(engine->mapping_engines, mapping_engine);
		}
		
		if (engine->error)
			osync_error_unref(&engine->error);
			
		if (engine->objtype)
			g_free(engine->objtype);
		
		if (engine->mapping_table)
			osync_mapping_table_unref(engine->mapping_table);
		
		g_free(engine);
	}
}

const char *osync_obj_engine_get_objtype(OSyncObjEngine *engine)
{
	osync_assert(engine);
	return engine->objtype;
}

void osync_obj_engine_set_slowsync(OSyncObjEngine *engine, osync_bool slowsync)
{
	osync_assert(engine);
	engine->slowsync = slowsync;
}

static OSyncObjFormat **_get_member_formats(OSyncFormatEnv *env, OSyncClientProxy *proxy, const char *objtype, OSyncError **error)
{
	OSyncMember *member = osync_client_proxy_get_member(proxy);
	
	const OSyncList *formats = osync_member_get_objformats(member, objtype, error);
	osync_trace(TRACE_INTERNAL, "Found %i possible sink formats", osync_list_length(formats));
	
	if (!formats) {
		if (!osync_error_is_set(error))
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find a valid target format");
		return NULL;
	}
						
	int num = osync_list_length(formats);
	
	OSyncObjFormat **formatArray = osync_try_malloc0(sizeof(OSyncObjFormat *) * (num + 1), error);
	if (!formatArray)
		return NULL;
	
	const OSyncList *f = NULL;
	int i = 0;
	for (f = formats; f; f = f->next) {
		const char *formatstr = f->data;
		OSyncObjFormat *format = osync_format_env_find_objformat(env, formatstr);
		if (!format) {
			g_free(formatArray);
			return NULL;
		}
		
		formatArray[i] = format;
		i++;
	}
	formatArray[i] = NULL;
	
	return formatArray;
}

osync_bool osync_obj_engine_command(OSyncObjEngine *engine, OSyncEngineCmd cmd, OSyncError **error)
{
	GList *p = NULL;
	GList *m = NULL;
	GList *e = NULL;
	OSyncSinkEngine *sinkengine =  NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, engine, cmd, error);
	osync_assert(engine);
	
	switch (cmd) {
		case OSYNC_ENGINE_COMMAND_CONNECT:
			for (p = engine->sink_engines; p; p = p->next) {
				sinkengine = p->data;
				if (!osync_client_proxy_connect(sinkengine->proxy, _obj_engine_connect_callback, sinkengine, engine->objtype, engine->slowsync, error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_COMMAND_READ:
			for (p = engine->sink_engines; p; p = p->next) {
				sinkengine = p->data;
				if (!osync_client_proxy_get_changes(sinkengine->proxy, _obj_engine_read_callback, sinkengine, engine->objtype, error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_COMMAND_WRITE:
			if (engine->conflicts) {
				osync_trace(TRACE_INTERNAL, "We still have conflict. Delaying write");
				break;
			}
		
			if (engine->written) {
				osync_trace(TRACE_INTERNAL, "Already written");
				break;
			}
				
			engine->written = TRUE;
		
			/* Write the changes. First, we can multiply the winner in the mapping */
			osync_trace(TRACE_INTERNAL, "Preparing write. multiplying %i mappings", g_list_length(engine->mapping_engines));
			for (m = engine->mapping_engines; m; m = m->next) {
				OSyncMappingEngine *mapping_engine = m->data;
				if (!osync_mapping_engine_multiply(mapping_engine, error))
					goto error;
			}
				
			osync_trace(TRACE_INTERNAL, "Starting to write");
			for (p = engine->sink_engines; p; p = p->next) {
				sinkengine = p->data;
				
				for (e = sinkengine->entries; e; e = e->next) {
					OSyncMappingEntryEngine *entry_engine = e->data;
					osync_assert(entry_engine);
					
					osync_trace(TRACE_INTERNAL, "Entry %s for member %lli: Dirty: %i", osync_mapping_entry_get_uid(entry_engine->entry), osync_member_get_id(osync_client_proxy_get_member(sinkengine->proxy)), osync_entry_engine_is_dirty(entry_engine));
					if (osync_entry_engine_is_dirty(entry_engine)) {
						osync_assert(entry_engine->change);
						OSyncChange *change = entry_engine->change;
						
						osync_trace(TRACE_INTERNAL, "Starting to convert from objtype %s and format %s", osync_change_get_objtype(entry_engine->change), osync_objformat_get_name(osync_change_get_objformat(entry_engine->change)));
						/* We have to save the objtype of the change so that it does not get
						 * overwritten by the conversion */
						char *objtype = g_strdup(osync_change_get_objtype(change));
						
						/* Now we have to convert to one of the formats
						 * that the client can understand */
						OSyncObjFormat **formats = _get_member_formats(engine->formatenv, sinkengine->proxy, osync_change_get_objtype(entry_engine->change), error);
						if (!formats)
							goto error;
						
						OSyncFormatConverterPath *path = osync_format_env_find_path_formats(engine->formatenv, osync_change_get_objformat(entry_engine->change), formats, error);
						if (!path) {
							g_free(formats);
							goto error;
						}
						g_free(formats);
						
						if (!osync_format_env_convert(engine->formatenv, path, osync_change_get_data(entry_engine->change), error)) {
							osync_converter_path_unref(path);
							goto error;
						}
						osync_trace(TRACE_INTERNAL, "converted to format %s", osync_objformat_get_name(osync_change_get_objformat(entry_engine->change)));
						
						osync_converter_path_unref(path);
						
						osync_change_set_objtype(change, objtype);
						g_free(objtype);
						
						osync_trace(TRACE_INTERNAL, "Writing change %s, changetype %i, format %s , objtype %s from member %lli", osync_change_get_uid(change), osync_change_get_changetype(change), osync_objformat_get_name(osync_change_get_objformat(change)), osync_change_get_objtype(change), osync_member_get_id(osync_client_proxy_get_member(sinkengine->proxy)));
	
						if (!osync_client_proxy_commit_change(sinkengine->proxy, _obj_engine_commit_change_callback, entry_engine, osync_entry_engine_get_change(entry_engine), error))
							goto error;
					} else if (entry_engine->change) {
						OSyncMapping *mapping = entry_engine->mapping_engine->mapping;
						OSyncMember *member = osync_client_proxy_get_member(sinkengine->proxy);
						OSyncMappingEntry *entry = entry_engine->entry;
						
						if (engine->archive) {
							if (osync_change_get_changetype(entry_engine->change) == OSYNC_CHANGE_TYPE_DELETED) {
								if (!osync_archive_delete_change(engine->archive, osync_mapping_entry_get_id(entry), error))
									goto error;
							} else {
								if (!osync_archive_save_change(engine->archive, osync_mapping_entry_get_id(entry), osync_change_get_uid(entry_engine->change), osync_change_get_objtype(entry_engine->change), osync_mapping_get_id(mapping), osync_member_get_id(member), error))
									goto error;
							}
						}
					}
				}
				
				if (!osync_client_proxy_committed_all(sinkengine->proxy, _obj_engine_written_callback, sinkengine, engine->objtype, error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_COMMAND_SYNC_DONE:
			for (p = engine->sink_engines; p; p = p->next) {
				sinkengine = p->data;
				if (!osync_client_proxy_sync_done(sinkengine->proxy, _obj_engine_sync_done_callback, sinkengine, engine->objtype, error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_COMMAND_DISCONNECT:
			for (p = engine->sink_engines; p; p = p->next) {
				sinkengine = p->data;
				if (!osync_client_proxy_disconnect(sinkengine->proxy, _obj_engine_disconnect_callback, sinkengine, engine->objtype, error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_COMMAND_SOLVE:
		case OSYNC_ENGINE_COMMAND_DISCOVER:
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
	if (engine->error) {
		osync_error_stack(&error, &engine->error);
		osync_error_unref(&engine->error);
	}
	engine->error = error;
	osync_error_ref(&error);
}

