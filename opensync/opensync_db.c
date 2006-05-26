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
 
#include "opensync.h"
#include "opensync_internals.h"

void osync_db_trace(void *data, const char *query)
{
	osync_trace(TRACE_INTERNAL, "query executed: %s", query);
}

OSyncDB *osync_db_open(char *filename, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, filename, error);
	
	OSyncDB *db = osync_try_malloc0(sizeof(OSyncDB), error);
	if (!db)
		goto error;
		
	int rc = sqlite3_open(filename, &(db->db));
	if (rc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot open database: %s", sqlite3_errmsg(db->db));
		goto error_free;
	}
	sqlite3_trace(db->db, osync_db_trace, NULL);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, db);
	return db;

error_free:
	g_free(db);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void osync_db_close(OSyncDB *db)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, db);
	
	int ret = sqlite3_close(db->db);
	if (ret)
		osync_trace(TRACE_INTERNAL, "Can't close database: %s", sqlite3_errmsg(db->db));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
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

OSyncDB *_open_changelog(OSyncGroup *group, OSyncError **error)
{
	g_assert(group);
	OSyncDB *log_db;

	char *filename = g_strdup_printf("%s/changelog.db", group->configdir);
	if (!(log_db = osync_db_open(filename, error))) {
		osync_error_update(error, "Unable to load changelog: %s", osync_error_print(error));
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	osync_debug("OSDB", 3, "Preparing to changelog from file %s", filename);
	g_free(filename);
	
	sqlite3 *sdb = log_db->db;
	
	if (sqlite3_exec(sdb, "CREATE TABLE tbl_log (uid VARCHAR, memberid INTEGER, changetype INTEGER)", NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 2, "Unable create log table! %s", sqlite3_errmsg(sdb));
	return log_db;
}

osync_bool osync_db_open_changelog(OSyncGroup *group, char ***uids, long long int **memberids, int **changetypes, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, group, uids, changetypes, error);
	
	OSyncDB *log_db = _open_changelog(group, error);
	if (!log_db) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	sqlite3 *sdb = log_db->db;
	
	int count = osync_db_count(log_db, "SELECT count(*) FROM tbl_log");

	*uids = g_malloc0(sizeof(char *) * (count + 1));
	*memberids = g_malloc0(sizeof(long long int) * (count + 1));
	*changetypes = g_malloc0(sizeof(int) * (count + 1));

	sqlite3_stmt *ppStmt = NULL;
	sqlite3_prepare(sdb, "SELECT uid, memberid, changetype FROM tbl_log", -1, &ppStmt, NULL);
	int i = 0;
	while (sqlite3_step(ppStmt) == SQLITE_ROW) {
		(*uids)[i] = g_strdup((gchar*)sqlite3_column_text(ppStmt, 0));
		(*memberids)[i] = sqlite3_column_int64(ppStmt, 1);
		(*changetypes)[i] = sqlite3_column_int(ppStmt, 2);
		i++;
	}
	(*uids)[i] = NULL;
	(*memberids)[i] = 0;
	(*changetypes)[i] = 0;
	sqlite3_finalize(ppStmt);

	char *query = g_strdup_printf("DELETE FROM tbl_log");
	if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to remove all logs! %s", sqlite3_errmsg(sdb));
		g_free(query);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	g_free(query);
	
	osync_db_close(log_db);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool osync_db_save_changelog(OSyncGroup *group, OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, group, change, error);
	
	OSyncDB *log_db = _open_changelog(group, error);
	if (!log_db) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	sqlite3 *sdb = log_db->db;
	
	char *query = g_strdup_printf("INSERT INTO tbl_log (uid, memberid, changetype) VALUES('%s', '%lli', '%i')", change->uid, change->member->id, change->changetype);
	if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to insert log! %s", sqlite3_errmsg(sdb));
		g_free(query);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	g_free(query);
	
	osync_db_close(log_db);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool osync_db_remove_changelog(OSyncGroup *group, OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, group, change, error);
	
	OSyncDB *log_db = _open_changelog(group, error);
	if (!log_db) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	sqlite3 *sdb = log_db->db;
	
	char *query = g_strdup_printf("DELETE FROM tbl_log WHERE uid='%s' AND memberid='%lli'", change->uid, change->member->id);
	if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to remove log! %s", sqlite3_errmsg(sdb));
		g_free(query);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	g_free(query);
	
	osync_db_close(log_db);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool osync_db_open_changes(OSyncGroup *group, OSyncChange ***changes, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, group, changes, error);
	g_assert(group);

	group->changes_path = g_strdup(group->configdir);
	char *filename = g_strdup_printf("%s/change.db", group->changes_path);
	if (!(group->changes_db = osync_db_open(filename, error))) {
		osync_error_update(error, "Unable to load changes: %s", osync_error_print(error));
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	osync_debug("OSDB", 3, "Preparing to load changes from file %s", filename);
	g_free(filename);
	
	sqlite3 *sdb = group->changes_db->db;
	
	if (sqlite3_exec(sdb, "CREATE TABLE tbl_changes (id INTEGER PRIMARY KEY, uid VARCHAR, objtype VARCHAR, format VARCHAR, memberid INTEGER, mappingid INTEGER)", NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 2, "Unable create changes table! %s", sqlite3_errmsg(sdb));
	
	int count = osync_db_count(group->changes_db, "SELECT count(*) FROM tbl_changes");
	*changes = g_malloc0(sizeof(OSyncChange *) * (count + 1));
	
	sqlite3_stmt *ppStmt = NULL;
	sqlite3_prepare(sdb, "SELECT id, uid, objtype, format, memberid, mappingid FROM tbl_changes ORDER BY mappingid", -1, &ppStmt, NULL);
	int i = 0;
	while (sqlite3_step(ppStmt) == SQLITE_ROW) {
		OSyncChange *change = osync_change_new();
		change->id = sqlite3_column_int64(ppStmt, 0);
		change->uid = g_strdup((gchar*)sqlite3_column_text(ppStmt, 1));
		change->objtype_name = g_strdup((gchar*)sqlite3_column_text(ppStmt, 2));
		change->format_name = g_strdup((gchar*)sqlite3_column_text(ppStmt, 3));
		change->initial_format_name = g_strdup(change->format_name);
		change->mappingid = sqlite3_column_int64(ppStmt, 5);
		long long int memberid = sqlite3_column_int64(ppStmt, 4);
		change->changes_db = group->changes_db;
    	osync_change_set_member(change, osync_member_from_id(group, memberid));
    		osync_trace(TRACE_INTERNAL, "Loaded change with uid %s, changetype %i, data %p, size %i, objtype %s and format %s from member %lli", osync_change_get_uid(change), osync_change_get_changetype(change), osync_change_get_data(change), osync_change_get_datasize(change), osync_change_get_objtype(change) ? osync_objtype_get_name(osync_change_get_objtype(change)) : "None", osync_change_get_objformat(change) ? osync_objformat_get_name(osync_change_get_objformat(change)) : "None", memberid);
		(*changes)[i] = change;
		i++;
	}
	(*changes)[i] = NULL;
	sqlite3_finalize(ppStmt);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool osync_db_save_change(OSyncChange *change, osync_bool save_format, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_db_save_change(%p, %i, %p) (Table: %p)", change, save_format, error, change->changes_db);
	osync_assert_msg(change, "Need to set change");
	osync_assert_msg(osync_change_get_objtype(change), "change->objtype was NULL while saving");
	osync_assert_msg(osync_change_get_objformat(change), "change->objformat was NULL while saving");
	
	if (!change || !change->changes_db) {
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "osync_db_save_change was called with wrong parameters");
		osync_trace(TRACE_EXIT_ERROR, "osync_db_save_change: %s", osync_error_print(error));
		return FALSE;
	}
	sqlite3 *sdb = change->changes_db->db;
	
	char *query = NULL;
	if (!change->id) {
		query = g_strdup_printf("INSERT INTO tbl_changes (uid, objtype, format, memberid, mappingid) VALUES('%s', '%s', '%s', '%lli', '%lli')", change->uid, osync_change_get_objtype(change)->name, osync_change_get_objformat(change)->name, change->member->id, change->mappingid);
		if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK) {
			osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to insert change! %s", sqlite3_errmsg(sdb));
			g_free(query);
			osync_trace(TRACE_EXIT_ERROR, "osync_db_save_change: %s", osync_error_print(error));
			return FALSE;
		}
		change->id = sqlite3_last_insert_rowid(sdb);
	} else {
		if (save_format)
			query = g_strdup_printf("UPDATE tbl_changes SET uid='%s', objtype='%s', format='%s', memberid='%lli', mappingid='%lli' WHERE id=%lli", change->uid, osync_change_get_objtype(change)->name, osync_change_get_objformat(change)->name, change->member->id, change->mappingid, change->id);
		else
			query = g_strdup_printf("UPDATE tbl_changes SET uid='%s', memberid='%lli', mappingid='%lli' WHERE id=%lli", change->uid, change->member->id, change->mappingid, change->id);
		if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK) {
			osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to update change! %s", sqlite3_errmsg(sdb));
			g_free(query);
			osync_trace(TRACE_EXIT_ERROR, "osync_db_save_change: %s", osync_error_print(error));
			return FALSE;
		}
	}
	g_free(query);
	osync_trace(TRACE_EXIT, "osync_db_save_change");
	return TRUE;
}

osync_bool osync_db_delete_change(OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_db_delete_change(%p, %p)", change, error);
	
	if (!change || !change->changes_db) {
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "osync_db_delete_change was called with wrong parameters");
		osync_trace(TRACE_EXIT_ERROR, "osync_db_delete_change: %s", osync_error_print(error));
		return FALSE;
	}
	sqlite3 *sdb = change->changes_db->db;
	
	char *query = g_strdup_printf("DELETE FROM tbl_changes WHERE id=%lli", change->id);
	if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to delete change! %s", sqlite3_errmsg(sdb));
		g_free(query);
		osync_trace(TRACE_EXIT_ERROR, "osync_db_delete_change: %s", osync_error_print(error));
		return FALSE;
	}
	g_free(query);
	osync_trace(TRACE_EXIT, "osync_db_delete_change");
	return TRUE;
}

