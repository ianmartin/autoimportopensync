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
 
#ifndef OPENSYNC_MAPPING_H_
#define OPENSYNC_MAPPING_H_

OSYNC_EXPORT OSyncMapping *osync_mapping_new(OSyncError **error);
OSYNC_EXPORT OSyncMapping *osync_mapping_ref(OSyncMapping *mapping);
OSYNC_EXPORT void osync_mapping_unref(OSyncMapping *mapping);

OSYNC_EXPORT long long int osync_mapping_get_id(OSyncMapping *mapping);
OSYNC_EXPORT void osync_mapping_set_id(OSyncMapping *mapping, long long int id);

OSYNC_EXPORT int osync_mapping_num_entries(OSyncMapping *mapping);
OSYNC_EXPORT OSyncMappingEntry *osync_mapping_nth_entry(OSyncMapping *mapping, int nth);

OSYNC_EXPORT void osync_mapping_add_entry(OSyncMapping *mapping, OSyncMappingEntry *entry);
OSYNC_EXPORT void osync_mapping_remove_entry(OSyncMapping *mapping, OSyncMappingEntry *entry);

OSYNC_EXPORT OSyncMappingEntry *osync_mapping_find_entry_by_member_id(OSyncMapping *mapping, long long int memberid);

#endif /*OPENSYNC_MAPPING_H_*/
