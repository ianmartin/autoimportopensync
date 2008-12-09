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
#include "opensync-archive.h"
#include "opensync-support.h"

#include "opensync_mapping_table_internals.h"

#include "archive/opensync_archive_internals.h"

/**
 * @brief Creates a new mapping table object
 * @param error Pointer to an error struct
 * @return The pointer to the newly allocated mapping table object or NULL in case of error
 */

OSyncMappingTable *osync_mapping_table_new(OSyncError **error)
{
        OSyncMappingTable *table = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	table = osync_try_malloc0(sizeof(OSyncMappingTable), error);
	if (!table)
		goto error;
	table->ref_count = 1;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, table);
	return table;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/**
 * @brief Increments the reference counter
 * @param archive The pointer to an mapping table object
 */
OSyncMappingTable *osync_mapping_table_ref(OSyncMappingTable *table)
{
	osync_assert(table);
	
	g_atomic_int_inc(&(table->ref_count));

	return table;
}

/**
 * @brief Decrement the reference counter. The mapping table object will 
 *  be freed if there is no more reference to it.
 * @param archive The pointer to an mapping table object
 */
void osync_mapping_table_unref(OSyncMappingTable *table)
{
	osync_assert(table);
		
	if (g_atomic_int_dec_and_test(&(table->ref_count))) {
		osync_trace(TRACE_ENTRY, "%s(%p)", __func__, table);

		osync_mapping_table_close(table);
		
		g_free(table);
		osync_trace(TRACE_EXIT, "%s", __func__);
	}
}

/**
 * @brief Loads all mappings from archive for a certain object type.
 *
 * @param table The mapping table object
 * @param archive The archive
 * @param objtype Requested object type 
 * @param error Pointer to an error struct
 * @return TRUE on when all mappings successfully loaded otherwise FALSE
 */ 
osync_bool osync_mapping_table_load(OSyncMappingTable *table, OSyncArchive *archive, const char *objtype, OSyncError **error)
{
	OSyncMappingEntry *entry = NULL;
	OSyncMapping *mapping = NULL;
	OSyncList *uids = NULL;
	OSyncList *ids = NULL;
	OSyncList *mappings = NULL;
	OSyncList *memberids = NULL;
	OSyncList *d = NULL, *u = NULL, *m = NULL, *i = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, %p)", __func__, table, archive, objtype, error);
	
	if (!osync_archive_load_changes(archive, objtype, &ids, &uids, &mappings, &memberids, error))
		goto error;
	
	d = ids;
	m = mappings;
	i = memberids;
	
	for (u = uids; u; u = u->next) {
		long long int id = (long long int)GPOINTER_TO_INT(d->data);
		char *uid = u->data;
		long long int memberid = (long long int)GPOINTER_TO_INT(i->data);
		long long int mappingid = (long long int)GPOINTER_TO_INT(m->data);
		
		entry = osync_mapping_entry_new(error);
		if (!entry)
			goto error_free;
		
		osync_mapping_entry_set_uid(entry, uid);
		g_free(uid);
		osync_mapping_entry_set_id(entry, id);
		osync_mapping_entry_set_member_id(entry, memberid);
		
		if (!mapping || osync_mapping_get_id(mapping) != mappingid) {
			mapping = osync_mapping_new(error);
			if (!error)
				goto error_free_entry;
			
			osync_mapping_set_id(mapping, mappingid);
			osync_mapping_table_add_mapping(table, mapping);
			osync_mapping_unref(mapping);
		}
		osync_mapping_add_entry(mapping, entry);
		osync_mapping_entry_unref(entry);
		
		m = m->next;
		d = d->next;
		i = i->next;
	}
	
	osync_list_free(ids);
	osync_list_free(uids);
	osync_list_free(mappings);
	osync_list_free(memberids);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_entry:
	osync_mapping_entry_unref(entry);
error_free:
	osync_list_free(ids);
	osync_list_free(uids);
	osync_list_free(mappings);
	osync_list_free(memberids);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/**
 * @brief Delete all mappings from the mapping table and the archive for a certain object type.
 *
 * @param table The mapping table object
 * @param archive The archive
 * @param objtype Reported object type of entry
 * @param error Pointer to an error struct
 * @return Returns TRUE on success, FALSE otherwise 
 */ 
osync_bool osync_mapping_table_flush(OSyncMappingTable *table, OSyncArchive *archive, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, %p)", __func__, table, archive, objtype, error);
	
	osync_mapping_table_close(table);

	if (!osync_archive_flush_changes(archive, objtype, error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/**
 * @brief Close the mapping table 
 *
 * @param table The mapping table object
 */ 
void osync_mapping_table_close(OSyncMappingTable *table)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, table);

	osync_assert(table);
       	
       	while (table->mappings) {
       		OSyncMapping *mapping = table->mappings->data;
       		osync_mapping_unref(mapping);
       		table->mappings = g_list_remove(table->mappings, mapping);
       	}

	osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * @brief Search for the mapping object with the mapping id 
 *
 * @param table The mapping table object
 * @param long long int The mapping id to search for
 * @return Returns Mapping object or NULL if no mapping matched the mapping id
 */ 
OSyncMapping *osync_mapping_table_find_mapping(OSyncMappingTable *table, long long int id)
{
	GList *m;
	osync_assert(table);
	
	for (m = table->mappings; m; m = m->next) {
		OSyncMapping *mapping = m->data;
		if (osync_mapping_get_id(mapping) == id)
			return mapping;
	}
	return NULL;
}

/**
 * @brief Add mapping to the mapping table 
 *
 * @param table The mapping table object
 * @param mapping The mapping to add to the mapping table
 */ 
void osync_mapping_table_add_mapping(OSyncMappingTable *table, OSyncMapping *mapping)
{
	osync_assert(table);
	osync_assert(mapping);
	
	table->mappings = g_list_append(table->mappings, mapping);
	osync_mapping_ref(mapping);
}

/**
 * @brief Remove mapping to the mapping table 
 *
 * @param table The mapping table object
 * @param mapping The mapping to remove to the mapping table
 */ 
void osync_mapping_table_remove_mapping(OSyncMappingTable *table, OSyncMapping *mapping)
{
	osync_assert(table);
	osync_assert(mapping);
	
	table->mappings = g_list_remove(table->mappings, mapping);
	osync_mapping_unref(mapping);
}

/**
 * @brief Number of mappings in the mapping table
 *
 * @param table The mapping table object to count mappings
 * @return Number of mappings
 */ 
int osync_mapping_table_num_mappings(OSyncMappingTable *table)
{
	osync_assert(table);
	return g_list_length(table->mappings);
}

/**
 * @brief Get nth mapping object from mapping table 
 *
 * @param table The mapping table object to count mappings
 * @param nth The position of the mapping object
 * @return The nth mapping object from mapping table or NULL if nth position isn't available 
 */ 
OSyncMapping *osync_mapping_table_nth_mapping(OSyncMappingTable *table, int nth)
{
	osync_assert(table);
	return g_list_nth_data(table->mappings, nth);
}

/**
 * @brief Get next free mapping id from mapping table 
 *
 * @param table The mapping table object
 * @return Next free mapping id of mapping table
 */ 
long long int osync_mapping_table_get_next_id(OSyncMappingTable *table)
{
	long long int new_id = 1;
	GList *m;
	for (m = table->mappings; m; m = m->next) {
		OSyncMapping *mapping = m->data;
		if (new_id <= osync_mapping_get_id(mapping))
			new_id = osync_mapping_get_id(mapping) + 1;
	}
	return new_id;
}