osync_bool osync_db_reset_changes(OSyncGroup *group, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_db_reset_changes(%p, %s, %p)", group, objtype, error);
	
	if (!group || !objtype) {
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "osync_db_reset_changes was called with wrong parameters");
		osync_trace(TRACE_EXIT_ERROR, "osync_db_reset_changes: %s", osync_error_print(error));
		return FALSE;
	}
	sqlite3 *sdb = group->changes_db->db;

	char *query = NULL;
	if (osync_conv_objtype_is_any(objtype)) {
		query = g_strdup_printf("DELETE FROM tbl_changes");
	} else {
		query = g_strdup_printf("DELETE FROM tbl_changes WHERE objtype='%s'", objtype);
	}
	if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to reset changes! %s", sqlite3_errmsg(sdb));
		g_free(query);
		osync_trace(TRACE_EXIT_ERROR, "osync_db_reset_changes: %s", osync_error_print(error));
		return FALSE;
	}
	g_free(query);
	osync_trace(TRACE_EXIT, "osync_db_reset_changes");
	return TRUE;
}

osync_bool osync_db_reset_group(OSyncGroup *group, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_db_reset_group(%p, %p)", group, error);
	OSyncDB *db = NULL;
	
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "osync_db_reset_group was called with wrong parameters");
		osync_trace(TRACE_EXIT_ERROR, "osync_db_reset_group: %s", osync_error_print(error));
		return FALSE;
	}
	
	char *changesdb = g_strdup_printf("%s/change.db", group->configdir);
	if (!(db = osync_db_open(changesdb, error))) {
		g_free(changesdb);
		osync_trace(TRACE_EXIT_ERROR, "osync_db_reset_group: %s", osync_error_print(error));
		return FALSE;
	}

	if (sqlite3_exec(db->db, "DELETE FROM tbl_changes", NULL, NULL, NULL) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to reset changes! %s", sqlite3_errmsg(db->db));
		g_free(changesdb);
		osync_trace(TRACE_EXIT_ERROR, "osync_db_reset_group: %s", osync_error_print(error));
		return FALSE;
	}
	
	osync_db_close(db);
	g_free(changesdb);
	
	osync_trace(TRACE_EXIT, "osync_db_reset_member");
	return TRUE;
}

