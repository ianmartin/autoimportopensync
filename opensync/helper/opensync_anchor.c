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

#include <sqlite3.h>

#include "opensync-helper.h"
#include "opensync_anchor_internals.h"

OSyncAnchorDB *osync_anchor_db_new(const char *filename, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, filename, error);
	
	OSyncAnchorDB *db = osync_try_malloc0(sizeof(OSyncAnchorDB), error);
	if (!db)
		goto error;
	
	int rc = sqlite3_open(filename, &(db->sdb));
	if (rc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot open database: %s", sqlite3_errmsg(db->sdb));
		goto error_free_db;
	}
	
	if (sqlite3_exec(db->sdb, "CREATE TABLE tbl_anchor (id INTEGER PRIMARY KEY, anchor VARCHAR, objtype VARCHAR UNIQUE)", NULL, NULL, NULL) != SQLITE_OK)
		osync_trace(TRACE_INTERNAL, "Unable create anchor table! %s", sqlite3_errmsg(db->sdb));
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, db);
	return db;

error_free_db:
	g_free(db);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void osync_anchor_db_free(OSyncAnchorDB *db)
{
	osync_assert(db);
	
	int ret = sqlite3_close(db->sdb);
	if (ret)
		osync_trace(TRACE_INTERNAL, "Can't close database: %s", sqlite3_errmsg(db->sdb));
	g_free(db);
}

char *osync_anchor_db_retrieve(OSyncAnchorDB *db, const char *key)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, db, key);
	osync_assert(db);
	osync_assert(key);
	
	sqlite3_stmt *ppStmt = NULL;
	char *query = g_strdup_printf("SELECT anchor FROM tbl_anchor WHERE objtype='%s'", key);
	if (sqlite3_prepare(db->sdb, query, -1, &ppStmt, NULL) != SQLITE_OK)
		osync_trace(TRACE_INTERNAL, "Unable prepare anchor! %s", sqlite3_errmsg(db->sdb));
	sqlite3_step(ppStmt);
	
	char *retanchor = g_strdup((gchar*)sqlite3_column_text(ppStmt, 0));
	sqlite3_finalize(ppStmt);
	g_free(query);
	
	osync_trace(TRACE_EXIT, "%s: %s", __func__, retanchor);
	return retanchor;
}

void osync_anchor_db_update(OSyncAnchorDB *db, const char *key, const char *anchor)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %, %s)", __func__, db, key, anchor);
	osync_assert(db);
	osync_assert(key);
	
	char *query = g_strdup_printf("REPLACE INTO tbl_anchor (objtype, anchor) VALUES('%s', '%s')", key, anchor);
	if (sqlite3_exec(db->sdb, query, NULL, NULL, NULL) != SQLITE_OK)
		osync_trace(TRACE_INTERNAL, "Unable put anchor! %s", sqlite3_errmsg(db->sdb));

	g_free(query);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool osync_anchor_db_compare(OSyncAnchorDB *db, const char *key, const char *new_anchor)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s)", __func__, db, key, new_anchor);
	osync_assert(db);
	osync_assert(key);
	
	char *old_anchor = osync_anchor_db_retrieve(db, key);
	
	if (old_anchor) {
		if (!strcmp(old_anchor, new_anchor)) {
			g_free(old_anchor);
			osync_trace(TRACE_EXIT, "%s: Anchors match", __func__);
			return TRUE;
		} else {
			g_free(old_anchor);
			osync_trace(TRACE_EXIT, "%s: Anchor mismatch", __func__);
			return FALSE;
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: No previous anchor", __func__);
	return FALSE;
}
