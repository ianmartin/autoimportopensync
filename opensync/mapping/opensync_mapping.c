/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2006  Armin Bauer <armin.bauer@desscon.com>
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

#include "opensync-mapping.h"

#include "opensync_mapping_internals.h"

OSyncMapping *osync_mapping_new(OSyncError **error)
{
        OSyncMapping *mapping = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	mapping = osync_try_malloc0(sizeof(OSyncMapping), error);
	if (!mapping)
		goto error;
	mapping->ref_count = 1;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, mapping);
	return mapping;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

OSyncMapping *osync_mapping_ref(OSyncMapping *mapping)
{
	osync_assert(mapping);
	
	g_atomic_int_inc(&(mapping->ref_count));

	return mapping;
}

void osync_mapping_unref(OSyncMapping *mapping)
{
	osync_assert(mapping);
		
	if (g_atomic_int_dec_and_test(&(mapping->ref_count))) {
		
		while (mapping->entries) {
			OSyncMappingEntry *entry = mapping->entries->data;
			osync_mapping_entry_unref(entry);
			mapping->entries = g_list_remove(mapping->entries, entry);
		}
		
		g_free(mapping);
	}
}

long long int osync_mapping_get_id(OSyncMapping *mapping)
{
	osync_assert(mapping);
	return mapping->id;
}

void osync_mapping_set_id(OSyncMapping *mapping, long long int id)
{
	osync_assert(mapping);
	mapping->id = id;
}

void osync_mapping_add_entry(OSyncMapping *mapping, OSyncMappingEntry *entry)
{
	osync_assert(mapping);
	osync_assert(entry);
	
	mapping->entries = g_list_append(mapping->entries, entry);
	osync_mapping_entry_ref(entry);
}

void osync_mapping_remove_entry(OSyncMapping *mapping, OSyncMappingEntry *entry)
{
	osync_assert(mapping);
	osync_assert(entry);
	
	mapping->entries = g_list_remove(mapping->entries, entry);
	osync_mapping_entry_unref(entry);
}

/*OSyncMappingEntry *osync_mapping_find_entry(OSyncMapping *mapping, OSyncChange *change)
{
	GList *e;
	for (e = mapping->entries; e; e = e->next) {
		OSyncMappingEntry *entry = e->data;
		if (change && osync_mapping_entry_get_change(entry) == change)
			return entry;
	}
	return NULL;
}*/

OSyncMappingEntry *osync_mapping_find_entry_by_member_id(OSyncMapping *mapping, long long int memberid)
{
	GList *e;
	for (e = mapping->entries; e; e = e->next) {
		OSyncMappingEntry *entry = e->data;
		if (osync_mapping_entry_get_member_id(entry) == memberid)
			return entry;
	}
	return NULL;
}

int osync_mapping_num_entries(OSyncMapping *mapping)
{
	osync_assert(mapping);
	return g_list_length(mapping->entries);
}

OSyncMappingEntry *osync_mapping_nth_entry(OSyncMapping *mapping, int nth)
{
	osync_assert(mapping);
	return g_list_nth_data(mapping->entries, nth);
}
