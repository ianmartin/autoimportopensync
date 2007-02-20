/*
 * libopensync - A synchronization framework
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
 * @defgroup OSEngineMappingPrivate OpenSync Mapping Internals
 * @ingroup OSEnginePrivate
 * @brief The internals the mappings
 * 
 */
/*@{*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS
OSyncMappingTable *osengine_mappingtable_new(OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "osengine_mappingtable_new(%p)", engine);
	OSyncMappingTable *table = g_malloc0(sizeof(OSyncMappingTable));
	table->engine = engine;
	table->group = engine->group;
	
	GList *c;
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		osengine_mappingview_new(table, client);
	}
	
	osync_trace(TRACE_EXIT, "osengine_mappingtable_new: %p", table);
	return table;
}

void osengine_mappingtable_reset(OSyncMappingTable *table)
{
	GList *v;
	for (v = table->views; v; v = v->next) {
		OSyncMappingView *view = v->data;
		osengine_mappingview_reset(view);
	}
}

void osengine_mappingtable_free(OSyncMappingTable *table)
{
	osync_trace(TRACE_ENTRY, "osengine_mappingtable_free(%p)", table);
	GList *c = NULL;
	GList *m = NULL;

	GList *mappings = g_list_copy(table->mappings);
	GList *unmapped = g_list_copy(table->unmapped);
	GList *views = g_list_copy(table->views);
	osync_trace(TRACE_INTERNAL, "Free mappings");
	for (m = mappings; m; m = m->next) {
		OSyncMapping *mapping = m->data;
		osengine_mapping_free(mapping);
	}
	osync_trace(TRACE_INTERNAL, "Free unmapped");
	for (c = unmapped; c; c = c->next) {
		OSyncMappingEntry *entry = c->data;
		osengine_mappingentry_free(entry);
	}
	for (c = views; c; c = c->next) {
		OSyncMappingView *view = c->data;
		osengine_mappingview_free(view);
	}
	g_list_free(mappings);
	g_list_free(unmapped);
	g_list_free(views);
	g_free(table);
	osync_trace(TRACE_EXIT, "osengine_mappingtable_free");
}

OSyncMappingEntry *osengine_mappingtable_find_entry(OSyncMappingTable *table, const char *uid, const char *objtype, long long int memberid)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s)", __func__, table, uid, objtype ? objtype : "None");
	GList *v;
	int count_of_entries = 0; /*must not be more the one for objtype=NULL*/
	OSyncMappingEntry *ret_entry = NULL;
	for (v = table->views; v; v = v->next) {
		OSyncMappingView *view = v->data;
		GList *c;
		
		if (memberid && memberid != osync_member_get_id(view->client->member))
			continue;
		
		for (c = view->changes; c; c = c->next) {
			OSyncMappingEntry *entry = c->data;
			g_assert(entry->change);
			if(objtype){
				if ( (!strcmp(
					osync_change_get_uid(entry->change), uid)) &&
				   (!strcmp(
					osync_objtype_get_name(
						osync_change_get_objtype(entry->change))
					, objtype))
				) {
					ret_entry = entry;
					count_of_entries++;
				}
			} else { /**objtype is NULL ... try to find a entry based on uid*/
				if (!strcmp(osync_change_get_uid(entry->change), uid)) {
					ret_entry = entry;
					count_of_entries++;
				}
			}
		}
	}
	if(count_of_entries == 1 && ret_entry){
		osync_trace(TRACE_EXIT, "%s: %p", __func__, ret_entry);
		return ret_entry;
	}
	if(count_of_entries >1){
		if (!objtype)
		{
			osync_trace(TRACE_EXIT_ERROR, "%s: possible dataloss", __func__ );
		} else {
			osync_trace(TRACE_EXIT_ERROR, "%s: changes.db corrupted", __func__ );
		}
		return NULL;
	}

	osync_trace(TRACE_EXIT, "%s: Not Found", __func__);
	return NULL;
}

OSyncMappingEntry *osengine_mappingtable_store_change(OSyncMappingTable *table, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "osengine_mappingtable_store_change(%p, %p)", table, change);
	OSyncMappingView *view = osengine_mappingtable_find_view(table, osync_change_get_member(change));
	g_assert(view);
	OSyncMappingEntry *entry = osengine_mappingview_store_change(view, change);
	osync_trace(TRACE_EXIT, "osengine_mappingtable_store_change: %p", entry);
	return entry;
}

