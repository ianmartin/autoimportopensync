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
 * @defgroup OSEngineMapping OpenSync Mapping Internals
 * @ingroup OSEnginePrivate
 * @brief The internals of the engine (communication part)
 * 
 * This gives you an insight in the inner workings of the sync engine
 * 
 * 
 */
/*@{*/

void send_change_changed(OSyncChange *change)
{
	OSyncMember *member = osync_change_get_member(change);
	OSyncClient *client = osync_member_get_data(member);
	OSyncEngine *engine = client->engine;
	ITMessage *message = itm_message_new_signal(NULL, "CHANGE_CHANGED");
	itm_message_set_data(message, "change", change);
	itm_queue_send(engine->incoming, message);
}

void send_mapping_changed(OSyncEngine *engine, OSyncMapping *mapping)
{
	ITMessage *message = itm_message_new_signal(NULL, "MAPPING_CHANGED");
	itm_message_set_data(message, "mapping", mapping);
	itm_queue_send(engine->incoming, message);
}

void _get_change_data_reply_receiver(OSyncClient *sender, ITMessage *message, OSyncEngine *engine)
{
	osync_debug("ENG", 3, "Received a reply %p to GET_DATA command from client %p", message, sender);
	OSyncChange *change = itm_message_get_data(message, "change");
	MSyncChangeFlags *flags = osync_change_get_engine_data(change);
	
	if (itm_message_is_error(message)) {
		OSyncError *error = itm_message_get_error(message);
		osync_error_duplicate(&engine->error, &error);
		osync_debug("MAP", 1, "Commit change command reply was a error: %s", error ? error->message : "None");
		osync_status_update_change(engine, change, CHANGE_RECV_ERROR, &error);
		osync_error_update(&engine->error, "Unable to read one or more objects");
		
		//FIXME Do we need to do anything here?
		osync_flag_unset(flags->fl_has_data);
	} else {
		osync_flag_set(flags->fl_has_data);
		osync_status_update_change(engine, change, CHANGE_RECEIVED, NULL);
	}
	
	osync_mappingtable_save_change(engine->maptable, change);
	osync_change_decider(engine, change);
}

void send_get_change_data(OSyncEngine *sender, OSyncChange *change)
{
	MSyncChangeFlags *flags = osync_change_get_engine_data(change);
	osync_flag_changing(flags->fl_has_data);
	OSyncClient *target = osync_member_get_data(osync_change_get_member(change));
	ITMessage *message = itm_message_new_methodcall(sender, "GET_DATA");
	itm_message_set_handler(message, sender->incoming, (ITMessageHandler)_get_change_data_reply_receiver, sender);
	itm_message_set_data(message, "change", change);
	osync_debug("ENG", 3, "Sending get_entry message %p to client %p", message, target);
	itm_queue_send(target->incoming, message);
}

void _commit_change_reply_receiver(OSyncClient *sender, ITMessage *message, OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "_commit_change_reply_receiver(%p, %p, %p)", sender, message, engine);
	OSyncChange *change = itm_message_get_data(message, "change");
	MSyncChangeFlags *flags = osync_change_get_engine_data(change);
	
	if (itm_message_is_error(message)) {
		OSyncError *error = itm_message_get_error(message);
		osync_error_duplicate(&engine->error, &error);
		osync_debug("MAP", 1, "Commit change command reply was a error: %s", error ? error->message : "None");
		osync_status_update_change(engine, change, CHANGE_WRITE_ERROR, &error);
		osync_status_update_mapping(engine, osync_change_get_mapping(change), MAPPING_WRITE_ERROR, &error);
		osync_error_update(&engine->error, "Unable to write one or more objects");
		
		//FIXME Do we need to do anything here?
		osync_flag_unset(flags->fl_dirty);
		osync_flag_set(flags->fl_synced);
	} else {
		osync_status_update_change(engine, change, CHANGE_SENT, NULL);
		osync_flag_unset(flags->fl_dirty);
		osync_flag_set(flags->fl_synced);
	}
	osync_mappingtable_save_change(engine->maptable, change);
	if (osync_change_get_changetype(change) == CHANGE_DELETED)
		osync_flag_set(flags->fl_deleted);
	
	osync_change_decider(engine, change);
	osync_trace(TRACE_EXIT, "_get_changes_reply_receiver");
}

