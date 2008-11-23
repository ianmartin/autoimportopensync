/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2008       Daniel Gollub <dgollub@suse.de>
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
 * @ingroup OSyncPrivate
 * @brief A Hashtable can be used to detect changes
 */

/*@{*/

/*! @brief Creates the database table for the hashtable 
 * 
 * @param table The hashtable
 * @param ame The name of the hastable inside the database 
 * @param error An error struct
 * @returns TRUE on success, or FALSE if an error occurred.
 * 
 */

static osync_bool osync_hashtable_create(OSyncHashTable *table, OSyncError **error)
{
	osync_assert(table);
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, table, error);

	char *query = g_strdup_printf("CREATE TABLE %s (id INTEGER PRIMARY KEY, uid VARCHAR UNIQUE, hash VARCHAR)", table->name);
	if (!osync_db_query(table->dbhandle, query, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		g_free(query);
		return FALSE;
	}

	g_free(query);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

#if !GLIB_CHECK_VERSION(2,12,0)
/*! \brief g_hash_table_foreach_remove foreach function
 */
static gboolean remove_entry(gpointer key, gpointer val, gpointer data)
{
   return TRUE;
}
#endif

/*! @brief Makes a hashtable forget reported entries
 * 
 * You can ask the hashtable to detect the changes. In the end you can
 * ask the hashtable for all items that have been deleted since the last sync.
 * For this the hashtable maintains a internal table of items you already reported and
 * reports the items it didn't see yet as deleted.
 * This function resets the internal table so it start to report deleted items again
 * 
 * @param table The hashtable
 * 
 */
static void osync_hashtable_reset_reports(OSyncHashTable *table)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, table);
	osync_assert(table);
	osync_assert(table->dbhandle);

	/* Only free the internal hashtable of reported entries.
	   Don't flush the real database. */
#if GLIB_CHECK_VERSION(2,12,0)
	g_hash_table_remove_all(table->reported_entries);
#else
	g_hash_table_foreach_remove(table->reported_entries, remove_entry, NULL);
#endif
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Makes hashtable in memory forget, not the persistent one
 * 
 * @param table The hashtable
 * 
 */
static void osync_hashtable_reset(OSyncHashTable *table)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, table);
	osync_assert(table);
	osync_assert(table->dbhandle);

	/* Only free the internal hashtable of reported entries.
	   Don't flush the real database. */
#if GLIB_CHECK_VERSION(2,12,0)
	g_hash_table_remove_all(table->db_entries);
#else
	g_hash_table_foreach_remove(table->db_entries, remove_entry, NULL);
#endif
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}


/*! @brief Report a item
 * 
 * When you use this function the item is marked as reported, so it will not get
 * listed as deleted. Use this function if there are problems accessing an object for
 * example so that the object does not get reported as deleted accidentally.
 * 
 * @param table The hashtable
 * @param change The change to report
 * 
 */
static void osync_hashtable_report(OSyncHashTable *table, OSyncChange *change)
{
	osync_assert(table);
	osync_assert(table->dbhandle);
	osync_assert(change);

	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, table, change);
	
	/* uid get freed when hashtable get's destroyed */
	char *uid = g_strdup(osync_change_get_uid(change));

	g_hash_table_insert(table->reported_entries, uid, GINT_TO_POINTER(1));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/* TODO */
static void _osync_hashtable_prepare_insert_query(const char *uid, const char *hash, void *user_data)
{
	OSyncHashTable *table = user_data;

	char *escaped_uid = osync_db_sql_escape(uid);
	char *escaped_hash = osync_db_sql_escape(hash);

	g_string_append_printf(table->query, 
			"REPLACE INTO %s ('uid', 'hash') VALUES('%s', '%s');", 
			table->name, escaped_uid, escaped_hash);

	g_free(escaped_uid);
	g_free(escaped_hash);
}

/*@}*/

