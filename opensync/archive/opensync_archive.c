/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2006  NetNix Finland Ltd <netnix@netnix.fi>
 * Copyright (C) 2008  Michael Bell <michael.bell@opensync.org>
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
#include "opensync_archive_private.h"
#include "opensync_archive_internals.h"
#include "opensync-db.h"

static osync_bool osync_archive_create_changes(OSyncDB *db, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, objtype, error); 

	osync_assert(db);
	osync_assert(objtype);
	osync_assert(strlen(objtype) <= 64);

	int ret = osync_db_table_exists(db, "tbl_changes", error);

	/* error if ret -1 */
	if (ret < 0)
		goto error;

	/* if ret is != 0 then the table already exist */
	if (ret) {
		osync_trace(TRACE_EXIT, "%s", __func__);
		return TRUE;
	}

	const char *query = "CREATE TABLE tbl_changes (objtype VARCHAR(64), id INTEGER, uid VARCHAR, memberid INTEGER, mappingid INTEGERi, PRIMARY KEY (objtype, id) )";
	if (!osync_db_query(db, query, error)) {
		goto error;
	}

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
	osync_assert(strlen(objtype) <= 64);

	int ret = osync_db_table_exists(db, "tbl_changelog", error);

	/* error if ret -1 */
	if (ret < 0)
		goto error;

	/* if ret != 0 table does not exist. continue and create it */
	if (ret) {
		osync_trace(TRACE_EXIT, "%s", __func__);
		return TRUE;
	}

	const char *query = "CREATE TABLE tbl_changelog (objtype VARCHAR(64), id INTEGER, entryid INTEGER, changetype INTEGER, PRIMARY KEY (objtype, id) )";
	if (!osync_db_query(db, query, error)) {
		goto error;
	}

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
	osync_assert(strlen(objtype) <= 64);

	int ret = osync_db_table_exists(db, "tbl_archive", error);

	/* error if ret -1 */
	if (ret < 0)
		goto error;

	/* if ret != 0 table does not exist. continue and create it */
	if (ret) {
		osync_trace(TRACE_EXIT, "%s", __func__); 
		return TRUE;
	}


	const char *query = "CREATE TABLE tbl_archive (objtype VARCHAR(64), mappingid INTEGER, data BLOB, PRIMARY KEY (objtype, mappingid) )";
	if (!osync_db_query(db, query, error)) {
		goto error;
	}

	osync_trace(TRACE_EXIT, "%s: created table.", __func__); 
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

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

OSyncArchive *osync_archive_ref(OSyncArchive *archive)
{
	osync_assert(archive);
	
	g_atomic_int_inc(&(archive->ref_count));

	return archive;
}

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

osync_bool osync_archive_save_data(OSyncArchive *archive, long long int id, const char *objtype, const char *data, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %lli, %s, %p, %u, %p)", __func__, archive, id, objtype, data, size, error);
	osync_assert(archive);
	osync_assert(data);
	osync_assert(size);

	if (!osync_archive_create(archive->db, objtype, error))
		goto error;

	// FIXME: Avoid subselect - this query needs up to 0.5s
	char *escaped_objtype = osync_db_sql_escape(objtype);
	char *query = g_strdup_printf("REPLACE INTO tbl_archive (objtype, mappingid, data) VALUES('%s', %lli, ?)", escaped_objtype, id);
	g_free(escaped_objtype);
	escaped_objtype = NULL;
	
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

int osync_archive_load_data(OSyncArchive *archive, const char *uid, const char *objtype, char **data, unsigned int *size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s, %p, %p, %p)", __func__, archive, uid, objtype, data, size, error);
	osync_assert(archive);
	osync_assert(uid);
	osync_assert(data);
	osync_assert(size);

	if (!osync_archive_create(archive->db, objtype, error))
		goto error;

	char *escaped_uid = osync_db_sql_escape(uid);
	char *escaped_objtype = osync_db_sql_escape(objtype);
	char *query = g_strdup_printf("SELECT data FROM tbl_archive WHERE objtype='%s' AND mappingid=(SELECT mappingid FROM tbl_changes WHERE objtype='%s' AND uid='%s' LIMIT 1)", escaped_objtype, escaped_objtype, escaped_uid);
	g_free(escaped_objtype);
	escaped_objtype = NULL;
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

