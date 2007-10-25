#include <check.h>
#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>

#include <opensync/opensync-engine.h>

#define OPENSYNC_TESTDATA "data"

char *setup_testbed(char *fkt_name);
void destroy_testbed(char *path);
// create_case() with timeout of 30seconds (default)
void create_case(Suite *s, const char *name, TFun function);
// create_case_timeout() allow to specific a specific timeout - intended for breaking testcases which needs longer then 30seconds (default)
void create_case_timeout(Suite *s, const char *name, TFun function, int timeout);

/*void conflict_handler_choose_first(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void conflict_handler_choose_modified(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void conflict_handler_choose_deleted(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void conflict_handler_duplication(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void conflict_handler_delay(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void conflict_handler_ignore(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);*/

void conflict_handler_random(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);

OSyncMappingTable *mappingtable_load(const char *path, const char *objtype, int num_mappings);
void mappingtable_close(OSyncMappingTable *maptable);

OSyncHashTable *hashtable_load(const char *path, const char *objtype, int entries);
void check_hash(OSyncHashTable *table, const char *cmpuid);
void check_mapping(OSyncMappingTable *table, int memberid, int mappingid, int numentries, const char *uid);
