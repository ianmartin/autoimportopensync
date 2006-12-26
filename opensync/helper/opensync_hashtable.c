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
#include "opensync_hashtable_internals.h"

#include "opensync-data.h"
#include "opensync-helper.h"
#include "opensync-db.h"


/**
 * @defgroup OSyncHashtableAPI OpenSync Hashtables
 * @ingroup OSyncPublic
 * @brief A Hashtable can be used to detect changes
 * 
 * Hashtables can be used to detect changes since the last invokation. They do this
 * by keeping track of all reported uids and the hashes of the objects.
 * 
 * Hashes are strings that change when the objects is updated or when the content of
 * the object changes. So hashes can either be a real hash like an MD5 or something 
 * like a timestamp. The only important thing is that the hash changes once the item
 * gets updated.
 * 
 * The hashtable works like this:
 * - You first malloc it with osync_hashtable_new()
 * - Then you load the saved hashtable from disk with osync_hashtable_load()
 * 
 * Now you can query and alter the table. You can ask if a item has changed by doing:
 * - osync_hashtable_get_changetype() to get the changetype of a certain uid and hash
 * - or the convience function osync_hashtable_detect_change which calls 
 * osync_hashtable_get_changetype() and sets this changetype on the change object and then
 * automatically calls osync_hashtable_report()
 * After you reported all objects you can query the table for the deleted objects using
 * osync_hashtable_get_deleted() or osync_hashtable_report_deleted()
 * 
 * After you are done call:
 * - osync_hashtable_free()
 * 
 * The hashtable works like this:
 * 
 * First the items are reported with a certain uid or hash. If the uid does not yet
 * exist in the database it is reported as ADDED. if the uid exists and the hash is different
 * it is reported as MODIFIED. if the uid exists but the hash is the same it means that the
 * object is UNMODIFIED.
 * 
 * To be able to report deleted objects the hashtables keeps track of the uids you reported.
 * After you are done with asking the hashtable for changes you can ask it for deleted objects.
 * All items that are in the hashtable but where not reported by you have to be DELETED.
 * 
 */
/*@{*/

osync_bool osync_hashtable_create(OSyncHashTable *table, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, table, objtype, error);

	char *query = g_strdup_printf("CREATE TABLE tbl_hash_%s (id INTEGER PRIMARY KEY, uid VARCHAR UNIQUE, hash VARCHAR)", objtype);
	if (!osync_db_query(table->dbhandle, query, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		g_free(query);
		return FALSE;
	}

	g_free(query);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/*! @brief Creates a new hashtable
 * 
 * Hashtables can be used to detect what has been changed since
 * the last sync
 * 
 * @returns A new hashtable
 * 
 */
OSyncHashTable *osync_hashtable_new(const char *path, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %s, %p)", __func__, path, objtype, error);
	
	OSyncHashTable *table = osync_try_malloc0(sizeof(OSyncHashTable), error);
	if (!table) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	table->used_entries = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	table->dbhandle = osync_db_new(error);
	if (!table->dbhandle)
		goto error;

	if (!osync_db_open(table->dbhandle, path, error))
		goto error_and_free;

	table->tablename = g_strdup_printf("tbl_hash_%s", objtype);

	int ret = osync_db_exists(table->dbhandle, table->tablename, error);
	if (ret > 0) {
		goto end;
	} else if (ret < 0) {
		goto error_and_free;
	}
	/* if ret == 0 then table does not exist yet. contiune and create one. */

	if (!osync_hashtable_create(table, objtype, error))
		goto error_and_free;

end:
	osync_trace(TRACE_EXIT, "%s: %p", __func__, table);
	return table;

error_and_free:	
	g_free(table->dbhandle);
	g_free(table);
error:	
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;

}

/*! @brief Frees a hashtable
 * 
 * @param table The hashtable to free
 * 
 */
