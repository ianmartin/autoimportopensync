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
#include "opensync_db_private.h"

/*
  static void _osync_db_trace(void *data, const char *query)
  {
  osync_trace(TRACE_INTERNAL, "osync_db query executed: %s", query);
  }
*/

OSyncDB *osync_db_new(OSyncError **error)
{
  OSyncDB *db = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);

  db = osync_try_malloc0(sizeof(OSyncDB), error);
  if (!db) {
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
    return NULL;
  }

  osync_trace(TRACE_EXIT, "%s: %p", __func__, db);
  return db;
}

osync_bool osync_db_open(OSyncDB *db, const char *dbfile, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, dbfile, error);

  osync_assert(db);
  osync_assert(dbfile);

  if (sqlite3_open(dbfile, &(db->sqlite3db)) != SQLITE_OK) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot open database \"%s\": %s", dbfile, sqlite3_errmsg(db->sqlite3db));
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, sqlite3_errmsg(db->sqlite3db));
    return FALSE;
  }

  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;
}

osync_bool osync_db_close(OSyncDB *db, OSyncError **error)
{
  int rc = 0;
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, db, error);

  osync_assert(db);
	
  rc = sqlite3_close(db->sqlite3db);
  if (rc) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot close database: %s", sqlite3_errmsg(db->sqlite3db));
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, sqlite3_errmsg(db->sqlite3db));
    return FALSE;
  }

  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;
}

int osync_db_count(OSyncDB *db, const char *query, OSyncError **error)
{
  int num;
  char **result = NULL;
  char *errmsg = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, query, error);	
  osync_assert(db);
  osync_assert(query);

  if (sqlite3_get_table(db->sqlite3db, query, &result, &num, NULL, &errmsg) != SQLITE_OK) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable count result of query: %s", errmsg);
    sqlite3_free_table(result);
    sqlite3_free(errmsg);
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
    return -1;
  }

  sqlite3_free_table(result);
  osync_trace(TRACE_EXIT, "%s: %i", __func__, num);
  return num;
}

osync_bool osync_db_query(OSyncDB *db, const char *query, OSyncError **error)
{
  char *errmsg = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, query, error);
	
  osync_assert(db);
  osync_assert(query);

  if (sqlite3_exec(db->sqlite3db, query, NULL, NULL, &errmsg) != SQLITE_OK) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to execute simple query: %s", errmsg);
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, errmsg);
    sqlite3_free(errmsg);
    return FALSE;
  }

  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;
}

OSyncList *osync_db_query_table(OSyncDB *db, const char *query, OSyncError **error)
{
  OSyncList *table = NULL;
  int i, j, column_count = 0;
  int numrows = 0, numcolumns = 0;
  char **result = NULL;
  char *errmsg = NULL;

  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, query, error);	
  osync_assert(db);
  osync_assert(query);

  if (sqlite3_get_table(db->sqlite3db, query, &result, &numrows, &numcolumns, &errmsg) != SQLITE_OK) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to query table: %s", errmsg);
    sqlite3_free(errmsg);
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
    return NULL;
  }
	
  /* First row contains only names of columns. So we skip this!
     We start reading the array with a offset (number of columns) */
  column_count = numcolumns;

  for (j=0; j < numrows; j++) {
    OSyncList *row = NULL;
    for (i=0; i < numcolumns; i++)
      /* speed up - prepend instead of append */
      row = osync_list_prepend(row, g_strdup(result[column_count++]));

    /* items got prepended, reverse the list again */
    row = osync_list_reverse(row);

    /* speed up - prepend instead of append. */
    table = osync_list_prepend(table, row);
  }

  /* items got prepended, reverse the list again */
  table = osync_list_reverse(table);

  sqlite3_free_table(result);

  osync_trace(TRACE_EXIT, "%s: %p", __func__, table);
  return table; 
}

void osync_db_free_list(OSyncList *list) {
  OSyncList *row;
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, list);

  for (row = list; row; row = row->next) {
    osync_list_foreach((OSyncList *) row->data, (GFunc) g_free, NULL);
    osync_list_free((OSyncList *) row->data);
  }

  osync_list_free(list);

  osync_trace(TRACE_EXIT, "%s", __func__);
}

char *osync_db_query_single_string(OSyncDB *db, const char *query, OSyncError **error)
{
  char *result = NULL;
  sqlite3_stmt *ppStmt = NULL;

  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, query, error);

  osync_assert(db);
  osync_assert(query);

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
	
  osync_trace(TRACE_EXIT, "%s: %s", __func__, result);
  return result; 
	
 error:
  g_free(result);
  sqlite3_finalize(ppStmt);
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return NULL;
}

int osync_db_query_single_int(OSyncDB *db, const char *query, OSyncError **error)
{
  int result = 0;
  sqlite3_stmt *ppStmt = NULL;

  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, query, error);

  osync_assert(db);
  osync_assert(query);

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
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return -1;
}

