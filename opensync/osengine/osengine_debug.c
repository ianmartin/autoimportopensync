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

void osengine_print_all(OSyncEngine *engine)
{
	GList *i;
	GList *n;
	osync_debug("ENG", 2, "ENGINE:");
	osync_debug("ENG", 2, "running: %s", osync_flag_get_state(engine->fl_running) ? "YES" : "NO");
	osync_debug("ENG", 2, "sync: %s", osync_flag_get_state(engine->fl_sync) ? "YES" : "NO");
	osync_debug("ENG", 2, "stop: %s", osync_flag_get_state(engine->fl_stop) ? "YES" : "NO");
	osync_debug("ENG", 2, "sent changes: %s (no: %i, yes: %i)", osync_flag_get_state(engine->cmb_sent_changes) ? "YES" : "NO", engine->cmb_sent_changes->num_not_set, engine->cmb_sent_changes->num_set);
	osync_debug("ENG", 2, "all mapped: %s (no: %i, yes: %i)", osync_flag_get_state(engine->cmb_entries_mapped) ? "YES" : "NO", engine->cmb_entries_mapped->num_not_set, engine->cmb_entries_mapped->num_set);
	osync_debug("ENG", 2, "synced: %s (no: %i, yes: %i)", osync_flag_get_state(engine->cmb_synced) ? "YES" : "NO", engine->cmb_synced->num_not_set, engine->cmb_synced->num_set);
	osync_debug("ENG", 2, "conflicts checked: %s (no: %i, yes: %i)", osync_flag_get_state(engine->cmb_chkconflict) ? "YES" : "NO", engine->cmb_chkconflict->num_not_set, engine->cmb_chkconflict->num_set);
	osync_debug("ENG", 2, "finished: %s (no: %i, yes: %i)", osync_flag_get_state(engine->cmb_finished) ? "YES" : "NO", engine->cmb_finished->num_not_set, engine->cmb_finished->num_set);
	osync_debug("ENG", 2, "connected: %s (no: %i, yes: %i)", osync_flag_get_state(engine->cmb_connected) ? "YES" : "NO", engine->cmb_connected->num_not_set, engine->cmb_connected->num_set);
	osync_debug("ENG", 2, "Multiplied: %s (no: %i, yes: %i)", osync_flag_get_state(engine->cmb_multiplied) ? "YES" : "NO", engine->cmb_multiplied->num_not_set, engine->cmb_multiplied->num_set);
	
	for (i = engine->clients; i; i = i->next) {
		OSyncClient *client = i->data;
		osync_debug("ENG", 2, "\tCLIENT %lli %s:", osync_member_get_id(client->member), osync_member_get_pluginname(client->member));
		osync_debug("ENG", 2, "\tconnected: %s", osync_flag_get_state(client->fl_connected) ? "YES" : "NO");
		osync_debug("ENG", 2, "\tsent changes: %s", osync_flag_get_state(client->fl_sent_changes) ? "YES" : "NO");
		osync_debug("ENG", 2, "\tdone: %s", osync_flag_get_state(client->fl_done) ? "YES" : "NO");
		osync_debug("ENG", 2, "\tfinished: %s", osync_flag_get_state(client->fl_finished) ? "YES" : "NO");
	}

	for (i = engine->maptable->mappings; i; i = i->next) {
		OSyncMapping *mapping = i->data;
		osync_debug("ENG", 2, "MAPPING %p ID: %lli:", mapping, mapping->id);
		osync_debug("ENG", 2, "solved: %s", osync_flag_get_state(mapping->fl_solved) ? "YES" : "NO");
		osync_debug("ENG", 2, "synced: %s (no: %i, yes: %i)", osync_flag_get_state(mapping->cmb_synced) ? "YES" : "NO", mapping->cmb_synced->num_not_set, mapping->cmb_synced->num_set);
		osync_debug("ENG", 2, "conflict checked: %s", osync_flag_get_state(mapping->fl_chkconflict) ? "YES" : "NO");
		osync_debug("ENG", 2, "muliplied: %s", osync_flag_get_state(mapping->fl_multiplied) ? "YES" : "NO");
		osync_debug("ENG", 2, "has data: %s", osync_flag_get_state(mapping->cmb_has_data) ? "YES" : "NO");
		osync_debug("ENG", 2, "has info: %s (no: %i, yes: %i)", osync_flag_get_state(mapping->cmb_has_info) ? "YES" : "NO",  mapping->cmb_has_info->num_not_set, mapping->cmb_has_info->num_set);
		osync_debug("ENG", 2, "delete: %s (no: %i, yes: %i)", osync_flag_get_state(mapping->cmb_deleted) ? "YES" : "NO",  mapping->cmb_deleted->num_not_set, mapping->cmb_deleted->num_set);
		
		for (n = mapping->entries; n; n = n->next) {
			OSyncMappingEntry *entry = n->data;
			osync_debug("ENG", 2, "\tENTRY: %p, CHANGE %p, Member %lli:", entry, entry->change, osync_member_get_id(entry->client->member));
			osync_debug("ENG", 2, "\tuid: %s, changetype: %i", osync_change_get_uid(entry->change), osync_change_get_changetype(entry->change));
			osync_debug("ENG", 2, "\tObjType: %s, Format %s", osync_change_get_objtype(entry->change) ? osync_objtype_get_name(osync_change_get_objtype(entry->change)) : "None", osync_change_get_objformat(entry->change) ? osync_objformat_get_name(osync_change_get_objformat(entry->change)) : "None");
			osync_debug("ENG", 2, "\thas data: %s", osync_flag_get_state(entry->fl_has_data) ? "YES" : "NO");
			osync_debug("ENG", 2, "\tdirty: %s", osync_flag_get_state(entry->fl_dirty) ? "YES" : "NO");
			osync_debug("ENG", 2, "\tmapped: %s", osync_flag_get_state(entry->fl_mapped) ? "YES" : "NO");
			osync_debug("ENG", 2, "\thas info: %s", osync_flag_get_state(entry->fl_has_info) ? "YES" : "NO");
			osync_debug("ENG", 2, "\tsynced: %s", osync_flag_get_state(entry->fl_synced) ? "YES" : "NO");
			osync_debug("ENG", 2, "\tdeleted: %s", osync_flag_get_state(entry->fl_deleted) ? "YES" : "NO");
		}
	}
	
	for (n = engine->maptable->unmapped; n; n = n->next) {
		OSyncMappingEntry *entry = n->data;
		osync_debug("ENG", 2, "UNMAPPED ENTRY %p with change %p:", entry, entry->change);
		osync_debug("ENG", 2, "uid: %s, changetype: %i", osync_change_get_uid(entry->change), osync_change_get_changetype(entry->change));
		osync_debug("ENG", 2, "ObjType: %s, Format %s", osync_change_get_objtype(entry->change) ? osync_objtype_get_name(osync_change_get_objtype(entry->change)) : "None", osync_change_get_objformat(entry->change) ? osync_objformat_get_name(osync_change_get_objformat(entry->change)) : "None");
		osync_debug("ENG", 2, "has data: %s", osync_flag_get_state(entry->fl_has_data) ? "YES" : "NO");
		osync_debug("ENG", 2, "dirty: %s", osync_flag_get_state(entry->fl_dirty) ? "YES" : "NO");
		osync_debug("ENG", 2, "mapped: %s", osync_flag_get_state(entry->fl_mapped) ? "YES" : "NO");
		osync_debug("ENG", 2, "has info: %s", osync_flag_get_state(entry->fl_has_info) ? "YES" : "NO");
		osync_debug("ENG", 2, "synced: %s", osync_flag_get_state(entry->fl_synced) ? "YES" : "NO");
	}
}

