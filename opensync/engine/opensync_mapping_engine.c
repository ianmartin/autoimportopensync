/*
 * libosengine - A synchronization engine for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007       Daniel Gollub <dgollub@suse.org>
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
#include "opensync-merger.h"
#include "opensync-plugin.h"

#include "opensync_obj_engine.h"
#include "opensync_obj_engine_internals.h"

#include "opensync_sink_engine_internals.h"
#include "opensync_mapping_entry_engine_internals.h"

#include "opensync_mapping_engine.h"
#include "opensync_mapping_engine_internals.h"

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

OSyncMappingEngine *osync_mapping_engine_ref(OSyncMappingEngine *engine)
{
	osync_assert(engine);
	
	g_atomic_int_inc(&(engine->ref_count));

	return engine;
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

static OSyncMappingEntryEngine *_osync_mapping_engine_find_entry(OSyncMappingEngine *engine, OSyncChange *change)
{
	GList *e;
	for (e = engine->entries; e; e = e->next) {
		OSyncMappingEntryEngine *entry = e->data;
		if (change && entry->change == change)
			return entry;
	}
	
	return NULL;
}

OSyncMember *osync_mapping_engine_change_find_member(OSyncMappingEngine *engine, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, change);

	OSyncMember *member = NULL;
	OSyncMappingEntryEngine *entry = _osync_mapping_engine_find_entry(engine, change);

	if (!entry)
		goto end;

	member = osync_client_proxy_get_member(entry->sink_engine->proxy);

end:	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, member);
	return member;
}

static OSyncMappingEntryEngine *_osync_mapping_engine_get_latest_entry(OSyncMappingEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, error);

	OSyncMappingEntryEngine *latest_entry = NULL; 
	OSyncChange *latest_change = NULL;
	time_t latest = 0;
	int i;

	osync_trace(TRACE_INTERNAL, "mapping number: %i", osync_mapping_engine_num_changes(engine));
	for (i=0; i < osync_mapping_engine_num_changes(engine); i++) {
		OSyncChange *change = osync_mapping_engine_nth_change(engine, i); 

		if (osync_change_get_changetype(change) == OSYNC_CHANGE_TYPE_UNKNOWN)
			continue;

		OSyncData *data = osync_change_get_data(change);

		if (!osync_data_has_data(data))
			continue;

		time_t cur = osync_data_get_revision(data, error);

		if (cur < 0)
			goto error;

		if (cur == latest) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Entries got changed at the same time. Can't decide.");
			goto error;
		}

		if (cur < latest)
			continue;

		latest = cur;
		latest_change = change;
	}

	if (!latest_change) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Can't find the latest change.");
		goto error;
	}

	latest_entry = _osync_mapping_engine_find_entry(engine, latest_change);

	if (!latest_entry) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Can't find the latest entry.");
		goto error;
	}

	osync_trace(TRACE_EXIT, "%s: %p", __func__, latest_entry);
	return latest_entry;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

osync_bool osync_mapping_engine_supports_ignore(OSyncMappingEngine *engine)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);
	osync_assert(engine);

	OSyncObjEngine *parent = engine->parent;

	osync_bool ignore_supported = TRUE;

	GList *s = NULL;
	for (s = parent->sink_engines; s; s = s->next) {
		OSyncSinkEngine *sink_engine = s->data;
		
		OSyncMember *member = osync_client_proxy_get_member(sink_engine->proxy);
		OSyncMappingEntryEngine *entry_engine = osync_mapping_engine_get_entry(engine, sink_engine);

		/* check if mapping could be solved by "ignore" conflict handler */
		const char *objtype = entry_engine->sink_engine->engine->objtype;
		OSyncObjTypeSink *objtype_sink = osync_member_find_objtype_sink(member, objtype);

		/* if there is no sink read function, ignore is not support for this mapping. */
		if (!objtype_sink || !osync_objtype_sink_get_function_read(objtype_sink))
			ignore_supported = FALSE; 

	}
	
	osync_trace(TRACE_EXIT, "%s: conflict handler ignore supported: %s", __func__, ignore_supported ? "TRUE" : "FALSE");
	return ignore_supported;
}

