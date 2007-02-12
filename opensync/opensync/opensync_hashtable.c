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
 
#include <opensync.h>
#include "opensync_internals.h"

/**
 * @defgroup OSyncHashtablePrivateAPI OpenSync Hashtable Internals
 * @ingroup OSyncPrivate
 * @brief The private API of the Hashtables
 * 
 * This gives you an insight in the private API of the Hashtables
 * 
 */
/*@{*/

static void osync_hashtable_assert_loaded(OSyncHashTable *table)
{
	osync_assert_msg(table, "You have to pass a valid hashtable to the call!");
	osync_assert_msg(table->dbhandle, "Hashtable not loaded yet. You have to load the hashtable first using osync_hashtable_load!");
}

/*@}*/

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
 * - osync_hashtable_close()
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

/*! @brief Creates a new hashtable
 * 
 * Hashtables can be used to detect what has been changed since
 * the last sync
 * 
 * @returns A new hashtable
 * 
 */
OSyncHashTable *osync_hashtable_new(void)
{
	OSyncHashTable *table = g_malloc0(sizeof(OSyncHashTable));
	g_assert(table);
	table->used_entries = g_hash_table_new(g_str_hash, g_str_equal);
	return table;
}

/*! @brief Frees a hashtable
 * 
 * 
 * @param table The hashtable to free
 * 
 */
void osync_hashtable_free(OSyncHashTable *table)
{
	g_hash_table_destroy(table->used_entries);
	g_free(table);
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
void osync_hashtable_forget(OSyncHashTable *table)
{
	g_hash_table_destroy(table->used_entries);
	table->used_entries = g_hash_table_new(g_str_hash, g_str_equal);
}

/*! @brief Loads a hashtable from disk
 * 
 * @param table The hashtable
 * @param member The member for which to load the table
 * @param error An error struct
 * @returns TRUE if successfull, FALSE otherwise
 * 
 */
osync_bool osync_hashtable_load(OSyncHashTable *table, OSyncMember *member, OSyncError **error)
{
	return osync_db_open_hashtable(table, member, error);
}

/*! @brief Closes a previously loaded table
 * 
 * This function also makes the hashtable "forget"
 * 
 * @param table The hashtable
 * 
 */
void osync_hashtable_close(OSyncHashTable *table)
{
	osync_hashtable_assert_loaded(table);
	
	osync_hashtable_forget(table);
	osync_db_close(table->dbhandle);
}

/*! @brief Returns the number of entries in this hashtable
 * 
 * @param table The hashtable
 * @returns The number of entries
 * 
 */
int osync_hashtable_num_entries(OSyncHashTable *table)
{
	osync_hashtable_assert_loaded(table);
	
	return osync_db_count(table->dbhandle, "SELECT count(*) FROM tbl_hash");
}

/*! @brief Gets the nth entry from the table
 * 
 * This is mainly usefull for debugging or special purposes
 * 
 * @param table The hashtable
 * @param i The number of the entry to return
 * @param uid A pointer to a char * that will hold the uid. The caller is responible for freeing
 * @param hash A pointer to a char * that will hold the hash. The caller is responible for freeing
 * @returns TRUE if successfull, FALSE otherwise
 * 
 */
osync_bool osync_hashtable_nth_entry(OSyncHashTable *table, int i, char **uid, char **hash)
{
	osync_hashtable_assert_loaded(table);
	
	sqlite3 *sdb = table->dbhandle->db;
	
	sqlite3_stmt *ppStmt = NULL;
	char *query = g_strdup_printf("SELECT uid, hash FROM tbl_hash LIMIT 1 OFFSET %i", i);
	sqlite3_prepare(sdb, query, -1, &ppStmt, NULL);
	sqlite3_step(ppStmt);
	*uid = g_strdup((gchar*)sqlite3_column_text(ppStmt, 0));
	*hash = g_strdup((gchar*)sqlite3_column_text(ppStmt, 1));
	sqlite3_finalize(ppStmt);
	g_free(query);
	return TRUE;
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
void osync_hashtable_update_hash(OSyncHashTable *table, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, table, change);
	osync_hashtable_assert_loaded(table);
	osync_assert_msg(change, "Change was NULL. Bug in a plugin");
	osync_assert_msg(change->uid, "No uid was set on change. Bug in a plugin");

	osync_trace(TRACE_INTERNAL, "Updating hashtable with hash \"%s\" and changetype %i",
			change->hash, osync_change_get_changetype(change));

	switch (osync_change_get_changetype(change)) {
		case CHANGE_MODIFIED:
		case CHANGE_ADDED:
			osync_db_save_hash(table, change->uid, change->hash,
					osync_change_get_objtype(change) ? osync_change_get_objtype(change)->name : NULL);
			break;
		case CHANGE_DELETED:
			osync_db_delete_hash(table, change->uid);
			break;
		default:
			g_assert_not_reached();
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
	osync_hashtable_assert_loaded(table);
	
	g_hash_table_insert(table->used_entries, g_strdup(uid), GINT_TO_POINTER(1));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Report all deleted items
 * 
 * @param table The hashtable
 * @param context The context in which to report the changes
 * @param objtype The object type which to report, NULL for all
 * 
 */
void osync_hashtable_report_deleted(OSyncHashTable *table, OSyncContext *context, const char *objtype)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s)", __func__, table, context, objtype);
	osync_hashtable_assert_loaded(table);
	
	char **uidarr = osync_db_get_deleted_hash(table, objtype);
	int i = 0;
	for (i = 0; uidarr[i]; i++) {
		char *uid = uidarr[i];
		OSyncChange *change = osync_change_new();
		change->changetype = CHANGE_DELETED;
		osync_change_set_objtype_string(change, objtype);
		osync_change_set_uid(change, uid);
		osync_context_report_change(context, change);
		osync_hashtable_update_hash(table, change);
		g_free(uid);
	}
	g_free(uidarr);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Get the uid of all deleted items
 * 
 * @param table The hashtable
 * @param objtype The object type which to report, NULL for all
 * @returns An Null terminated array of uids. The uids and this array have to be freed.
 * 
 */
char **osync_hashtable_get_deleted(OSyncHashTable *table, const char *objtype)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, table, objtype);
	osync_hashtable_assert_loaded(table);
	
	char **retarr = osync_db_get_deleted_hash(table, objtype);	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, retarr);
	return retarr;
}

