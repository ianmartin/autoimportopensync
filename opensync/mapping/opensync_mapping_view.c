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
#include "opensync-format.h"
#include "opensync-data.h"

#include "opensync_mapping_view_internals.h"

/*OSyncMappingView *osync_mapping_view_new(OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	OSyncMappingView *view = osync_try_malloc0(sizeof(OSyncMappingView), error);
	if (!view)
		goto error;
	view->ref_count = 1;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, view);
	return view;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return NULL;
}

void osync_mapping_view_ref(OSyncMappingView *view)
{
	osync_assert(view);
	
	g_atomic_int_inc(&(view->ref_count));
}

void osync_mapping_view_unref(OSyncMappingView *view)
{
	osync_assert(view);
		
	if (g_atomic_int_dec_and_test(&(view->ref_count))) {
		g_free(view);
	}
}

osync_bool osync_mapping_view_add_entry(OSyncMappingView *view, OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, view, change, error);
	osync_assert(view);
	osync_assert(change);
	
	GList *e;
	for (e = view->entries; e; e = e->next) {
		OSyncMappingEntry *entry = e->data;
		
		if (osync_mapping_entry_matches(entry, change)) {
			osync_mapping_entry_update(entry, change);
			osync_trace(TRACE_EXIT, "%s: Updated", __func__);
			return TRUE;
		}
	}
	
	OSyncMappingEntry *newentry = osync_mapping_entry_new(error);
	if (!newentry)
		goto error;
	
	osync_mapping_entry_update(newentry, change);
	view->unmapped = g_list_append(view->unmapped, newentry);
	
	osync_trace(TRACE_EXIT, "%s: Added new", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return FALSE;
}

int osync_mapping_view_num_entries(OSyncMappingView *view)
{
	osync_assert(view);
	return g_list_length(view->entries);
}

OSyncMappingEntry *osync_mapping_view_nth_entry(OSyncMappingView *view, int nth)
{
	osync_assert(view);
	return g_list_nth_data(view->entries, nth);
}

int osync_mapping_view_num_unmapped(OSyncMappingView *view)
{
	osync_assert(view);
	return g_list_length(view->unmapped);
}

OSyncMappingEntry *osync_mapping_view_nth_unmapped(OSyncMappingView *view, int nth)
{
	osync_assert(view);
	return g_list_nth_data(view->unmapped, nth);
}

void osync_mapping_view_remove_unmapped(OSyncMappingView *view, OSyncMappingEntry *entry)
{
	osync_assert(view);
	osync_assert(entry);
	
	view->unmapped = g_list_remove(view->unmapped, entry);
	osync_mapping_entry_unref(entry);
}*/
