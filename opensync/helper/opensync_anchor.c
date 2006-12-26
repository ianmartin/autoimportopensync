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

#include "opensync-helper.h"
#include "opensync-db.h"

static osync_bool _osync_anchor_db_create(OSyncDB *db, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, db, error);

	char *query = g_strdup("CREATE TABLE tbl_anchor (id INTEGER PRIMARY KEY, anchor VARCHAR, objtype VARCHAR UNIQUE)");

	if (!osync_db_query(db, query, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		g_free(query);
		return FALSE;
	}

	g_free(query);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}	

static OSyncDB *_osync_anchor_db_new(const char *filename, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, filename, error);
	
	OSyncDB *db = osync_db_new(error); 
	if (!db)
		goto error;
	
	if (!osync_db_open(db, filename, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		goto error_free_db;
	}
	
	int ret = osync_db_exists(db, "tbl_anchor", error);
	if (ret > 0) {
		osync_trace(TRACE_EXIT, "%s: %p", __func__, db);
		return db;
	/* error if ret == -1 */	
	} else if (ret < 0) {
		goto error_free_db;
	}	
	/* ret equal 0 means table does not exist yet. continue and create one. */

	if (!_osync_anchor_db_create(db, error))
		goto error_free_db;

	osync_trace(TRACE_EXIT, "%s: %p", __func__, db);
	return db;

error_free_db:
	g_free(db);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void _osync_anchor_db_free(OSyncDB *db)
{
	osync_assert(db);

	if (!osync_db_close(db, NULL))
		osync_trace(TRACE_INTERNAL, "Can't close database");

	g_free(db);
}

static char *_osync_anchor_db_retrieve(OSyncDB *db, const char *key)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, db, key);
	osync_assert(db);
	osync_assert(key);

	char *retanchor = NULL;
	
	char *query = g_strdup_printf("SELECT anchor FROM tbl_anchor WHERE objtype='%s'", key);
	retanchor = osync_db_query_single_string(db, query, NULL); 
	g_free(query);
	
	osync_trace(TRACE_EXIT, "%s: %s", __func__, retanchor);
	return retanchor;
}

static void _osync_anchor_db_update(OSyncDB *db, const char *key, const char *anchor)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %, %s)", __func__, db, key, anchor);
	osync_assert(db);
	osync_assert(key);
	
	char *query = g_strdup_printf("REPLACE INTO tbl_anchor (objtype, anchor) VALUES('%s', '%s')", key, anchor);
	/* TODO: Add Error handling in this funciton for osync_db_query() */
	if (!osync_db_query(db, query, NULL)) {
		osync_trace(TRACE_INTERNAL, "Unable put anchor!");
	}
	g_free(query);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool osync_anchor_compare(const char *anchordb, const char *key, const char *new_anchor)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %s, %s)", __func__, anchordb, key, new_anchor);
	osync_assert(anchordb);
	
	OSyncDB *db = _osync_anchor_db_new(anchordb, NULL);
	if (!db)
		return FALSE;
	
	char *old_anchor = _osync_anchor_db_retrieve(db, key);
	osync_bool retval = FALSE;
	
	if (old_anchor) {
		if (!strcmp(old_anchor, new_anchor)) {
			retval = TRUE;
		} else {
			retval = FALSE;
		}
		g_free(old_anchor);
	}
	
	_osync_anchor_db_free(db);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, retval);
	return retval;
}

void osync_anchor_update(const char *anchordb, const char *key, const char *new_anchor)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %s, %s)", __func__, anchordb, key, new_anchor);
	osync_assert(anchordb);
	
	OSyncDB *db = _osync_anchor_db_new(anchordb, NULL);
	if (!db)
		return;
	
	_osync_anchor_db_update(db, key, new_anchor);
	
	_osync_anchor_db_free(db);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

char *osync_anchor_retrieve(const char *anchordb, const char *key)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %s)", __func__, anchordb, key);
	osync_assert(anchordb);
	
	OSyncDB *db = _osync_anchor_db_new(anchordb, NULL);
	if (!db)
		return NULL;
	
	char *retval = _osync_anchor_db_retrieve(db, key);
	
	_osync_anchor_db_free(db);
	
	osync_trace(TRACE_EXIT, "%s: %s", __func__, retval);
	return retval;
}
