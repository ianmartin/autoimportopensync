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

void osync_db_trace(void *data, const char *query)
{
	osync_debug("OSDB", 4, "query executed: %s", query);
}

OSyncDB *osync_db_open(char *filename, OSyncError **error)
{
	OSyncDB *db = g_malloc0(sizeof(OSyncDB));
	int rc;
	rc = sqlite3_open(filename, &(db->db));
	if (rc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot open database: %s", sqlite3_errmsg(db->db));
		sqlite3_close(db->db);
		return NULL;
	}
	sqlite3_trace(db->db, osync_db_trace, NULL);
	return db;
}

void osync_db_close(OSyncDB *db)
{
	int ret = sqlite3_close(db->db);
	if (ret)
		osync_debug("OSDB", 1, "Can't close database: %s", sqlite3_errmsg(db->db));
}

int osync_db_count(OSyncDB *db, char *query)
{
	int ret = 0;

	sqlite3_stmt *ppStmt = NULL;
	if (sqlite3_prepare(db->db, query, -1, &ppStmt, NULL) != SQLITE_OK)
		osync_debug("OSDB", 3, "Unable prepare count! %s", sqlite3_errmsg(db->db));
	if (sqlite3_step(ppStmt) != SQLITE_OK)
		osync_debug("OSDB", 3, "Unable step count! %s", sqlite3_errmsg(db->db));
	ret = sqlite3_column_int64(ppStmt, 0);
	sqlite3_finalize(ppStmt);
	return ret;
}

void osync_db_save_change(OSyncMappingTable *table, OSyncChange *change)
{
	g_assert(table->entrytable);
	sqlite3 *sdb = table->entrytable->db;
	long long int mappingid = 0;
	
	if (change->mapping) {
		if (!change->mapping->id) {
			sqlite3_stmt *ppStmt = NULL;
			if (sqlite3_prepare(sdb, "SELECT max(mappingid) FROM tbl_changes", -1, &ppStmt, NULL) != SQLITE_OK)
				osync_debug("OSDB", 3, "Unable prepare max! %s", sqlite3_errmsg(sdb));
			if (sqlite3_step(ppStmt) != SQLITE_OK)
				osync_debug("OSDB", 3, "Unable step max! %s", sqlite3_errmsg(sdb));
			change->mapping->id = sqlite3_column_int64(ppStmt, 0) + 1;
			sqlite3_finalize(ppStmt);
		}
		mappingid = change->mapping->id;
	}
	
	char *query = NULL;
	if (!change->id) {
		query = g_strdup_printf("INSERT INTO tbl_changes (uid, objtype, format, memberid, mappingid) VALUES('%s', '%s', '%s', '%lli', '%lli')", change->uid, change->objtype->name, osync_objformat_get_name(osync_change_get_objformat(change)), change->member->id, mappingid);
		if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK) {
			osync_debug("OSDB", 1, "Unable to insert change! %s", sqlite3_errmsg(sdb));
			g_free(query);
			return;
		}
		change->id = sqlite3_last_insert_rowid(sdb);
	} else {
		query = g_strdup_printf("UPDATE tbl_changes SET uid='%s', objtype='%s', format='%s', memberid='%lli', mappingid='%lli' WHERE id=%lli", change->uid, change->objtype->name, osync_change_get_objformat(change) ? osync_objformat_get_name(osync_change_get_objformat(change)) : NULL, change->member->id, mappingid, change->id);
		if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK)
			osync_debug("OSDB", 1, "Unable to update change! %s", sqlite3_errmsg(sdb));
	}
	g_free(query);
}


void osync_db_delete_change(OSyncMappingTable *table, OSyncChange *change)
{
	g_assert(table->entrytable);
	sqlite3 *sdb = table->entrytable->db;
	char *query = g_strdup_printf("DELETE FROM tbl_changes WHERE id=%lli", change->id);
	if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 1, "Unable to delete change! %s", sqlite3_errmsg(sdb));
	g_free(query);
}

