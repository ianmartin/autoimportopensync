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

void osync_engine_print_all(OSyncEngine *engine)
{
	int i;
	int n;
	OSyncChange *change;
	osync_debug("ENG", 2, "ENGINE:");
	osync_debug("ENG", 2, "running: %s", osync_flag_get_state(engine->fl_running) ? "YES" : "NO");
	osync_debug("ENG", 2, "sync: %s", osync_flag_get_state(engine->fl_sync) ? "YES" : "NO");
	osync_debug("ENG", 2, "stop: %s", osync_flag_get_state(engine->fl_stop) ? "YES" : "NO");
	osync_debug("ENG", 2, "sent changes: %s (no: %i, yes: %i)", osync_flag_get_state(engine->cmb_sent_changes) ? "YES" : "NO", engine->cmb_sent_changes->num_not_set, engine->cmb_sent_changes->num_set);
	osync_debug("ENG", 2, "all mapped: %s (no: %i, yes: %i)", osync_flag_get_state(engine->cmb_entries_mapped) ? "YES" : "NO", engine->cmb_entries_mapped->num_not_set, engine->cmb_entries_mapped->num_set);
	osync_debug("ENG", 2, "synced: %s (no: %i, yes: %i)", osync_flag_get_state(engine->cmb_synced) ? "YES" : "NO", engine->cmb_synced->num_not_set, engine->cmb_synced->num_set);
	osync_debug("ENG", 2, "finished: %s", osync_flag_get_state(engine->cmb_finished) ? "YES" : "NO");
	osync_debug("ENG", 2, "connected: %s (no: %i, yes: %i)", osync_flag_get_state(engine->cmb_connected) ? "YES" : "NO", engine->cmb_connected->num_not_set, engine->cmb_connected->num_set);
	
	for (i = 0; i < g_list_length(engine->clients); i++) {
		OSyncClient *client = g_list_nth_data(engine->clients, i);
		osync_debug("ENG", 2, "\tCLIENT %lli %s:", osync_member_get_id(client->member), osync_member_get_pluginname(client->member));
		osync_debug("ENG", 2, "\tconnected: %s", osync_flag_get_state(client->fl_connected) ? "YES" : "NO");
		osync_debug("ENG", 2, "\tsent changes: %s", osync_flag_get_state(client->fl_sent_changes) ? "YES" : "NO");
		osync_debug("ENG", 2, "\tdone: %s", osync_flag_get_state(client->fl_done) ? "YES" : "NO");
		osync_debug("ENG", 2, "\tfinished: %s", osync_flag_get_state(client->fl_finished) ? "YES" : "NO");
	}

	for (i = 0; i < osync_mappingtable_num_mappings(engine->maptable); i++) {
		OSyncMapping *mapping = osync_mappingtable_nth_mapping(engine->maptable, i);
		osync_debug("ENG", 2, "MAPPING %p:", mapping);
		MSyncMappingFlags *mapflags = osync_mapping_get_flags(mapping);
		osync_debug("ENG", 2, "solved: %s", osync_flag_get_state(mapflags->fl_solved) ? "YES" : "NO");
		osync_debug("ENG", 2, "synced: %s (no: %i, yes: %i)", osync_flag_get_state(mapflags->cmb_synced) ? "YES" : "NO", mapflags->cmb_synced->num_not_set, mapflags->cmb_synced->num_set);
		osync_debug("ENG", 2, "has data: %s", osync_flag_get_state(mapflags->cmb_has_data) ? "YES" : "NO");
		osync_debug("ENG", 2, "has info: %s (no: %i, yes: %i)", osync_flag_get_state(mapflags->cmb_has_info) ? "YES" : "NO",  mapflags->cmb_has_info->num_not_set, mapflags->cmb_has_info->num_set);
		osync_debug("ENG", 2, "delete: %s (no: %i, yes: %i)", osync_flag_get_state(mapflags->cmb_deleted) ? "YES" : "NO",  mapflags->cmb_deleted->num_not_set, mapflags->cmb_deleted->num_set);
		
		for (n = 0; n < osync_mapping_num_entries(mapping); n++) {
			change = osync_mapping_nth_entry(mapping, n);
			MSyncChangeFlags *chflags = osync_change_get_flags(change);
			osync_debug("ENG", 2, "\tCHANGE %p, Member %lli:", change, osync_member_get_id(osync_change_get_member(change)));
			osync_debug("ENG", 2, "\tuid: %s, changetype: %i", osync_change_get_uid(change), osync_change_get_changetype(change));
			osync_debug("ENG", 2, "\tObjType: %s, Format %s", osync_change_get_objtype(change) ? osync_objtype_get_name(osync_change_get_objtype(change)) : "None", osync_change_get_objformat(change) ? osync_objformat_get_name(osync_change_get_objformat(change)) : "None");
			osync_debug("ENG", 2, "\thas data: %s", osync_flag_get_state(chflags->fl_has_data) ? "YES" : "NO");
			osync_debug("ENG", 2, "\tdirty: %s", osync_flag_get_state(chflags->fl_dirty) ? "YES" : "NO");
			osync_debug("ENG", 2, "\tmapped: %s", osync_flag_get_state(chflags->fl_mapped) ? "YES" : "NO");
			osync_debug("ENG", 2, "\thas info: %s", osync_flag_get_state(chflags->fl_has_info) ? "YES" : "NO");
			osync_debug("ENG", 2, "\tsynced: %s", osync_flag_get_state(chflags->fl_synced) ? "YES" : "NO");
			osync_debug("ENG", 2, "\tdeleted: %s", osync_flag_get_state(chflags->fl_deleted) ? "YES" : "NO");
		}
	}
	
	for (i = 0; i < osync_mappingtable_num_unmapped(engine->maptable); i++) {
		change = osync_mappingtable_nth_unmapped(engine->maptable, i);
		MSyncChangeFlags *flags = osync_change_get_flags(change);
		osync_debug("ENG", 2, "UNMAPPED CHANGE %p:", change);
		osync_debug("ENG", 2, "uid: %s, changetype: %i", osync_change_get_uid(change), osync_change_get_changetype(change));
		osync_debug("ENG", 2, "ObjType: %s, Format %s", osync_change_get_objtype(change) ? osync_objtype_get_name(osync_change_get_objtype(change)) : "None", osync_change_get_objformat(change) ? osync_objformat_get_name(osync_change_get_objformat(change)) : "None");
		osync_debug("ENG", 2, "has data: %s", osync_flag_get_state(flags->fl_has_data) ? "YES" : "NO");
		osync_debug("ENG", 2, "dirty: %s", osync_flag_get_state(flags->fl_dirty) ? "YES" : "NO");
		osync_debug("ENG", 2, "mapped: %s", osync_flag_get_state(flags->fl_mapped) ? "YES" : "NO");
		osync_debug("ENG", 2, "has info: %s", osync_flag_get_state(flags->fl_has_info) ? "YES" : "NO");
		osync_debug("ENG", 2, "synced: %s", osync_flag_get_state(flags->fl_synced) ? "YES" : "NO");
	}
}