osync_bool osync_db_reset_member(OSyncMember *member, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_db_reset_member(%p, %p)", member, error);
	OSyncDB *db = NULL;
	
	if (!member) {
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "osync_db_reset_member was called with wrong parameters");
		osync_trace(TRACE_EXIT_ERROR, "osync_db_reset_member: %s", osync_error_print(error));
		return FALSE;
	}
	
	char *hashtable = g_strdup_printf("%s/hash.db", member->configdir);
	if (g_file_test(hashtable, G_FILE_TEST_EXISTS)) {
		if (!(db = osync_db_open(hashtable, error))) {
			g_free(hashtable);
			osync_trace(TRACE_EXIT_ERROR, "osync_db_reset_member: %s", osync_error_print(error));
			return FALSE;
		}
	
		if (sqlite3_exec(db->db, "DELETE FROM tbl_hash", NULL, NULL, NULL) != SQLITE_OK) {
			osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to reset hashtable! %s", sqlite3_errmsg(db->db));
			g_free(hashtable);
			osync_trace(TRACE_EXIT_ERROR, "osync_db_reset_changes: %s", osync_error_print(error));
			return FALSE;
		}
		
		osync_db_close(db);
	}
	g_free(hashtable);
	
	char *anchordb = g_strdup_printf("%s/anchor.db", member->configdir);
	if (g_file_test(anchordb, G_FILE_TEST_EXISTS)) {
		if (!(db = osync_db_open(anchordb, error))) {
			g_free(anchordb);
			osync_trace(TRACE_EXIT_ERROR, "osync_db_reset_member: %s", osync_error_print(error));
			return FALSE;
		}
	
		if (sqlite3_exec(db->db, "DELETE FROM tbl_anchor", NULL, NULL, NULL) != SQLITE_OK) {
			osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to reset anchors! %s", sqlite3_errmsg(db->db));
			g_free(anchordb);
			osync_trace(TRACE_EXIT_ERROR, "osync_db_reset_changes: %s", osync_error_print(error));
			return FALSE;
		}
		
		osync_db_close(db);
	}
	g_free(anchordb);
	
	osync_trace(TRACE_EXIT, "osync_db_reset_member");
	return TRUE;
}