/**
 * @defgroup OSyncHashtableAPI OpenSync Hashtables
 * @ingroup OSyncPublic
 * @brief A Hashtable can be used to detect changes
 * 
 * Hashtables can be used to detect changes since the last invocation. They do this
 * by keeping track of all reported uids and the hashes of the objects.
 * 
 * A hash is a string that changes when an object is updated or when the content of
 * the object changes. So hashes can either be a real hash like an MD5, or something 
 * like a timestamp. The only important thing is that the hash changes when the item
 * gets updated.
 * 
 * The hashtable is created from a .db file using the osync_hashtable_new() function.
 *
 * With osync_hashtable_load() the persinent database gets read and loads all hashtable
 * entries into memory.
 * 
 * Now you can query and alter the hashtable in memory. You can ask if a item has changed 
 * by doing:
 *
 * - osync_hashtable_get_changetype() 
 *   To get the changetype of a certain OSyncChange object. Don't forget to update the hash for 
 *   the change in advance. Update your OSyncChange with this detect changetype with
 *   osync_change_set_changetype()
 *
 * - osync_hashtable_update_change()
 *   When the changetype got updated for the OSyncChange object, update the hash entry with
 *   calling osync_hashtable_update_change(). Call this function even if the entry has changetype
 *   unmodified. Otherwise the hashtable will report this entry later as deleted.
 *  
 * - osync_hashtable_get_deleted()
 *   Once all available changes got reported call osync_hashtable_get_deleted() to get an OSyncList
 *   of changes which got deleted since last sync. Entries get determined as deleted if they
 *   got not reported as osync_hashtable_update_change(), independent of the changetype.
 *
 * - osync_hashtable_save()
 *   For performance reason the hashtable in memory got only stored persistence with calling
 *   osync_hashtable_save(). Call this function everytime when the synchronization finished.
 *   This is usually inside the sync_done() function.
 * 
 * After you are finished using the hashtable, call:
 * - osync_hashtable_unref()
 * 
 * The hashtable works like this:
 * 
 * First the items are reported with a certain uid or hash. If the uid does not yet
 * exist in the database it is reported as ADDED. If the uid exists and the hash is different
 * it is reported as MODIFIED. If the uid exists but the hash is the same it means that the
 * object is UNMODIFIED.
 * 
 * To be able to report deleted objects the hashtables keeps track of the uids you reported.
 * After you are done with asking the hashtable for changes you can ask it for deleted objects.
 * All items that are in the hashtable but where not reported by you have to be DELETED.
 * 
 */
/*@{*/

/*! @brief Loads or creates a hashtable
 * 
 * Hashtables can be used to detect what has been changed since
 * the last sync
 * 
 * @param path the full path and file name of the hashtable .db file to load from or create
 * @param objtype the object type of the hashtable
 * @param error An error struct
 * @returns A new hashtable, or NULL if an error occurred.
 * 
 */
OSyncHashTable *osync_hashtable_new(const char *path, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, path, error);
	
	OSyncHashTable *table = osync_try_malloc0(sizeof(OSyncHashTable), error);
	if (!table)
		goto error;

	table->ref_count = 1;
	

	table->reported_entries = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	table->db_entries = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	table->dbhandle = osync_db_new(error);
	if (!table->dbhandle)
		goto error_and_free_db;

	if (!osync_db_open(table->dbhandle, path, error))
		goto error_and_free;

	table->name = g_strdup_printf(OSYNC_HASHTABLE_DB_PREFIX"%s", objtype);

	int ret = osync_db_table_exists(table->dbhandle, table->name, error);
	/* greater then 0 means evrything is O.k. */

	if (ret < 0)
		goto error;
	else if (ret == 0)
		/* if ret == 0 then table does not exist yet. contiune and create one. */
		if (!osync_hashtable_create(table, error))
			goto error;


	osync_trace(TRACE_EXIT, "%s: %p", __func__, table);
	return table;

error_and_free_db:	
	g_free(table->dbhandle);
error_and_free:
	g_free(table);
error:	
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;

}

/*! @brief Increase the refernece count of the hashtable.
 *
 * @param table The hashtable to increase the reference count
 * @returns Pointer to increased hashtable object
 */
OSyncHashTable *osync_hashtable_ref(OSyncHashTable *table)
{
	osync_assert(table);
	
	g_atomic_int_inc(&(table->ref_count));

	return table;
}

