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

static void osync_hashtable_assert_loaded(OSyncHashTable *table)
{
	osync_assert(table, "You have to pass a valid hashtable to the call!");
	osync_assert(table->dbhandle, "Hashtable not loaded yet. You have to load the hashtable first using osync_hashtable_load!");
}

OSyncHashTable *osync_hashtable_new(void)
{
	OSyncHashTable *table = g_malloc0(sizeof(OSyncHashTable));
	g_assert(table);
	table->used_entries = g_hash_table_new(g_str_hash, g_str_equal);
	return table;
}

void osync_hashtable_free(OSyncHashTable *table)
{
	g_hash_table_destroy(table->used_entries);
	g_free(table);
}

void osync_hashtable_forget(OSyncHashTable *table)
{
	g_hash_table_destroy(table->used_entries);
	table->used_entries = g_hash_table_new(g_str_hash, g_str_equal);
}

osync_bool osync_hashtable_load(OSyncHashTable *table, OSyncMember *member, OSyncError **error)
{
	return osync_db_open_hashtable(table, member, error);
}

void osync_hashtable_close(OSyncHashTable *table)
{
	osync_hashtable_assert_loaded(table);
	
	osync_hashtable_forget(table);
	osync_db_close(table->dbhandle);
}


int osync_hashtable_num_entries(OSyncHashTable *table)
{
	osync_hashtable_assert_loaded(table);
	
	return osync_db_count(table->dbhandle, "SELECT count(*) FROM tbl_hash");
}

//FIXME!!!
osync_bool osync_hashtable_nth_entry(OSyncHashTable *table, int i, char **uid, char **hash)
{
	osync_hashtable_assert_loaded(table);
	
	sqlite3 *sdb = table->dbhandle->db;
	
	sqlite3_stmt *ppStmt = NULL;
	char *query = g_strdup_printf("SELECT uid, hash FROM tbl_hash LIMIT 1 OFFSET %i", i);
	sqlite3_prepare(sdb, query, -1, &ppStmt, NULL);
	sqlite3_step(ppStmt);
	*uid = g_strdup((gchar*)sqlite3_column_text(ppStmt, 0));
	*hash = g_strdup((gchar*)sqlite3_column_text(ppStmt, 1));
	sqlite3_finalize(ppStmt);
	g_free(query);
	return TRUE;
}

void osync_hashtable_update_hash(OSyncHashTable *table, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, table, change);
	osync_hashtable_assert_loaded(table);
	osync_assert(change, "Change was NULL. Bug in a plugin");
	osync_assert(change->uid, "No uid was set on change. Bug in a plugin");

	osync_trace(TRACE_INTERNAL, "Updating hashtable with hash \"%s\" and changetype %i", change->hash, osync_change_get_changetype(change));
	switch (osync_change_get_changetype(change)) {
		case CHANGE_MODIFIED:
		case CHANGE_ADDED:
			osync_db_save_hash(table, change->uid, change->hash, osync_change_get_objtype(change) ? osync_change_get_objtype(change)->name : NULL);
			break;
		case CHANGE_DELETED:
			osync_db_delete_hash(table, change->uid);
			break;
		default:
			g_assert_not_reached();
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_hashtable_report_deleted(OSyncHashTable *table, OSyncContext *context, const char *objtype)
{
	osync_hashtable_assert_loaded(table);
	
	osync_db_report_hash(table, context, objtype);
}

osync_bool osync_hashtable_detect_change(OSyncHashTable *table, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, table, change);
	osync_hashtable_assert_loaded(table);
	osync_bool retval = FALSE;

	char *hash = NULL;
	osync_db_get_hash(table, change->uid, &hash);
	osync_trace(TRACE_INTERNAL, "Comparing %s with %s", hash, change->hash);
	
	if (hash) {
		if (strcmp(hash, change->hash) == 0) {
			change->changetype = CHANGE_UNMODIFIED;
			retval = FALSE;
		} else {
			change->changetype = CHANGE_MODIFIED;
			retval = TRUE;
		}
	} else {
		change->changetype = CHANGE_ADDED;
		retval = TRUE;
	}

	g_hash_table_insert(table->used_entries, change->uid, (void *)1);
	osync_trace(TRACE_EXIT, "%s: %s", __func__, retval ? "TRUE" : "FALSE");
	return retval;
}

void osync_hashtable_set_slow_sync(OSyncHashTable *table, const char *objtype)
{
	osync_hashtable_assert_loaded(table);
	
	osync_db_reset_hash(table, objtype);
}
