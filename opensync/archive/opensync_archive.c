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
#include "opensync-db.h"
#include <db/opensync_db_internals.h>

/**
 * @defgroup OSyncArchivePrivateAPI OpenSync Archive Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncArchive API
 * 
 */
/*@{*/

void _osync_archive_trace(void *data, const char *query)
{
	osync_trace(TRACE_INTERNAL, "query executed: %s", query);
}

/*@}*/

/**
 * @defgroup OSyncArchiveAPI OpenSync Archive
 * @ingroup OSyncPublic
 * @brief Functions for manipulating the archive
 * 
 * The archive is a persistent store of all of the objects seen/handled 
 * in a sync group. It is necessary for the merger code to work.
 */
/*@{*/

static osync_bool osync_archive_create_changes(OSyncDB *db, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, objtype, error); 

	osync_assert(db);
	osync_assert(objtype);

	char *tbl_changes = g_strdup_printf("tbl_changes_%s", objtype);
	int ret = osync_db_exists(db, tbl_changes, error);
	g_free(tbl_changes);

	/* error if ret -1 */
	if (ret < 0)
		goto error;

	/* if ret is != 0 then the table already exist */
	if (ret) {
		osync_trace(TRACE_EXIT, "%s", __func__);
		return TRUE;
	}

	char *query = g_strdup_printf("CREATE TABLE tbl_changes_%s (id INTEGER PRIMARY KEY, uid VARCHAR, memberid INTEGER, mappingid INTEGER)", objtype);
	if (!osync_db_query(db, query, error)) {
		g_free(query);
		goto error;
	}

	g_free(query);

	osync_trace(TRACE_EXIT, "%s: created table.", __func__);
	return TRUE;

error:	
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool osync_archive_create_changelog(OSyncDB *db, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, objtype, error); 

	osync_assert(db);
	osync_assert(objtype);

	char *tbl_changelog = g_strdup_printf("tbl_changelog_%s", objtype);
	int ret = osync_db_exists(db, tbl_changelog, error);
	g_free(tbl_changelog);

	/* error if ret -1 */
	if (ret < 0)
		goto error;

	/* if ret != 0 table does not exist. continue and create it */
	if (ret) {
		osync_trace(TRACE_EXIT, "%s", __func__);
		return TRUE;
	}

	char *query = g_strdup_printf("CREATE TABLE tbl_changelog_%s (id INTEGER PRIMARY KEY, entryid INTEGER, changetype INTEGER)", objtype);
	if (!osync_db_query(db, query, error)) {
		g_free(query);
		goto error;
	}
	g_free(query);

	osync_trace(TRACE_EXIT, "%s: created table.", __func__);
	return TRUE;

error:	
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool osync_archive_create(OSyncDB *db, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, objtype, error); 

	osync_assert(db);
	osync_assert(objtype);

	char *tbl_archive = g_strdup_printf("tbl_archive_%s", objtype);
	int ret = osync_db_exists(db, tbl_archive, error);
	g_free(tbl_archive);

	/* error if ret -1 */
	if (ret < 0)
		goto error;

	/* if ret != 0 table does not exist. continue and create it */
	if (ret) {
		osync_trace(TRACE_EXIT, "%s", __func__); 
		return TRUE;
	}


	char *query = g_strdup_printf("CREATE TABLE tbl_archive_%s (mappingid INTEGER PRIMARY KEY, data BLOB)", objtype);
	if (!osync_db_query(db, query, error)) {
		g_free(query);
		goto error;
	}
	g_free(query);

	osync_trace(TRACE_EXIT, "%s: created table.", __func__); 
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/**
 * @brief Creates a new archive object
 * @param filename the full path to the archive database file
 * @param error Pointer to an error struct
 * @return The pointer to the newly allocated archive object or NULL in case of error
 */
OSyncArchive *osync_archive_new(const char *filename, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, filename, error);
	osync_assert(filename);
	
	OSyncArchive *archive = osync_try_malloc0(sizeof(OSyncArchive), error);
	if (!archive)
		goto error;

	archive->ref_count = 1;
	
	archive->db = osync_db_new(error);
	if (!archive->db)
		goto error_and_free;

	if (!osync_db_open(archive->db, filename, error)) {
		g_free(archive->db);
		goto error_and_free;
	}

	osync_trace(TRACE_EXIT, "%s: %p", __func__, archive);
	return archive;