OSyncMapping *osengine_mappingtable_find_mapping(OSyncMappingTable *table, OSyncChange *change)
{
	GList *m;
	for (m = table->mappings; m; m = m->next) {
		OSyncMapping *mapping = m->data;
		if (osengine_mapping_find_entry(mapping, change, NULL))
			return mapping;
	}
	return NULL;
}

OSyncMapping *osengine_mappingtable_mapping_from_id(OSyncMappingTable *table, long long int id)
{
	GList *m;
	for (m = table->mappings; m; m = m->next) {
		OSyncMapping *mapping = m->data;
		if (mapping->id == id)
			return mapping;
	}
	return NULL;
}

OSyncMappingView *osengine_mappingtable_find_view(OSyncMappingTable *table, OSyncMember *member)
{
	GList *v;
	for (v = table->views; v; v = v->next) {
		OSyncMappingView *view = v->data;
		if (view->memberid == osync_member_get_id(member))
			return view;
	}
	return NULL;
}

void osengine_mappingtable_add_mapping(OSyncMappingTable *table, OSyncMapping *mapping)
{
	table->mappings = g_list_append(table->mappings, mapping);
	mapping->table = table;
}

osync_bool osengine_mappingtable_load(OSyncMappingTable *table, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osengine_mappingtable_load(%p, %p)", table, error);
	OSyncChange **changes = NULL;
	if (!osync_changes_load(table->group, &changes, error)) {
		osync_trace(TRACE_EXIT_ERROR, "osengine_mappingtable_load: %s", osync_error_print(error));
		return FALSE;
	}
	
	int i = 0;
	OSyncChange *change = NULL;
	OSyncMapping *mapping = NULL;
	while ((change = changes[i])) {
		OSyncMappingEntry *entry = osengine_mappingentry_new(NULL);
		entry->change = change;
		//entry->orig_change = change;
		entry->client = (OSyncClient *)osync_member_get_data(osync_change_get_member(change));

		if (!osync_change_get_mappingid(change)) {
    		table->unmapped = g_list_append(table->unmapped, entry);
		} else {
    		if (!mapping || mapping->id != osync_change_get_mappingid(change)) {
				mapping = osengine_mapping_new(table);
				mapping->id = osync_change_get_mappingid(change);
    		}
    		osengine_mapping_add_entry(mapping, entry);
    	}
    	
    	osync_flag_set(entry->fl_has_data);
    	
    	OSyncMappingView *view = osengine_mappingtable_find_view(table, osync_change_get_member(change));
    	if (view)
    		osengine_mappingview_add_entry(view, entry);

    	i++;
	}
	
	osync_trace(TRACE_EXIT, "osengine_mappingtable_load: TRUE");
	return TRUE;
}

long long int osengine_mappingtable_get_next_id(OSyncMappingTable *table)
{
	long long int new_id = 1;
	GList *m;
	for (m = table->mappings; m; m = m->next) {
		OSyncMapping *mapping = m->data;
		if (new_id <= mapping->id)
			new_id = mapping->id + 1;
	}
	return new_id;
}

