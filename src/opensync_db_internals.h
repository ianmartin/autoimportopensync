#include <sqlite3.h>

struct OSyncDB {
	sqlite3 *db;
};

OSyncDB *osync_db_open(char *filename);
void osync_db_close(OSyncDB *db);
int osync_db_count(OSyncDB *db, char *table);

void osync_db_open_mappingtable(OSyncMappingTable *table);
void osync_db_close_mappingtable(OSyncMappingTable *table);

void osync_db_delete_change(OSyncMappingTable *table, OSyncChange *change);
void osync_db_save_change(OSyncMappingTable *table, OSyncChange *change);

OSyncDB *osync_db_open_anchor(OSyncMember *member);
void osync_db_close_anchor(OSyncDB *db);
void osync_db_get_anchor(OSyncDB *sdb, char *objtype, char **retanchor);
void osync_db_put_anchor(OSyncDB *sdb, char *objtype, char *anchor);

osync_bool osync_db_open_hashtable(OSyncHashTable *table, OSyncMember *member);
void osync_db_close_hashtable(OSyncHashTable *table);
void osync_db_save_hash(OSyncHashTable *table, char *uid, char *hash);
void osync_db_delete_hash(OSyncHashTable *table, char *uid);
void osync_db_report_hash(OSyncHashTable *table, OSyncContext *ctx);
void osync_db_get_hash(OSyncHashTable *table, char *uid, char **rethash);
