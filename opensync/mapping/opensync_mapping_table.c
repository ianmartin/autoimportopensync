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
#include "opensync-archive.h"
#include "opensync-support.h"

#include "opensync_mapping_table_internals.h"

OSyncMappingTable *osync_mapping_table_new(OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	OSyncMappingTable *table = osync_try_malloc0(sizeof(OSyncMappingTable), error);
	if (!table)
		goto error;
	table->ref_count = 1;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, table);
	return table;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void osync_mapping_table_ref(OSyncMappingTable *table)
{
	osync_assert(table);
	
	g_atomic_int_inc(&(table->ref_count));
}

void osync_mapping_table_unref(OSyncMappingTable *table)
{
	osync_assert(table);
		
	if (g_atomic_int_dec_and_test(&(table->ref_count))) {
		osync_trace(TRACE_ENTRY, "%s(%p)", __func__, table);
		/*while (table->views) {
			OSyncMappingView *view = table->views->data;
			osync_mapping_view_unref(view);
			table->views = g_list_remove(table->views, view);
		}*/
		
		while (table->mappings) {
			OSyncMapping *mapping = table->mappings->data;
			osync_mapping_unref(mapping);
			table->mappings = g_list_remove(table->mappings, mapping);
		}
		
		g_free(table);
		osync_trace(TRACE_EXIT, "%s", __func__);
	}
}

osync_bool osync_mapping_table_load(OSyncMappingTable *table, OSyncArchive *archive, const char *objtype, OSyncError **error)
{
	OSyncMappingEntry *entry = NULL;
	OSyncMapping *mapping = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, %p)", __func__, table, archive, objtype, error);
	
	OSyncList *uids = NULL;
	OSyncList *ids = NULL;
	OSyncList *mappings = NULL;
	OSyncList *memberids = NULL;

	if (!osync_archive_load_changes(archive, objtype, &ids, &uids, &mappings, &memberids, error))
		goto error;
	
	OSyncList *d = ids;
	OSyncList *u = NULL;
	OSyncList *m = mappings;
	OSyncList *i = memberids;
	
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

#if 0
osync_bool osync_mapping_table_load(OSyncMappingTable *table, const char *path, OSyncError **error)
{
	OSyncMappingEntry *entry = NULL;
	OSyncMapping *mapping = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, table, path, error);
	
	int rc = sqlite3_open(path, &(table->db));
	if (rc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot open database: %s", sqlite3_errmsg(table->db));
		goto error;
	}
	
	if (sqlite3_exec(table->db, "CREATE TABLE tbl_changes (id INTEGER PRIMARY KEY, uid VARCHAR, objtype VARCHAR, format VARCHAR, memberid INTEGER, mappingid INTEGER)", NULL, NULL, NULL) != SQLITE_OK)
		osync_trace(TRACE_INTERNAL, "Unable create changes table! %s", sqlite3_errmsg(table->db));
	
	sqlite3_stmt *ppStmt = NULL;
	sqlite3_prepare(table->db, "SELECT uid, memberid, mappingid FROM tbl_changes ORDER BY mappingid", -1, &ppStmt, NULL);

	while (sqlite3_step(ppStmt) == SQLITE_ROW) {
		entry = osync_mapping_entry_new(error);
		if (!entry)
			goto error_close;
		
		osync_mapping_entry_set_uid(entry, (const char *)sqlite3_column_text(ppStmt, 0));
		long long int mappingid = sqlite3_column_int64(ppStmt, 1);
		
		if (!mapping || osync_mapping_get_id(mapping) != mappingid) {
			mapping = osync_mapping_new(error);
			if (!error)
				goto error_free_entry;
			
			osync_mapping_set_id(mapping, mappingid);
			osync_mapping_table_add_mapping(table, mapping);
		}
		osync_mapping_add_entry(mapping, entry);
    	
    	/*OSyncMappingView *view = osengine_mappingtable_find_view(table, osync_change_get_member(change));
    	if (view)
    		osengine_mappingview_add_entry(view, entry);*/
		
		long long int memberid = sqlite3_column_int64(ppStmt, 2);
		osync_mapping_entry_set_member_id(entry, memberid);
		
    	osync_trace(TRACE_INTERNAL, "Loaded change with uid %s, mappingid %lli from member %lli", osync_mapping_entry_get_uid(entry), mappingid, memberid);
	}
	sqlite3_finalize(ppStmt);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_entry:
	osync_mapping_entry_unref(entry);
error_close:
	sqlite3_finalize(ppStmt);
	sqlite3_close(table->db);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}
#endif

void osync_mapping_table_close(OSyncMappingTable *table)
{
	
}

/*void osync_mapping_table_add_view(OSyncMappingTable *table, OSyncMappingView *view)
{
	osync_assert(table);
	osync_assert(view);
	
	table->views = g_list_append(table->views, view);
	osync_mapping_view_ref(view);
}*/

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

void osync_mapping_table_add_mapping(OSyncMappingTable *table, OSyncMapping *mapping)
{
	osync_assert(table);
	osync_assert(mapping);
	
	table->mappings = g_list_append(table->mappings, mapping);
	osync_mapping_ref(mapping);
}

void osync_mapping_table_remove_mapping(OSyncMappingTable *table, OSyncMapping *mapping)
{
	osync_assert(table);
	osync_assert(mapping);
	
	table->mappings = g_list_remove(table->mappings, mapping);
	osync_mapping_unref(mapping);
}

int osync_mapping_table_num_mappings(OSyncMappingTable *table)
{
	osync_assert(table);
	return g_list_length(table->mappings);
}

OSyncMapping *osync_mapping_table_nth_mapping(OSyncMappingTable *table, int nth)
{
	osync_assert(table);
	return g_list_nth_data(table->mappings, nth);
}

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
