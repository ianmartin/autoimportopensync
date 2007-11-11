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
#include "opensync-merger.h"
#include "opensync-plugin.h"

#include "opensync_sink_engine_internals.h"
#include "opensync_mapping_entry_engine_internals.h"

#include "opensync_mapping_engine.h"
#include "opensync_mapping_engine_internals.h"

#include "opensync_obj_engine.h"
#include "opensync_obj_engine_internals.h"

OSyncMappingEngine *_osync_obj_engine_create_mapping_engine(OSyncObjEngine *engine, OSyncError **error)
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

static void _obj_engine_connect_callback(OSyncClientProxy *proxy, void *userdata, osync_bool slowsync, OSyncError *error)
{
	OSyncSinkEngine *sinkengine = userdata;
	OSyncObjEngine *engine = sinkengine->engine;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p)", __func__, proxy, userdata, slowsync, error);
	
	if (error) {
		osync_trace(TRACE_INTERNAL, "Obj Engine received connect error: %s", osync_error_print(&error));
		osync_obj_engine_set_error(engine, error);
		engine->sink_errors = engine->sink_errors | (0x1 << sinkengine->position);
		osync_status_update_member(engine->parent, osync_client_proxy_get_member(proxy), OSYNC_CLIENT_EVENT_ERROR, engine->objtype, error);
	} else {
		engine->sink_connects = engine->sink_connects | (0x1 << sinkengine->position);
		osync_status_update_member(engine->parent, osync_client_proxy_get_member(proxy), OSYNC_CLIENT_EVENT_CONNECTED, engine->objtype, NULL);
	}

	if (slowsync) {
		osync_obj_engine_set_slowsync(engine, TRUE);
		osync_trace(TRACE_INTERNAL, "SlowSync requested during connect.");
	}
			
	if (osync_bitcount(engine->sink_errors | engine->sink_connects) == g_list_length(engine->sink_engines)) {
		if (osync_bitcount(engine->sink_connects) < 2) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Less than 2 sink_engines are connected");
			osync_obj_engine_set_error(engine, locerror);
			
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else {
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_CONNECTED);
		}
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", osync_bitcount(engine->sink_errors | engine->sink_connects));
	
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
			
	if (osync_bitcount(engine->sink_errors | engine->sink_disconnects) == g_list_length(engine->sink_engines)) {
		if (osync_bitcount(engine->sink_disconnects) < osync_bitcount(engine->sink_connects)) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Fewer sink_engines disconnected than connected");
			osync_obj_engine_set_error(engine, locerror);
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_DISCONNECTED);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", osync_bitcount(engine->sink_errors | engine->sink_disconnects));
	
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
				mapping_engine = _osync_obj_engine_create_mapping_engine(engine, error);
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

static void _obj_engine_read_ignored_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	// NOOP	
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
	
	if (osync_bitcount(engine->sink_errors | engine->sink_get_changes) == g_list_length(engine->sink_engines)) {
		
		if (osync_bitcount(engine->sink_get_changes) < osync_bitcount(engine->sink_connects)) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Fewer sink_engines reported get_changes than connected");
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
		osync_trace(TRACE_INTERNAL, "Not yet: %i", osync_bitcount(engine->sink_errors | engine->sink_get_changes));
	
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

		OSyncMember *member = osync_client_proxy_get_member(sinkengine->proxy);
		OSyncObjTypeSink *objtype_sink = osync_member_find_objtype_sink(member, engine->objtype);

		/* If the sink engine isn't able/allowed to write we don't care if everything got written ("how dirty is it!?") */ 
		if (!objtype_sink || !osync_objtype_sink_get_write(objtype_sink)) 
			break;
		
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
	if (osync_bitcount(engine->sink_errors | engine->sink_written) == g_list_length(engine->sink_engines)) {
		if (osync_bitcount(engine->sink_written) < osync_bitcount(engine->sink_connects)) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Fewer sink_engines reported committed all than connected");
			osync_obj_engine_set_error(engine, locerror);
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_WRITTEN);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", osync_bitcount(engine->sink_errors | engine->sink_written));
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
	const char *objtype = osync_change_get_objtype(entry_engine->change);
	long long int id = osync_mapping_entry_get_id(entry);
	
	if (error) {
		osync_status_update_change(engine->parent, entry_engine->change, osync_client_proxy_get_member(proxy), entry_engine->mapping_engine->mapping, OSYNC_CHANGE_EVENT_ERROR, error);
		goto end;
	}
	
	if (uid)
		osync_change_set_uid(entry_engine->change, uid);
	
	if (engine->archive) {
		if (osync_change_get_changetype(entry_engine->change) == OSYNC_CHANGE_TYPE_DELETED) {
			osync_archive_delete_change(engine->archive, id, objtype, &locerror);
		} else {
			osync_archive_save_change(engine->archive, id, osync_change_get_uid(entry_engine->change), objtype, osync_mapping_get_id(mapping), osync_member_get_id(member), &locerror);
		}
	}

	osync_assert(entry_engine->mapping_engine);
	osync_status_update_change(engine->parent, entry_engine->change, osync_client_proxy_get_member(proxy), entry_engine->mapping_engine->mapping, OSYNC_CHANGE_EVENT_WRITTEN, NULL);
	osync_entry_engine_update(entry_engine, NULL);
	
end:	
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
			
	if (osync_bitcount(engine->sink_errors | engine->sink_sync_done) == g_list_length(engine->sink_engines)) {
		if (osync_bitcount(engine->sink_sync_done) < osync_bitcount(engine->sink_connects)) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Fewer sink_engines reported sync_done than connected");
			osync_obj_engine_set_error(engine, locerror);
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_ERROR);
		} else
			osync_obj_engine_event(engine, OSYNC_ENGINE_EVENT_SYNC_DONE);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", osync_bitcount(engine->sink_errors | engine->sink_sync_done));
	
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