osync_bool osync_mapping_engine_supports_use_latest(OSyncMappingEngine *engine)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);
	osync_assert(engine);

	osync_bool latest_supported = TRUE;

	/* we ignore the error for now ... it's just a test if it would be possible/supported. */
	OSyncMappingEntryEngine *latest_entry = _osync_mapping_engine_get_latest_entry(engine, NULL);

	if (!latest_entry)
		latest_supported = FALSE; 
	
	osync_trace(TRACE_EXIT, "%s: conflict handler \"latest entry\" supported: %s", __func__, latest_supported ? "TRUE" : "FALSE");
	return latest_supported;
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
		
		osync_trace(TRACE_INTERNAL, "Propagating change %s to %p from %p", osync_mapping_entry_get_uid(entry_engine->entry), entry_engine, engine->master);
		
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
		
		osync_trace(TRACE_INTERNAL, "Orig change type: %i New change type: %i", existChangeType, newChangeType);

		/* Now update the entry with the change */
		osync_entry_engine_update(entry_engine, existChange);
		
		/* We have to use the uid of the entry, so that the member
		 * can correctly identify the entry 
		 *
		 * prahal: added a check if the entry has a uid to send the existing uid
		 * to the plugins in case we have a slow-sync (both are added and have a uid) 
		 * This to avoid creating duplicates by sending the plugins a different uid 
		 * with the same or merged data 
		 *
		 * dgollub: enhanced the check if the entry has a uid to send the existing uid
		 * to the plugins in case we have a slow-sync - both have changetype ADDED - and
		 * for odd plugins/protocolls which mark new entries as MODIFIED all the time.
		 * Changetype MODIFIED of new entries has atleast the IrMC plugin and likely some
		 * SE SyncML implementation...
		 * 
		 * ^^irmc hacks in the irmc plugin ;-)
		 *
		 * dgollub: Set masterChange UID for existChange if entry_engine->entry doesn't have 
		 * mapping uid, even if the newChangeType is UNKOWN. Bug: #571  
		 *
		 * prahal : rely on the fact that there are no mapping nor the entry existed to detect new change
		 *	    Also avoid changing the id of the change if one existed and there where no mapping :
		 *	    this way we send the id known to the member in case of a "modify". Fixing syncml plugin
		 *	    freezing the phone and mozilla sync receiving an id it cannot do anything with for modify
		 *	    in slow sync (no existing mapping).
		 *
		 */
		if ((!osync_mapping_entry_get_uid(entry_engine->entry) && !osync_change_get_uid(existChange))  ) 
			osync_change_set_uid(existChange, osync_change_get_uid(masterChange));
		else if(osync_mapping_entry_get_uid(entry_engine->entry)) 
			osync_change_set_uid(existChange, osync_mapping_entry_get_uid(entry_engine->entry));

		osync_change_set_data(existChange, newData);
		osync_change_set_changetype(existChange, osync_change_get_changetype(masterChange));
		
		/* We also have to update the changetype of the new change */
		if (newChangeType == OSYNC_CHANGE_TYPE_ADDED && (existChangeType != OSYNC_CHANGE_TYPE_DELETED && existChangeType != OSYNC_CHANGE_TYPE_UNKNOWN)) {
			osync_trace(TRACE_INTERNAL, "Updating change type to MODIFIED");
			osync_change_set_changetype(existChange, OSYNC_CHANGE_TYPE_MODIFIED);
		/* Only adapt the change to ADDED if the existing Change got deleted. Don't update it to ADDED if existChangeType is UNKOWN.
		   The exitChangeType is at least also UNKOWN if the file-sync has only one modified entry. */
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
	
	int is_same = 0;

	if (engine->master != NULL) {
		osync_trace(TRACE_EXIT, "%s: Already has a master", __func__);
		return;
	}
	
	if (engine->conflict) {
		osync_trace(TRACE_INTERNAL, "Detected conflict early");
		goto conflict;
	}
	
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

/*! @brief Search for the nth entry in the mapping
 * 
 * @param engine A pointer to the mapping engine
 * @param nth The value of the position
 * @returns The pointer to the nth change. NULL if there isn't enough entries in the mapping. 
 */
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

/*! @brief Search in the mapping for the change of the member.
 * 
 * @param engine A pointer to the mapping engine
 * @param memberid The member id of the request change.
 * @returns The pointer to the change of the member. NULL if member doesn't have an entry in this mapping.
 */
OSyncChange *osync_mapping_engine_member_change(OSyncMappingEngine *engine, int memberid)
{
	osync_assert(engine);
	
	GList *e = NULL;
	for (e = engine->entries; e; e = e->next) {
		OSyncMappingEntryEngine *entry = e->data;
		if (entry->change) {
			if (osync_mapping_entry_get_member_id(entry->entry) == memberid)
				return entry->change;
		}
	}
	
	return NULL;
}

osync_bool osync_mapping_engine_solve(OSyncMappingEngine *engine, OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, change);
	
	OSyncMappingEntryEngine *entry = _osync_mapping_engine_find_entry(engine, change);
	engine->conflict = FALSE;
	osync_mapping_engine_set_master(engine, entry);
	osync_status_update_mapping(engine->parent->parent, engine, OSYNC_MAPPING_EVENT_SOLVED, NULL);
	engine->parent->conflicts = g_list_remove(engine->parent->conflicts, engine);
	
	if (osync_engine_check_get_changes(engine->parent->parent) && osync_bitcount(engine->parent->sink_errors | engine->parent->sink_get_changes) == g_list_length(engine->parent->sink_engines)) {
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

osync_bool osync_mapping_engine_ignore(OSyncMappingEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, error);
	
	engine->conflict = FALSE;
	engine->synced = TRUE;

	OSyncObjEngine *objengine = engine->parent;
	OSyncArchive *archive = objengine->archive;
	char *objtype = objengine->objtype;
	long long int id = osync_mapping_get_id(engine->mapping);

	GList *c = NULL;
	for (c = engine->entries; c; c = c->next) {
		OSyncMappingEntryEngine *entry = c->data;
		osync_archive_save_ignored_conflict(archive, objtype, id, osync_change_get_changetype(entry->change), error);
	}

	osync_status_update_mapping(engine->parent->parent, engine, OSYNC_MAPPING_EVENT_SOLVED, NULL);
	engine->parent->conflicts = g_list_remove(engine->parent->conflicts, engine);
	
	if (osync_engine_check_get_changes(engine->parent->parent) && osync_bitcount(engine->parent->sink_errors | engine->parent->sink_get_changes) == g_list_length(engine->parent->sink_engines)) {
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

osync_bool osync_mapping_engine_use_latest(OSyncMappingEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, error);
	
	OSyncMappingEntryEngine *latest_entry = NULL;
	
	latest_entry = _osync_mapping_engine_get_latest_entry(engine, error);

	if (!latest_entry)
		goto error;

	osync_mapping_engine_set_master(engine, latest_entry);

	engine->conflict = FALSE;
	osync_status_update_mapping(engine->parent->parent, engine, OSYNC_MAPPING_EVENT_SOLVED, NULL);
	engine->parent->conflicts = g_list_remove(engine->parent->conflicts, engine);
	
	if (osync_engine_check_get_changes(engine->parent->parent) && osync_bitcount(engine->parent->sink_errors | engine->parent->sink_get_changes) == g_list_length(engine->parent->sink_engines)) {
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
		OSyncMappingEntryEngine *existingEntry = entries->data;
		
		/* Now lets see which mapping is the correct one for the entry */
		GList *m = NULL;
		OSyncMappingEngine *mapping = NULL;
		elevation = 0;
		OSyncChange *existingChange = NULL;
		for (m = mappings; m; m = m->next) {
			mapping = m->data;
			
			/* Get the first change of the mapping to test. Compare the given change with this change.
			 * If they are not the same, we have found a new mapping */
			GList *e = NULL;
			OSyncChange *change = NULL;
			OSyncMappingEntryEngine *entry = NULL;
			for (e = mapping->entries; e; e = e->next) {
				entry = e->data;
				change = entry->change;
				if (change)
					break;
			}
			
			if (!change || osync_change_compare(existingEntry->change, change) == OSYNC_CONV_DATA_SAME){
				existingChange = existingEntry->change;
				osync_change_ref(existingChange);
				break;
			}


			mapping = NULL;
			elevation++;
		
			existingChange = osync_change_clone(existingEntry->change, error);
		}
		
		
		if (!mapping) {
			/* Unable to find a mapping. We have to create a new one */
			mapping = _osync_obj_engine_create_mapping_engine(objengine, error);
			if (!mapping)
				goto error;
			mappings = g_list_append(mappings, mapping);
			objengine->mapping_engines = g_list_append(objengine->mapping_engines, mapping);
			osync_mapping_engine_ref(mapping);
		}
		
		/* update the uid and the content to suit the new level */
		osync_bool dirty = FALSE;
		if (!_osync_change_elevate(existingChange, elevation, &dirty, error))
			goto error;

		/* Lets add the entry to the mapping */
		OSyncMappingEntryEngine *newEntry = osync_mapping_engine_get_entry(mapping, existingEntry->sink_engine);
		osync_assert(newEntry);
		osync_entry_engine_update(newEntry, existingChange);
		osync_mapping_entry_set_uid(newEntry->entry, osync_change_get_uid(existingChange));
		osync_change_unref(existingChange);
		
		/* Set the last entry as the master */
		osync_mapping_engine_set_master(mapping, newEntry);
		
		/* Update the dirty status. If the duplicate function said
		 * that the returned item needs to be written, we will set
		 * this information here */
		newEntry->dirty = dirty;
		
		entries = g_list_remove(entries, existingEntry);
	}
	
	
	while (mappings) {
		OSyncMappingEngine *mapping = mappings->data;
		osync_mapping_engine_unref(mapping);
		mappings = g_list_remove(mappings, mapping);
	}
	
	objengine->conflicts = g_list_remove(objengine->conflicts, existingMapping);
	osync_status_update_mapping(objengine->parent, existingMapping, OSYNC_MAPPING_EVENT_SOLVED, NULL);
	
	if (osync_engine_check_get_changes(objengine->parent) && osync_bitcount(objengine->sink_errors | objengine->sink_get_changes) == g_list_length(objengine->sink_engines)) {
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