osync_bool osync_db_open_mappingtable(OSyncMappingTable *table, OSyncError **error)
{
	g_assert(table != NULL);
	g_assert(table->db_path != NULL);
	g_assert(table->group);
	OSyncMapping *mapping = NULL;

	char *filename = g_strdup_printf("%s/change.db", table->db_path);
	if (!(table->entrytable = osync_db_open(filename, error))) {
		osync_error_update(error, "Unable to open mappingtable: %s", (*error)->message);
		return FALSE;
	}
	osync_debug("OSDB", 3, "Preparing to load changes from file %s", filename);
	g_assert(table->entrytable);
	g_free(filename);
	
	sqlite3 *sdb = table->entrytable->db;
	
	if (sqlite3_exec(sdb, "CREATE TABLE tbl_changes (id INTEGER PRIMARY KEY, uid VARCHAR, objtype VARCHAR, format VARCHAR, memberid INTEGER, mappingid INTEGER)", NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 2, "Unable create mapping table! %s", sqlite3_errmsg(sdb));
	
	sqlite3_stmt *ppStmt = NULL;
	sqlite3_prepare(sdb, "SELECT id, uid, objtype, format, memberid, mappingid FROM tbl_changes ORDER BY mappingid", -1, &ppStmt, NULL);
	while (sqlite3_step(ppStmt) == SQLITE_ROW) {
		OSyncChange *change = osync_change_new();
		change->id = sqlite3_column_int64(ppStmt, 0);
		change->uid = g_strdup(sqlite3_column_text(ppStmt, 1));
		char *objtype = g_strdup(sqlite3_column_text(ppStmt, 2));
		char *objformat = g_strdup(sqlite3_column_text(ppStmt, 3));
		long long int memberid = sqlite3_column_int64(ppStmt, 4);
		long long int mappingid = sqlite3_column_int64(ppStmt, 5);
    	if (table && table->group) {
    		change->member = osync_member_from_id(table->group, memberid);
			osync_member_add_changeentry(change->member, change);
			if (objtype) {
				change->objtype = osync_conv_find_objtype(table->group->conv_env, objtype);
				if (!change->objtype) {
					osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open mappingtable: Unable to find the format-plugin which has the object-type \"%s\"", objtype);
					return FALSE;
				}
			}
			
			if (objformat) {
				if (!osync_change_set_objformat_string(change, objformat)) {
					osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open mappingtable: Unable to find the format-plugin which has the format \"%s\"", objformat);
					return FALSE;
				}
			}
		}
		
		if (!mappingid) {
    		osync_mappingtable_add_unmapped(table, change);
    	} else {
    		if (!mapping || mapping->id != mappingid) {
				mapping = osync_mapping_new(table);
				mapping->id = mappingid;
    		}
    		osync_mapping_add_entry(mapping, change);
    	}
	}
	sqlite3_finalize(ppStmt);
	return TRUE;
}

void osync_db_close_mappingtable(OSyncMappingTable *table)
{
	osync_db_close(table->entrytable);
}

void osync_db_reset_mappingtable(OSyncMappingTable *table, const char *objtype)
{
	sqlite3 *sdb = table->entrytable->db;
	char *query = NULL;
	if (osync_conv_objtype_is_any(objtype)) {
		query = g_strdup_printf("DELETE FROM tbl_changes");
	} else {
		query = g_strdup_printf("DELETE FROM tbl_changes WHERE objtype='%s'", objtype);
	}
	if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 1, "Unable to reset mappingtable! %s", sqlite3_errmsg(sdb));
	g_free(query);
}