void osengine_mappingtable_inject_changes(OSyncMappingTable *table)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, table);
	//OSyncEngine *engine = table->engine;

	char **uids = NULL;
	char **objtypes = NULL;
	long long int *memberids = NULL;
	int *types = NULL;
	char *uid = NULL;
	char *objtype = NULL;
	int type = 0;
	int i = 0;
	OSyncError *error = NULL;
	osync_group_open_changelog(table->engine->group, &uids, &objtypes, &memberids, &types, &error);	
	
	for (i = 0; (uid = uids[i]) ; i++) {
		type = types[i];
		objtype = objtypes[i];
		long long int memberid = memberids[i];
		OSyncMappingEntry *entry = osengine_mappingtable_find_entry(table, uid, objtype, memberid);

		if (!entry) {
			osync_trace(TRACE_INTERNAL, "Mappingtable and changelog inconsistent: no entry with uid %s", uid);
			/*FIXME: We should be able to return error here. What if entry == NULL? */
			g_assert_not_reached();
		}

		osync_change_set_changetype(entry->change, type);
		osync_trace(TRACE_INTERNAL, "Injecting %p with changetype %i", entry, osync_change_get_changetype(entry->change));
		osync_flag_attach(entry->fl_read, table->engine->cmb_read_all);

		/* Set fl_mapped accordingly, if the entry was already mapped previously */
		if (entry->mapping)
			osync_flag_set(entry->fl_mapped);

		//send_read_change(engine, entry);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

OSyncMappingTable *_osengine_mappingtable_load_group(OSyncGroup *group)
{
	OSyncMappingTable *table = g_malloc0(sizeof(OSyncMappingTable));
	table->group = group;
	
	int i;
	for (i = 0; i < osync_group_num_members(group); i++) {
		OSyncMember *member = osync_group_nth_member(group, i);
		OSyncMappingView *view = g_malloc0(sizeof(OSyncMappingView));
		table->views = g_list_append(table->views, view);
		view->table = table;
		view->memberid = osync_member_get_id(member);
	}
	
	if (!osengine_mappingtable_load(table, NULL))
		return NULL;
	return table;
}

void osengine_mappingtable_close(OSyncMappingTable *table)
{
	osync_changes_close(table->group);
	//FIXME Free the changes on the views
}

OSyncMapping *osengine_mapping_new(OSyncMappingTable *table)
{
	g_assert(table);
	OSyncMapping *mapping = g_malloc0(sizeof(OSyncMapping));
	osengine_mappingtable_add_mapping(table, mapping);
	if (table->engine) {
		mapping->fl_solved = osync_flag_new(NULL);
		
		mapping->fl_chkconflict = osync_flag_new(NULL);
		osync_flag_set(mapping->fl_chkconflict);
		
		mapping->fl_multiplied = osync_flag_new(NULL);
		osync_flag_set(mapping->fl_multiplied);
		
		mapping->cmb_has_data = osync_comb_flag_new(FALSE, FALSE);
		osync_flag_set_pos_trigger(mapping->cmb_has_data, (OSyncFlagTriggerFunc)send_mapping_changed, table->engine, mapping);
		
		mapping->cmb_has_info = osync_comb_flag_new(FALSE, FALSE);
		
		mapping->cmb_synced = osync_comb_flag_new(FALSE, TRUE);
		
		mapping->cmb_deleted = osync_comb_flag_new(FALSE, FALSE);
		
		osync_flag_attach(mapping->cmb_synced, table->engine->cmb_synced);
		osync_flag_attach(mapping->fl_multiplied, table->engine->cmb_multiplied);
		osync_flag_attach(mapping->fl_chkconflict, table->engine->cmb_chkconflict);
	}
	osync_trace(TRACE_INTERNAL, "osengine_mapping_new(%p): %p", table, mapping);
	return mapping;
}

void osengine_mapping_free(OSyncMapping *mapping)
{
	osync_trace(TRACE_ENTRY, "osengine_mapping_free(%p)", mapping);
	
	while (g_list_nth_data(mapping->entries, 0))
		osengine_mappingentry_free(g_list_nth_data(mapping->entries, 0));

	osync_flag_detach(mapping->cmb_synced);
	osync_flag_detach(mapping->fl_chkconflict);
	osync_flag_detach(mapping->fl_multiplied);

	mapping->table->mappings = g_list_remove(mapping->table->mappings, mapping);
	osync_flag_free(mapping->fl_solved);
	osync_flag_free(mapping->cmb_has_data);
	osync_flag_free(mapping->cmb_has_info);
	osync_flag_free(mapping->cmb_synced);
	osync_flag_free(mapping->fl_chkconflict);
	osync_flag_free(mapping->cmb_deleted);
	osync_flag_free(mapping->fl_multiplied);
	
	g_free(mapping);
	osync_trace(TRACE_EXIT, "osengine_mapping_free");
}

void osengine_mapping_add_entry(OSyncMapping *mapping, OSyncMappingEntry *entry)
{
	osync_trace(TRACE_INTERNAL, "osengine_mapping_add_entry(%p, %p)", mapping, entry);
	g_assert(!osengine_mapping_find_entry(mapping, NULL, entry->view));
	mapping->entries = g_list_append(mapping->entries, entry);
	entry->mapping = mapping;
	
	if (mapping->table->engine) {
		osync_flag_attach(entry->fl_has_data, mapping->cmb_has_data);
		osync_flag_attach(entry->fl_has_info, mapping->cmb_has_info);
		osync_flag_attach(entry->fl_synced, mapping->cmb_synced);
		osync_flag_attach(entry->fl_deleted, mapping->cmb_deleted);
		osync_flag_set_pos_trigger(entry->fl_dirty, (OSyncFlagTriggerFunc)send_mappingentry_changed, mapping->table->engine, entry);
	}
	osync_change_set_mappingid(entry->change, mapping->id);
	
	mapping->table->unmapped = g_list_remove(mapping->table->unmapped, entry);
	mapping->table->entries = g_list_append(mapping->table->entries, entry);
}

void osengine_mapping_remove_entry(OSyncMapping *mapping, OSyncMappingEntry *entry)
{
	mapping->entries = g_list_remove(mapping->entries, entry);
	mapping->table->entries = g_list_remove(mapping->table->entries, entry);
	entry->mapping = NULL;
	
	osync_flag_detach(entry->fl_has_data);
	osync_flag_detach(entry->fl_has_info);
	osync_flag_detach(entry->fl_synced);
	osync_flag_detach(entry->fl_deleted);
}

OSyncMappingEntry *osengine_mapping_find_entry(OSyncMapping *mapping, OSyncChange *change, OSyncMappingView *view)
{
	GList *e;
	for (e = mapping->entries; e; e = e->next) {
		OSyncMappingEntry *entry = e->data;
		if (change && entry->change == change)
			return entry;
		if (view && entry->view == view)
			return entry;
	}
	return NULL;
}

OSyncMappingEntry *osengine_mapping_nth_entry(OSyncMapping *mapping, int nth)
{
	return (OSyncMappingEntry *)g_list_nth_data(mapping->entries, nth);
}

int osengine_mapping_num_changes(OSyncMapping *mapping)
{
	return g_list_length(mapping->entries);
}

OSyncChange *osengine_mapping_nth_change(OSyncMapping *mapping, int nth)
{
	OSyncMappingEntry *entry = g_list_nth_data(mapping->entries, nth);
	if (!entry)
		return NULL;
	return entry->change;
}

long long osengine_mapping_get_id(OSyncMapping *mapping)
{
	return mapping->id;
}

void osengine_mapping_reset(OSyncMapping *mapping)
{
	osync_trace(TRACE_ENTRY, "osengine_mapping_reset(%p)", mapping);
	GList *e;
	for (e = mapping->entries; e; e = e->next) {
		OSyncMappingEntry *entry = e->data;
		osengine_mappingentry_reset(entry);
	}
	
	osync_flag_set(mapping->fl_multiplied);
	osync_flag_set(mapping->fl_chkconflict);
	mapping->master = NULL;
	osync_trace(TRACE_EXIT, "osengine_mapping_reset");
}

void osengine_mapping_delete(OSyncMapping *mapping)
{
	osync_trace(TRACE_ENTRY, "osengine_mapping_delete(%p)", mapping);
	GList *entries = g_list_copy(mapping->entries);
	GList *c = NULL;
	for (c = entries; c; c = c->next) {
		OSyncMappingEntry *entry = c->data;
		osync_change_delete(entry->change, NULL);
	}
	g_list_free(entries);
	osengine_mapping_free(mapping);
	osync_trace(TRACE_EXIT, "osengine_mapping_delete");
}

OSyncMappingView *osengine_mappingview_new(OSyncMappingTable *table, OSyncClient *client)
{
	g_assert(table);
	OSyncMappingView *view = g_malloc0(sizeof(OSyncMappingView));
	table->views = g_list_append(table->views, view);
	view->client = client;
	view->table = table;
	view->memberid = osync_member_get_id(client->member);
	osync_trace(TRACE_INTERNAL, "osengine_mappingview_new(%p)", view);
	return view;
}

void osengine_mappingview_free(OSyncMappingView *view)
{
	osync_trace(TRACE_INTERNAL, "osengine_mappingview_free(%p)", view);
	g_list_free(view->changes);
	view->changes = NULL;
	g_free(view);
}

void osengine_mappingview_add_entry(OSyncMappingView *view, OSyncMappingEntry *entry)
{
	view->changes = g_list_append(view->changes, entry);
	entry->view = view;
}

OSyncMappingEntry *osengine_mappingview_store_change(OSyncMappingView *view, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "osengine_mappingview_store_change(%p, %p)", view, change);
	g_assert(change);
	GList *c;
	for (c = view->changes; c; c = c->next) {
		OSyncMappingEntry *entry = c->data;
		g_assert(entry->change);
		if (
			(!strcmp(osync_change_get_uid(entry->change), osync_change_get_uid(change))) &&
			(osync_change_get_objtype(entry->change) == osync_change_get_objtype(change))
		) {
			osengine_mappingentry_update(entry, change);
			osync_trace(TRACE_EXIT, "osengine_mappingview_store_change: %p", entry);
			return entry;
		}
	}
	
	OSyncMappingEntry *newentry = osengine_mappingentry_new(NULL);
	newentry->change = change;
	newentry->client = view->client;
	view->table->unmapped = g_list_append(view->table->unmapped, newentry);
	osengine_mappingview_add_entry(view, newentry);
	osync_trace(TRACE_EXIT, "osengine_mappingview_store_change: %p (New MappingEntry)", newentry);
	return newentry;
}