void osync_hashtable_free(OSyncHashTable *table)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, table);
	osync_assert(table);
	
	if (!osync_db_close(table->dbhandle, NULL))
		osync_trace(TRACE_INTERNAL, "Can't close database");

		
	g_hash_table_destroy(table->used_entries);

	g_free(table->tablename);
	g_free(table->dbhandle);
	g_free(table);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Makes a hashtable forget
 * 
 * You can ask the hashtable to detect the changes. In the end you can
 * ask the hashtable for all items that have been deleted since the last sync.
 * For this the hashtable maintains a internal table of items you already reported and
 * reports the items it didnt see yet as deleted.
 * This function resets the internal table so it start to report deleted items again
 * 
 * @param table The hashtable
 * 
 */
void osync_hashtable_reset(OSyncHashTable *table)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, table);
	osync_assert(table);
	osync_assert(table->dbhandle);

	osync_db_reset(table->dbhandle, table->tablename, NULL);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Returns the number of entries in this hashtable
 * 
 * @param table The hashtable
 * @returns The number of entries, on error -1.
 * 
 */
int osync_hashtable_num_entries(OSyncHashTable *table)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, table);
	osync_assert(table);
	osync_assert(table->dbhandle);
	
	char *query = g_strdup_printf("SELECT * FROM %s", table->tablename);
	int ret = osync_db_count(table->dbhandle, query, NULL);
	g_free(query);
	
	if (ret < 0) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Cannot count number of hashtable entries!", __func__);
		return -1;
	}

	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

/*! @brief Gets the nth entry from the table
 * 
 * This is mainly usefull for debugging or special purposes
 * 
 * @param table The hashtable
 * @param nth The number of the entry to return
 * @param uid A pointer to a char * that will hold the uid. The caller is responible for freeing
 * @param hash A pointer to a char * that will hold the hash. The caller is responible for freeing
 * @returns TRUE if successful, FALSE otherwise
 * 
 */
osync_bool osync_hashtable_nth_entry(OSyncHashTable *table, int nth, char **uid, char **hash)
{
	osync_assert(table);
	osync_assert(table->dbhandle);
	
	GList *list = NULL;
	OSyncError *error = NULL;
	

	char *query = g_strdup_printf("SELECT uid, hash FROM %s LIMIT 1 OFFSET %i", table->tablename, nth);
	list = osync_db_query_table(table->dbhandle, query, &error);
	g_free(query);

	if (osync_error_is_set(&error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Cannot get #%i entry from hashtable: %s", __func__, nth, osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
		
	GList *column = list->data; 

	*uid = g_strdup((char*)g_list_nth_data(column, 0));
	*hash = g_strdup((char*)g_list_nth_data(column, 1));
	
	osync_db_free_list(list);

	return TRUE;
}

void osync_hashtable_write(OSyncHashTable *table, const char *uid, const char *hash)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s)", __func__, table, uid, hash);
	osync_assert(table);
	osync_assert(table->dbhandle);
	
	char *query = g_strdup_printf("REPLACE INTO %s ('uid', 'hash') VALUES('%s', '%s')", table->tablename, uid, hash);
	if (!osync_db_query(table->dbhandle, query, NULL)) {
		g_free(query);
		osync_trace(TRACE_EXIT, "%s: Cannot write hashtable entry.", __func__);
		return;
	}
	g_free(query);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_hashtable_delete(OSyncHashTable *table, const char *uid)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, table, uid);
	osync_assert(table);
	osync_assert(table->dbhandle);
	
	char *query = g_strdup_printf("DELETE FROM %s WHERE uid='%s'", table->tablename, uid);
	if (!osync_db_query(table->dbhandle, query, NULL)) {
		g_free(query);
		osync_trace(TRACE_EXIT_ERROR, "%s: Cannot delete hashtable entry.", __func__);
		return;
	}
	g_free(query);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Update the hash for a entry
 * 
 * Updates the hash for a entry in the hashtable. Do this after you see that a hash
 * has changed, for example after reading it during get_changes or after you
 * wrote it
 * 
 * @param table The hashtable
 * @param change The change with the new hash information
 */
