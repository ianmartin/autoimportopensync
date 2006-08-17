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
 * @ingroup OSEngineMappingPrivate
 */
/*@{*/

static OSyncMappingEntry *_osync_find_next_diff(OSyncMapping *mapping, OSyncMappingEntry *orig_entry)
{
	GList *e;
	for (e = mapping->entries; e; e = e->next) {
		OSyncMappingEntry *entry = e->data;
		if (osync_change_get_changetype(entry->change) == CHANGE_UNKNOWN)
			continue;
		if ((entry->change != orig_entry->change) && osync_change_compare(orig_entry->change, entry->change) != CONV_DATA_SAME)
			return entry;
	}
	osync_debug("MAP", 3, "Could not find next diff");
	return NULL;
}

static OSyncMappingEntry *_osync_find_next_same(OSyncMapping *mapping, OSyncMappingEntry *orig_entry)
{
	GList *e;
	for (e = mapping->entries; e; e = e->next) {
		OSyncMappingEntry *entry = e->data;
		if (osync_change_get_changetype(entry->change) == CHANGE_UNKNOWN)
			continue;
		if ((entry->change != orig_entry->change) && osync_change_compare(orig_entry->change, entry->change) == CONV_DATA_SAME)
			return entry;
	}
	osync_debug("MAP", 3, "Could not find next diff");
	return NULL;
}

static OSyncMappingEntry *_osync_change_clone(OSyncEngine *engine, OSyncMapping *new_mapping, OSyncMappingEntry *comp_entry)
{
	OSyncMappingEntry *newentry = osengine_mappingentry_new(NULL);
	newentry->change = osync_change_new();
	newentry->client = comp_entry->client;
	osengine_mapping_add_entry(new_mapping, newentry);
	osengine_mappingview_add_entry(comp_entry->view, newentry);
	osengine_mappingentry_update(newentry, comp_entry->change);
	osync_change_set_uid(newentry->change, osync_change_get_uid(comp_entry->change));
	osync_flag_set(newentry->fl_has_data);
	osync_flag_set(newentry->fl_mapped);
	osync_flag_set(newentry->fl_has_info);
	osync_flag_set(newentry->fl_dirty);
	osync_flag_unset(newentry->fl_synced);
	osync_change_save(newentry->change, TRUE, NULL);
	return newentry;
}

osync_bool osync_change_elevate(OSyncEngine *engine, OSyncChange *change, int level)
{
	osync_debug("MAP", 3, "elevating change %s (%p) to level %i", osync_change_get_uid(change), change, level);
	int i = 0;
	for (i = 0; i < level; i++) {
		if (!osync_change_duplicate(change))
			return FALSE;
	}
	osync_debug("MAP", 3, "change after being elevated %s (%p)", osync_change_get_uid(change), change);
	osync_change_save(change, TRUE, NULL);
	return TRUE;
}

osync_bool osync_change_check_level(OSyncEngine *engine, OSyncMappingEntry *entry)
{
	GList *c;
	osync_debug("MAP", 3, "checking level for change %s (%p)", osync_change_get_uid(entry->change), entry);
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		OSyncMappingView *view = osengine_mappingtable_find_view(engine->maptable, client->member);
		if (!osengine_mappingview_uid_is_unique(view, entry, TRUE))
			return FALSE;
	}
	return TRUE;
}

static int prod(int n)
{
	int ret;
	for (ret = 0; n > 0; n--)
		ret += n;
	return ret;
}