osync_bool osengine_mappingview_uid_is_unique(OSyncMappingView *view, OSyncMappingEntry *entry, osync_bool spare_deleted)
{
	GList *e = NULL;

	for (e = view->changes; e; e = e->next) {
		OSyncMappingEntry *exentry = e->data;
		if ((exentry != entry) && (!spare_deleted || (osync_change_get_changetype(exentry->change) != CHANGE_DELETED)) && !strcmp(osync_change_get_uid(exentry->change), osync_change_get_uid(entry->change)))
			return FALSE;
	}
	return TRUE;
}

void osengine_mappingview_reset(OSyncMappingView *view)
{
	//g_list_free(view->changes);
	//view->changes = NULL;
}

OSyncMappingEntry *osengine_mappingentry_new(OSyncMapping *mapping)
{
	OSyncMappingEntry *entry = g_malloc0(sizeof(OSyncMappingEntry));
	osync_trace(TRACE_INTERNAL, "osengine_mappingentry_new(%p): %p", mapping, entry);
	entry->fl_has_data = osync_flag_new(NULL);
	entry->fl_dirty = osync_flag_new(NULL);
	entry->fl_mapped = osync_flag_new(NULL);
	entry->fl_has_info = osync_flag_new(NULL);
	entry->fl_synced = osync_flag_new(NULL);
	entry->fl_deleted = osync_flag_new(NULL);
	entry->fl_read = osync_flag_new(NULL);
	entry->fl_committed = osync_flag_new(NULL);
	osync_flag_set(entry->fl_synced);
	
	if (mapping)
		osengine_mapping_add_entry(mapping, entry);
	
	return entry;
}