error_and_free:	
	g_free(archive);

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/**
 * @brief Increments the reference counter
 * @param archive The pointer to an archive object
 */
void osync_archive_ref(OSyncArchive *archive)
{
	osync_assert(archive);
	
	g_atomic_int_inc(&(archive->ref_count));
}

/**
 * @brief Decrement the reference counter. The archive object will 
 *  be freed if there is no more reference to it.
 * @param archive The pointer to an archive object
 */
void osync_archive_unref(OSyncArchive *archive)
{
	osync_assert(archive);
	
	if (g_atomic_int_dec_and_test(&(archive->ref_count))) {
		osync_trace(TRACE_ENTRY, "%s(%p)", __func__, archive);
		
		if (archive->db) {
			if (!osync_db_close(archive->db, NULL))	
				osync_trace(TRACE_INTERNAL, "Can't close database");
		}
		
		g_free(archive->db);
		g_free(archive);

		osync_trace(TRACE_EXIT, "%s", __func__);
	}
}

/**
 * @brief Stores data of an entry in the group archive database (blob).
 *
 * @param archive The group archive
 * @param uid UID of requested entry
 * @param objtype The object type of the entry
 * @param data The data to store 
 * @param size Total size of data 
 * @param error Pointer to an error struct
 * @return Returns TRUE on success otherwise FALSE
 */ 
osync_bool osync_archive_save_data(OSyncArchive *archive, const char *uid, const char *objtype, const char *data, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s, %p, %u, %p)", __func__, archive, uid, objtype, data, size, error);
	osync_assert(archive);
	osync_assert(uid);
	osync_assert(data);
	osync_assert(size);

	if (!osync_archive_create(archive->db, objtype, error))
		goto error;

	char *escaped_uid = _osync_db_sql_escape(uid);
	// FIXME: Avoid subselect - this query needs up to 0.5s
	char *query = g_strdup_printf("REPLACE INTO tbl_archive_%s (mappingid, data) VALUES((SELECT mappingid FROM tbl_changes_%s WHERE uid='%s' LIMIT 1), ?)", objtype, objtype, escaped_uid);
	g_free(escaped_uid);
	
	if (!osync_db_bind_blob(archive->db, query, data, size, error)) {
		g_free(query);
		goto error;
	}

	g_free(query);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/**
 * @brief Loads data of an entry which is stored in the group archive database (blob).
 *
 * @param archive The group archive
 * @param uid UID of requestd entry
 * @param objtype The objtype type of the entry
 * @param data Pointer to store the requested data 
 * @param size Pointer to store the size of requested data
 * @param error Pointer to an error struct
 * @return Returns 0 if no data is present else 1. On error -1.
 */ 
int osync_archive_load_data(OSyncArchive *archive, const char *uid, const char *objtype, char **data, unsigned int *size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s, %p, %p, %p)", __func__, archive, uid, objtype, data, size, error);
	osync_assert(archive);
	osync_assert(uid);
	osync_assert(data);
	osync_assert(size);

	if (!osync_archive_create(archive->db, objtype, error))
		goto error;

	char *escaped_uid = _osync_db_sql_escape(uid);
	char *query = g_strdup_printf("SELECT data FROM tbl_archive_%s WHERE mappingid=(SELECT mappingid FROM tbl_changes_%s WHERE uid='%s' LIMIT 1)", objtype, objtype, escaped_uid);
	int ret = osync_db_get_blob(archive->db, query, data, size, error);
	g_free(query);
	g_free(escaped_uid);

	if (ret < 0) {
		goto error;
	} else if (ret == 0) {
		osync_trace(TRACE_EXIT, "%s: no data stored in archive.", __func__); 
		return 0;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return 1;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return -1;
}

/**
 * @brief Saves an entry in the group archive. 
 *
 * @param archive The group archive
 * @param id Archive (database) id of entry to update (if it already exists), specify 0 to add a new entry.
 * @param uid Reported UID of entry
 * @param objtype Reported object type of entry
 * @param mappingid Mapped ID of entry 
 * @param memberid ID of member which reported entry 
 * @param error Pointer to an error struct
 * @return Returns number of entries in archive group database. 0 on error. 
 */ 
