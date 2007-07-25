/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  Daniel Gollub <dgollub@suse.de>
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
#include "opensync_db.h"
#include "opensync_db_internals.h"

/*
static void _osync_db_trace(void *data, const char *query)
{
	osync_trace(TRACE_INTERNAL, "osync_db query executed: %s", query ? query: "nil");
}
*/

char *_osync_db_sql_escape(const char *s)
{
	return osync_strreplace(s, "'", "''");
}

OSyncDB *osync_db_new(OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);

	OSyncDB *db = osync_try_malloc0(sizeof(OSyncDB), error);
	if (!db) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
		return NULL;
	}

	osync_trace(TRACE_EXIT, "%s: %p", __func__, db);
	return db;
}

/**
 * @brief Open a database
 * 
 * @param db Pointer to database struct
 * @param dbfile Full file path to database file
 * @param error Pointer to a error struct 
 * @return If database opened successfully then TRUE else FALSE
 */
osync_bool osync_db_open(OSyncDB *db, const char *dbfile, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, dbfile ? dbfile : "nil", error);

	osync_assert(db);
	osync_assert(dbfile);

	if (sqlite3_open(dbfile, &(db->sqlite3db)) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot open database: %s", sqlite3_errmsg(db->sqlite3db));
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, sqlite3_errmsg(db->sqlite3db) ? sqlite3_errmsg(db->sqlite3db) : "nil");
		return FALSE;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/**
 * @brief Close a sqlite3 database file
 * 
 * @param db Pointer to database struct
 * @param error Pointer to a error struct 
 * @return If database closed successfully then TRUE else FALSE
 */
osync_bool osync_db_close(OSyncDB *db, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, db, error);

	osync_assert(db);
	
	int rc = sqlite3_close(db->sqlite3db);
	if (rc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot close database: %s", sqlite3_errmsg(db->sqlite3db));
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, sqlite3_errmsg(db->sqlite3db) ? sqlite3_errmsg(db->sqlite3db) : "nil");
		return FALSE;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/**
 * @brief Counts result of a database query 
 * 
 * @param db Pointer to database struct
 * @param query SQL database query 
 * @param error Pointer to a error struct 
 * @return Returns number of query result or -1 on error 
 */
int osync_db_count(OSyncDB *db, const char *query, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, query ? query : "nil", error);
	
	osync_assert(db);
	osync_assert(query);

	int num;
	char **result = NULL;
	char *errmsg = NULL;

	if (sqlite3_get_table(db->sqlite3db, query, &result, &num, NULL, &errmsg) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable count result of query: %s", errmsg);
		sqlite3_free_table(result);
		g_free(errmsg);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
		return -1;
	}

	sqlite3_free_table(result);
	osync_trace(TRACE_EXIT, "%s: %i", __func__, num);
	return num;
}

/**
 * @brief Executes a simple SQL query 
 * 
 * @param db Pointer to database struct
 * @param query SQL database query 
 * @param error Pointer to a error struct 
 * @return Returns TRUE on success otherwise FALSE 
 */
osync_bool osync_db_query(OSyncDB *db, const char *query, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, query ? query :"nil", error);
	
	osync_assert(db);
	osync_assert(query);

	char *errmsg = NULL;

	if (sqlite3_exec(db->sqlite3db, query, NULL, NULL, &errmsg) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to execute simple query: %s", errmsg);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, errmsg ? errmsg : "nil");
		g_free(errmsg);
		return FALSE;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/**
 * @brief Exectues a SQL query and fill all requested data in a GList. 
 *        Check error with osync_error_is_set().
 * 
 * @param db Pointer to database struct
 * @param query SQL database query 
 * @param error Pointer to a error struct 
 * @return Returns pointer to GList which contains the each result as another GList ptr. Freeing is recommend with osync_db_free_list() 
 */
GList *osync_db_query_table(OSyncDB *db, const char *query, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, query ? query : "nil", error);
	
	osync_assert(db);
	osync_assert(query);

	GList *table = NULL;
	int i, j, column_count = 0;
	int numrows = 0, numcolumns = 0;
	char **result = NULL;
	char *errmsg = NULL;

	if (sqlite3_get_table(db->sqlite3db, query, &result, &numrows, &numcolumns, &errmsg) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to query table: %s", errmsg);
		g_free(errmsg);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
		return NULL;
	}
	
	/* First row contains only names of columns. So we skip this!
	   We start reading the array with a offset (number of columns) */
	column_count = numcolumns;

	for (j=0; j < numrows; j++) {
		GList *row = NULL;
		for (i=0; i < numcolumns; i++)
			row = g_list_append(row, g_strdup(result[column_count++]));

		table = g_list_append(table, row);
	}

	sqlite3_free_table(result);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, table);
	return table; 
}

