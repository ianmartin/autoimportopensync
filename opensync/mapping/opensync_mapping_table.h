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
 
#ifndef OPENSYNC_MAPPING_TABLE_H_
#define OPENSYNC_MAPPING_TABLE_H_

OSyncMappingTable *osync_mapping_table_new(OSyncError **error);
void osync_mapping_table_ref(OSyncMappingTable *table);
void osync_mapping_table_unref(OSyncMappingTable *table);

osync_bool osync_mapping_table_load(OSyncMappingTable *table, OSyncArchive *archive, OSyncError **error);
void osync_mapping_table_close(OSyncMappingTable *table);

void osync_mapping_table_add_view(OSyncMappingTable *table, OSyncMappingView *view);

OSyncMapping *osync_mapping_table_find_mapping(OSyncMappingTable *table, long long int id);
void osync_mapping_table_add_mapping(OSyncMappingTable *table, OSyncMapping *mapping);
void osync_mapping_table_remove_mapping(OSyncMappingTable *table, OSyncMapping *mapping);
int osync_mapping_table_num_mappings(OSyncMappingTable *table);
OSyncMapping *osync_mapping_table_nth_mapping(OSyncMappingTable *table, int nth);

long long int osync_mapping_table_get_next_id(OSyncMappingTable *table);

#endif /*OPENSYNC_MAPPING_TABLE_H_*/