long long int osync_archive_save_change(OSyncArchive *archive, long long int id, const char *uid, const char *objtype, long long int mappingid, long long int memberid, OSyncError **error)
{
	
	osync_trace(TRACE_ENTRY, "%s(%p, %lli, %s, %s, %lli, %lli, %p)", __func__, archive, id, uid, objtype, mappingid, memberid, error);
	osync_assert(archive);
	osync_assert(uid);
	osync_assert(objtype);

	if (!osync_archive_create_changes(archive->db, objtype, error))
		goto error;

	char *query = NULL;
	char *escaped_uid = _osync_db_sql_escape(uid);



	if (!id) {
		query = g_strdup_printf("INSERT INTO tbl_changes_%s (uid, mappingid, memberid) VALUES('%s', '%lli', '%lli')", objtype, escaped_uid, mappingid, memberid);
	} else {
		query = g_strdup_printf("UPDATE tbl_changes_%s SET uid='%s', mappingid='%lli', memberid='%lli' WHERE id=%lli", objtype, escaped_uid, mappingid, memberid, id);
	}
	g_free(escaped_uid);
	
	if (!osync_db_query(archive->db, query, error)) {
		g_free(query);
		goto error;
	}

	g_free(query);
	
	if (!id)
		id = osync_db_last_rowid(archive->db);
	
	osync_trace(TRACE_EXIT, "%s: %lli", __func__, id);
	return id;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return 0;
}

/**
 * @brief Deletes an entry from a group archive.
 *
 * @param archive The group archive
 * @param id Archive (database) id of entry to be deleted
 * @param objtype The object type of the entry
 * @param error Pointer to an error struct
 * @return TRUE if the specified change was deleted successfully, otherwise FALSE
 */ 
osync_bool osync_archive_delete_change(OSyncArchive *archive, long long int id, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %lli, %s, %p)", __func__, archive, id, objtype, error);
	osync_assert(archive);
	osync_assert(objtype);

	if (!osync_archive_create_changes(archive->db, objtype, error))
		goto error;

	char *query = g_strdup_printf("DELETE FROM tbl_changes_%s WHERE id=%lli", objtype, id);
	if (!osync_db_query(archive->db, query, error)) {
		g_free(query);
		goto error;
	}

	g_free(query);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:	
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/**
 * @brief Loads all changes from group archive for a certain object type.
 *
 * @param archive The group archive
 * @param objtype Requested object type 
 * @param ids List to store the archive (database) ids of each entry
 * @param uids List to store uids of each entry
 * @param mappingids List to store mappingids for each entry
 * @param memberids List to store member IDs for each entry 
 * @param error Pointer to an error struct
 * @return TRUE on when all changes successfully loaded otherwise FALSE
 */ 
osync_bool osync_archive_load_changes(OSyncArchive *archive, const char *objtype, OSyncList **ids, OSyncList **uids, OSyncList **mappingids, OSyncList **memberids, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p, %p, %p, %p)", __func__, archive, objtype, ids, uids, mappingids, memberids, error);

	osync_assert(archive);
	osync_assert(objtype);
	osync_assert(ids);
	osync_assert(uids);
	osync_assert(mappingids);
	osync_assert(memberids);

	GList *result = NULL, *row = NULL;

	if (!osync_archive_create_changes(archive->db, objtype, error))
		goto error;

	char *query = g_strdup_printf("SELECT id, uid, mappingid, memberid FROM tbl_changes_%s ORDER BY mappingid", objtype);
	result = osync_db_query_table(archive->db, query, error);

	g_free(query);
	
	/* Check for error of osync_db_query_table() call. */
	if (osync_error_is_set(error))
		goto error;

	for (row = result; row; row = row->next) { 
		GList *column = row->data;

		long long int id = g_ascii_strtoull(g_list_nth_data(column, 0), NULL, 0);
		const char *uid = g_list_nth_data(column, 1); 
		long long int mappingid = g_ascii_strtoull(g_list_nth_data(column, 2), NULL, 0);
		long long int memberid = g_ascii_strtoull(g_list_nth_data(column, 3), NULL, 0);
		
		*ids = osync_list_append((*ids), GINT_TO_POINTER((int)id));
		*uids = osync_list_append((*uids), g_strdup(uid));
		*mappingids = osync_list_append((*mappingids), GINT_TO_POINTER((int)mappingid));
		*memberids = osync_list_append((*memberids), GINT_TO_POINTER((int)memberid));
		
	    	osync_trace(TRACE_INTERNAL, "Loaded change with uid %s, mappingid %lli from member %lli", uid, mappingid, memberid);
	}

	osync_db_free_list(result);	

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;	
}