void osync_hashtable_update_hash(OSyncHashTable *table, OSyncChangeType type, const char *uid, const char *hash)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %s)", __func__, table, type, uid, hash);
	osync_assert(table);
	osync_assert(table->dbhandle);

	switch (type) {
		case OSYNC_CHANGE_TYPE_DELETED:
			osync_hashtable_delete(table, uid);
			break;
		case OSYNC_CHANGE_TYPE_UNMODIFIED:
		case OSYNC_CHANGE_TYPE_UNKNOWN:
		case OSYNC_CHANGE_TYPE_MODIFIED:
		case OSYNC_CHANGE_TYPE_ADDED:
			osync_hashtable_write(table, uid, hash);
			break;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Report a item
 * 
 * When you use this function the item is marked as reported, so it will not get
 * listed as deleted. Use this function if there are problems accessing an object for
 * example so that the object does not get reported as deleted accidently.
 * 
 * @param table The hashtable
 * @param uid The uid to report
 * 
 */
void osync_hashtable_report(OSyncHashTable *table, const char *uid)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, table, uid);
	osync_assert(table);
	osync_assert(table->dbhandle);
	
	g_hash_table_insert(table->used_entries, g_strdup(uid), GINT_TO_POINTER(1));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Get the uid of all deleted items
 * 
 * @param table The hashtable
 * @param objtype The object type which to report, NULL for all
 * @returns An Null terminated array of uids. The uids and this array have to be freed.
 * 
 */
char **osync_hashtable_get_deleted(OSyncHashTable *table)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, table);

	osync_assert(table);
	osync_assert(table->dbhandle);

	GList *row = NULL, *result = NULL;

	char *query = g_strdup_printf("SELECT uid FROM %s", table->tablename);
	result = osync_db_query_table(table->dbhandle, query, NULL); 
	g_free(query);

	int numrows = g_list_length(result);
	char **ret = g_malloc0((numrows + 1) * sizeof(char *));
	
	int num = 0;
	for (row = result; row; row = row->next) {
		GList *column = row->data;

		const char *uid = (const char *) g_list_nth_data(column, 0);

		if (!g_hash_table_lookup(table->used_entries, uid))
			ret[num++] = g_strdup(uid);
	}

	osync_db_free_list(result);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, ret);
	return ret;
}

/*! @brief Gets the changetype for a given uid and hash
 * 
 * This functions does not report the object so if you only use this function
 * it will get reported as deleted! Please use osync_hashtable_report() for reporting
 * and object.
 * 
 * @param table The hashtable
 * @param uid The uid to lookup
 * @param hash The hash to compare
 * @returns The changetype
 * 
 */
OSyncChangeType osync_hashtable_get_changetype(OSyncHashTable *table, const char *uid, const char *hash)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s)", __func__, table, uid, hash);
	osync_assert(table);
	osync_assert(table->dbhandle);

	char *orighash = NULL;
	
	OSyncChangeType retval = OSYNC_CHANGE_TYPE_UNMODIFIED;

	char *query = g_strdup_printf("SELECT hash FROM %s WHERE uid='%s'", table->tablename, uid);
	orighash = osync_db_query_single_string(table->dbhandle, query, NULL); 
	g_free(query);
	
	osync_trace(TRACE_INTERNAL, "Comparing %s with %s", hash, orighash);
	
	if (orighash) {
		if (!strcmp(hash, orighash))
			retval = OSYNC_CHANGE_TYPE_UNMODIFIED;
		else
			retval = OSYNC_CHANGE_TYPE_MODIFIED;
	} else
		retval = OSYNC_CHANGE_TYPE_ADDED;

	g_free(orighash);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, retval);
	return retval;
}

/*@}*/