/**
 * @brief Frees the full result of osync_db_query_table().
 *
 * @param list Result GList pointer of osync_db_query_table()
 */
void osync_db_free_list(GList *list) {
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, list);

	GList *row;
	for (row = list; row; row = row->next) {
		g_list_foreach((GList *) row->data, (GFunc) g_free, NULL);
		g_list_free((GList *) row->data);
	}

	g_list_free(list);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * @brief Execute a SQL query for a single string (char *). Do only use this
 *        function when you expect a single return. The function will return
 *        NULL if there are more then one results! 
 *
 * @param db Pointer to database struct
 * @param query SQL database query 
 * @param error Pointer to a error struct 
 * @return Returns result string from database. NULL on more then one result or on error (the caller is responsible for freeing)
 */
char *osync_db_query_single_string(OSyncDB *db, const char *query, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, query ? query : "nil", error);

	osync_assert(db);
	osync_assert(query);
	
	char *result = NULL;
	sqlite3_stmt *ppStmt = NULL;

	if (sqlite3_prepare(db->sqlite3db, query, -1, &ppStmt, NULL) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Query Error: %s", sqlite3_errmsg(db->sqlite3db));
		goto error;
	}

	if (sqlite3_step(ppStmt) != SQLITE_ROW) {
		sqlite3_finalize(ppStmt);
		osync_trace(TRACE_EXIT, "%s: no result of query", __func__);
		return NULL;
	}
	
	result = g_strdup((const char *)sqlite3_column_text(ppStmt, 0));

	if (sqlite3_step(ppStmt) == SQLITE_ROW) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Returned more than one result! This function only handle a single string!");
		goto error;
	}
	
	sqlite3_finalize(ppStmt);
	
	osync_trace(TRACE_EXIT, "%s: %s", __func__, result ? result : "nil");
	return result; 
	
error:
	g_free(result);
	sqlite3_finalize(ppStmt);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return NULL;
}

/**
 * @brief Execute a SQL query for a single integer. Do only use this
 *        function when you expect a single return. The function will return
 *        NULL if there are more then one results! 
 *
 * @param db Pointer to database struct
 * @param query SQL database query 
 * @param error Pointer to a error struct 
 * @return Returns result integer from database.  On more then one result or on error -1 
 */
int osync_db_query_single_int(OSyncDB *db, const char *query, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, query ? query : "nil", error);

	osync_assert(db);
	osync_assert(query);
	
	int result = 0;
	sqlite3_stmt *ppStmt = NULL;

	if (sqlite3_prepare(db->sqlite3db, query, -1, &ppStmt, NULL) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Query Error: %s", sqlite3_errmsg(db->sqlite3db));
		goto error;
	}

	if (sqlite3_step(ppStmt) != SQLITE_ROW) {
		sqlite3_finalize(ppStmt);
		osync_trace(TRACE_EXIT, "%s: no result of query", __func__);
		return -1;
	}
	
	result = sqlite3_column_int(ppStmt, 0);

	if (sqlite3_step(ppStmt) == SQLITE_ROW) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Returned more than one result! This function only handle a single integer!");
		goto error;
	}
	
	sqlite3_finalize(ppStmt);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, result);
	return result; 
	
