#include <opensync.h>
#include "opensync_internals.h"

OSyncDB *osync_db_open(char *filename)
{
	OSyncDB *db = g_malloc0(sizeof(OSyncDB));
	int rc;
	rc = sqlite3_open(filename, &(db->db));
	if (rc) {
		osync_debug("OSDB", 3, "Can't open database: %s", sqlite3_errmsg(db->db));
		sqlite3_close(db->db);
		return NULL;
	}
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
		query = g_strdup_printf("UPDATE tbl_changes SET uid='%s', objtype='%s', format='%s', memberid='%lli', mappingid='%lli' WHERE id=%lli", change->uid, change->objtype->name, osync_objformat_get_name(osync_change_get_objformat(change)), change->member->id, mappingid, change->id);
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

void osync_db_open_mappingtable(OSyncMappingTable *table)
{
	g_assert(table != NULL);
	g_assert(table->db_path != NULL);
	g_assert(table->group);
	OSyncMapping *mapping = NULL;

	char *filename = g_strdup_printf("%s/change.db", table->db_path);
	table->entrytable = osync_db_open(filename);
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
			if (objtype)
				change->objtype = osync_conv_find_objtype(table->group->conv_env, objtype);
			//FIXME: handle object type not found
			if (objformat)
				osync_change_set_objformat(change, osync_conv_find_objformat(table->group->conv_env, objformat));
			//FIXME: handle objformat not found
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
}

void osync_db_close_mappingtable(OSyncMappingTable *table)
{
	osync_db_close(table->entrytable);
}

OSyncDB *osync_db_open_anchor(OSyncMember *member)
{
	g_assert(member);
	char *filename = g_strdup_printf ("%s/anchor.db", member->configdir);
	OSyncDB *sdb = osync_db_open(filename);
	g_free(filename);
	
	if (sqlite3_exec(sdb->db, "CREATE TABLE tbl_anchor (id INTEGER PRIMARY KEY, anchor VARCHAR, objtype VARCHAR UNIQUE)", NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 3, "Unable create anchor table! %s", sqlite3_errmsg(sdb->db));
	
	return sdb;
}

void osync_db_close_anchor(OSyncDB *db)
{
	osync_db_close(db);
}

void osync_db_get_anchor(OSyncDB *sdb, char *objtype, char **retanchor)
{
	sqlite3_stmt *ppStmt = NULL;
	char *query = g_strdup_printf("SELECT anchor FROM tbl_anchor WHERE objtype='%s'", objtype);
	if (sqlite3_prepare(sdb->db, query, -1, &ppStmt, NULL) != SQLITE_OK)
		osync_debug("OSDB", 3, "Unable prepare anchor! %s", sqlite3_errmsg(sdb->db));
	if (sqlite3_step(ppStmt) != SQLITE_OK)
		osync_debug("OSDB", 3, "Unable step anchor! %s", sqlite3_errmsg(sdb->db));
	*retanchor = g_strdup(sqlite3_column_text(ppStmt, 0));
	sqlite3_finalize(ppStmt);
	g_free(query);
}

void osync_db_put_anchor(OSyncDB *sdb, char *objtype, char *anchor)
{
	char *query = g_strdup_printf("REPLACE INTO tbl_anchor (objtype, anchor) VALUES('%s', '%s')", objtype, anchor);
	if (sqlite3_exec(sdb->db, query, NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 1, "Unable put anchor! %s", sqlite3_errmsg(sdb->db));

	g_free(query);
}


osync_bool osync_db_open_hashtable(OSyncHashTable *table, OSyncMember *member)
{
	g_assert(member);
	char *filename = g_strdup_printf ("%s/hash.db", member->configdir);
	table->dbhandle = osync_db_open(filename);
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

void osync_db_report_hash(OSyncHashTable *table, OSyncContext *ctx)
{
	g_assert(table->dbhandle);
	sqlite3 *sdb = table->dbhandle->db;
	
	char **azResult;
	int numrows = 0;
	sqlite3_get_table(sdb, "SELECT uid, hash FROM tbl_hash", &azResult, &numrows, NULL, NULL);
	
	int i;
	int ccell = 2;
	for (i = 0; i < numrows; i++) {
		char *uid = azResult[ccell];
		ccell++;
		char *hash = azResult[ccell];
		ccell++;
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
	char *query = g_strdup_printf("SELECT hash FROM tbl_hash WHERE uid=\"%s\"", uid);
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
	char *query = g_strdup_printf("DELETE FROM tbl_hash WHERE objtype='%s'", objtype);
	if (sqlite3_exec(sdb, query, NULL, NULL, NULL) != SQLITE_OK)
		osync_debug("OSDB", 1, "Unable to reset hash! %s", sqlite3_errmsg(sdb));
	g_free(query);
}