OSyncDB *osync_db_open_anchor(OSyncMember *member, OSyncError **error)
{
	g_assert(member);
	OSyncDB *sdb = NULL;
	char *filename = g_strdup_printf ("%s/anchor.db", member->configdir);
	if (!(sdb = osync_db_open(filename, error))) {
		osync_error_update(error, "Unable to open anchor table: %s", (*error)->message);
		return NULL;
	}
	g_free(filename);
	
	if (sqlite3_exec(sdb->db, "CREATE TABLE tbl_anchor (id INTEGER PRIMARY KEY, anchor VARCHAR, objtype VARCHAR UNIQUE)", NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 3, "Unable create anchor table! %s", sqlite3_errmsg(sdb->db));
	
	return sdb;
}

void osync_db_close_anchor(OSyncDB *db)
{
	osync_db_close(db);
}

void osync_db_get_anchor(OSyncDB *sdb, const char *objtype, char **retanchor)
{
	sqlite3_stmt *ppStmt = NULL;
	char *query = g_strdup_printf("SELECT anchor FROM tbl_anchor WHERE objtype='%s'", objtype);
	if (sqlite3_prepare(sdb->db, query, -1, &ppStmt, NULL) != SQLITE_OK)
		osync_debug("OSDB", 3, "Unable prepare anchor! %s", sqlite3_errmsg(sdb->db));
	sqlite3_step(ppStmt);
	*retanchor = g_strdup(sqlite3_column_text(ppStmt, 0));
	sqlite3_finalize(ppStmt);
	g_free(query);
}

void osync_db_put_anchor(OSyncDB *sdb, const char *objtype, const char *anchor)
{
	char *query = g_strdup_printf("REPLACE INTO tbl_anchor (objtype, anchor) VALUES('%s', '%s')", objtype, anchor);
	if (sqlite3_exec(sdb->db, query, NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 1, "Unable put anchor! %s", sqlite3_errmsg(sdb->db));

	g_free(query);
}


osync_bool osync_db_open_hashtable(OSyncHashTable *table, OSyncMember *member, OSyncError **error)
{
	g_assert(member);
	char *filename = g_strdup_printf ("%s/hash.db", member->configdir);
	if (!(table->dbhandle = osync_db_open(filename, error))) {
		osync_error_update(error, "Unable to open hashtable: %s", (*error)->message);
		return FALSE;
	}
	g_free(filename);
	
	sqlite3 *db = table->dbhandle->db;
	
	if (sqlite3_exec(db, "CREATE TABLE tbl_hash (id INTEGER PRIMARY KEY, uid VARCHAR UNIQUE, hash VARCHAR, objtype VARCHAR)", NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 3, "Unable create hash table! %s", sqlite3_errmsg(db));
	
	return TRUE; //FIXME
}

void osync_db_close_hashtable(OSyncHashTable *table)
{
	osync_db_close(table->dbhandle);
}

void osync_db_save_hash(OSyncHashTable *table, char *uid, char *hash, char *objtype)
{
	g_assert(table->dbhandle);
	sqlite3 *sdb = table->dbhandle->db;
	
	char *query = g_strdup_printf("REPLACE INTO tbl_hash ('uid', 'hash', 'objtype') VALUES('%s', '%s', '%s')", uid, hash, objtype);
	if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 1, "Unable to insert hash! %s", sqlite3_errmsg(sdb));
	g_free(query);
}


void osync_db_delete_hash(OSyncHashTable *table, char *uid)
{
	g_assert(table->dbhandle);
	sqlite3 *sdb = table->dbhandle->db;
	
	char *query = g_strdup_printf("DELETE FROM tbl_hash WHERE uid='%s'", uid);
	if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 1, "Unable to delete hash! %s", sqlite3_errmsg(sdb));
	g_free(query);
}

void osync_db_report_hash(OSyncHashTable *table, OSyncContext *ctx, const char *objtype)
{
	g_assert(table->dbhandle);
	sqlite3 *sdb = table->dbhandle->db;
	
	char **azResult = NULL;
	int numrows = 0;
	char *query = NULL;
	if (osync_conv_objtype_is_any(objtype)) {
		query = g_strdup_printf("SELECT uid, hash FROM tbl_hash");
	} else {
		query = g_strdup_printf("SELECT uid, hash FROM tbl_hash WHERE objtype='%s'", objtype);
	}
	sqlite3_get_table(sdb, query, &azResult, &numrows, NULL, NULL);
	g_free(query);
	
	int i;
	int ccell = 2;
	for (i = 0; i < numrows; i++) {
		char *uid = azResult[ccell];
		ccell++;
		char *hash = azResult[ccell];
		ccell++;
		/*FIXME: should we reset used_entries somewhere? */
		if (!g_hash_table_lookup(table->used_entries, uid)) {
			OSyncChange *change = osync_change_new();
			change->changetype = CHANGE_DELETED;
			osync_change_set_hash(change, hash);
			osync_change_set_uid(change, uid);
			osync_context_report_change(ctx, change);
			osync_hashtable_update_hash(table, change);
		}
	}
	sqlite3_free_table(azResult);
}	

void osync_db_get_hash(OSyncHashTable *table, char *uid, char **rethash)
{
	sqlite3 *sdb = table->dbhandle->db;
	sqlite3_stmt *ppStmt = NULL;
	char *query = g_strdup_printf("SELECT hash FROM tbl_hash WHERE uid='%s'", uid);
	if (sqlite3_prepare(sdb, query, -1, &ppStmt, NULL) != SQLITE_OK)
		osync_debug("OSDB", 3, "Unable prepare get hash! %s", sqlite3_errmsg(sdb));
	if (sqlite3_step(ppStmt) != SQLITE_OK)
		osync_debug("OSDB", 3, "Unable step get hash! %s", sqlite3_errmsg(sdb));
	*rethash = g_strdup(sqlite3_column_text(ppStmt, 0));
	sqlite3_finalize(ppStmt);
	g_free(query);
}

void osync_db_reset_hash(OSyncHashTable *table, const char *objtype)
{
	sqlite3 *sdb = table->dbhandle->db;
	char *query = NULL;
	if (osync_conv_objtype_is_any(objtype)) {
		query = g_strdup_printf("DELETE FROM tbl_hash");
	} else {
		query = g_strdup_printf("DELETE FROM tbl_hash WHERE objtype='%s'", objtype);
	}
	if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 1, "Unable to reset hash! %s", sqlite3_errmsg(sdb));
	g_free(query);
}
