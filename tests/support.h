#include <check.h>
#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>

#include <opensync/opensync-engine.h>

char *setup_testbed(char *fkt_name);
void destroy_testbed(char *path);
void create_case(Suite *s, const char *name, void (*function)(void));

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
void check_mapping(OSyncMappingTable *table, int memberid, int mappingid, int numentries, const char *uid, const char *format, const char *objecttype);
