/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2006  NetNix Finland Ltd <netnix@netnix.fi>
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
 * Author: Armin Bauer <armin.bauer@opensync.org>
 * Author: Daniel Friedrich <daniel.friedrich@opensync.org>
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

/**
 * @brief Creates a new archive object
 * @param
 * @return The pointer to the newly allocated archive object or NULL in case of error
 */
OSyncArchive *osync_archive_new(const char *filename, OSyncError **error)
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
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, archive);
	return archive;

error_free:
	g_free(archive);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/**
 * @brief Increments the reference counter
 * @param archive The pointer to a archive object
 */
void osync_archive_ref(OSyncArchive *archive)
{
	osync_assert(archive);
	
	g_atomic_int_inc(&(archive->ref_count));
}

/**
 * @brief Decrement the reference counter. The archive object will 
 *  be freed if there is no more reference to it.
 * @param archive The pointer to a archive object
 */
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
		
		g_free(archive);
		
		osync_trace(TRACE_EXIT, "%s", __func__);
	}
}

/**
 * @brief Store data of a entry in the group archive database (blob).
 *
 * @param archive The group archive
 * @param uid UID of requested entry
 * @param data The data to store 
 * @param size Total size of data 
 * @param error Pointer to a error struct
 * @return Returns TRUE on success otherwise FALSE
 */ 
osync_bool osync_archive_save_data(OSyncArchive *archive, const char *uid, const char *data, unsigned int size, OSyncError **error)
{
	char *query = NULL;
	sqlite3_stmt *sqlite_stmt = NULL;
		
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %u, %p)", __func__, archive, uid, data, size, error);
	osync_assert(archive);
	osync_assert(uid);
	osync_assert(data);
	osync_assert(size);
	
	char *escaped_uid = _osync_archive_sql_escape(uid);
	// TODO: question marks stands for?
	query = g_strdup_printf("UPDATE tbl_changes SET data=? WHERE uid='%s'", escaped_uid);
	g_free(escaped_uid);
	
	int rc = sqlite3_prepare(archive->db, query, -1, &sqlite_stmt, NULL);
	g_free(query);
	if(rc != SQLITE_OK)
		goto error_msg;
	
	rc = sqlite3_bind_blob(sqlite_stmt, 1, data, size, SQLITE_TRANSIENT);
	if(rc != SQLITE_OK)
		goto error_msg;
	
	rc = sqlite3_step(sqlite_stmt);
	if (rc != SQLITE_DONE) {
		if(rc != SQLITE_ERROR)
			goto error_msg;
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to insert data! %s", sqlite3_errmsg(archive->db));
		goto error;
	}
	
	sqlite3_reset(sqlite_stmt);
	sqlite3_finalize(sqlite_stmt);

	osync_trace(TRACE_EXIT, "%s: %i", __func__, TRUE);
	return TRUE;

error_msg:
	osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to insert data! sqlite rc: %i", rc);
error:
	if(sqlite_stmt) {
		sqlite3_reset(sqlite_stmt);
		sqlite3_finalize(sqlite_stmt);	
	}
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/**
 * @brief Load data of a entry which is stored in the group archive database (blob).
 *
 * @param archive The group archive
 * @param uid UID of requested entry
 * @param data Pointer to store the requested data 
 * @param size Pointer to store the size of requested data
 * @param error Pointer to a error struct
 * @return Returns TRUE on success otherwise FALSE
 */ 
osync_bool osync_archive_load_data(OSyncArchive *archive, const char *uid, char **data, unsigned int *size, OSyncError **error)
{
	sqlite3_stmt *sqlite_stmt = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p, %p)", __func__, archive, uid, data, size, error);
	osync_assert(archive);
	osync_assert(uid);
	osync_assert(data);
	osync_assert(size);
	
	char *query = g_strdup_printf("SELECT data FROM tbl_changes WHERE uid='%s'", uid);
	int rc = sqlite3_prepare(archive->db, query, -1, &sqlite_stmt, NULL);
	g_free(query);
	if(rc != SQLITE_OK)
		goto error_msg;
	
	rc = sqlite3_step(sqlite_stmt);
	if(rc != SQLITE_ROW) {
		if(rc == SQLITE_ROW) {
			osync_error_set(error, OSYNC_ERROR_PARAMETER, "UID not found!");
			goto error;
		}
		if(rc == SQLITE_ERROR) {
			osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to insert data! %s", sqlite3_errmsg(archive->db));
			goto error;
		}
		goto error_msg;		
	}
	
	const char *tmp = sqlite3_column_blob(sqlite_stmt, 0);
	*size = sqlite3_column_bytes(sqlite_stmt, 0);
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

	osync_trace(TRACE_EXIT, "%s: %i", __func__, TRUE);
	return TRUE;
	
error_msg:
	osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to insert data! sqlite rc: %i", rc);	