/**
 * @brief Delete all changes from group archive for a certain object type.
 *
 * @param archive The group archive
 * @param objtype Reported object type of entry
 * @param error Pointer to an error struct
 * @return Returns TRUE on success, FALSE otherwise 
 */ 
osync_bool osync_archive_flush_changes(OSyncArchive *archive, const char *objtype, OSyncError **error)
{
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, archive, objtype, error);
	osync_assert(archive);
	osync_assert(objtype);

	if (!osync_archive_create_changelog(archive->db, objtype, error))
		goto error;
	
	char *query = g_strdup_printf("DELETE FROM tbl_changes_%s", objtype);
	
	if (!osync_db_query(archive->db, query, error)) {
		g_free(query);
		goto error;
	}

	g_free(query);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/**
 * @brief Loads all conficting changes which were ignored in the previous sync. 
 *
 * @param archive The group archive
 * @param objtype Requested object type 
 * @param ids List to store the archive (database) ids of each entry
 * @param changetypes List to store the changetypes for each entry
 * @param error Pointer to an error struct
 * @return TRUE on when all changes successfully loaded otherwise FALSE
 */ 
osync_bool osync_archive_load_ignored_conflicts(OSyncArchive *archive, const char *objtype, OSyncList **ids, OSyncList **changetypes, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p)", __func__, archive, objtype, ids, error);

	osync_assert(archive);
	osync_assert(objtype);
	osync_assert(ids);
	osync_assert(changetypes);

	GList *result = NULL;
	GList *row = NULL;

	if (!osync_archive_create_changelog(archive->db, objtype, error))
		goto error;

	char *query = g_strdup_printf("SELECT entryid, changetype FROM tbl_changelog_%s ORDER BY id", objtype);
	result = osync_db_query_table(archive->db, query, error);

	g_free(query);
	
	/* Check for error of osync_db_query_table() call. */
	if (osync_error_is_set(error))
		goto error;

	for (row = result; row; row = row->next) { 
		GList *column = row->data;

		long long int id = g_ascii_strtoull(g_list_nth_data(column, 0), NULL, 0);
		int changetype = atoi(g_list_nth_data(column, 1));
		
		*ids = osync_list_append((*ids), GINT_TO_POINTER((int)id));
		*changetypes = osync_list_append((*changetypes), GINT_TO_POINTER((int)changetype));
		
	    	osync_trace(TRACE_INTERNAL, "Loaded ignored mapping with entryid %lli", id);
	}

	osync_db_free_list(result);	

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;	
}

/**
 * @brief Saves an entry in the ignored conflict list.
 *
 * @param archive The group archive
 * @param objtype Reported object type of entry
 * @param id Mapping Entry ID of entry 
 * @param changetype Changetype of entry 
 * @param error Pointer to an error struct
 * @return Returns TRUE on success, FALSE otherwise 
 */ 
osync_bool osync_archive_save_ignored_conflict(OSyncArchive *archive, const char *objtype, long long int id, OSyncChangeType changetype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %lli, %p)", __func__, archive, objtype, id, error);

	osync_assert(archive);
	osync_assert(objtype);

	if (!osync_archive_create_changelog(archive->db, objtype, error))
		goto error;
	
	char *query = g_strdup_printf("INSERT INTO tbl_changelog_%s (entryid, changetype) VALUES('%lli', '%i')", objtype, id, changetype);
	
	if (!osync_db_query(archive->db, query, error)) {
		g_free(query);
		goto error;
	}

	g_free(query);
	
	osync_trace(TRACE_EXIT, "%s: %lli", __func__, id);
	return TRUE;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/**
 * @brief Deletes all ignored conflict entries of the changelog with the objtype.
 *
 * @param archive The group archive
 * @param objtype Reported object type of entry
 * @param error Pointer to an error struct
 * @return Returns TRUE on success, FALSE otherwise 
 */ 
osync_bool osync_archive_flush_ignored_conflict(OSyncArchive *archive, const char *objtype, OSyncError **error)
{
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, archive, objtype, error);
	osync_assert(archive);
	osync_assert(objtype);

	if (!osync_archive_create_changelog(archive->db, objtype, error))
		goto error;
	
	char *query = g_strdup_printf("DELETE FROM tbl_changelog_%s", objtype);
	
	if (!osync_db_query(archive->db, query, error)) {
		g_free(query);
		goto error;
	}

	g_free(query);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/*@}*/
