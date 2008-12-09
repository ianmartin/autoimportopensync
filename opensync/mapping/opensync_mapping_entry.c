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

#include "opensync_mapping_entry_internals.h"

OSyncMappingEntry *osync_mapping_entry_new(OSyncError **error)
{
  OSyncMappingEntry *entry = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
  entry = osync_try_malloc0(sizeof(OSyncMappingEntry), error);
  if (!entry)
    goto error;
  entry->ref_count = 1;
	
  osync_trace(TRACE_EXIT, "%s: %p", __func__, entry);
  return entry;

 error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return NULL;
}

OSyncMappingEntry *osync_mapping_entry_ref(OSyncMappingEntry *entry)
{
  osync_assert(entry);
	
  g_atomic_int_inc(&(entry->ref_count));

  return entry;
}

void osync_mapping_entry_unref(OSyncMappingEntry *entry)
{
  osync_assert(entry);
		
  if (g_atomic_int_dec_and_test(&(entry->ref_count))) {
		
    if (entry->uid)
      g_free(entry->uid);
		
    g_free(entry);
  }
}

osync_bool osync_mapping_entry_matches(OSyncMappingEntry *entry, OSyncChange *change)
{
  osync_assert(entry);
  osync_assert(change);
	
  if (!strcmp(entry->uid, osync_change_get_uid(change)))
    return TRUE;
	
  return FALSE;
}

/*void osync_mapping_entry_update(OSyncMappingEntry *entry, OSyncChange *change)
  {
  osync_assert(entry);
  osync_assert(change);
	
  if (entry->change)
  osync_change_unref(entry->change);
	
  entry->change = change;
  osync_change_ref(change);
  }

  OSyncChange *osync_mapping_entry_get_change(OSyncMappingEntry *entry)
  {
  osync_assert(entry);
  return entry->change;
  }

  osync_bool osync_mapping_entry_is_dirty(OSyncMappingEntry *entry)
  {
  osync_assert(entry);
  return entry->dirty;
  }

  void osync_mapping_entry_set_dirty(OSyncMappingEntry *entry, osync_bool dirty)
  {
  osync_assert(entry);
  entry->dirty = dirty;
  }*/

void osync_mapping_entry_set_uid(OSyncMappingEntry *entry, const char *uid)
{
  osync_assert(entry);
  osync_assert(uid);

  if (entry->uid)
    g_free(entry->uid);
  entry->uid = g_strdup(uid);
}

const char *osync_mapping_entry_get_uid(OSyncMappingEntry *entry)
{
  osync_assert(entry);
  return entry->uid;
}

long long int osync_mapping_entry_get_member_id(OSyncMappingEntry *entry)
{
  osync_assert(entry);
  return entry->member_id;
}

void osync_mapping_entry_set_member_id(OSyncMappingEntry *entry, long long int id)
{
  osync_assert(entry);
  entry->member_id = id;
}

long long int osync_mapping_entry_get_id(OSyncMappingEntry *entry)
{
  osync_assert(entry);
  return entry->id;
}

void osync_mapping_entry_set_id(OSyncMappingEntry *entry, long long int id)
{
  osync_assert(entry);
  entry->id = id;
}