void send_commit_change(OSyncEngine *sender, OSyncChange *change)
{
	OSyncClient *target = osync_member_get_data(osync_change_get_member(change));
	ITMessage *message = itm_message_new_methodcall(sender, "COMMIT_CHANGE");
	itm_message_set_data(message, "change", change);
	itm_message_set_handler(message, sender->incoming, (ITMessageHandler)_commit_change_reply_receiver, sender);
	MSyncChangeFlags *flags = osync_change_get_engine_data(change);
	osync_flag_changing(flags->fl_dirty);
	itm_queue_send_with_timeout(target->incoming, message, 5, sender);
}

void osync_mapping_multiply_master(OSyncEngine *engine, OSyncMapping *mapping)
{
	int i = 0;
	OSyncChange *change = NULL;
	OSyncChange *master = NULL;
	MSyncChangeFlags *chflags = NULL;
	
	g_assert(engine);

	osync_debug("MAP", 2, "Multiplying mapping %p", mapping);

	master = osync_mapping_get_masterentry(mapping);
	g_assert(master);
	
	chflags = osync_change_get_flags(master);
	if (osync_flag_is_not_set(chflags->fl_dirty))
		osync_flag_set(chflags->fl_synced);
	
	//Send the change to every source that is different to the master source and set state to writing in the changes
	for (i = 0; i < g_list_length(engine->clients); i++) {
		OSyncClient *client = g_list_nth_data(engine->clients, i);
		//Check if this client is already listed in the mapping
		change = osync_mapping_get_entry_by_owner(mapping, client->member);
		if (change == master)
			continue;
		if (change && (osync_change_compare(change, master) == CONV_DATA_SAME)) {
			chflags = osync_change_get_flags(change);
			if (osync_flag_is_not_set(chflags->fl_dirty))
				osync_flag_set(chflags->fl_synced);
			continue;
		}
		if (!change) {
			change = osync_change_new();
			osync_change_set_uid(change, osync_change_get_uid(master));
			osync_member_add_changeentry(client->member, change);
			osync_mapping_add_entry(mapping, change);
			osync_change_update(master, change);
		} else {
			osync_change_update(master, change);
			if (osync_change_get_changetype(change) == CHANGE_ADDED) {
				osync_change_set_changetype(change, CHANGE_MODIFIED);
			}
		}
		if (osync_flag_is_set(client->fl_sent_changes)) {	
			chflags = osync_change_get_flags(change);
			osync_change_flags_attach(change, mapping);
			
			osync_flag_set(chflags->fl_dirty);
			osync_flag_set(chflags->fl_has_data);
			osync_flag_set(chflags->fl_mapped);
			osync_flag_set(chflags->fl_has_info);
			osync_flag_unset(chflags->fl_synced);
			osync_mappingtable_save_change(engine->maptable, change);
		}
	}
	
	MSyncMappingFlags *flags = osync_mapping_get_flags(mapping);
	osync_flag_set(flags->fl_solved);
}

void osengine_change_reset(OSyncChange *change)
{
	osync_change_free_flags(change);
	osync_change_reset(change);
}

void osync_mapping_reset(OSyncMapping *mapping)
{
	int i = 0;
	osync_debug("MAP", 3, "Reseting mapping %p", mapping);
	for (i = 0; i < osync_mapping_num_entries(mapping); i++) {
		OSyncChange *change = osync_mapping_nth_entry(mapping, i);
		osengine_change_reset(change);
	}
	osync_mapping_set_masterentry(mapping, NULL);
	osync_mapping_free_flags(mapping);
}

