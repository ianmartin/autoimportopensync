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

#include "opensync-client.h"
#include "opensync-engine.h"
#include "opensync-group.h"
#include "opensync-format.h"
#include "opensync-data.h"

#include "opensync_engine_internals.h"

void osync_status_free_member_update(OSyncMemberUpdate *update)
{
	osync_assert(update);
	
	if (update->objtype)
		g_free(update->objtype);
	
	osync_member_unref(update->member);
	
	if (update->error)
		osync_error_unref(&update->error);
	
	g_free(update);
}

void osync_status_free_engine_update(OSyncEngineUpdate *update)
{
	osync_assert(update);
	
	if (update->error)
		osync_error_unref(&update->error);
	
	g_free(update);
}

void osync_status_free_change_update(OSyncChangeUpdate *update)
{
	osync_assert(update);
	
	osync_member_unref(update->member);
	
	if (update->change)
		osync_change_unref(update->change);
	
	if (update->error)
		osync_error_unref(&update->error);
	
	g_free(update);
}

void osync_status_free_mapping_update(OSyncMappingUpdate *update)
{
	osync_assert(update);
	
	/*osync_member_unref(update->member);*/
	
	if (update->error)
		osync_error_unref(&update->error);
	
	g_free(update);
}

void osync_status_conflict(OSyncEngine *engine, OSyncMappingEngine *mapping_engine)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, mapping_engine);
	if (engine->conflict_callback)
		engine->conflict_callback(engine, mapping_engine, engine->conflict_userdata);
	else
		osync_trace(TRACE_INTERNAL, "Conflict Ignored");
		
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_status_update_member(OSyncEngine *engine, OSyncMember *member, OSyncMemberEvent type, const char *objtype, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %s, %p)", __func__, engine, member, type, objtype, error);
	
	if (engine->mebstat_callback) {
		OSyncMemberUpdate *update = g_malloc0(sizeof(OSyncMemberUpdate));
		if (!update)
			return;
		
		update->type = type;
		
		update->member = member;
		osync_member_ref(member);
		
		update->error = error;
		osync_error_ref(&error);
		
		update->objtype = g_strdup(objtype);
		
		engine->mebstat_callback(update, engine->mebstat_userdata);
		
		osync_status_free_member_update(update);
	} else
		osync_trace(TRACE_INTERNAL, "Status Update Ignored");
		
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_status_update_change(OSyncEngine *engine, OSyncChange *change, OSyncMember *member, OSyncMapping *mapping, OSyncChangeEvent type, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %i, %p)", __func__, engine, change, member, mapping, type, error);
	
	if (engine->changestat_callback) {
		OSyncChangeUpdate *update = g_malloc0(sizeof(OSyncChangeUpdate));
		if (!update)
			return;
		
		update->type = type;
		
		update->change = change;
		osync_change_ref(change);
		
		update->member = member;
		osync_member_ref(member);
		
		update->error = error;
		osync_error_ref(&error);
		
		engine->changestat_callback(update, engine->changestat_userdata);
		
		osync_status_free_change_update(update);
	} else
		osync_trace(TRACE_INTERNAL, "Status Update Ignored");
		
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_status_update_mapping(OSyncEngine *engine, OSyncMappingEngine *mapping, OSyncMappingEvent type, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p)", __func__, engine, mapping, type, error);
	
	if (engine->mapstat_callback) {
		OSyncMappingUpdate *update = g_malloc0(sizeof(OSyncMappingUpdate));
		if (!update)
			return;
		
		update->type = type;
		
		/*update->mapping = mapping->mapping;*/
		
		update->error = error;
		osync_error_ref(&error);
		
		engine->mapstat_callback(update, engine->mapstat_userdata);
		
		osync_status_free_mapping_update(update);
	} else
		osync_trace(TRACE_INTERNAL, "Status Update Ignored");
		
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_status_update_engine(OSyncEngine *engine, OSyncEngineEvent type, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, engine, type, error);
	
	if (engine->engstat_callback) {
		OSyncEngineUpdate *update = g_malloc0(sizeof(OSyncEngineUpdate));
		if (!update)
			return;
		
		update->type = type;
		
		update->error = error;
		osync_error_ref(&error);
		
		engine->engstat_callback(update, engine->engstat_userdata);
		
		osync_status_free_engine_update(update);
	} else
		osync_trace(TRACE_INTERNAL, "Status Update Ignored");
		
	osync_trace(TRACE_EXIT, "%s", __func__);
}
