#include <check.h>
#include "opensync.h"
#include "opensync_internals.h"
#include "engine.h"
#include "engine_internals.h"

char *setup_testbed(char *fkt_name);
void destroy_testbed(char *path);
int num_conflicts;
int num_written;
int num_read;
int num_connected;
int num_disconnected;


void conflict_handler_choose_first(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void conflict_handler_choose_modified(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void conflict_handler_choose_deleted(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void conflict_handler_duplication(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void entry_status(OSyncEngine *engine, MSyncChangeUpdate *status, void *user_data);
void member_status(MSyncMemberUpdate *status);
void conflict_handler_random(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void synchronize_once(OSyncEngine *engine);
void create_case(Suite *s, const char *name, void (*function)(void));
OSyncMappingTable *mappingtable_load(OSyncGroup *group, int num_mappings, int num_unmapped);
OSyncHashTable *hashtable_load(OSyncGroup *group, int member, int entries);
void check_hash(OSyncHashTable *table, const char *cmpuid);
void check_mapping(OSyncMappingTable *table, int memberid, int mappingid, int numentries, const char *uid, const char *format, const char *objecttype);