int prod(int n)
{
	int ret;
	for (ret = 0; n > 0; n--)
		ret += n;
	return ret;
}

void osync_mapping_check_conflict(OSyncEngine *engine, OSyncMapping *mapping)
{
	int i = 0;
	int n = 0;
	osync_bool is_conflict = FALSE;
	int is_same = 0;
	OSyncChange *leftchange = NULL;
	OSyncChange *rightchange = NULL;
	
	osync_debug("MAP", 2, "Checking conflict for mapping %p", mapping);
	
	g_assert(engine != NULL);
	g_assert(mapping != NULL);
	
	if (!osync_mapping_get_masterentry(mapping)) {
		for (i = 0; i < osync_mapping_num_entries(mapping); i++) {
			leftchange = osync_mapping_nth_entry(mapping, i);
			if (osync_change_get_changetype(leftchange) == CHANGE_UNKNOWN)
				continue;
			osync_mapping_set_masterentry(mapping, leftchange);
			for (n = i + 1; n < osync_mapping_num_entries(mapping); n++) {
				rightchange = osync_mapping_nth_entry(mapping, n);
				if (osync_change_get_changetype(rightchange) == CHANGE_UNKNOWN)
					continue;
						
				/*FIXME: Do we need to do any detection or conversion here? */
#if 0
/*				OSyncFormatEnv *env = osync_group_get_format_env(engine->group);
				osync_conv_detect_and_convert(env, leftchange);
				osync_mappingtable_save_change(engine->maptable, leftchange);
				osync_conv_detect_and_convert(env, rightchange);
				osync_mappingtable_save_change(engine->maptable, rightchange);*/
#endif
				if (osync_change_compare(leftchange, rightchange) != CONV_DATA_SAME) {
					is_conflict = TRUE;
					goto conflict;
				} else {
					is_same++;
				}
			}	
		}
	}
	
	
	conflict:
	if (is_conflict) {
		//conflict, solve conflict
		osync_debug("MAP", 2, "Got conflict for mapping %p", mapping);
		osync_status_conflict(engine, mapping);
		return;
	}
	
	if (is_same != prod(osync_group_num_members(engine->group) - 1)) {
		osync_mapping_multiply_master(engine, mapping);
	} else {
		MSyncMappingFlags *flags = osync_mapping_get_flags(mapping);
		osync_flag_set(flags->fl_solved);
		osync_flag_set(flags->cmb_synced);
	}
}

void osync_change_decider(OSyncEngine *engine, OSyncChange *change)
{
	MSyncChangeFlags *flags = osync_change_get_flags(change);
	
	osync_debug("MAP", 3, "Mappingentry decider called for mappingentry %p", change);

	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(engine->fl_sync) && osync_flag_is_set(flags->fl_has_info) && osync_flag_is_not_set(flags->fl_has_data)) {
		osync_debug("ENT", 2, "Getting entry data for entry %p for client %p", change, osync_member_get_data(osync_change_get_member(change)));
		send_get_change_data(engine, change);
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(engine->cmb_sent_changes) && osync_flag_is_set(engine->fl_sync) && osync_flag_is_set(flags->fl_has_info) && osync_flag_is_set(flags->fl_has_data)) {
		if (osync_flag_is_not_set(flags->fl_mapped)) {
			osync_debug("ENT", 2, "Mapping change now %p", change);
			osync_change_map(engine, change);
			return;
		}
		if (osync_flag_is_set(flags->fl_dirty)) {
			osync_debug("ENT", 2, "Writing entry to remote side");
			send_commit_change(engine, change);
			return;
		}
	}
	
	osync_debug("MAP", 3, "Waste cycle in change decider %p", change);
}

