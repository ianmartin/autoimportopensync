#include <opensync.h>
#include "opensync_internals.h"

OSyncHashTable *osync_hashtable_new(void)
{
	OSyncHashTable *table = g_malloc0(sizeof(OSyncHashTable));
	g_assert(table);
	table->used_entries = g_hash_table_new(g_str_hash, g_str_equal);
	return table;
}

void osync_hashtable_free(OSyncHashTable *table)
{
	g_hash_table_destroy(table->used_entries);
	g_free(table);
}

void osync_hashtable_forget(OSyncHashTable *table)
{
	g_hash_table_destroy(table->used_entries);
	table->used_entries = g_hash_table_new(g_str_hash, g_str_equal);
}

void osync_hashtable_reset(OSyncHashTable *table)
{
	osync_db_empty(table->dbhandle);
	osync_db_sync(table->dbhandle);
}

osync_bool osync_hashtable_load_file(OSyncHashTable *table, char *file, OSyncGroup *group)
{
	table->dbhandle = osync_db_open(file, "Hash", DB_BTREE, group->dbenv);
	g_assert(table->dbhandle);
	return TRUE;
}

osync_bool osync_hashtable_load(OSyncHashTable *table, OSyncMember *member)
{
	g_assert(member != NULL);
	char *filename = g_strdup_printf ("%s/hash.table", member->configdir);
	osync_hashtable_load_file(table, filename, member->group);
	g_free(filename);
	return TRUE; //FIXME
}

int osync_hashtable_num_entries(OSyncHashTable *table)
{
	osync_assert(table, "No table was given");
	osync_assert(table->dbhandle, "Table has no dbhandle. Did you open the hashtable already?");
	DB_BTREE_STAT *statp;
	int ret;
	u_long uid;
	
	if ((ret = table->dbhandle->stat(table->dbhandle, &statp,  0)) != 0) {
		table->dbhandle->err(table->dbhandle, ret, "DB->stat");
		return 0;
	}
	uid = ((u_long)statp->bt_nkeys);
	free(statp);
	return uid;
}

osync_bool osync_hashtable_nth_entry(OSyncHashTable *table, int i, char **uid, char **hash)
{
	osync_assert(table, "No table was given");
	osync_assert(table->dbhandle, "Table has no dbhandle. Did you open the hashtable already?");
	
	DBC *dbcp = osync_db_cursor_new(table->dbhandle);
	
	while (osync_db_cursor_next(dbcp, (void **)uid, (void **)hash)) {
		if (i == 0) {
			osync_db_cursor_close(dbcp);
			return TRUE;
		}
		i--;
	}
	osync_db_cursor_close(dbcp);
	return FALSE;
}

void osync_hashtable_close(OSyncHashTable *table)
{
	osync_db_close(table->dbhandle);
}

void osync_hashtable_update_hash(OSyncHashTable *table, OSyncChange *change)
{
	osync_assert(table, "Table was NULL. Bug in a plugin");
	osync_assert(change, "Change was NULL. Bug in a plugin");
	osync_assert(change->uid, "No uid was set on change. Bug in a plugin");
	switch (osync_change_get_changetype(change)) {
		case CHANGE_MODIFIED:
		case CHANGE_ADDED:
			osync_db_put(table->dbhandle, change->uid, strlen(change->uid) + 1, change->hash, strlen(change->hash) + 1);
			break;
		case CHANGE_DELETED:
			osync_db_del(table->dbhandle, change->uid, strlen(change->uid) + 1);
			break;
		default:
			break;
	}
}

void osync_hashtable_report_deleted(OSyncHashTable *table, OSyncContext *context)
{
	DBC *dbcp = osync_db_cursor_new(table->dbhandle);

	void *uidp;
	void *hashp;

	while (osync_db_cursor_next(dbcp, &uidp, &hashp)) {
		char *uid = (char *)uidp;
		char *hash = (char *)hashp;
		if (!g_hash_table_lookup(table->used_entries, uid)) {
			OSyncChange *change = osync_change_new();
			change->changetype = CHANGE_DELETED;
			osync_change_set_hash(change, hash);
			osync_change_set_uid(change, uid);
			osync_context_report_change(context, change);
			osync_hashtable_update_hash(table, change);
		}
	}
	osync_db_cursor_close(dbcp);
}

osync_bool osync_hashtable_detect_change(OSyncHashTable *table, OSyncChange *change)
{
	osync_bool retval = FALSE;;
	void *hashp = NULL;
	retval = FALSE;
	
	if (osync_db_get(table->dbhandle, change->uid, strlen(change->uid) + 1, &hashp)) {
		char *hash = (char *)hashp;
		if (strcmp(hash, change->hash) == 0) {
			change->changetype = CHANGE_UNMODIFIED;
			retval = FALSE;
		} else {
			change->changetype = CHANGE_MODIFIED;
			retval = TRUE;
		}
	} else {
		change->changetype = CHANGE_ADDED;
		retval = TRUE;
	}
	g_hash_table_insert(table->used_entries, change->uid, (void *)1);
	return retval;
}