osync_bool osync_db_reset_table(OSyncDB *db, const char *tablename, OSyncError **error)
{
  char *query = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, tablename, error);

  osync_assert(db);
  osync_assert(tablename);
	
  query = g_strdup_printf("DELETE FROM %s", tablename);

  if (!osync_db_query(db, query, error))
    goto error;

  g_free(query);
	
  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;
 error:
  g_free(query);
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));	
  return FALSE;
}

osync_bool osync_db_reset_full(OSyncDB *db, OSyncError **error)
{
  sqlite3_stmt *ppStmt = NULL;
  char *query = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, db, error);

  osync_assert(db);

  query = g_strdup("SELECT name FROM (SELECT * FROM sqlite_master) WHERE type='table'");

  if (sqlite3_prepare(db->sqlite3db, query, -1, &ppStmt, NULL) != SQLITE_OK) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Query Error: %s", sqlite3_errmsg(db->sqlite3db));
    goto error;
  }

  while (sqlite3_step(ppStmt) == SQLITE_ROW) {
    const char *table = (const char *) sqlite3_column_text(ppStmt, 0);
    if (!osync_db_reset_table(db, table, error))
      goto error;
  }

  sqlite3_finalize(ppStmt);

  osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
  return TRUE;

 error:
  sqlite3_finalize(ppStmt);
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error)); 
  return FALSE;
}

osync_bool osync_db_reset_full_by_path(const char *path, OSyncError **error)
{
  OSyncDB *db = NULL;
  osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, path, error);

  osync_assert(path);

  if (!osync_db_open(db, path, error))
    goto error;

  if (!osync_db_reset_full(db, error))
    goto error;

  osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
  return TRUE;
 error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error)); 
  return FALSE;

}

int osync_db_table_exists(OSyncDB *db, const char *tablename, OSyncError **error)
{
  sqlite3_stmt *ppStmt = NULL;
  char *query = NULL;

  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, tablename, error);

  osync_assert(db);
  osync_assert(tablename);

  query = g_strdup_printf("SELECT name FROM (SELECT * FROM sqlite_master UNION ALL SELECT * FROM sqlite_temp_master) WHERE type='table' AND name='%s'",
                          tablename);

  if (sqlite3_prepare(db->sqlite3db, query, -1, &ppStmt, NULL) != SQLITE_OK) {
    sqlite3_finalize(ppStmt);
    g_free(query);

    osync_error_set(error, OSYNC_ERROR_GENERIC, "Query Error: %s", sqlite3_errmsg(db->sqlite3db));
    osync_trace(TRACE_EXIT_ERROR, "Database query error: %s", sqlite3_errmsg(db->sqlite3db));
    return -1;
  }


  if (sqlite3_step(ppStmt) != SQLITE_ROW) {
    sqlite3_finalize(ppStmt);
    g_free(query);

    osync_trace(TRACE_EXIT, "%s: table \"%s\" doesn't exist.", __func__, tablename);
    return 0;
  }

  sqlite3_finalize(ppStmt);
  g_free(query);
	
  osync_trace(TRACE_EXIT, "%s: table \"%s\" exists.", __func__, tablename);
  return 1;
}

osync_bool osync_db_bind_blob(OSyncDB *db, const char *query, const char *data, unsigned int size, OSyncError **error)
{
  sqlite3_stmt *sqlite_stmt = NULL;
  int rc = 0;

  osync_trace(TRACE_ENTRY, "%s(%p, %s, %u, %p)", __func__, db, query, size, error);
  osync_trace(TRACE_SENSITIVE, "data parameter : %s", data);

  osync_assert(db);
  osync_assert(query);
  osync_assert(data);
  osync_assert(size);

  rc = sqlite3_prepare(db->sqlite3db, query, -1, &sqlite_stmt, NULL);
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
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return FALSE;
}

int osync_db_get_blob(OSyncDB *db, const char *query, char **data, unsigned int *size, OSyncError **error)
{
  sqlite3_stmt *sqlite_stmt = NULL;
  int rc = 0;
  const char *tmp = NULL;

  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p, %p)", __func__, db, query, data, size, error);

  osync_assert(db);
  osync_assert(query);
  osync_assert(data);
  osync_assert(size);

  rc = sqlite3_prepare(db->sqlite3db, query, -1, &sqlite_stmt, NULL);
  if(rc != SQLITE_OK)
    goto error_msg;
	
  rc = sqlite3_step(sqlite_stmt);
  if(rc != SQLITE_ROW) {
    sqlite3_reset(sqlite_stmt);
    sqlite3_finalize(sqlite_stmt);
    osync_trace(TRACE_EXIT, "%s: no result!", __func__);
    return 0;
  }
	
  tmp = sqlite3_column_blob(sqlite_stmt, 0);
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
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return -1;
}

long long int osync_db_last_rowid(OSyncDB *db) {
  osync_assert(db);

  return sqlite3_last_insert_rowid(db->sqlite3db);
}	

char *osync_db_sql_escape(const char *query)
{
  return osync_strreplace(query, "'", "''");
}