void osync_mapping_all_change_deciders(OSyncEngine *engine, OSyncMapping *mapping)
{
	int i = 0;
	osync_debug("ENG", 3, "Calling all mappingentry deciders (%i) for mapping %p", osync_mapping_num_entries(mapping), mapping);
	for (i = 0; i < osync_mapping_num_entries(mapping); i++) {
		OSyncChange *change = osync_mapping_nth_entry(mapping, i);
		send_change_changed(change);
	}
}

void osengine_mapping_free(OSyncEngine *engine, OSyncMapping *mapping)
{
	osync_debug("MAP", 3, "Freeing mapping %p", mapping);
	osync_mapping_free_flags(mapping);
	osync_mapping_delete(mapping);
	int i = 0;
	for (i = 0; i < osync_mapping_num_entries(mapping); i++) {
		OSyncChange *change = osync_mapping_nth_entry(mapping, i);
		osync_change_free_flags(change);
		osync_change_free(change);
	}
	osync_mapping_free(mapping);
}

void osync_mapping_decider(OSyncEngine *engine, OSyncMapping *mapping)
{
	MSyncMappingFlags *flags = osync_mapping_get_flags(mapping);
	osync_debug("MAP", 3, "Mapping decider called for mapping %p", mapping);
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(engine->cmb_sent_changes) && osync_flag_is_set(engine->cmb_entries_mapped) && osync_flag_is_set(flags->cmb_has_info) && osync_flag_is_set(flags->cmb_has_data) && osync_flag_is_not_set(flags->cmb_synced) && osync_flag_is_not_set(flags->fl_solved)) {
		osync_mapping_check_conflict(engine, mapping);
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(flags->cmb_synced) && osync_flag_is_set(flags->cmb_has_info) && osync_flag_is_not_set(flags->cmb_deleted)) {
		osync_debug("ENT", 2, "Reseting mapping2 %p", mapping);
		osync_mapping_reset(mapping);
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(flags->cmb_synced) && osync_flag_is_set(flags->cmb_deleted)) {
		osync_debug("ENT", 2, "Freeing mapping %p", mapping);
		osengine_mapping_free(engine, mapping);
		return;
	}
	
	osync_debug("MAP", 3, "Waste cycle in mapping decider %p", mapping);
}

void osync_mapping_all_deciders(OSyncEngine *engine)
{
	int i = 0;
	osync_debug("ENG", 4, "Calling all mapping deciders (%i)", osync_mappingtable_num_mappings(engine->maptable));
	for (i = 0; i < osync_mappingtable_num_mappings(engine->maptable); i++) {
		OSyncMapping *mapping = osync_mappingtable_nth_mapping(engine->maptable, i);
		send_mapping_changed(engine, mapping);
	}
}

static OSyncChange *_osync_find_next_diff(OSyncMapping *mapping, OSyncChange *orig_change)
{
	int i;
	for (i = 0; i < osync_mapping_num_entries(mapping); i++) {
		OSyncChange *change = osync_mapping_nth_entry(mapping, i);
		if (osync_change_get_changetype(change) == CHANGE_UNKNOWN)
			continue;
		if ((change != orig_change) && osync_change_compare(orig_change, change) != CONV_DATA_SAME)
			return change;
	}
	osync_debug("MAP", 3, "Could not find next diff");
	return NULL;
}

static OSyncChange *_osync_find_next_same(OSyncMapping *mapping, OSyncChange *orig_change)
{
	int i;
	for (i = 0; i < osync_mapping_num_entries(mapping); i++) {
		OSyncChange *change = osync_mapping_nth_entry(mapping, i);
		if ((change != orig_change) && osync_change_compare(orig_change, change) == CONV_DATA_SAME)
			return change;
	}
	osync_debug("MAP", 3, "Could not find next same");
	return NULL;
}

