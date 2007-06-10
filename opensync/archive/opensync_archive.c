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

osync_bool osync_archive_create_changes(OSyncDB *db, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, db, error); 
	osync_assert(db);

	char *query = g_strdup("CREATE TABLE tbl_changes (id INTEGER PRIMARY KEY, uid VARCHAR, objtype VARCHAR, memberid INTEGER, mappingid INTEGER)");
	if (!osync_db_query(db, query, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		g_free(query);
		return FALSE;
	}
	g_free(query);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool osync_archive_create_changelog(OSyncDB *db, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, db, error); 
	osync_assert(db);

	char *query = g_strdup("CREATE TABLE tbl_changelog (id INTEGER PRIMARY KEY, entryid INTEGER, changetype INTEGER, objtype VARCHAR)");
	if (!osync_db_query(db, query, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		g_free(query);
		return FALSE;
	}
	g_free(query);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool osync_archive_create(OSyncDB *db, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, db, error); 

	char *query = g_strdup("CREATE TABLE tbl_archive (mappingid INTEGER PRIMARY KEY, data BLOB)");
	if (!osync_db_query(db, query, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		g_free(query);
		return FALSE;
	}
	g_free(query);

	osync_trace(TRACE_EXIT, "%s", __func__); 
	return TRUE;
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

	/* tbl_archive */
	int ret = osync_db_exists(archive->db, "tbl_archive", error);

	/* error if ret -1 */
	if (ret < 0)
		goto error_and_free;
	/* if ret == 0 table does not exist. continue and create it */
	if (ret == 0 && !osync_archive_create(archive->db, error)) {
		g_free(archive->db);
		goto error_and_free;
	}

	/* tbl_changes */
	ret = osync_db_exists(archive->db, "tbl_changes", error);

	/* error if ret -1 */
	if (ret < 0)
		goto error_and_free;
	/* if ret == 0 table does not exist. continue and create it */

	if (ret == 0 && !osync_archive_create_changes(archive->db, error)) {
		g_free(archive->db);
		goto error_and_free;
	}

	/* tbl_changelog */
	ret = osync_db_exists(archive->db, "tbl_changelog", error);

	/* error if ret -1 */
	if (ret < 0)
		goto error_and_free;
	/* if ret == 0 table does not exist. continue and create it */
	if (ret == 0 && !osync_archive_create_changelog(archive->db, error)) {
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
 * @brief Store data of an entry in the group archive database (blob).
 *
 * @param archive The group archive
 * @param uid UID of requested entry
 * @param data The data to store 
 * @param size Total size of data 
 * @param error Pointer to an error struct
 * @return Returns TRUE on success otherwise FALSE
 */ 
osync_bool osync_archive_save_data(OSyncArchive *archive, const char *uid, const char *data, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %u, %p)", __func__, archive, uid, data, size, error);
	osync_assert(archive);
	osync_assert(uid);
	osync_assert(data);
	osync_assert(size);

	char *escaped_uid = _osync_archive_sql_escape(uid);
	char *query = g_strdup_printf("REPLACE INTO tbl_archive (mappingid, data) VALUES((SELECT mappingid FROM tbl_changes WHERE uid='%s' LIMIT 1), ?)", uid);
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
 * @brief Load data of an entry which is stored in the group archive database (blob).
 *
 * @param archive The group archive
 * @param uid UID of requestd entry
 * @param data Pointer to store the requested data 
 * @param size Pointer to store the size of requested data
 * @param error Pointer to an error struct
 * @return Returns 0 if no data is present else 1. On error -1.
 */ 
int osync_archive_load_data(OSyncArchive *archive, const char *uid, char **data, unsigned int *size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p, %p)", __func__, archive, uid, data, size, error);
	osync_assert(archive);
	osync_assert(uid);
	osync_assert(data);
	osync_assert(size);

	char *query = g_strdup_printf("SELECT data FROM tbl_archive WHERE mappingid=(SELECT mappingid FROM tbl_changes WHERE uid='%s' LIMIT 1)", uid);
	int ret = osync_db_get_blob(archive->db, query, data, size, error);
	g_free(query);

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
 * @brief Save an entry in the group archive. 
 *
 * @param archive The group archive
 * @param id Archive (database) id of entry which gets deleted
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

	char *query = NULL;
	char *escaped_uid = _osync_archive_sql_escape(uid);

	if (!id) {
		query = g_strdup_printf("INSERT INTO tbl_changes (uid, objtype, mappingid, memberid) VALUES('%s', '%s', '%lli', '%lli')", escaped_uid, objtype, mappingid, memberid);
	} else {
		query = g_strdup_printf("UPDATE tbl_changes SET uid='%s', objtype='%s', mappingid='%lli', memberid='%lli' WHERE id=%lli", escaped_uid, objtype, mappingid, memberid, id);
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
 * @brief Delete certain entry form group archive. 
 *
 * @param archive The group archive
 * @param id Archive (database) id of entry which gets deleted
 * @param error Pointer to an error struct
 * @return TRUE on when all changes successfully loaded otherwise FALSE
 */ 
osync_bool osync_archive_delete_change(OSyncArchive *archive, long long int id, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %lli, %p)", __func__, archive, id, error);
	osync_assert(archive);
	
	char *query = g_strdup_printf("DELETE FROM tbl_changes WHERE id=%lli", id);
	if (!osync_db_query(archive->db, query, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		g_free(query);
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

	char *query = g_strdup_printf("SELECT id, uid, mappingid, memberid FROM tbl_changes WHERE objtype='%s' ORDER BY mappingid", objtype);
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
 * @brief Get the object type for an entry of the group archive.
 *
 * @param archive The group archive
 * @param memberid The member id
 * @param uid The uid of the entry
 * @param error Pointer to an error struct 
 * @return Returns the object type of the specified entry, or NULL if the entry doesn't exist. (the caller is responsible for freeing) 
 */
char *osync_archive_get_objtype(OSyncArchive *archive, long long int memberid, const char *uid, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, archive, uid, error);

	osync_assert(archive);
	osync_assert(uid);
	
	char *query = g_strdup_printf("SELECT objtype FROM tbl_changes WHERE memberid='%lli' AND uid='%s'", memberid, uid);
	char *objtype = osync_db_query_single_string(archive->db, query, error);
	g_free(query);

	/* TODO improve error handling... */
	if (osync_error_is_set(error)) {
		goto error;
	}

	
	osync_trace(TRACE_EXIT, "%s: %s", __func__, objtype);
	return objtype;
	
error:
	g_free(objtype);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/**
 * @brief Load all conficting changes which got ignored in previous sync. 
 *
 * @param archive The group archive
 * @param objtype Requested object type 
 * @param ids List unique databse entry id 
 * @param changetypes List to store changetypes for each entry
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

	char *query = g_strdup_printf("SELECT entryid, changetype FROM tbl_changelog WHERE objtype='%s' ORDER BY id", objtype);
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
 * @brief Save an entry in the ignored conflict list.
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
	
	char *query = g_strdup_printf("INSERT INTO tbl_changelog (entryid, changetype, objtype) VALUES('%lli', '%i', '%s')", id, changetype, objtype);
	
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
 * @brief Delete all ignored conflict entries of the changelog with the objtype.
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
	
	char *query = g_strdup_printf("DELETE FROM tbl_changelog WHERE objtype=\"%s\"", objtype);
	
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