error:
	sqlite3_finalize(ppStmt);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return -1;
}

/**
 * @brief Full reset of a sqlite3 table. 
 *
 * @param db Pointer to database struct
 * @param tablename Name of table which gets reseted
 * @param error Pointer to a error struct 
 * @return TRUE on success otherwise FALSE
 */
osync_bool osync_db_reset(OSyncDB *db, const char *tablename, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, tablename ? tablename : "nil", error);

	osync_assert(db);
	osync_assert(tablename);
	
	char *query = g_strdup_printf("DELETE FROM %s", tablename);

	if (!osync_db_query(db, query, error))
		goto error;

	g_free(query);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:
	g_free(query);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");	
	return FALSE;
}

osync_bool osync_db_reset_full(OSyncDB *db, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, db, error);

	osync_assert(db);

	sqlite3_stmt *ppStmt = NULL;
	char *query = g_strdup("SELECT name FROM (SELECT * FROM sqlite_master) WHERE type='table'");

	if (sqlite3_prepare(db->sqlite3db, query, -1, &ppStmt, NULL) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Query Error: %s", sqlite3_errmsg(db->sqlite3db));
		goto error;
	}

	while (sqlite3_step(ppStmt) == SQLITE_ROW) {
		const char *table = (const char *) sqlite3_column_text(ppStmt, 0);
		if (!osync_db_reset(db, table, error))
			goto error;
	}

	sqlite3_finalize(ppStmt);

	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;

error:
	sqlite3_finalize(ppStmt);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil"); 
	return FALSE;
}

osync_bool osync_db_reset_full_by_path(const char *path, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, path ? path : "nil", error);

	osync_assert(path);

	OSyncDB *db = NULL;
	if (!osync_db_open(db, path, error))
		goto error;

	if (!osync_db_reset_full(db, error))
		goto error;

	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil"); 
	return FALSE;

}


/**
 * @brief Checks if requested table exists in database
 *
 * @param db Pointer to database struct
 * @param tablename Name of table which gets reseted
 * @param error Pointer to a error struct 
 * @return If the table exist 1 else 0. On error -1.
 */
int osync_db_exists(OSyncDB *db, const char *tablename, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, tablename ? tablename : "nil", error);

	osync_assert(db);
	osync_assert(tablename);
	
	sqlite3_stmt *ppStmt = NULL;

	char *query = g_strdup_printf("SELECT name FROM (SELECT * FROM sqlite_master UNION ALL SELECT * FROM sqlite_temp_master) WHERE type='table' AND name='%s'",
			tablename);

	if (sqlite3_prepare(db->sqlite3db, query, -1, &ppStmt, NULL) != SQLITE_OK) {
		sqlite3_finalize(ppStmt);
		g_free(query);

		osync_error_set(error, OSYNC_ERROR_GENERIC, "Query Error: %s", sqlite3_errmsg(db->sqlite3db));
		osync_trace(TRACE_EXIT_ERROR, "Database query error: %s", sqlite3_errmsg(db->sqlite3db) ? sqlite3_errmsg(db->sqlite3db) : "nil");
		return -1;
	}


	if (sqlite3_step(ppStmt) != SQLITE_ROW) {
		sqlite3_finalize(ppStmt);
		g_free(query);

		osync_trace(TRACE_EXIT, "%s: table \"%s\" doesn't exist.", __func__, tablename ? tablename : "nil");
		return 0;
	}

	sqlite3_finalize(ppStmt);
	g_free(query);
	
	osync_trace(TRACE_EXIT, "%s: table \"%s\" exists.", __func__, tablename ? tablename : "nil");
	return 1;
}


/**
 * @brief Insert a data (blob) in database. 
 *
 * @param db Pointer to database struct
 * @param query SQL query to bind the data blob
 * @param data Pointer to the data
 * @param size The size of the data which should be binded
 * @param error Pointer to a error struct 
 * @return TRUE on success otherwise FALSE
 */