error:
	if(sqlite_stmt) {
		sqlite3_reset(sqlite_stmt);
		sqlite3_finalize(sqlite_stmt);	
	}
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/**
 * @brief Save a entry in the group archive. 
 *
 * @param archive The group archive
 * @param id Arhive (database) id of entry which gets deleted
 * @param uid Reported UID of entry
 * @param objtype Reported object type of entry
 * @param mappingid Mapped ID of entry 
 * @param memberid ID of member which reported entry 
 * @param error Pointer to a error struct
 * @return Returns number of entries in archive group database. 0 on error. 
 */ 
long long int osync_archive_save_change(OSyncArchive *archive, long long int id, const char *uid, const char *objtype, long long int mappingid, long long int memberid, OSyncError **error)
{
	char *query = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %lli, %s, %s, %lli, %lli, %p)", __func__, archive, id, uid, objtype, mappingid, memberid, error);
	osync_assert(archive);
	
	char *escaped_uid = _osync_archive_sql_escape(uid);
	if (!id) {
		query = g_strdup_printf("INSERT INTO tbl_changes (uid, objtype, memberid, mappingid) VALUES('%s', '%s', '%lli', '%lli')", escaped_uid, objtype, memberid, mappingid);
	} else {
		query = g_strdup_printf("UPDATE tbl_changes SET uid='%s', objtype='%s', memberid='%lli', mappingid='%lli' WHERE id=%lli", escaped_uid, objtype, memberid, mappingid, id);
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

/**
 * @brief Delete certain entry form group archive. 
 *
 * @param archive The group archive
 * @param id Archive (database) id of entry which gets deleted
 * @param error Pointer to a error struct
 * @return TRUE on when all changes successfully loaded otherwise FALSE
 */ 
osync_bool osync_archive_delete_change(OSyncArchive *archive, long long int id, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %lli, %p)", __func__, archive, id, error);
	osync_assert(archive);
	
	char *query = g_strdup_printf("DELETE FROM tbl_changes WHERE id=%lli", id);
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

/**
 * @brief Load all changes from group archive for a certain object type.
 *
 * @param archive The group archive
 * @param objtype Requested object type 
 * @param ids List to store the archive (database) ids of each entry
 * @param uids List to store uids of each entry
 * @param mappingids List to store mappingids for each entry
 * @param error Pointer to a error struct
 * @return TRUE on when all changes successfully loaded otherwise FALSE
 */ 
osync_bool osync_archive_load_changes(OSyncArchive *archive, const char *objtype, OSyncList **ids, OSyncList **uids, OSyncList **mappingids, OSyncList **memberids, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p, %p, %p, %p)", __func__, archive, objtype, ids, uids, mappingids, memberids, error);
	
	/* TODO Check if table already exist and don't CREATE TABLE when it exists. Better error handling! FALSE on Error! */
	if (sqlite3_exec(archive->db, "CREATE TABLE tbl_changes (id INTEGER PRIMARY KEY, uid VARCHAR, objtype VARCHAR, memberid INTEGER, mappingid INTEGER, data BLOB)", NULL, NULL, NULL) != SQLITE_OK)
		osync_trace(TRACE_INTERNAL, "Unable to create changes table! %s", sqlite3_errmsg(archive->db));
	
	sqlite3_stmt *ppStmt = NULL;
	char *query = g_strdup_printf("SELECT id, uid, mappingid, memberid FROM tbl_changes WHERE objtype='%s' ORDER BY mappingid", objtype);
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

/**
 * @brief Get the object type for a entry of the group archive.
 *
 * @param archive The group archive
 * @param uid The uid of the entry
 * @param error Pointer to a error struct 
 * @return Returns object type of entry. NULL if entry doesn't exit. 
 */
char *osync_archive_get_objtype(OSyncArchive *archive, const char *uid, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, archive, uid, error);
	
	sqlite3_stmt *ppStmt = NULL;
	char *query = g_strdup_printf("SELECT objtype FROM tbl_changes WHERE uid='%s'", uid);
	sqlite3_prepare(archive->db, query, -1, &ppStmt, NULL);
	g_free(query);
	
	if (sqlite3_step(ppStmt) != SQLITE_ROW) {
		osync_trace(TRACE_EXIT, "%s: uid not found", __func__);
		return NULL;
	}
	
	char *objtype = g_strdup((const char *)sqlite3_column_text(ppStmt, 0));
	
	if (sqlite3_step(ppStmt) == SQLITE_ROW) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Returned more than one result for a uid");
		goto error;
	}
	
	sqlite3_finalize(ppStmt);
	
	osync_trace(TRACE_EXIT, "%s: %s", __func__, objtype);
	return objtype;
	
error:
	g_free(objtype);
	sqlite3_finalize(ppStmt);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/*@}*/