void osengine_mapping_multiply_master(OSyncEngine *engine, OSyncMapping *mapping)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, mapping);
	g_assert(engine);
	
	OSyncMappingTable *table = engine->maptable;
	OSyncMappingEntry *entry = NULL;
	OSyncMappingEntry *master = NULL;

	master = mapping->master;
	g_assert(master);
	if (osync_flag_is_not_set(master->fl_dirty))
		osync_flag_set(master->fl_synced);
	else
		osync_flag_attach(master->fl_committed, table->engine->cmb_committed_all);
		
	//Send the change to every source that is different to the master source and set state to writing in the changes
	GList *v;
	for (v = table->views; v; v = v->next) {
		OSyncMappingView *view = v->data;
		//Check if this client is already listed in the mapping
		entry = osengine_mapping_find_entry(mapping, NULL, view);
		if (entry == master)
			continue;
		if (entry && (osync_change_compare(entry->change, master->change) == CONV_DATA_SAME)) {
			if (osync_flag_is_not_set(entry->fl_dirty))
				osync_flag_set(entry->fl_synced);
			continue;
		}
		if (!entry) {
			entry = osengine_mappingentry_new(NULL);
			entry->change = osync_change_new();
			entry->client = view->client;
			osengine_mappingview_add_entry(view, entry);
			osengine_mappingentry_update(entry, master->change);
			osync_change_set_uid(entry->change, osync_change_get_uid(master->change));
			osync_change_set_member(entry->change, view->client->member);
			osengine_mapping_add_entry(mapping, entry);
		} else {
			osync_bool had_data = osync_change_has_data(entry->change);
			osengine_mappingentry_update(entry, master->change);
			if (osync_change_get_changetype(entry->change) == CHANGE_ADDED || osync_change_get_changetype(entry->change) == CHANGE_UNKNOWN) {
				osync_change_set_changetype(entry->change, CHANGE_MODIFIED);
			}
			
			if (osync_member_get_slow_sync(view->client->member, osync_objtype_get_name(osync_change_get_objtype(entry->change))) && !had_data) {
				osync_change_set_changetype(entry->change, CHANGE_ADDED);
			}
		}
		if (osync_flag_is_set(view->client->fl_sent_changes)) {	
			//osync_change_flags_attach(change, mapping);
			osync_flag_set(entry->fl_dirty);
			osync_flag_set(entry->fl_has_data);
			osync_flag_set(entry->fl_mapped);
			osync_flag_set(entry->fl_has_info);
			osync_flag_unset(entry->fl_synced);
			OSyncError *error = NULL;
			osync_change_save(entry->change, TRUE, &error);
			osync_flag_attach(entry->fl_committed, table->engine->cmb_committed_all);
		}
	}
	
	OSyncError *error = NULL;
	osync_change_save(master->change, TRUE, &error);
	
	osync_flag_set(mapping->fl_multiplied);
	osync_trace(TRACE_EXIT, "%s", __func__);
}



void osengine_mapping_check_conflict(OSyncEngine *engine, OSyncMapping *mapping)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, mapping);
	GList *e;
	GList *n;
	osync_bool is_conflict = FALSE;
	int is_same = 0;
	OSyncMappingEntry *leftentry = NULL;
	OSyncMappingEntry *rightentry = NULL;
	
	g_assert(engine != NULL);
	g_assert(mapping != NULL);
	g_assert(!mapping->master);
	
	for (e = mapping->entries; e; e = e->next) {
		leftentry = e->data;
		if (osync_change_get_changetype(leftentry->change) == CHANGE_UNKNOWN)
			continue;
		mapping->master = leftentry;
		for (n = e->next; n; n = n->next) {
			rightentry = n->data;
			if (osync_change_get_changetype(rightentry->change) == CHANGE_UNKNOWN)
				continue;
			
			if (osync_change_compare(leftentry->change, rightentry->change) != CONV_DATA_SAME) {
				is_conflict = TRUE;
				goto conflict;
			} else {
				is_same++;
			}
		}	
	}
	
	conflict:
	if (is_conflict) {
		//conflict, solve conflict
		osync_debug("MAP", 2, "Got conflict for mapping %p", mapping);
		osync_status_conflict(engine, mapping);
		osync_flag_set(mapping->fl_chkconflict);
		osync_trace(TRACE_EXIT, "%s: Got conflict", __func__);
		return;
	}
	g_assert(mapping->master);
	osync_flag_set(mapping->fl_chkconflict);
	
	//Our mapping is already solved since there is no conflict
	osync_flag_set(mapping->fl_solved);
	
	if (is_same == prod(g_list_length(engine->maptable->views) - 1)) {
		osync_trace(TRACE_INTERNAL, "No need to sync. All entries are the same");
		osync_flag_set(mapping->cmb_synced);
		osync_flag_set(mapping->fl_multiplied);
	}

	send_mapping_changed(engine, mapping);
	osync_trace(TRACE_EXIT, "%s: No conflict", __func__);
}