/*! @brief Get the hash value from the hash table
 *
 */
void osync_hashtable_get_hash(OSyncHashTable *table, OSyncChange *chg)
{
	char *orighash = NULL;
	osync_db_get_hash(table, chg->uid, osync_change_get_objtype(chg)->name, &orighash);
	osync_change_set_hash(chg, orighash);
	g_free(orighash);
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
OSyncChangeType osync_hashtable_get_changetype(OSyncHashTable *table, const char *uid, const char *objtype, const char *hash)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s, %s)", __func__, table, uid, objtype, hash);
	osync_hashtable_assert_loaded(table);
	OSyncChangeType retval = CHANGE_UNMODIFIED;

	char *orighash = NULL;
	osync_db_get_hash(table, uid, objtype, &orighash);
	osync_trace(TRACE_INTERNAL, "Comparing %s with %s", hash, orighash);
	
	if (orighash) {
		if (strcmp(hash, orighash) == 0)
			retval = CHANGE_UNMODIFIED;
		else
			retval = CHANGE_MODIFIED;
	} else
		retval = CHANGE_ADDED;

	osync_trace(TRACE_EXIT, "%s: %s", __func__, retval ? "TRUE" : "FALSE");
	return retval;
}

/*! @brief Gets the changetype of an object and sets it directly
 * 
 * This functions also call report
 * 
 * @param table The hashtable
 * @param change The change to check
 * @returns TRUE if the object was not changed, FALSE if it was changed or added
 * 
 */
osync_bool osync_hashtable_detect_change(OSyncHashTable *table, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, table, change);
	osync_bool retval = FALSE;

	change->changetype = osync_hashtable_get_changetype(table, change->uid, osync_objtype_get_name(osync_change_get_objtype(change)), change->hash);
	if (change->changetype != CHANGE_UNMODIFIED)
		retval = TRUE;
	
	g_hash_table_insert(table->used_entries, g_strdup(change->uid), GINT_TO_POINTER(1));
	osync_trace(TRACE_EXIT, "%s: %s", __func__, retval ? "TRUE" : "FALSE");
	return retval;
}

/*! @brief Resets the hashtable for a given object type
 * 
 * @param table The hashtable
 * @param objtype The object type to slow-sync, NULL for all
 */
void osync_hashtable_set_slow_sync(OSyncHashTable *table, const char *objtype)
{
	osync_hashtable_assert_loaded(table);
	
	osync_db_reset_hash(table, objtype);
}

/*@}*/