long long int osync_archive_save_change(OSyncArchive *archive, long long int id, const char *uid, const char *objtype, long long int mappingid, long long int memberid, OSyncError **error)
{
	
	osync_trace(TRACE_ENTRY, "%s(%p, %lli, %s, %s, %lli, %lli, %p)", __func__, archive, id, uid, objtype, mappingid, memberid, error);
	osync_assert(archive);
	osync_assert(uid);
	osync_assert(objtype);

	if (!osync_archive_create_changes(archive->db, objtype, error))
		goto error;

	char *query = NULL;
	char *escaped_uid = osync_db_sql_escape(uid);
	char *escaped_objtype = osync_db_sql_escape(objtype);

	if (!id) {
		query = g_strdup_printf("INSERT INTO tbl_changes (objtype, uid, mappingid, memberid) VALUES('%s', '%s', '%lli', '%lli')", escaped_objtype, escaped_uid, mappingid, memberid);
	} else {
		query = g_strdup_printf("UPDATE tbl_changes SET uid='%s', mappingid='%lli', memberid='%lli' WHERE objtype='%s' AND id=%lli", escaped_uid, mappingid, memberid, escaped_objtype, id);
	}
	g_free(escaped_objtype);
	g_free(escaped_uid);
	escaped_objtype = NULL;
	escaped_uid = NULL;
	
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

osync_bool osync_archive_delete_change(OSyncArchive *archive, long long int id, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %lli, %s, %p)", __func__, archive, id, objtype, error);
	osync_assert(archive);
	osync_assert(objtype);

	if (!osync_archive_create_changes(archive->db, objtype, error))
		goto error;

	char *escaped_objtype = osync_db_sql_escape(objtype);
	char *query = g_strdup_printf("DELETE FROM tbl_changes WHERE objtype='%s' AND id=%lli", escaped_objtype, id);
	g_free(escaped_objtype);
	escaped_objtype = NULL;
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

osync_bool osync_archive_load_changes(OSyncArchive *archive, const char *objtype, OSyncList **ids, OSyncList **uids, OSyncList **mappingids, OSyncList **memberids, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p, %p, %p, %p)", __func__, archive, objtype, ids, uids, mappingids, memberids, error);

	osync_assert(archive);
	osync_assert(objtype);
	osync_assert(ids);
	osync_assert(uids);
	osync_assert(mappingids);
	osync_assert(memberids);

	OSyncList *result = NULL, *row = NULL;

	if (!osync_archive_create_changes(archive->db, objtype, error))
		goto error;

	char *escaped_objtype = osync_db_sql_escape(objtype);
	char *query = g_strdup_printf("SELECT id, uid, mappingid, memberid FROM tbl_changes WHERE objtype='%s' ORDER BY mappingid", escaped_objtype);
	g_free(escaped_objtype);
	escaped_objtype = NULL;
	result = osync_db_query_table(archive->db, query, error);

	g_free(query);
	
	/* Check for error of osync_db_query_table() call. */
	if (osync_error_is_set(error))
		goto error;

	for (row = result; row; row = row->next) { 
		OSyncList *column = row->data;

		long long int id = g_ascii_strtoull(osync_list_nth_data(column, 0), NULL, 0);
		const char *uid = osync_list_nth_data(column, 1); 
		long long int mappingid = g_ascii_strtoull(osync_list_nth_data(column, 2), NULL, 0);
		long long int memberid = g_ascii_strtoull(osync_list_nth_data(column, 3), NULL, 0);
		
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

osync_bool osync_archive_flush_changes(OSyncArchive *archive, const char *objtype, OSyncError **error)
{
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, archive, objtype, error);
	osync_assert(archive);
	osync_assert(objtype);

	if (!osync_archive_create_changes(archive->db, objtype, error))
		goto error;
	
	char *escaped_objtype = osync_db_sql_escape(objtype);
	char *query = g_strdup_printf("DELETE FROM tbl_changes WHERE objtype='%s'", escaped_objtype);
	g_free(escaped_objtype);
	escaped_objtype = NULL;
	
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

osync_bool osync_archive_load_ignored_conflicts(OSyncArchive *archive, const char *objtype, OSyncList **ids, OSyncList **changetypes, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p)", __func__, archive, objtype, ids, error);

	osync_assert(archive);
	osync_assert(objtype);
	osync_assert(ids);
	osync_assert(changetypes);

	OSyncList *result = NULL;
	OSyncList *row = NULL;

	if (!osync_archive_create_changelog(archive->db, objtype, error))
		goto error;

	char *escaped_objtype = osync_db_sql_escape(objtype);
	char *query = g_strdup_printf("SELECT entryid, changetype FROM tbl_changelog WHERE objtype='%s' ORDER BY id", escaped_objtype);
	g_free(escaped_objtype);
	escaped_objtype = NULL;
	result = osync_db_query_table(archive->db, query, error);

	g_free(query);
	
	/* Check for error of osync_db_query_table() call. */
	if (osync_error_is_set(error))
		goto error;

	for (row = result; row; row = row->next) { 
		OSyncList *column = row->data;

		long long int id = g_ascii_strtoull(osync_list_nth_data(column, 0), NULL, 0);
		int changetype = atoi(osync_list_nth_data(column, 1));
		
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

osync_bool osync_archive_save_ignored_conflict(OSyncArchive *archive, const char *objtype, long long int id, OSyncChangeType changetype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %lli, %p)", __func__, archive, objtype, id, error);

	osync_assert(archive);
	osync_assert(objtype);

	if (!osync_archive_create_changelog(archive->db, objtype, error))
		goto error;
	
	char *escaped_objtype = osync_db_sql_escape(objtype);
	char *query = g_strdup_printf("INSERT INTO tbl_changelog (objtype, entryid, changetype) VALUES('%s', '%lli', '%i')", escaped_objtype, id, changetype);
	g_free(escaped_objtype);
	escaped_objtype = NULL;
	
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

osync_bool osync_archive_flush_ignored_conflict(OSyncArchive *archive, const char *objtype, OSyncError **error)
{
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, archive, objtype, error);
	osync_assert(archive);
	osync_assert(objtype);

	if (!osync_archive_create_changelog(archive->db, objtype, error))
		goto error;
	
	char *escaped_objtype = osync_db_sql_escape(objtype);
	char *query = g_strdup_printf("DELETE FROM tbl_changelog WHERE objtype='%s'", escaped_objtype);
	g_free(escaped_objtype);
	escaped_objtype = NULL;
	
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

