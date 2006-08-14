/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  Daniel Friedrich <daniel.friedrich@opensync.org>
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

#include "opensync-archive.h"
#include "opensync_archive_internals.h"

/**
 * @defgroup OSyncArchivePrivateAPI OpenSync Archive Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncArchive
 * 
 */
/*@{*/

void _osync_archive_trace(void *data, const char *query)
{
	osync_trace(TRACE_INTERNAL, "query executed: %s", query);
}

char *_osync_archive_sql_escape(const char *s)
{
	return osync_strreplace(s, "'", "''");
}

/*@}*/

/**
 * @defgroup OSyncArchiveAPI OpenSync Archive
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncArchive
 * 
 */
/*@{*/

OSyncArchive *osync_archive_new(const char *filename, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, filename, error);
	
	OSyncArchive *archive = osync_try_malloc0(sizeof(OSyncArchive), error);
	if (!archive)
		goto error;
	archive->ref_count = 1;
	
	int rc = sqlite3_open(filename, &(archive->db));
	if (rc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot open database: %s", sqlite3_errmsg(archive->db));
		goto error_free;
	}
	sqlite3_trace(archive->db, _osync_archive_trace, NULL);
	
	archive->tablename = g_strdup_printf("tbl_changes_%s", objtype);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, archive);
	return archive;

error_free:
	g_free(archive);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void osync_archive_ref(OSyncArchive *archive)
{
	osync_assert(archive);
	
	g_atomic_int_inc(&(archive->ref_count));
}

void osync_archive_unref(OSyncArchive *archive)
{
	osync_assert(archive);
	
	if (g_atomic_int_dec_and_test(&(archive->ref_count))) {
		osync_trace(TRACE_ENTRY, "%s(%p)", __func__, archive);
		
		if (archive->db) {
			int ret = sqlite3_close(archive->db);
			if (ret)
				osync_trace(TRACE_INTERNAL, "Can't close database: %s", sqlite3_errmsg(archive->db));
		}
		
		if (archive->tablename)
			g_free(archive->tablename);
		
		g_free(archive);
		
		osync_trace(TRACE_EXIT, "%s", __func__);
	}
}

osync_bool osync_archive_save_data(OSyncArchive *archive, const char *uid, const char *data, unsigned int size, OSyncError **error)
{
	return TRUE;
}

osync_bool osync_archive_load_data(OSyncArchive *archive, const char *uid, const char **data, unsigned int *size, OSyncError **error)
{
	return TRUE;
}

long long int osync_archive_save_change(OSyncArchive *archive, long long int id, const char *uid, long long int mappingid, long long int memberid, OSyncError **error)
{
	char *query = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %lli, %s, %lli, %lli, %p)", __func__, archive, id, uid, mappingid, memberid, error);
	osync_assert(archive);
	
	char *escaped_uid = _osync_archive_sql_escape(uid);
	if (!id) {
		query = g_strdup_printf("INSERT INTO %s (uid, memberid, mappingid) VALUES('%s', '%lli', '%lli')", archive->tablename, escaped_uid, memberid, mappingid);
	} else {
		query = g_strdup_printf("UPDATE %s SET uid='%s', memberid='%lli', mappingid='%lli' WHERE id=%lli", archive->tablename, escaped_uid, memberid, mappingid, id);
	}
	g_free(escaped_uid);
	
	if (sqlite3_exec(archive->db, query, NULL, NULL, NULL) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to insert change! %s", sqlite3_errmsg(archive->db));
		g_free(query);
		goto error;
	}
	g_free(query);
	
	if (!id)
		id = sqlite3_last_insert_rowid(archive->db);
	
	osync_trace(TRACE_EXIT, "%s: %lli", __func__, id);
	return id;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return 0;
}

osync_bool osync_archive_delete_change(OSyncArchive *archive, long long int id, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %lli, %p)", __func__, archive, id, error);
	osync_assert(archive);
	
	char *query = g_strdup_printf("DELETE FROM %s WHERE id=%lli", archive->tablename, id);
	if (sqlite3_exec(archive->db, query, NULL, NULL, NULL) != SQLITE_OK) {
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to delete change! %s", sqlite3_errmsg(archive->db));
		g_free(query);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	g_free(query);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool osync_archive_load_changes(OSyncArchive *archive, OSyncList **ids, OSyncList **uids, OSyncList **mappingids, OSyncList **memberids, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p, %p)", __func__, archive, ids, uids, mappingids, memberids, error);
	
	char *query = g_strdup_printf("CREATE TABLE %s (id INTEGER PRIMARY KEY, uid VARCHAR, objtype VARCHAR, format VARCHAR, memberid INTEGER, mappingid INTEGER)", archive->tablename);
	if (sqlite3_exec(archive->db, query, NULL, NULL, NULL) != SQLITE_OK)
		osync_trace(TRACE_INTERNAL, "Unable create changes table! %s", sqlite3_errmsg(archive->db));
	g_free(query);
	
	sqlite3_stmt *ppStmt = NULL;
	query = g_strdup_printf("SELECT id, uid, mappingid, memberid FROM %s ORDER BY mappingid", archive->tablename);
	sqlite3_prepare(archive->db, query, -1, &ppStmt, NULL);
	g_free(query);

	while (sqlite3_step(ppStmt) == SQLITE_ROW) {
		long long int id = sqlite3_column_int64(ppStmt, 0);
		const char *uid = (const char *)sqlite3_column_text(ppStmt, 1);
		long long int mappingid = sqlite3_column_int64(ppStmt, 2);
		long long int memberid = sqlite3_column_int64(ppStmt, 3);
		
		*ids = osync_list_append((*ids), GINT_TO_POINTER((int)id));
		*uids = osync_list_append((*uids), g_strdup(uid));
		*mappingids = osync_list_append((*mappingids), GINT_TO_POINTER((int)mappingid));
		*memberids = osync_list_append((*memberids), GINT_TO_POINTER((int)memberid));
		
    	osync_trace(TRACE_INTERNAL, "Loaded change with uid %s, mappingid %lli from member %lli", uid, mappingid, memberid);
	}
	sqlite3_finalize(ppStmt);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/*@}*/
