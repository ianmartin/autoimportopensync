#include <sqlite3.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS
struct OSyncDB {
	sqlite3 *db;
};
#endif

OSyncDB *osync_db_open(char *filename, OSyncError **error);
void osync_db_close(OSyncDB *db);
int osync_db_count(OSyncDB *db, char *table);

osync_bool osync_db_open_changes(OSyncGroup *group, OSyncChange ***changes, OSyncError **error);
osync_bool osync_db_save_change(OSyncChange *change, osync_bool save_format, OSyncError **error);
osync_bool osync_db_delete_change(OSyncChange *change, OSyncError **error);
osync_bool osync_db_reset_changes(OSyncGroup *group, const char *objtype, OSyncError **error);
void osync_db_close_changes(OSyncGroup *group);

OSyncDB *osync_db_open_anchor(OSyncMember *member, OSyncError **error);
void osync_db_close_anchor(OSyncDB *db);
void osync_db_get_anchor(OSyncDB *sdb, const char *objtype, char **retanchor);
void osync_db_put_anchor(OSyncDB *sdb, const char *objtype, const char *anchor);

osync_bool osync_db_open_hashtable(OSyncHashTable *table, OSyncMember *member, OSyncError **error);
void osync_db_close_hashtable(OSyncHashTable *table);
void osync_db_save_hash(OSyncHashTable *table, char *uid, char *hash, char *objtype);
void osync_db_delete_hash(OSyncHashTable *table, char *uid);
void osync_db_report_hash(OSyncHashTable *table, OSyncContext *ctx, const char *objtype);
void osync_db_get_hash(OSyncHashTable *table, char *uid, char **rethash);
void osync_db_reset_hash(OSyncHashTable *table, const char *objtype);

osync_bool osync_db_reset_member(OSyncMember *member, OSyncError **error);
osync_bool osync_db_reset_group(OSyncGroup *group, OSyncError **error);

osync_bool osync_db_open_changelog(OSyncGroup *group, char ***uids, long long int **memberids, int **changetypes, OSyncError **error);
osync_bool osync_db_save_changelog(OSyncGroup *group, OSyncChange *change, OSyncError **error);
osync_bool osync_db_remove_changelog(OSyncGroup *group, OSyncChange *change, OSyncError **error);
