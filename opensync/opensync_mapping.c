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
 
#include <opensync.h>
#include "opensync_internals.h"

OSyncMappingTable *osync_mappingtable_new(OSyncGroup *group)
{
	OSyncMappingTable *table = g_malloc0(sizeof(OSyncMappingTable));
	table->group = group;
	return table;
}

void osync_mappingtable_free(OSyncMappingTable *table)
{
	GList *c = NULL;
	GList *m = NULL;

	GList *mappings = g_list_copy(table->mappings);
	GList *unmapped = g_list_copy(table->unmapped);

	for (m = mappings; m; m = m->next) {
		OSyncMapping *mapping = m->data;
		osync_mapping_free(mapping);
	}
	for (c = unmapped; c; c = c->next) {
		OSyncChange *change = c->data;
		osync_change_free(change);
	}
	g_list_free(mappings);
	g_list_free(unmapped);
	g_free(table->db_path);
	g_free(table);
}

osync_bool osync_mappingtable_load(OSyncMappingTable *table, OSyncError **error)
{
	return osync_db_open_mappingtable(table, error);
}

void osync_mappingtable_close(OSyncMappingTable *table)
{
	osync_db_close_mappingtable(table);
}

void osync_mappingtable_save_change(OSyncMappingTable *table, OSyncChange *change)
{
	osync_db_save_change(table, change);
}

void osync_mappingtable_delete_change(OSyncMappingTable *table, OSyncChange *change)
{
	osync_db_delete_change(table, change);
}

void osync_mappingtable_add_mapping(OSyncMappingTable *table, OSyncMapping *mapping)
{
	table->mappings = g_list_append(table->mappings, mapping);
	mapping->table = table;
}

void osync_mappingtable_remove_mapping(OSyncMappingTable *table, OSyncMapping *mapping)
{
	g_assert(table);
	table->mappings = g_list_remove(table->mappings, mapping);
}

int osync_mappingtable_num_mappings(OSyncMappingTable *table)
{
	return g_list_length(table->mappings);
}

OSyncGroup *osync_mapping_get_group(OSyncMapping *mapping)
{
	OSyncMappingTable *table = mapping->table;
	g_assert(table);
	return table->group;
}

OSyncMapping *osync_mappingtable_nth_mapping(OSyncMappingTable *table, int num)
{
	return g_list_nth_data(table->mappings, num);
}

void *osync_mapping_get_engine_data(OSyncMapping *mapping)
{
	g_assert(mapping);
	return mapping->engine_data;
}

void osync_mapping_set_engine_data(OSyncMapping *mapping, void *engine_data)
{
	g_assert(mapping);
	mapping->engine_data = engine_data;
}

void osync_mapping_set_masterentry(OSyncMapping *mapping, OSyncChange *master)
{
	g_assert(mapping);
	mapping->master = master;
}

OSyncChange *osync_mapping_get_masterentry(OSyncMapping *mapping)
{
	g_assert(mapping);
	return mapping->master;
}

void osync_mapping_add_entry(OSyncMapping *mapping, OSyncChange *entry)
{
	g_assert(mapping);
	g_assert(entry);
	osync_assert(!osync_mapping_get_entry_by_owner(mapping, entry->member), "WTF?")
	mapping->entries = g_list_append(mapping->entries, entry);
	entry->mapping = mapping;
}

void osync_mapping_remove_entry(OSyncMapping *mapping, OSyncChange *entry)
{
	g_assert(mapping);
	g_assert(entry);
	mapping->entries = g_list_remove(mapping->entries, entry);
	entry->mapping = NULL;
}

OSyncMapping *osync_mapping_new(OSyncMappingTable *table)
{
	g_assert(table);
	OSyncMapping *mapping = g_malloc0(sizeof(OSyncMapping));
	osync_mappingtable_add_mapping(table, mapping);
	return mapping;
}

void osync_mapping_free(OSyncMapping *mapping)
{
	GList *c = NULL;
	GList *entries = g_list_copy(mapping->entries);
	for (c = entries; c; c = c->next) {
		OSyncChange *change = c->data;
		osync_change_free(change);
	}
	g_list_free(entries);
	osync_mappingtable_remove_mapping(mapping->table, mapping);
	g_free(mapping);
}

int osync_mapping_num_entries(OSyncMapping *mapping)
{
	g_assert(mapping);
	return g_list_length(mapping->entries);
}

int osync_mappingtable_num_unmapped(OSyncMappingTable *table)
{
	g_assert(table);
	return g_list_length(table->unmapped);
}

OSyncChange *osync_mappingtable_nth_unmapped(OSyncMappingTable *table, int i)
{
	g_assert(table);
	return g_list_nth_data(table->unmapped, i);
}

void osync_mappingtable_add_unmapped(OSyncMappingTable *table, OSyncChange *change)
{
	g_assert(table);
	table->unmapped = g_list_append(table->unmapped, change);
}

void osync_mappingtable_remove_unmapped(OSyncMappingTable *table, OSyncChange *change)
{
	g_assert(table);
	table->unmapped = g_list_remove(table->unmapped, change);
}

OSyncChange *osync_mapping_nth_entry(OSyncMapping *mapping, int nth)
{
	g_assert(mapping);
	return g_list_nth_data(mapping->entries, nth);
}

//FIXME Do we need this function, or is there a more elegant way?
OSyncChange *osync_mapping_get_entry_by_owner(OSyncMapping *mapping, OSyncMember *member)
{
	g_assert(mapping);
	int i;
	for (i = 0; i < g_list_length(mapping->entries); i++) {
		OSyncChange *change = g_list_nth_data(mapping->entries, i);
		if (osync_member_get_id(change->member) == osync_member_get_id(member))
			return change;
	}
	return NULL;
}

void osync_mappingtable_set_dbpath(OSyncMappingTable *table, const char *path)
{
	g_assert(table);
	//FIXME Free previous path
	table->db_path = g_strdup(path);
}

void osync_mapping_delete(OSyncMapping *mapping)
{
	int i;
	OSyncMappingTable *table = mapping->table;
	for (i = 0; i < osync_mapping_num_entries(mapping); i++) {
		OSyncChange *change = osync_mapping_nth_entry(mapping, i);
		osync_db_delete_change(table, change);
	}
}

long long int osync_mapping_get_id(OSyncMapping *mapping)
{
	return mapping->id;
}

void osync_mapping_set_id(OSyncMapping *mapping, long long int id)
{
	mapping->id = id;
}

void osync_mappingtable_set_slow_sync(OSyncMappingTable *table, const char *objtype)
{
	osync_db_reset_mappingtable(table, objtype);
}