void osengine_mappingentry_free(OSyncMappingEntry *entry)
{
	osync_trace(TRACE_INTERNAL, "osengine_mappingentry_free(%p)", entry);
	
	if (entry->mapping)
		osengine_mapping_remove_entry(entry->mapping, entry);
	
	osync_flag_free(entry->fl_has_data);
	osync_flag_free(entry->fl_dirty);
	osync_flag_free(entry->fl_mapped);
	osync_flag_free(entry->fl_has_info);
	osync_flag_free(entry->fl_synced);
	osync_flag_free(entry->fl_deleted);
	osync_flag_free(entry->fl_read);
	osync_flag_free(entry->fl_committed);
	
	entry->view->changes = g_list_remove(entry->view->changes, entry);
	entry->view = NULL;

	g_free(entry);
}

void osengine_mappingentry_update(OSyncMappingEntry *entry, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, entry, change);
	
	OSyncObjFormat *format = osync_change_get_objformat(entry->change);
	OSyncObjType *type = osync_change_get_objtype(entry->change);

	osync_change_update(change, entry->change);
	
	if (osync_change_get_changetype(change) == CHANGE_DELETED && format && type) {
		osync_change_set_objformat(entry->change, format);
		osync_change_set_objtype(entry->change, type);
		
		osync_trace(TRACE_INTERNAL, "Change was deleted. Old objtype %s and format %s", osync_change_get_objtype(entry->change) ? osync_objtype_get_name(osync_change_get_objtype(entry->change)) : "None", osync_change_get_objformat(entry->change) ? osync_objformat_get_name(osync_change_get_objformat(entry->change)) : "None");
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

OSyncMappingEntry *osengine_mappingentry_copy(OSyncMappingEntry *entry)
{
	OSyncMappingEntry *newentry = osengine_mappingentry_new(NULL);
	
	OSyncError *error  = NULL;
	newentry->change = osync_change_copy(entry->change, &error);
	newentry->client = entry->client;
	osengine_mappingview_add_entry(entry->view, newentry);
	return newentry;
}

void osengine_mappingentry_reset(OSyncMappingEntry *entry)
{
	osync_trace(TRACE_INTERNAL, "osengine_mappingentry_reset(%p)", entry);
	
	osync_flag_set(entry->fl_has_data);
	osync_flag_unset(entry->fl_dirty);
	osync_flag_unset(entry->fl_has_info);
	osync_flag_unset(entry->fl_deleted);
	osync_flag_set(entry->fl_synced);
	
	osync_change_reset(entry->change);
}
#endif

/** @} */