/*! @brief Decrease the reference count of the hastable. Hashtable
 *         gets freed if the reference count get less then one. 
 * 
 * @param table The hashtable to decrease the reference count 
 * 
 */
void osync_hashtable_unref(OSyncHashTable *table)
{
	osync_assert(table);

	if (g_atomic_int_dec_and_test(&(table->ref_count))) {
		osync_trace(TRACE_ENTRY, "%s(%p)", __func__, table);

		OSyncError *error = NULL;
		
		if (!osync_db_close(table->dbhandle, &error)) {
			osync_trace(TRACE_ERROR, "Couldn't close database: %s", osync_error_print(&error));
			osync_error_unref(&error);
		}
			
		g_hash_table_destroy(table->reported_entries);
		g_hash_table_destroy(table->db_entries);

		g_free(table->name);
		g_free(table->dbhandle);
		g_free(table);

		osync_trace(TRACE_EXIT, "%s", __func__);
	}
	
}

osync_bool osync_hashtable_load(OSyncHashTable *table, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, table, error);

	char *query;
	OSyncList *row, *result;

	query = g_strdup_printf("SELECT uid, hash FROM %s", table->name);
	result = osync_db_query_table(table->dbhandle, query, error); 
	g_free(query);

	/* If result is NULL, this means no entries - just check for error. */
	if (osync_error_is_set(error))
		goto error;

	for (row = result; row; row = row->next) {
		OSyncList *column = row->data;

		char *uid =  g_strdup(osync_list_nth_data(column, 0));
		char *hash = g_strdup(osync_list_nth_data(column, 1)); 

		g_hash_table_insert(table->db_entries, uid, hash);
	}
	osync_list_free(result);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_hashtable_save(OSyncHashTable *table, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, table, error);

	osync_bool ret;

	/* Should only be used by this function */
	osync_assert(!table->query);

	table->query = g_string_new("BEGIN TRANSACTION;");
	g_string_append_printf(table->query, "DELETE FROM %s;", table->name);

	osync_hashtable_foreach(table, _osync_hashtable_prepare_insert_query, table); 

	table->query = g_string_append(table->query, "COMMIT TRANSACTION;");

	char *query = g_string_free(table->query, FALSE);
	ret = osync_db_query(table->dbhandle, query, error);
	g_free(query);

	table->query = NULL;

	if (!ret)
		goto error;

	osync_hashtable_reset_reports(table);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


/*! @brief Prepares the hashtable for a slowsync and flush the entire hashtable
 * 
 * This function should be called to prepare the hashtable for a slowsync.
 * The entire database, which stores the values of the hashtable beyond the
 * synchronization, gets flushed.
 * 
 * @param table The hashtable
 * @param error An error struct
 * @returns TRUE on success, or FALSE if an error occurred.
 * 
 */
osync_bool osync_hashtable_slowsync(OSyncHashTable *table, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, table, error);
	osync_assert(table);
	osync_assert(table->dbhandle);

	/* Reset persistent hashtable in database */
	if (!osync_db_reset_table(table->dbhandle, table->name, error))
		goto error;

	/* Reset hashtable in memory */
	osync_hashtable_reset(table);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;

}

/*! @brief Update the an entry
 * 
 * Updates the entry in the hashtable. Use this even if the change entry
 * is unmodified! Usually this function get called in get_changes(). In some
 * rare cases this get even called inside of the commit() plugin functions,
 * to update the UID inside the hashtable of a changed entry.
 * 
 * @param table The hashtable
 * @param type The type of change (added, modified, etc.)
 */
