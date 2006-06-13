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

#include "opensync-db.h"
#include "opensync_db_internals.h"

static void osync_db_trace(void *data, const char *query)
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
		osync_trace(TRACE_ERROR, "Unable prepare count! %s", sqlite3_errmsg(db->db));
	if (sqlite3_step(ppStmt) != SQLITE_OK)
		osync_trace(TRACE_ERROR, "Unable step count! %s", sqlite3_errmsg(db->db));
	ret = sqlite3_column_int64(ppStmt, 0);
	sqlite3_finalize(ppStmt);
	return ret;
}