void osengine_print_flags(OSyncEngine *engine)
{
	osync_trace(TRACE_INTERNAL, "ENG(RUN%i,STOP%i,SENT%i,READ%i,MAP%i,CHK%i,MUL%i,SYNC%i,COMMITTED%i)", \
		osync_flag_is_set(engine->fl_running), \
		osync_flag_is_not_set(engine->fl_stop), \
		osync_flag_is_set(engine->cmb_sent_changes), \
		osync_flag_is_set(engine->cmb_read_all), \
		osync_flag_is_set(engine->cmb_entries_mapped), \
		osync_flag_is_set(engine->cmb_chkconflict), \
		osync_flag_is_set(engine->cmb_multiplied), \
		osync_flag_is_set(engine->cmb_synced), \
		osync_flag_is_set(engine->cmb_committed_all));
}

void osync_client_print_flags(OSyncClient *client)
{
	osync_trace(TRACE_INTERNAL, "CL(CON%i,SENT%i,DONE%i,FIN%i,COMMITTED%i)", \
		osync_flag_is_set(client->fl_connected), \
		osync_flag_is_set(client->fl_sent_changes), \
		osync_flag_is_set(client->fl_done), \
		osync_flag_is_set(client->fl_finished), \
		osync_flag_is_set(client->fl_committed_all));
}

void osengine_mappingentry_print_flags(OSyncMappingEntry *entry)
{
	osync_trace(TRACE_INTERNAL, "ENT(DATA%i,DRY%i,MAP%i,INFO%i,SYNC%i,DEL%i)", \
		osync_flag_is_set(entry->fl_has_data), \
		osync_flag_is_set(entry->fl_dirty), \
		osync_flag_is_set(entry->fl_mapped), \
		osync_flag_is_set(entry->fl_has_info), \
		osync_flag_is_set(entry->fl_synced), \
		osync_flag_is_set(entry->fl_deleted));
}

void osengine_mapping_print_flags(OSyncMapping *mapping)
{
	osync_trace(TRACE_INTERNAL, "MAP(SOLV%i,SYNC%i,DATA%i,INFO%i,DEL%i,CHK%i,MUL%i)", \
		osync_flag_is_set(mapping->fl_solved), \
		osync_flag_is_set(mapping->cmb_synced), \
		osync_flag_is_set(mapping->cmb_has_data), \
		osync_flag_is_set(mapping->cmb_has_info), \
		osync_flag_is_set(mapping->cmb_deleted), \
		osync_flag_is_set(mapping->fl_chkconflict), \
		osync_flag_is_set(mapping->fl_multiplied));
}

void osengine_get_wasted(OSyncEngine *engine, int *all, int *wasted)
{
	*all = engine->alldeciders;
	*wasted = engine->wasted;
}
