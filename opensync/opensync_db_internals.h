#include <sqlite3.h>

struct OSyncDB {
	sqlite3 *db;
};

OSyncDB *osync_db_open(char *filename, OSyncError **error);
void osync_db_close(OSyncDB *db);
int osync_db_count(OSyncDB *db, char *table);

osync_bool osync_db_open_mappingtable(OSyncMappingTable *table, OSyncError **error);
void osync_db_close_mappingtable(OSyncMappingTable *table);
void osync_db_reset_mappingtable(OSyncMappingTable *table, const char *objtype);

void osync_db_delete_change(OSyncMappingTable *table, OSyncChange *change);
void osync_db_save_change(OSyncMappingTable *table, OSyncChange *change);

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