static OSyncChange *_osync_change_clone(OSyncEngine *engine, OSyncMapping *new_mapping, OSyncChange *comp_change)
{
	OSyncChange *newchange = osync_change_new();
	osync_change_update(comp_change, newchange);
	osync_change_set_member(newchange, osync_change_get_member(comp_change));
	osync_mapping_add_entry(new_mapping, newchange);
	osync_change_set_uid(newchange, osync_change_get_uid(comp_change));
	
	MSyncChangeFlags *chflags = osync_change_get_flags(newchange);
	osync_change_flags_attach(newchange, new_mapping);
	osync_flag_set(chflags->fl_has_data);
	osync_flag_set(chflags->fl_mapped);
	osync_flag_set(chflags->fl_has_info);
	osync_flag_set(chflags->fl_dirty);
	osync_flag_unset(chflags->fl_synced);
	osync_mappingtable_save_change(engine->maptable, newchange);
	return newchange;
}

static OSyncMapping *_osync_mapping_new(OSyncEngine *engine)
{
	OSyncMapping *new_mapping = osync_mapping_new(engine->maptable);
	osync_debug("MAP", 3, "Creating new duplicated mapping %p", new_mapping);
	MSyncMappingFlags *mapflags = osync_mapping_get_flags(new_mapping);
	osync_flag_unset(mapflags->cmb_synced);
	send_mapping_changed(engine, new_mapping);
	return new_mapping;
}

static osync_bool _osync_change_elevate(OSyncEngine *engine, OSyncChange *change, int level)
{
	osync_debug("MAP", 3, "elevating change %s (%p) to level %i", osync_change_get_uid(change), change, level);
	int i = 0;
	for (i = 0; i < level; i++) {
		if (!osync_change_duplicate(change))
			return FALSE;
	}
	osync_debug("MAP", 3, "change after being elevated %s (%p)", osync_change_get_uid(change), change);
	osync_mappingtable_save_change(engine->maptable, change);
	return TRUE;
}

static osync_bool _osync_change_check_level(OSyncEngine *engine, OSyncChange *change)
{
	GList *c;
	osync_debug("MAP", 3, "checking level for change %s (%p)", osync_change_get_uid(change), change);
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		if (!osync_member_uid_is_unique(client->member, change, TRUE))
			return FALSE;
	}
	return TRUE;
}

static void _osync_change_overwrite(OSyncEngine *engine, OSyncChange *source, OSyncChange *target)
{
	osync_debug("MAP", 3, "overwriting change %s (%p) with change %s (%p)", osync_change_get_uid(source), source, osync_change_get_uid(target), target);
	osync_change_update(source, target);
	osync_mappingtable_save_change(engine->maptable, target);
}

