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

void _osync_debug(gpointer sender, char *subpart, int level, char *message, ...)
{
	va_list arglist;
	char buffer[4096];
	
	int color = ((int)sender % 6) + 31;
	
	va_start(arglist, message);
	vsprintf(buffer, message, arglist);
	
	const char *dbgstr = g_getenv("MSYNC_DEBUG");
	if (!dbgstr)
		return;
	int debug = atoi(dbgstr);
	if (debug < level)
		return;
		
	switch (level) {
		case 0:
			//Error
			printf("[%p:%s] ERROR: %s\n", sender, subpart, buffer);
			break;
		case 1:
			// Warning
			printf("[%p:%s] WARNING: %s\n", sender, subpart, buffer);
			break;
		case 2:
			//Information
			printf("[%p:%s] INFORMATION: %s\n", sender, subpart, buffer);
			break;
		case 3:
			//debug
			printf("\033[%im[%p:%s]\033[0m DEBUG: %s\n", color, sender, subpart, buffer);
			break;
		case 4:
			//fulldebug
			printf("\033[%im[%p:%s]\033[0m FULLDEBUG: %s\n", color, sender, subpart, buffer);
			break;
	}
	va_end(arglist);
}

void osync_engine_print_all(OSyncEngine *engine)
{
	int i;
	int n;
	OSyncChange *change;
	printf("\nENGINE:\n");
	printf("running: %s\n", osync_flag_get_state(engine->fl_running) ? "YES" : "NO");
	printf("sync: %s\n", osync_flag_get_state(engine->fl_sync) ? "YES" : "NO");
	printf("stop: %s\n", osync_flag_get_state(engine->fl_stop) ? "YES" : "NO");
	printf("sent changes: %s (no: %i, yes: %i)\n", osync_flag_get_state(engine->cmb_sent_changes) ? "YES" : "NO", engine->cmb_sent_changes->num_not_set, engine->cmb_sent_changes->num_set);
	printf("all mapped: %s (no: %i, yes: %i)\n", osync_flag_get_state(engine->cmb_entries_mapped) ? "YES" : "NO", engine->cmb_entries_mapped->num_not_set, engine->cmb_entries_mapped->num_set);
	printf("synced: %s (no: %i, yes: %i)\n", osync_flag_get_state(engine->cmb_synced) ? "YES" : "NO", engine->cmb_synced->num_not_set, engine->cmb_synced->num_set);
	printf("finished: %s\n", osync_flag_get_state(engine->cmb_finished) ? "YES" : "NO");
	
	for (i = 0; i < g_list_length(engine->clients); i++) {
		OSyncClient *client = g_list_nth_data(engine->clients, i);
		printf("\tCLIENT %lli %s:\n", osync_member_get_id(client->member), osync_member_get_pluginname(client->member));
		printf("\tconnected: %s\n", osync_flag_get_state(client->fl_connected) ? "YES" : "NO");
		printf("\tsent changes: %s\n", osync_flag_get_state(client->fl_sent_changes) ? "YES" : "NO");
		printf("\tdone: %s\n", osync_flag_get_state(client->fl_done) ? "YES" : "NO");
		printf("\tfinished: %s\n", osync_flag_get_state(client->fl_finished) ? "YES" : "NO");
	}

	for (i = 0; i < osync_mappingtable_num_mappings(engine->maptable); i++) {
		OSyncMapping *mapping = osync_mappingtable_nth_mapping(engine->maptable, i);
		printf("\nMAPPING %p:\n", mapping);
		MSyncMappingFlags *mapflags = osync_mapping_get_flags(mapping);
		printf("solved: %s\n", osync_flag_get_state(mapflags->fl_solved) ? "YES" : "NO");
		printf("synced: %s (no: %i, yes: %i)\n", osync_flag_get_state(mapflags->cmb_synced) ? "YES" : "NO", mapflags->cmb_synced->num_not_set, mapflags->cmb_synced->num_set);
		printf("has data: %s\n", osync_flag_get_state(mapflags->cmb_has_data) ? "YES" : "NO");
		printf("has info: %s (no: %i, yes: %i)\n", osync_flag_get_state(mapflags->cmb_has_info) ? "YES" : "NO",  mapflags->cmb_has_info->num_not_set, mapflags->cmb_has_info->num_set);
		printf("delete: %s (no: %i, yes: %i)\n", osync_flag_get_state(mapflags->cmb_deleted) ? "YES" : "NO",  mapflags->cmb_deleted->num_not_set, mapflags->cmb_deleted->num_set);
		
		for (n = 0; n < osync_mapping_num_entries(mapping); n++) {
			change = osync_mapping_nth_entry(mapping, n);
			MSyncChangeFlags *chflags = osync_change_get_flags(change);
			printf("\tCHANGE %p, Member %lli:\n", change, osync_member_get_id(osync_change_get_member(change)));
			printf("\tuid: %s, changetype: %i\n", osync_change_get_uid(change), osync_change_get_changetype(change));
			printf("\tObjType: %s, Format %s\n", osync_change_get_objtype(change) ? osync_objtype_get_name(osync_change_get_objtype(change)) : "None", osync_change_get_objformat(change) ? osync_objformat_get_name(osync_change_get_objformat(change)) : "None");
			printf("\thas data: %s\n", osync_flag_get_state(chflags->fl_has_data) ? "YES" : "NO");
			printf("\tdirty: %s\n", osync_flag_get_state(chflags->fl_dirty) ? "YES" : "NO");
			printf("\tmapped: %s\n", osync_flag_get_state(chflags->fl_mapped) ? "YES" : "NO");
			printf("\thas info: %s\n", osync_flag_get_state(chflags->fl_has_info) ? "YES" : "NO");
			printf("\tsynced: %s\n", osync_flag_get_state(chflags->fl_synced) ? "YES" : "NO");
			printf("\tdeleted: %s\n", osync_flag_get_state(chflags->fl_deleted) ? "YES" : "NO");
		}
	}
	printf("\n");
	for (i = 0; i < osync_mappingtable_num_unmapped(engine->maptable); i++) {
		change = osync_mappingtable_nth_unmapped(engine->maptable, i);
		MSyncChangeFlags *flags = osync_change_get_flags(change);
		printf("UNMAPPED CHANGE %p:\n", change);
		printf("uid: %s, changetype: %i\n", osync_change_get_uid(change), osync_change_get_changetype(change));
		printf("ObjType: %s, Format %s\n", osync_change_get_objtype(change) ? osync_objtype_get_name(osync_change_get_objtype(change)) : "None", osync_change_get_objformat(change) ? osync_objformat_get_name(osync_change_get_objformat(change)) : "None");
		printf("has data: %s\n", osync_flag_get_state(flags->fl_has_data) ? "YES" : "NO");
		printf("dirty: %s\n", osync_flag_get_state(flags->fl_dirty) ? "YES" : "NO");
		printf("mapped: %s\n", osync_flag_get_state(flags->fl_mapped) ? "YES" : "NO");
		printf("has info: %s\n", osync_flag_get_state(flags->fl_has_info) ? "YES" : "NO");
		printf("synced: %s\n", osync_flag_get_state(flags->fl_synced) ? "YES" : "NO");
	}
}