static OSyncMapping *_osengine_mapping_find(OSyncMappingTable *table, OSyncMappingEntry *orig_entry)
{
	GList *i;
	GList *n;
	osync_bool mapping_found = FALSE;

	for (i = table->mappings; i; i = i->next) {
		OSyncMapping *mapping = i->data;
		//We only need mapping where our member isnt listed yet.
		if (!osengine_mapping_find_entry(mapping, NULL, orig_entry->view)) {
			mapping_found = TRUE;
			for (n = mapping->entries; n; n = n->next) {
				OSyncMappingEntry *entry = n->data;
				if (osync_change_compare_data(entry->change, orig_entry->change) == CONV_DATA_MISMATCH) {
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

void osengine_change_map(OSyncEngine *engine, OSyncMappingEntry *entry)
{
	osync_trace(TRACE_ENTRY, "osengine_change_map(%p, %p)", engine, entry);
	OSyncMapping *mapping = NULL;
	if (!(mapping = _osengine_mapping_find(engine->maptable, entry))) {
		mapping = osengine_mapping_new(engine->maptable);
		osync_flag_unset(mapping->fl_chkconflict);
		osync_flag_unset(mapping->fl_multiplied);
		mapping->id = osengine_mappingtable_get_next_id(engine->maptable);
		osync_trace(TRACE_INTERNAL, "No previous mapping found. Creating new one: %p", mapping);
	}
	osengine_mapping_add_entry(mapping, entry);
	osync_flag_set(entry->fl_mapped);
	osync_change_save(entry->change, FALSE, NULL);
	osync_trace(TRACE_EXIT, "osengine_change_map");
}

/*@}*/

/**
 * @defgroup OSEngineMapping OpenSync Mapping
 * @ingroup OSEnginePublic
 * @brief The commands to manipulate mappings
 * 
 */
/*@{*/

/** @brief Solves the conflict by duplicating the conflicting entries
 * 
 * @param engine The engine
 * @param dupe_mapping The conflicting mapping to duplicate
 * 
 */
void osengine_mapping_duplicate(OSyncEngine *engine, OSyncMapping *dupe_mapping)
{
	osync_trace(TRACE_ENTRY, "osengine_mapping_duplicate(%p, %p)", engine, dupe_mapping);
	g_assert(dupe_mapping);
	int elevation = 0;
	OSyncMappingEntry *orig_entry = NULL;
	OSyncMappingEntry *first_diff_entry = NULL;
	OSyncMappingEntry *next_entry = NULL;
	OSyncMapping *new_mapping = NULL;
	
	//Remove all deleted items first.
	GList *entries, *e;
	entries = g_list_copy(dupe_mapping->entries);
	for (e = entries; e; e = e->next) {
		OSyncMappingEntry *entry = e->data;
		if (osync_change_get_changetype(entry->change) == CHANGE_DELETED) {
			osync_change_delete(entry->change, NULL);
			osengine_mappingentry_free(entry);
		}
	}
	g_list_free(entries);
	
	//Choose the first modified change as the master of the mapping to duplicate
	GList *i = dupe_mapping->entries;
	do {
		orig_entry = i->data;
		i = i->next;
	} while (osync_change_get_changetype(orig_entry->change) != CHANGE_MODIFIED && osync_change_get_changetype(orig_entry->change) != CHANGE_ADDED);
	dupe_mapping->master = orig_entry;
	osync_change_set_changetype(orig_entry->change, CHANGE_MODIFIED);
	
	/* Now we go through the list of changes in the mapping to
	 * duplicate and search for the next entry that is different
	 * to our choosen master. This entry then has to be moved to a
	 * new mapping along with all changes to are the same as this next
	 * different change
	 */
	while ((first_diff_entry = _osync_find_next_diff(dupe_mapping, orig_entry))) {
		//We found a different change
		elevation = 0;
		new_mapping = osengine_mapping_new(engine->maptable);
		new_mapping->id = osengine_mappingtable_get_next_id(engine->maptable);
		osync_flag_unset(new_mapping->cmb_synced);
		osync_flag_set(new_mapping->fl_chkconflict);
		osync_flag_unset(new_mapping->fl_multiplied);
		osync_flag_set(new_mapping->fl_solved);
		send_mapping_changed(engine, new_mapping);
		osync_debug("MAP", 3, "Created new mapping for duplication %p with mappingid %lli", new_mapping, new_mapping->id);
		
		/* Now we copy the change that differs, and set it as the master of the new
		 * mapping.*/
		OSyncMappingEntry *newentry = osengine_mappingentry_copy(first_diff_entry);	
		new_mapping->master = newentry;
		osengine_mapping_add_entry(new_mapping, newentry);
		osync_change_set_changetype(newentry->change, CHANGE_ADDED);
		osync_flag_set(newentry->fl_has_data);
		osync_flag_set(newentry->fl_mapped);
		osync_flag_set(newentry->fl_has_info);
		osync_flag_set(newentry->fl_dirty);
		osync_flag_unset(newentry->fl_synced);
		
		/* Now we elevate the change (which might be done by adding a -dupe
		 * or a (2) to the change uid. We then check if there is already
		 * another change on this level and if there is, we elevate again */
		do {
			if (!osync_change_elevate(engine, newentry->change, 1))
				break;
			elevation += 1;
		} while (!osync_change_check_level(engine, newentry));
		OSyncError *error = NULL;
		osync_change_save(newentry->change, TRUE, &error);
		
		/* Now we search for all changes to belong to the new mapping, so
		 * we are searching for changes to do not differ from the change we found
		 * to be different from the master of the mapping to duplicate */
		while ((next_entry = _osync_find_next_same(dupe_mapping, first_diff_entry))) {
			newentry = _osync_change_clone(engine, new_mapping, first_diff_entry);
			osync_change_elevate(engine, newentry->change, elevation);
			osengine_mappingentry_update(orig_entry, next_entry->change);
			osync_change_save(next_entry->change, TRUE, NULL);
		}
		
		/* Now we can reset the different change and prepare it for
		 * being overwriten during mulitply_master */
		osync_change_set_changetype(first_diff_entry->change, CHANGE_UNKNOWN);

		//We can now add the new mapping into the queue so it get processed
		send_mapping_changed(engine, new_mapping);
	}

	//Multiply our original mapping
	osync_flag_set(dupe_mapping->fl_solved);
	send_mapping_changed(engine, dupe_mapping);
	osync_trace(TRACE_EXIT, "osengine_mapping_duplicate");
}

/** @brief Solves the mapping by choosing a winner
 * 
 * The winner will overwrite all other entries of this mapping
 * 
 * @param engine The engine
 * @param mapping The conflicting mapping
 * @param change The winning change
 * 
 */
void osengine_mapping_solve(OSyncEngine *engine, OSyncMapping *mapping, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "osengine_mapping_solve(%p, %p, %p)", engine, mapping, change);
	OSyncMappingEntry *entry = osengine_mapping_find_entry(mapping, change, NULL);
	mapping->master = entry;
	osync_flag_set(mapping->fl_solved);
	send_mapping_changed(engine, mapping);
	osync_trace(TRACE_EXIT, "osengine_mapping_solve");
}

/** @brief Ignores a conflict
 * 
 * This ignores the conflict until the next sync. When the group is synchronized again
 * the conflict is brought up again (unless the user solved it already outside of the engine)
 * 
 * @param engine The engine
 * @param mapping The mapping to ignore
 * 
 */
osync_bool osengine_mapping_ignore_conflict(OSyncEngine *engine, OSyncMapping *mapping, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, engine, mapping, error);
	
	if (!osengine_mapping_ignore_supported(engine, mapping)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Ignore is not supported for this mapping");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	GList *e = NULL;
	for (e = mapping->entries; e; e = e->next) {
		OSyncMappingEntry *entry = e->data;
		osync_trace(TRACE_INTERNAL, "Adding %p to logchanges", entry);
		OSyncError *error = NULL;
		if (osync_change_get_changetype(entry->change) != CHANGE_UNKNOWN)
			osync_group_save_changelog(engine->group, entry->change, &error);
	}
	
	//And make sure we dont synchronize it this time
	//osengine_mapping_reset(mapping);
	osync_flag_set(mapping->fl_multiplied);
	osync_flag_set(mapping->cmb_synced);
	osync_flag_set(mapping->cmb_has_info);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/** @brief Checks if a conflict can be ignore
 * 
 * To be able to ignore a conflict, you opensync must be able to read
 * the changes of the conflict again during the next synchronization. This must be done
 * even if they are not reported by the plugin. Therefore, all plugins should provide
 * a "read" method. If there is a member in the engine's group that does not have this
 * method (either since it is not possible to implement one or since it has not been done
 * yet), this function will return FALSE.
 *
 * @param engine The engine
 * @param mapping The mapping to check
 * @returns TRUE if conflicts can be ignored, FALSE otherwise
 */
osync_bool osengine_mapping_ignore_supported(OSyncEngine *engine, OSyncMapping *mapping)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, mapping);
	
	int i, count = 0;
	OSyncChange *change = NULL;
	OSyncMember *member = NULL;
	OSyncObjType *objtype = NULL;

	count = osengine_mapping_num_changes(mapping);
	for (i = 0; i < count; ++i) {
		change = osengine_mapping_nth_change(mapping, i);
		objtype = osync_change_get_objtype(change);

		member = osync_change_get_member(change);

		if (!osync_member_has_read_function(member, objtype)) {
			osync_trace(TRACE_EXIT, "%s: Ignore NOT supported", __func__);
			return FALSE;
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: Ignore supported", __func__);
	return TRUE;
}

/** @brief Solves a mapping by choosing the entry that was last modified
 * 
 * Solves the mapping by choosing the last modified entry. Note that this can fail
 * if one of the entries does not have a timestamp set or of the 2 latest timestamps
 * were exactly equal. If it could not be solved you have to solve it with another function!
 * 
 * @param engine The engine
 * @param mapping The conflicting mapping
 * @param error A pointer to an error
 * @returns TRUE if the mapping was solved, FALSE otherwise
 * 
 */
osync_bool osengine_mapping_solve_latest(OSyncEngine *engine, OSyncMapping *mapping, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, engine, mapping, error);
	
	time_t time = 0;
	time_t latesttime = 0;
	osync_bool preveq = FALSE;
	
	GList *e = NULL;
	for (e = mapping->entries; e; e = e->next) {
		OSyncMappingEntry *entry = e->data;
		
		if (osync_change_get_changetype(entry->change) != CHANGE_UNKNOWN) {
			time = osync_change_get_revision(entry->change, error);
			if (time == -1) {
				osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
				mapping->master = NULL;
				return FALSE;
			}
			
			if (time > latesttime) {
				latesttime = time;
				mapping->master = entry;
				preveq = FALSE;
			} else if (time == latesttime)
				preveq = TRUE;
		}
	}
	
	if (preveq == TRUE) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Could not decide for one entry. Timestamps where equal");
		mapping->master = NULL;
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_flag_set(mapping->fl_solved);
	send_mapping_changed(engine, mapping);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, mapping->master);
	return TRUE;
}

/** @brief Checks if the mapping could be solved with solve_latest
 * 
 * This functions checks all changes to see if they contain valid
 * timestamp information and if they could be used to solve but does
 * not actually solve the mapping
 * 
 * @param engine The engine
 * @param mapping The conflicting mapping
 * @param error A pointer to an error
 * @returns TRUE if the mapping could be solved, FALSE otherwise
 * 
 */
osync_bool osengine_mapping_check_timestamps(OSyncEngine *engine, OSyncMapping *mapping, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, engine, mapping, error);
	
	time_t time = 0;
	time_t latesttime = 0;
	osync_bool preveq = FALSE;
	
	GList *e = NULL;
	for (e = mapping->entries; e; e = e->next) {
		OSyncMappingEntry *entry = e->data;
		
		if (osync_change_get_changetype(entry->change) != CHANGE_UNKNOWN) {
			time = osync_change_get_revision(entry->change, error);
			if (time == -1) {
				osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
				return FALSE;
			}
			
			if (time > latesttime) {
				latesttime = time;
				preveq = FALSE;
			} else if (time == latesttime)
				preveq = TRUE;
		}
	}
	
	if (preveq == TRUE) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Could not decide for one entry. Timestamps where equal");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/** @brief Solves a mapping by setting an updated change
 * 
 * Solves the mapping by setting an updated change. The change should have been edited by the user.
 * This change will then be declared winner.
 * 
 * @param engine The engine
 * @param mapping The conflicting mapping
 * @param change The updated change
 * 
 */
void osengine_mapping_solve_updated(OSyncEngine *engine, OSyncMapping *mapping, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, engine, mapping, change);
	OSyncMappingEntry *entry = osengine_mapping_find_entry(mapping, change, NULL);
	mapping->master = entry;
	
	osync_flag_set(entry->fl_dirty);
	osync_flag_unset(entry->fl_synced);
	send_mappingentry_changed(engine, entry);
	
	osync_flag_set(mapping->fl_solved);
	send_mapping_changed(engine, mapping);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*@}*/
