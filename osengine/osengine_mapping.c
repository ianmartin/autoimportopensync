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
		mapping->cmb_has_data = osync_comb_flag_new(FALSE);
		osync_flag_set_pos_trigger(mapping->cmb_has_data, (MSyncFlagTriggerFunc)send_mapping_changed, table->engine, mapping);
		mapping->cmb_has_info = osync_comb_flag_new(FALSE);
		mapping->cmb_synced = osync_comb_flag_new(FALSE);
		mapping->cmb_deleted = osync_comb_flag_new(FALSE);
		osync_flag_attach(mapping->cmb_synced, table->engine->cmb_synced);
		osync_flag_set(mapping->cmb_synced);
	}
	osync_trace(TRACE_INTERNAL, "osengine_mapping_new(%p): %p", table, mapping);
	return mapping;
}

void osengine_mapping_free(OSyncMapping *mapping)
{
	osync_trace(TRACE_ENTRY, "osengine_mapping_free(%p)", mapping);
	GList *c = NULL;
	GList *entries = g_list_copy(mapping->entries);
	for (c = entries; c; c = c->next) {
		OSyncMappingEntry *entry = c->data;
		osengine_mappingentry_free(entry);
	}
	g_list_free(entries);
	mapping->table->mappings = g_list_remove(mapping->table->mappings, mapping);
	osync_flag_free(mapping->fl_solved);
	osync_flag_free(mapping->cmb_has_data);
	osync_flag_free(mapping->cmb_has_info);
	osync_flag_free(mapping->cmb_synced);
	osync_flag_free(mapping->cmb_deleted);
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
		osync_flag_set_pos_trigger(entry->fl_dirty, (MSyncFlagTriggerFunc)send_mappingentry_changed, mapping->table->engine, entry);
	}
	osync_change_set_mappingid(entry->change, mapping->id);
	
	mapping->table->unmapped = g_list_remove(mapping->table->unmapped, entry);
}

void osengine_mapping_remove_entry(OSyncMapping *mapping, OSyncMappingEntry *entry)
{
	mapping->entries = g_list_remove(mapping->entries, entry);
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
	return ((OSyncMappingEntry *)g_list_nth_data(mapping->entries, nth))->change;
}

void osengine_mapping_reset(OSyncMapping *mapping)
{
	osync_trace(TRACE_ENTRY, "osengine_mapping_reset(%p)", mapping);
	GList *e;
	for (e = mapping->entries; e; e = e->next) {
		OSyncMappingEntry *entry = e->data;
		osengine_mappingentry_reset(entry);
	}
	
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
		if (!strcmp(osync_change_get_uid(entry->change), osync_change_get_uid(change))) {
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
	osync_flag_set(entry->fl_synced);
	
	if (mapping)
		osengine_mapping_add_entry(mapping, entry);
	
	return entry;
}

void osengine_mappingentry_free(OSyncMappingEntry *entry)
{
	osync_trace(TRACE_INTERNAL, "osengine_mappingentry_free(%p)", entry);
	osync_flag_free(entry->fl_has_data);
	osync_flag_free(entry->fl_dirty);
	osync_flag_free(entry->fl_mapped);
	osync_flag_free(entry->fl_has_info);
	osync_flag_free(entry->fl_synced);
	osync_flag_free(entry->fl_deleted);
	
	entry->view->changes = g_list_remove(entry->view->changes, entry);
	entry->view = NULL;

	g_free(entry);
}

void osengine_mappingentry_update(OSyncMappingEntry *entry, OSyncChange *change)
{
	osync_trace(TRACE_INTERNAL, "osengine_mappingentry_update(%p, %p)", entry, change);
	osync_change_update(change, entry->change);
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



































/*void osengine_mappingtable_remove_mapping(OSyncMappingTable *table, OSyncMapping *mapping)
{
	g_assert(table);
	
}

void osengine_mapping_remove_entry(OSyncMapping *mapping, OSyncChange *entry)
{
	g_assert(mapping);
	g_assert(entry);
	mapping->entries = g_list_remove(mapping->entries, entry);
	entry->mapping = NULL;
}

//FIXME Do we need this function, or is there a more elegant way?




void osengine_mappingtable_set_slow_sync(OSyncMappingTable *table, const char *objtype)
{
	osengine_db_reset_mappingtable(table, objtype);
}

//FIXME Remove this and replace with "views"
OSyncChange *osync_member_find_change(OSyncMember *member, const char *uid)
{
	int i;
	for (i = 0; i < g_list_length(member->entries); i++) {
		OSyncChange *entry = g_list_nth_data(member->entries, i);
		if (!strcmp(osync_change_get_uid(entry), uid)) {
			return entry;
		}
	}
	return NULL;
}





//FIXME Remove this and replace with "views"
int osync_member_num_changeentries(OSyncMember *member)
{
	return g_list_length(member->entries);
}

//FIXME Remove this and replace with "views"
OSyncChange *osync_member_nth_changeentry(OSyncMember *member, int n)
{
	return g_list_nth_data(member->entries, n);
}*/