osync_bool osync_db_bind_blob(OSyncDB *db, const char *query, const char *data, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s, %u, %p)", __func__, db, query ? query : "nil", data ? data : "nil", size, error);

	osync_assert(db);
	osync_assert(query);
	osync_assert(data);
	osync_assert(size);

	sqlite3_stmt *sqlite_stmt = NULL;

        int rc = sqlite3_prepare(db->sqlite3db, query, -1, &sqlite_stmt, NULL);
        if(rc != SQLITE_OK)
                goto error_msg;

	/* TODO Handle the case of multiple data blobs.... index = 1 will break with multiple blobs. */
	rc = sqlite3_bind_blob(sqlite_stmt, 1, data, size, SQLITE_TRANSIENT);

	if(rc != SQLITE_OK)
		goto error_msg;
	
	rc = sqlite3_step(sqlite_stmt);
	if (rc != SQLITE_DONE) {
		if(rc != SQLITE_ERROR)
			goto error_msg;
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to insert data! %s", sqlite3_errmsg(db->sqlite3db));
		goto error;
	}
	
	sqlite3_reset(sqlite_stmt);
	sqlite3_finalize(sqlite_stmt);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_msg:
	osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to insert data: %s", sqlite3_errmsg(db->sqlite3db));

error:
	if(sqlite_stmt) {
		sqlite3_reset(sqlite_stmt);
		sqlite3_finalize(sqlite_stmt);	
	}
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return FALSE;
}

/**
 * @brief Get a data (blob) from the database. 
 *
 * @param db Pointer to database struct
 * @param query SQL query to get the data blob
 * @param data Pointer where to store the data
 * @param size Pointer to store the size of data
 * @param error Pointer to a error struct 
 * @return 1 on success, 0 no result, -1 on error.
 */
int osync_db_get_blob(OSyncDB *db, const char *query, char **data, unsigned int *size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p, %p)", __func__, db, query ? query : "nil", data, size, error);

	osync_assert(db);
	osync_assert(query);
	osync_assert(data);
	osync_assert(size);

	sqlite3_stmt *sqlite_stmt = NULL;

	int rc = sqlite3_prepare(db->sqlite3db, query, -1, &sqlite_stmt, NULL);
	if(rc != SQLITE_OK)
		goto error_msg;
	
	rc = sqlite3_step(sqlite_stmt);
	if(rc != SQLITE_ROW) {
		sqlite3_reset(sqlite_stmt);
		sqlite3_finalize(sqlite_stmt);
		osync_trace(TRACE_EXIT, "%s: no result!", __func__);
		return 0;
	}
	
	const char *tmp = sqlite3_column_blob(sqlite_stmt, 0);
	*size = sqlite3_column_bytes(sqlite_stmt, 0);
	if (*size == 0) {
		sqlite3_reset(sqlite_stmt);
		sqlite3_finalize(sqlite_stmt);
		osync_trace(TRACE_EXIT, "%s: no data!", __func__);
		return 0;
	}

	*data = osync_try_malloc0(*size, error);

	if(!*data)
		goto error;

	memcpy(*data, tmp, *size);
	
	if (sqlite3_step(sqlite_stmt) == SQLITE_ROW) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Returned more than one result for a uid");
		goto error;
	}
	
	sqlite3_reset(sqlite_stmt);
	sqlite3_finalize(sqlite_stmt);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return 1; 
	
error_msg:
	osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get data: %s", sqlite3_errmsg(db->sqlite3db));
error:
	if(sqlite_stmt) {
		sqlite3_reset(sqlite_stmt);
		sqlite3_finalize(sqlite_stmt);	
	}
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return -1;
}

long long int osync_db_last_rowid(OSyncDB *db) {
	osync_assert(db);

	return sqlite3_last_insert_rowid(db->sqlite3db);
}	