void osync_mapping_duplicate(OSyncEngine *engine, OSyncMapping *dupe_mapping)
{
	g_assert(dupe_mapping);
	int elevation = 0;
	OSyncChange *orig_change = NULL;
	OSyncChange *first_diff_change = NULL;
	OSyncChange *next_change = NULL;
	OSyncChange *new_change = NULL;
	OSyncMapping *new_mapping = NULL;
	
	osync_debug("MAP", 2, "Duplicating mapping %p", dupe_mapping);
	
	orig_change = osync_mapping_nth_entry(dupe_mapping, 0);
	
	//Remove all deleted items first.
	int i;
	for (i = osync_mapping_num_entries(dupe_mapping); i > 0; i--) {
		OSyncChange *change = osync_mapping_nth_entry(dupe_mapping, i - 1);
		if (osync_change_get_changetype(change) == CHANGE_DELETED) {
			osync_change_flags_detach(change);
			osync_mappingtable_delete_change(engine->maptable, change);
			osync_mapping_remove_entry(dupe_mapping, change);
			osync_change_free_flags(change);
			osync_change_free(change);
		}
	}
	
	//Choose the first modified change
	i = 0;
	do {
		orig_change = osync_mapping_nth_entry(dupe_mapping, i);
		g_assert(orig_change);
		i++;
	} while (osync_change_get_changetype(orig_change) != CHANGE_MODIFIED && osync_change_get_changetype(orig_change) != CHANGE_ADDED);
	osync_mapping_set_masterentry(dupe_mapping, orig_change);
	osync_change_set_changetype(orig_change, CHANGE_MODIFIED);
	
	while ((first_diff_change = _osync_find_next_diff(dupe_mapping, orig_change))) {
		
		elevation = 0;
		new_mapping = _osync_mapping_new(engine);
		osync_debug("MAP", 3, "Created new mapping for duplication %p", new_mapping);
		new_change = first_diff_change;
		//new_change = _osync_change_clone(engine, new_mapping, first_diff_change);
		//osync_member_add_changeentry(osync_change_get_member(first_diff_change), new_change);
		do {
			if (!_osync_change_elevate(engine, new_change, 1))
				break;
			elevation += 1;
		} while (!_osync_change_check_level(engine, new_change));
		
		while ((next_change = _osync_find_next_same(dupe_mapping, first_diff_change))) {
			new_change = _osync_change_clone(engine, new_mapping, first_diff_change);
			_osync_change_elevate(engine, new_change, elevation);
			_osync_change_overwrite(engine, next_change, orig_change);
		}
		//_osync_change_overwrite(engine, orig_change, first_diff_change);
		osync_mapping_remove_entry(dupe_mapping, first_diff_change);
		osync_change_flags_detach(first_diff_change);
		osync_change_flags_attach(first_diff_change, new_mapping);
		osync_mapping_set_masterentry(new_mapping, first_diff_change);
		osync_mapping_add_entry(new_mapping, first_diff_change);
		osync_change_set_changetype(first_diff_change, CHANGE_ADDED);
		
		MSyncChangeFlags *changeflags = osync_change_get_flags(first_diff_change);
		osync_flag_set(changeflags->fl_dirty);
		
		osync_mappingtable_save_change(engine->maptable, first_diff_change);
		send_mapping_changed(engine, new_mapping);
	}
	
	//FIXME multiplying the master here might not work of you duplicate a maping
	//that did not need duplication
	
	osync_mapping_multiply_master(engine, dupe_mapping);
}

void osengine_mapping_solve(OSyncEngine *engine, OSyncMapping *mapping, OSyncChange *change)
{
	osync_mapping_set_masterentry(mapping, change);
	send_mapping_changed(engine, mapping);
}

void osync_change_map(OSyncEngine *engine, OSyncChange *change)
{
	MSyncChangeFlags *changeflags = osync_change_get_flags(change);
	OSyncMapping *mapping = NULL;
	if (!(mapping = osync_mapping_find(engine, change))) {
		mapping = osync_mapping_new(engine->maptable);
	}
	osync_mapping_add_entry(mapping, change);
	osync_change_flags_attach(change, mapping);
	
	osync_flag_set(changeflags->fl_mapped);
	osync_mappingtable_remove_unmapped(engine->maptable, change);
	osync_mappingtable_save_change(engine->maptable, change);
}

OSyncMapping *osync_mapping_find(OSyncEngine *engine, OSyncChange *change)
{
	int i = 0;
	int n = 0;
	osync_bool mapping_found = FALSE;

	for (i = 0; i < osync_mappingtable_num_mappings(engine->maptable); i++) {
		OSyncMapping *mapping = osync_mappingtable_nth_mapping(engine->maptable, i);
		//We only need mapping where our member isnt listed yet.
		if (!osync_mapping_get_entry_by_owner(mapping, osync_change_get_member(change))) {
			mapping_found = TRUE;
			for (n = 0; n < osync_mapping_num_entries(mapping); n++) {
				OSyncChange *mapchange = osync_mapping_nth_entry(mapping, n);
				if (osync_change_compare(mapchange, change) == CONV_DATA_MISMATCH) {
					mapping_found = FALSE;
					continue;
				}
			}
			if (mapping_found)
				return mapping;
		}
	}
	return NULL;
}

/*@}*/