void osync_db_close_changes(OSyncGroup *group)
{
	if (group->changes_db)
		osync_db_close(group->changes_db);
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
	*retanchor = g_strdup((gchar*)sqlite3_column_text(ppStmt, 0));
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

void osync_db_save_hash(OSyncHashTable *table, const char *uid, const char *hash, const char *objtype)
{
	g_assert(table->dbhandle);
	sqlite3 *sdb = table->dbhandle->db;
	
	char *query = g_strdup_printf("REPLACE INTO tbl_hash ('uid', 'hash', 'objtype') VALUES('%s', '%s', '%s')", uid, hash, objtype);
	if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 1, "Unable to insert hash! %s", sqlite3_errmsg(sdb));
	g_free(query);
}


void osync_db_delete_hash(OSyncHashTable *table, const char *uid)
{
	g_assert(table->dbhandle);
	sqlite3 *sdb = table->dbhandle->db;
	
	char *query = g_strdup_printf("DELETE FROM tbl_hash WHERE uid='%s'", uid);
	if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 1, "Unable to delete hash! %s", sqlite3_errmsg(sdb));
	g_free(query);
}

char **osync_db_get_deleted_hash(OSyncHashTable *table, const char *objtype)
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
	
	char **ret = g_malloc0((numrows + 1) * sizeof(char *));
	
	int i;
	int ccell = 2;
	int num = 0;
	for (i = 0; i < numrows; i++) {
		char *uid = azResult[ccell];
		ccell += 2;
		
		if (!g_hash_table_lookup(table->used_entries, uid)) {
			ret[num] = g_strdup(uid);
			num++;
		}
	}
	sqlite3_free_table(azResult);
	return ret;
}	

void osync_db_get_hash(OSyncHashTable *table, const char *uid, char **rethash)
{
	sqlite3 *sdb = table->dbhandle->db;
	sqlite3_stmt *ppStmt = NULL;
	char *query = g_strdup_printf("SELECT hash FROM tbl_hash WHERE uid='%s'", uid);
	if (sqlite3_prepare(sdb, query, -1, &ppStmt, NULL) != SQLITE_OK)
		osync_debug("OSDB", 3, "Unable prepare get hash! %s", sqlite3_errmsg(sdb));
	if (sqlite3_step(ppStmt) != SQLITE_OK)
		osync_debug("OSDB", 3, "Unable step get hash! %s", sqlite3_errmsg(sdb));
	*rethash = g_strdup((gchar*)sqlite3_column_text(ppStmt, 0));
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
