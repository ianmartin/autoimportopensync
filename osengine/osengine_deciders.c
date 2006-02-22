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
 
#include "engine.h"
#include "engine_internals.h"

/**
 * @defgroup OSEngineDeciders OpenSync Deciders Internals
 * @ingroup OSEnginePrivate
 * @brief The internals of the engine (communication part)
 * 
 * This gives you an insight in the inner workings of the sync engine
 * 
 * 
 */
/*@{*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS
void osengine_mappingentry_decider(OSyncEngine *engine, OSyncMappingEntry *entry)
{
	osync_trace(TRACE_ENTRY, "osengine_mappingentry_decider(%p, %p)", engine, entry);
	osengine_print_flags(engine);
	osengine_mappingentry_print_flags(entry);
	
	engine->alldeciders++;
	
	if (osync_flag_is_set(engine->fl_running) \
	&& osync_flag_is_set(engine->fl_sync) \
	&& osync_flag_is_set(entry->fl_has_info) \
	&& osync_flag_is_not_set(entry->fl_has_data)) {
		osync_trace(TRACE_INTERNAL, "++++ ENGINE COMMAND: Get data (Entry %p) ++++", entry);
		//send_get_change_data(engine, entry);
		osync_trace(TRACE_EXIT, "osengine_mappingentry_decider");
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) \
	&& osync_flag_is_set(engine->cmb_read_all) \
	&& osync_flag_is_set(engine->cmb_sent_changes) \
	&& osync_flag_is_set(engine->fl_sync) \
	&& osync_flag_is_set(entry->fl_has_info) \
	&& osync_flag_is_set(entry->fl_has_data)) {
		if (osync_flag_is_not_set(entry->fl_mapped)) {
			osync_trace(TRACE_INTERNAL, "++++ ENGINE COMMAND: Mapping entry (Entry %p) ++++", entry);
			osengine_change_map(engine, entry);
			osync_trace(TRACE_EXIT, "osengine_mappingentry_decider");
			return;
		}
		if (osync_flag_is_set(entry->fl_dirty)) {
			osync_trace(TRACE_INTERNAL, "++++ ENGINE COMMAND: Commiting (Entry %p) ++++", entry);
			//send_commit_change(engine, entry);
			osync_trace(TRACE_EXIT, "osengine_mappingentry_decider");
			return;
		}
	}
	
	engine->wasted++;
	osync_trace(TRACE_EXIT, "osengine_mappingentry_decider: Waste");
}

void osengine_mappingentry_all_deciders(OSyncEngine *engine, OSyncMapping *mapping)
{
	osync_debug("ENG", 3, "Calling all mappingentry deciders (%i) for mapping %p", g_list_length(mapping->entries), mapping);
	GList *e;
	for (e = mapping->entries; e ; e = e->next) {
		//OSyncMappingEntry *entry = e->data;
		//send_mappingentry_changed(engine, entry);
	}
}

void osengine_mapping_decider(OSyncEngine *engine, OSyncMapping *mapping)
{
	osync_trace(TRACE_ENTRY, "osengine_mapping_decider(%p, %p)", engine, mapping);
	osengine_print_flags(engine);
	osengine_mapping_print_flags(mapping);

	engine->alldeciders++;
	
	if (osync_flag_is_set(engine->fl_running) \
	&& osync_flag_is_set(engine->cmb_sent_changes) \
	&& osync_flag_is_set(engine->cmb_read_all) \
	&& osync_flag_is_set(engine->cmb_entries_mapped) \
	&& osync_flag_is_set(mapping->cmb_has_data) \
	&& osync_flag_is_not_set(mapping->cmb_synced) \
	&& osync_flag_is_not_set(mapping->fl_solved) \
	&& osync_flag_is_not_set(mapping->fl_chkconflict)) {
		osync_trace(TRACE_INTERNAL, "++++ ENGINE COMMAND: Check Conflict (Mapping %p) ++++", mapping);
		osengine_mapping_check_conflict(engine, mapping);
		osync_trace(TRACE_EXIT, "osengine_mapping_decider");
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) \
	&& osync_flag_is_set(engine->cmb_sent_changes) \
	&& osync_flag_is_set(engine->cmb_read_all) \
	&& osync_flag_is_set(engine->cmb_entries_mapped) \
	&& osync_flag_is_set(mapping->cmb_has_data) \
	&& osync_flag_is_not_set(mapping->cmb_synced) \
	&& osync_flag_is_set(mapping->fl_solved) \
	&& osync_flag_is_set(mapping->fl_chkconflict) \
	&& osync_flag_is_not_set(mapping->fl_multiplied)) {
		osync_trace(TRACE_INTERNAL, "++++ ENGINE COMMAND: Multiply (Mapping %p) ++++", mapping);
		osengine_mapping_multiply_master(engine, mapping);
		osync_trace(TRACE_EXIT, "osengine_mapping_decider");
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) \
	&& osync_flag_is_set(engine->cmb_synced) \
	&& osync_flag_is_set(mapping->cmb_has_info) \
	&& osync_flag_is_not_set(mapping->cmb_deleted) \
	&& osync_flag_is_set(engine->cmb_multiplied)) {
		osync_trace(TRACE_INTERNAL, "++++ ENGINE COMMAND: Reset Mapping (Mapping %p) ++++", mapping);
		osengine_mapping_reset(mapping);
		osync_trace(TRACE_EXIT, "osengine_mapping_decider");
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) \
	&& osync_flag_is_set(mapping->cmb_synced) \
	&& osync_flag_is_set(mapping->cmb_deleted)) {
		osync_trace(TRACE_INTERNAL, "++++ ENGINE COMMAND: Deleting Mapping (Mapping %p) ++++", mapping);
		osengine_mapping_delete(mapping);
		osync_trace(TRACE_EXIT, "osengine_mapping_decider");
		return;
	}
	
	engine->wasted++;
	osync_trace(TRACE_EXIT, "osengine_mapping_decider: Waste");
}

void osengine_mapping_all_deciders(OSyncEngine *engine)
{
	GList *m;
	osync_trace(TRACE_INTERNAL, "Calling all mapping deciders (%i)", g_list_length(engine->maptable->mappings));
	for (m = engine->maptable->mappings; m; m = m->next) {
		//OSyncMapping *mapping = m->data;
		//send_mapping_changed(engine, mapping);
	}
}

void osengine_client_decider(OSyncEngine *engine, OSyncClient *client)
{
	osync_trace(TRACE_ENTRY, "osengine_client_decider(%p, %p)", engine, client);
	osengine_print_flags(engine);
	osync_client_print_flags(client);
	
	engine->alldeciders++;
	
	if (osync_flag_is_set(engine->fl_running) \
	&& osync_flag_is_not_set(engine->fl_stop) \
	&& osync_flag_is_not_set(client->fl_done) \
	&& osync_flag_is_not_set(client->fl_connected) \
	&& osync_flag_is_not_set(client->fl_finished)) {
		osync_trace(TRACE_INTERNAL, "++++ ENGINE COMMAND: Connecting (Client %p) ++++", client);
		osync_client_connect(client, engine, NULL);
		osync_trace(TRACE_EXIT, "osengine_client_decider");
		return;
	}
	
	//FIXME Remove cmb_connected once we fix the all conect "bug"
	if (osync_flag_is_set(engine->fl_running) \
	&& osync_flag_is_not_set(engine->fl_stop) \
	&& osync_flag_is_not_set(client->fl_done) \
	&& osync_flag_is_set(client->fl_connected) \
	&& osync_flag_is_not_set(client->fl_sent_changes) \
	&& osync_flag_is_set(engine->cmb_connected)) {
		osync_trace(TRACE_INTERNAL, "++++ ENGINE COMMAND: Get changes (Client %p) ++++", client);
		osync_client_get_changes(client, engine, NULL);
		osync_trace(TRACE_EXIT, "osengine_client_decider");
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) \
	&& osync_flag_is_not_set(engine->fl_stop) \
	&& osync_flag_is_not_set(client->fl_done) \
	&& osync_flag_is_set(client->fl_connected) \
	&& osync_flag_is_set(client->fl_sent_changes) \
	&& osync_flag_is_not_set(client->fl_committed_all) \
	&& osync_flag_is_set(engine->cmb_sent_changes) \
	&& osync_flag_is_set(engine->cmb_multiplied) \
	&& osync_flag_is_set(engine->cmb_read_all) \
	&& osync_flag_is_set(engine->cmb_entries_mapped) \
	&& osync_flag_is_set(engine->cmb_committed_all)) {
		osync_trace(TRACE_INTERNAL, "++++ ENGINE COMMAND: Committed all (Client %p) ++++", client);
		//send_committed_all(client, engine);
		osync_trace(TRACE_EXIT, "osengine_client_decider");
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) \
	&& osync_flag_is_not_set(engine->fl_stop) \
	&& osync_flag_is_not_set(client->fl_done) \
	&& osync_flag_is_set(client->fl_connected) \
	&& osync_flag_is_set(client->fl_sent_changes) \
	&& osync_flag_is_set(client->fl_committed_all) \
	&& osync_flag_is_set(engine->cmb_read_all) \
	&& osync_flag_is_set(engine->cmb_sent_changes) \
	&& osync_flag_is_set(engine->cmb_synced) \
	&& osync_flag_is_set(engine->cmb_entries_mapped)) {
		osync_trace(TRACE_INTERNAL, "++++ ENGINE COMMAND: Sync done (Client %p) ++++", client);
		//send_sync_done(client, engine);
		osync_trace(TRACE_EXIT, "osengine_client_decider");
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) \
	&& (osync_flag_is_set(client->fl_done) \
	|| osync_flag_is_set(engine->fl_stop)) \
	&& osync_flag_is_set(client->fl_connected)) {
		osync_trace(TRACE_INTERNAL, "++++ ENGINE COMMAND: Disconnecting (Client %p) ++++", client);
		//send_disconnect(client, engine);
		osync_trace(TRACE_EXIT, "osengine_client_decider");
		return;
	}
	
	engine->wasted++;
	osync_trace(TRACE_EXIT, "osengine_client_decider: Waste");
}

void osengine_client_all_deciders(OSyncEngine *engine)
{
	GList *c;
	osync_debug("ENG", 3, "Calling all client deciders (%i)", g_list_length(engine->clients));
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		osengine_client_decider(engine, client);
	}
}
#endif

/** @} */
