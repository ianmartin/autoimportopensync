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

void osync_status_conflict(OSyncEngine *engine, OSyncMapping *mapping)
{
	if (engine->conflict_callback)
		engine->conflict_callback(engine, mapping, engine->conflict_userdata);
}

void osync_status_update_member(OSyncEngine *engine, OSyncClient *client, memberupdatetype type, OSyncError **error)
{
	if (engine->mebstat_callback) {
		MSyncMemberUpdate update;
		memset(&update, 0, sizeof(MSyncMemberUpdate));
		update.type = type;
		update.member = client->member;
		if (error)
			update.error = *error;
		engine->mebstat_callback(&update, engine->mebstat_userdata);
	}
}

void osync_status_update_change(OSyncEngine *engine, OSyncChange *change, changeupdatetype type)
{
	OSyncMapping *mapping = osync_change_get_mapping(change);
	if (engine->changestat_callback) {
		MSyncChangeUpdate update;
		update.type = type;
		update.member_id = osync_member_get_id(osync_change_get_member(change));
		update.change = change;
		if (mapping)
			update.mapping_id = osync_mapping_get_id(mapping);
		engine->changestat_callback(engine, &update, engine->changestat_userdata);
	}
}

void osync_status_update_mapping(OSyncEngine *engine, OSyncMapping *mapping, mappingupdatetype type)
{
	OSyncChange *master;
	if (engine->mapstat_callback) {
		MSyncMappingUpdate update;
		update.type = type;
		update.mapping_id = osync_mapping_get_id(mapping);
		if ((master = osync_mapping_get_masterentry(mapping)))
			update.winner = osync_member_get_id(osync_change_get_member(master));
		engine->mapstat_callback(&update, engine->mapstat_userdata);
	}
}

void osync_status_update_engine(OSyncEngine *engine, engineupdatetype type, OSyncError **error)
{
	if (engine->engstat_callback) {
		OSyncEngineUpdate update;
		memset(&update, 0, sizeof(OSyncEngineUpdate));
		update.type = type;
		if (error)
			update.error = *error;
		engine->engstat_callback(engine, &update, engine->engstat_userdata);
	}
}