void osync_hashtable_update_change(OSyncHashTable *table, OSyncChange *change)
{
	osync_assert(table);
	osync_assert(table->dbhandle);
	osync_assert(change);

	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, table, change);
	
	const char *uid = osync_change_get_uid(change);
	const char *hash = osync_change_get_hash(change);

	osync_assert_msg(uid, "Some plugin forgot to set the UID for the change. Please report this bug.");

	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_DELETED:
			g_hash_table_remove(table->db_entries, uid);
			break;
		case OSYNC_CHANGE_TYPE_UNMODIFIED:
			/* Nothing to do. Just ignore. */
			break;
		case OSYNC_CHANGE_TYPE_UNKNOWN:
			/* Someone violets against the rules of the hashtable API!
			 
			   Changetype needs to get set before calling this function!
			   Even if the change entry got not modified, then the change type
			   should get set at least to OSYNC_CHANGE_TYPE_UNMODIFIED. Otherwise:
			
			   BOOOOOOOOOOOOOOOOOOOOOM!
			 
			 */
			osync_assert_msg(FALSE, "Got called with unknown changetype. This looks like a plugin makes wrong use of a hashtable. Please, contact the plugin author!");
			break;
		case OSYNC_CHANGE_TYPE_MODIFIED:
			osync_assert_msg(hash, "Some plugin forgot to set the HASH for the change for the changetype MODIFIED. Please report this bug.");
			/* This works even if the UID/key is new to the hashtable */
			g_hash_table_replace(table->db_entries, g_strdup(uid), g_strdup(hash));
			break;
		case OSYNC_CHANGE_TYPE_ADDED:
			osync_assert_msg(hash, "Some plugin forgot to set the HASH for the change for the changetype ADDED. Please report this bug.");
			g_hash_table_insert(table->db_entries, g_strdup(uid), g_strdup(hash));
			break;
	}

	osync_hashtable_report(table, change);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Get a list of uids which deleted 
 * 
 * @param table The hashtable
 * @returns OSyncList containing UIDs of deleted entries. Caller is responsible for freeing the ist,
 *          not the content, with osync_list_free() .
 * 
 */
struct callback_data {
  OSyncList *deleted_entries;
  OSyncHashTable *table;
};
static void callback_check_deleted(gpointer key, gpointer value, gpointer user_data)
{
  	struct callback_data *cbdata = user_data;
	if (!g_hash_table_lookup(cbdata->table->reported_entries, key))
	  cbdata->deleted_entries = osync_list_prepend(cbdata->deleted_entries, key);
}
OSyncList *osync_hashtable_get_deleted(OSyncHashTable *table)
{
	osync_assert(table);
	osync_assert(table->dbhandle);

	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, table);

  	struct callback_data cbdata = {NULL, table};

	g_hash_table_foreach(table->db_entries, callback_check_deleted, &cbdata);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, cbdata.deleted_entries);
	return cbdata.deleted_entries;
}

/*! @brief Gets the changetype for the given OSyncChange object, by comparing the hashs
 *         of the hashtable and OSyncChange object.
 * 
 * This function does not report the object so if you only use this function then
 * the object will get reported as deleted! Please use osync_hashtable_report() for reporting
 * an object.
 * 
 * @param table The hashtable
 * @param uid The uid to lookup
 * @param newhash The new hash to compare with the stored hash
 * @returns The changetype
 * 
 */
OSyncChangeType osync_hashtable_get_changetype(OSyncHashTable *table, OSyncChange *change)
{
	osync_assert(table);
	osync_assert(table->dbhandle);
	osync_assert(change);

	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, table, change);

	OSyncChangeType retval = OSYNC_CHANGE_TYPE_UNKNOWN;

	const char *uid = osync_change_get_uid(change);
	const char *newhash = osync_change_get_hash(change);

	const char *orighash = osync_hashtable_get_hash(table, uid);
	
	if (orighash) {
		if (!strcmp(newhash, orighash))
			retval = OSYNC_CHANGE_TYPE_UNMODIFIED;
		else
			retval = OSYNC_CHANGE_TYPE_MODIFIED;
	} else
		retval = OSYNC_CHANGE_TYPE_ADDED;

	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, retval);
	return retval;
}

unsigned int osync_hashtable_num_entries(OSyncHashTable *table)
{
	osync_assert(table);
	return g_hash_table_size(table->db_entries);
}

void osync_hashtable_foreach(OSyncHashTable *table, OSyncHashtableForEach func, void *user_data)
{
	osync_assert(table);
	g_hash_table_foreach(table->db_entries, (GHFunc) func, user_data);
}

const char *osync_hashtable_get_hash(OSyncHashTable *table, const char *uid)
{
	osync_assert(table);
	osync_assert(uid);

	return (const char *)  g_hash_table_lookup(table->db_entries, uid);
}

/*@}*/
