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

void osengine_mappingentry_decider(OSyncEngine *engine, OSyncMappingEntry *entry)
{
	osync_trace(TRACE_ENTRY, "osengine_mappingentry_decider(%p, %p)", engine, entry);

	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(engine->fl_sync) && osync_flag_is_set(entry->fl_has_info) && osync_flag_is_not_set(entry->fl_has_data)) {
		osync_debug("CHG", 2, "Getting entry data for entry %p for client %p", entry, entry->view->client->member);
		send_get_change_data(engine, entry);
		osync_trace(TRACE_EXIT, "osengine_mappingentry_decider");
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(engine->cmb_sent_changes) && osync_flag_is_set(engine->fl_sync) && osync_flag_is_set(entry->fl_has_info) && osync_flag_is_set(entry->fl_has_data)) {
		if (osync_flag_is_not_set(entry->fl_mapped)) {
			osengine_change_map(engine, entry);
			osync_trace(TRACE_EXIT, "osengine_mappingentry_decider");
			return;
		}
		if (osync_flag_is_set(entry->fl_dirty)) {
			osync_debug("CHG", 2, "Writing entry to remote side (commit)");
			send_commit_change(engine, entry);
			osync_trace(TRACE_EXIT, "osengine_mappingentry_decider");
			return;
		}
	}
	
	osync_trace(TRACE_EXIT, "osengine_mapping_decider: Waste");
}

void osengine_mappingentry_all_deciders(OSyncEngine *engine, OSyncMapping *mapping)
{
	osync_debug("ENG", 3, "Calling all mappingentry deciders (%i) for mapping %p", g_list_length(mapping->entries), mapping);
	GList *e;
	for (e = mapping->entries; e ; e = e->next) {
		OSyncMappingEntry *entry = e->data;
		send_mappingentry_changed(engine, entry);
	}
}

void osengine_mapping_decider(OSyncEngine *engine, OSyncMapping *mapping)
{
	osync_trace(TRACE_ENTRY, "osengine_mapping_decider(%p, %p)", engine, mapping);
	osync_trace(TRACE_INTERNAL, "SOLV%i,SYNC%i,DATA%i,INFO%i,DEL%i,CHK%i", osync_flag_is_set(mapping->fl_solved), osync_flag_is_set(mapping->cmb_synced), osync_flag_is_set(mapping->cmb_has_data), osync_flag_is_set(mapping->cmb_has_info), osync_flag_is_set(mapping->cmb_deleted), osync_flag_is_set(mapping->fl_chkconflict));

	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(engine->cmb_sent_changes) && osync_flag_is_set(engine->cmb_entries_mapped) && osync_flag_is_set(mapping->cmb_has_data) && osync_flag_is_not_set(mapping->cmb_synced) && osync_flag_is_not_set(mapping->fl_solved)) {
		osengine_mapping_check_conflict(engine, mapping);
		osync_trace(TRACE_EXIT, "osengine_mapping_decider");
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(mapping->cmb_synced) && osync_flag_is_set(mapping->cmb_has_info) && osync_flag_is_not_set(mapping->cmb_deleted)) {
		osync_debug("MAP", 2, "Reseting mapping %p", mapping);
		osengine_mapping_reset(mapping);
		osync_trace(TRACE_EXIT, "osengine_mapping_decider");
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(mapping->cmb_synced) && osync_flag_is_set(mapping->cmb_deleted)) {
		osync_debug("MAP", 2, "Deleting mapping %p", mapping);
		osengine_mapping_delete(mapping);
		osync_trace(TRACE_EXIT, "osengine_mapping_decider");
		return;
	}
	
	osync_trace(TRACE_EXIT, "osengine_mapping_decider: Waste");
}

void osengine_mapping_all_deciders(OSyncEngine *engine)
{
	GList *m;
	osync_debug("ENG", 4, "Calling all mapping deciders (%i)", g_list_length(engine->maptable->mappings));
	for (m = engine->maptable->mappings; m; m = m->next) {
		OSyncMapping *mapping = m->data;
		send_mapping_changed(engine, mapping);
	}
}

void osengine_client_decider(OSyncEngine *engine, OSyncClient *client)
{
	osync_trace(TRACE_ENTRY, "osengine_client_decider(%p, %p)", engine, client);
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_not_set(engine->fl_stop) && osync_flag_is_not_set(client->fl_done) && osync_flag_is_not_set(client->fl_connected) && osync_flag_is_not_set(client->fl_finished)) {
		osync_debug("ENG", 2, "Telling client %p to connect", client);
		send_connect(client, engine);
		osync_trace(TRACE_EXIT, "osengine_client_decider");
		return;
	}
	
	//FIXME Remove cmb_connected once we fix the all conect "bug"
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_not_set(engine->fl_stop) && osync_flag_is_not_set(client->fl_done) && osync_flag_is_set(client->fl_connected) && osync_flag_is_not_set(client->fl_sent_changes) && osync_flag_is_set(engine->cmb_connected)) {
		if (osync_flag_is_set(engine->fl_sync)) {
			osync_debug("ENG", 2, "Telling client %p to send changes with data", client);
			send_get_changes(client, engine, TRUE);
		} else {
			osync_debug("ENG", 2, "Telling client %p to send changes without data", client);
			send_get_changes(client, engine, FALSE);
		}
		osync_trace(TRACE_EXIT, "osengine_client_decider");
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_not_set(engine->fl_stop) && osync_flag_is_not_set(client->fl_done) && osync_flag_is_set(client->fl_connected) && osync_flag_is_set(client->fl_sent_changes) && osync_flag_is_set(engine->cmb_sent_changes) && osync_flag_is_set(engine->cmb_synced) && osync_flag_is_set(engine->cmb_entries_mapped)) {
		osync_debug("ENG", 2, "Telling client %p to call sync_done", client);
		send_sync_done(client, engine);
		osync_trace(TRACE_EXIT, "osengine_client_decider");
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(client->fl_done) && osync_flag_is_set(client->fl_connected)) {
		osync_debug("ENG", 2, "Telling client %p to disconnect", client);
		send_disconnect(client, engine);
		osync_trace(TRACE_EXIT, "osengine_client_decider");
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(engine->fl_stop) && osync_flag_is_set(client->fl_connected)) {
		osync_debug("ENG", 2, "Telling client %p to disconnect", client);
		send_disconnect(client, engine);
		osync_trace(TRACE_EXIT, "osengine_client_decider");
		return;
	}

	osync_trace(TRACE_EXIT, "osengine_client_decider: Waste");
}

void osengine_client_all_deciders(OSyncEngine *engine)
{
	GList *c;
	osync_debug("ENG", 3, "Calling all client deciders (%i)", g_list_length(engine->clients));
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		send_client_changed(engine, client);
	}
}