static osync_bool _inject_changelog_entries(OSyncObjEngine *engine, OSyncError **error) {

	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);

	osync_assert(engine);
	osync_assert(engine->archive);
	osync_assert(engine->objtype);
	
	OSyncList *ids = NULL;
	OSyncList *changetypes = NULL;

	if (!osync_archive_load_ignored_conflicts(engine->archive, engine->objtype, &ids, &changetypes, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	OSyncList *j;
	OSyncList *t = changetypes;
	for (j = ids; j; j = j->next) {
		long long int id = (long long int)GPOINTER_TO_INT(j->data);

		OSyncMapping *ignored_mapping = osync_mapping_table_find_mapping(engine->mapping_table, id);

		GList *e;
		for (e = engine->mapping_engines; e; e = e->next) {
			OSyncMappingEngine *mapping_engine = e->data;

			if (mapping_engine->mapping == ignored_mapping) {
				GList *m;
				for (m = mapping_engine->entries; m; m = m->next) {
					OSyncMappingEntryEngine *entry = m->data;

					OSyncChangeType changetype = (OSyncChangeType) t->data;

					OSyncChange *ignored_change = osync_change_new(error);
					osync_change_set_changetype(ignored_change, changetype); 
					osync_entry_engine_update(entry, ignored_change);

					OSyncObjFormat *dummyformat = osync_objformat_new("plain", engine->objtype, NULL);
					OSyncData *data = osync_data_new(NULL, 0, dummyformat, NULL);
					osync_change_set_data(ignored_change, data);
					osync_objformat_unref(dummyformat);

					osync_change_set_uid(ignored_change, osync_mapping_entry_get_uid(entry->entry));

					osync_trace(TRACE_INTERNAL, "CHANGE: %p", entry->change);
				}
				break;
			}
		}

		t = t->next;
	}

	osync_list_free(ids);
	osync_list_free(changetypes);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
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
	engine->written = FALSE;
	
	/* we dont reference the parent to avoid circular dependencies. This object is completely
	 * dependent on the engine anyways */
	engine->parent = parent;
	
	engine->objtype = g_strdup(objtype);
	engine->formatenv = formatenv;
	
	engine->mapping_table = osync_mapping_table_new(error);
	if (!engine->mapping_table)
		goto error_free_engine;
	
	engine->archive = osync_engine_get_archive(parent);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, engine);
	return engine;

error_free_engine:
	osync_obj_engine_unref(engine);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

OSyncObjEngine *osync_obj_engine_ref(OSyncObjEngine *engine)
{
	osync_assert(engine);
	
	g_atomic_int_inc(&(engine->ref_count));

	return engine;
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

static int _osync_obj_engine_num_write_sinks(OSyncObjEngine *objengine) {
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, objengine);

	int num = 0;
	GList *p = NULL;
	OSyncSinkEngine *sink;

	for (p = objengine->sink_engines; p; p = p->next) {
		sink = p->data;


	}

	osync_trace(TRACE_EXIT, "%s: %i", __func__, num);
	return num;
}

osync_bool osync_obj_engine_initialize(OSyncObjEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, error);

	osync_trace(TRACE_INTERNAL, "Loaded %i mappings", osync_mapping_table_num_mappings(engine->mapping_table));
	
	int num = osync_engine_num_proxies(engine->parent);
	int i = 0;
	for (i = 0; i < num; i++) {
		OSyncClientProxy *proxy = osync_engine_nth_proxy(engine->parent, i);
		
		OSyncSinkEngine *sinkengine = osync_sink_engine_new(i, proxy, engine, error);
		if (!sinkengine)
			goto error;
		
		engine->sink_engines = g_list_append(engine->sink_engines, sinkengine);
	}

	if (engine->archive && engine->slowsync) {
		if (!osync_mapping_table_flush(engine->mapping_table, engine->archive, engine->objtype, error))
			goto error;
	}

	if (engine->archive) {
		if (!osync_mapping_table_load(engine->mapping_table, engine->archive, engine->objtype, error))
			goto error;
	}

	if (!_create_mapping_engines(engine, error))
		goto error;
	
	osync_trace(TRACE_INTERNAL, "Created %i mapping engine", g_list_length(engine->mapping_engines));

	if (engine->archive) {
		/* inject ignored conflicts from previous syncs */
		if (!_inject_changelog_entries(engine, error))
			goto error;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

void osync_obj_engine_finalize(OSyncObjEngine *engine)
{
	engine->slowsync = FALSE;
	engine->written = FALSE;

	engine->sink_errors = 0;
	engine->sink_connects = 0;
	engine->sink_disconnects = 0;
	engine->sink_get_changes = 0;
	engine->sink_sync_done = 0;
	engine->sink_written = 0;

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
	
	if (engine->mapping_table)
		osync_mapping_table_close(engine->mapping_table);

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

osync_bool osync_obj_engine_get_slowsync(OSyncObjEngine *engine)
{
	osync_assert(engine);
	return engine->slowsync;
}

static OSyncObjFormat **_get_member_formats(OSyncFormatEnv *env, OSyncClientProxy *proxy, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, %p)", __func__, env, proxy, objtype, error);
	OSyncMember *member = osync_client_proxy_get_member(proxy);
	
	const OSyncList *formats = osync_member_get_objformats(member, objtype, error);
	if (!formats) {
		if (!osync_error_is_set(error))
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find a valid target format");
		goto error;
	}

	osync_trace(TRACE_INTERNAL, "Found %i possible sink formats", osync_list_length(formats));
						
	int num = osync_list_length(formats);
	
	OSyncObjFormat **formatArray = osync_try_malloc0(sizeof(OSyncObjFormat *) * (num + 1), error);
	if (!formatArray)
		goto error;
	
	const OSyncList *f = NULL;
	int i = 0;
	for (f = formats; f; f = f->next) {
		const char *formatstr = f->data;
		OSyncObjFormat *format = osync_format_env_find_objformat(env, formatstr);
		if (!format) {
			g_free(formatArray);
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find a valid object format for \"%s\"", formatstr);
			goto error;
		}
		
		formatArray[i] = format;
		i++;
	}
	formatArray[i] = NULL;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, formatArray);
	return formatArray;

error:	
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
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
				for (m = sinkengine->entries; m; m = m->next) {
					OSyncMappingEntryEngine *entry = m->data;
					OSyncChange *change = entry->change;

					if (!change)
						continue;

					if (!osync_client_proxy_read(sinkengine->proxy, _obj_engine_read_ignored_callback, sinkengine, change, error))
						goto error;
				}
			}

			if (engine->archive) {
				/* Flush the changelog - to avoid double entries of ignored entries */
				if (!osync_archive_flush_ignored_conflict(engine->archive, engine->objtype, error))
					goto error;
			}

			int write_sinks = _osync_obj_engine_num_write_sinks(engine);

			/* Get change entries since last sync. (get_changes) */
			for (p = engine->sink_engines; p; p = p->next) {
				sinkengine = p->data;


				OSyncMember *member = osync_client_proxy_get_member(sinkengine->proxy);
				OSyncObjTypeSink *objtype_sink = osync_member_find_objtype_sink(member, engine->objtype);

				/* Is there at least one other writeable sink? */
				if (objtype_sink && osync_objtype_sink_get_write(objtype_sink) && write_sinks) {
					_obj_engine_read_callback(sinkengine->proxy, sinkengine, *error);
					osync_trace(TRACE_INTERNAL, "no other writable sinks .... SKIP");
					continue;
				}

				if (!osync_client_proxy_get_changes(sinkengine->proxy, _obj_engine_read_callback, sinkengine, engine->objtype, engine->slowsync, error))
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
				
				OSyncMember *member = osync_client_proxy_get_member(sinkengine->proxy);
				long long int memberid = osync_member_get_id(member);

				OSyncObjTypeSink *objtype_sink = osync_member_find_objtype_sink(member, engine->objtype);

				for (e = sinkengine->entries; e; e = e->next) {
					OSyncMappingEntryEngine *entry_engine = e->data;
					osync_assert(entry_engine);

					/* Merger - Save the entire xml and demerge */
					/* TODO: is here the right place to save the xml???? */
					if (osync_group_get_merger_enabled(osync_engine_get_group(engine->parent)) &&
						osync_group_get_converter_enabled(osync_engine_get_group(engine->parent)) &&	
						entry_engine->change &&
						(osync_change_get_changetype(entry_engine->change) != OSYNC_CHANGE_TYPE_DELETED) &&
						!strncmp(osync_objformat_get_name(osync_change_get_objformat(entry_engine->change)), "xmlformat-", 10) )
					{
						osync_trace(TRACE_INTERNAL, "Entry %s for member %lli: Dirty: %i", osync_change_get_uid(entry_engine->change), memberid, osync_entry_engine_is_dirty(entry_engine));

						osync_trace(TRACE_INTERNAL, "Save the entire XMLFormat and demerge.");
						char *buffer = NULL;
						unsigned int size = 0;
						OSyncXMLFormat *xmlformat = NULL;
						const char *uid = osync_change_get_uid(entry_engine->change);
						const char *objtype = osync_change_get_objtype(entry_engine->change);
						
						xmlformat = (OSyncXMLFormat *) osync_data_get_data_ptr(osync_change_get_data(entry_engine->change));
						if(!osync_xmlformat_assemble(xmlformat, &buffer, &size)) {
							osync_error_set(error, OSYNC_ERROR_GENERIC, "Could not assamble the xmlformat");
							goto error;	
						}

						if(!osync_archive_save_data(engine->archive, uid, objtype, buffer, size, error)) {
							g_free(buffer);	
							goto error;			
						}
						g_free(buffer);
						
						OSyncMerger *merger = NULL; 
						merger = osync_member_get_merger(osync_client_proxy_get_member(sinkengine->proxy));
						if(merger)
							osync_merger_demerge(merger, xmlformat);
					}


					/* Only commit change if the objtype sink is able/allowed to write. */
					if (objtype_sink && osync_objtype_sink_get_write(objtype_sink) && osync_entry_engine_is_dirty(entry_engine)) {
						osync_assert(entry_engine->change);
						OSyncChange *change = entry_engine->change;

						/* Convert to requested target format */
						if (osync_group_get_converter_enabled(osync_engine_get_group(engine->parent))) {

							
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
						}
						
						osync_trace(TRACE_INTERNAL, "Writing change %s, changetype %i, format %s , objtype %s from member %lli", osync_change_get_uid(change), osync_change_get_changetype(change), osync_objformat_get_name(osync_change_get_objformat(change)), osync_change_get_objtype(change), osync_member_get_id(osync_client_proxy_get_member(sinkengine->proxy)));
	
						if (!osync_client_proxy_commit_change(sinkengine->proxy, _obj_engine_commit_change_callback, entry_engine, osync_entry_engine_get_change(entry_engine), error))
							goto error;
					} else if (entry_engine->change) {
						OSyncMapping *mapping = entry_engine->mapping_engine->mapping;
						OSyncMember *member = osync_client_proxy_get_member(sinkengine->proxy);
						OSyncMappingEntry *entry = entry_engine->entry;
						const char *objtype = osync_change_get_objtype(entry_engine->change);
						
						if (engine->archive) {
							if (osync_change_get_changetype(entry_engine->change) == OSYNC_CHANGE_TYPE_DELETED) {
								if (!osync_archive_delete_change(engine->archive, osync_mapping_entry_get_id(entry), objtype, error))
									goto error;
							} else {
								if (!osync_archive_save_change(engine->archive, osync_mapping_entry_get_id(entry), osync_change_get_uid(entry_engine->change), objtype, osync_mapping_get_id(mapping), osync_member_get_id(member), error))
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
