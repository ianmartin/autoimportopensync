#include <check.h>

#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>
#include <opensync/opensync-engine.h>
#include <opensync/opensync-mapping.h>
#include <opensync/opensync-archive.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-group.h>

#include "config.h"


int num_client_connected;
int num_client_main_connected;
int num_client_disconnected;
int num_client_main_disconnected;
int num_client_read;
int num_client_main_read;
int num_client_written;
int num_client_main_written;
int num_client_errors;
int num_client_sync_done;
int num_client_main_sync_done;
int num_client_discovered;

int num_change_read;
int num_change_written;
int num_change_error;

int num_engine_connected;
int num_engine_read;
int num_engine_written;
int num_engine_disconnected;
int num_engine_errors;
int num_engine_successful;
int num_engine_end_conflicts;
int num_engine_prev_unclean;
int num_engine_sync_done;


int num_mapping_solved;
int num_mapping_written;
int num_mapping_errors;
int num_mapping_conflicts;

void check_env(void);

char *setup_testbed(char *fkt_name);
void destroy_testbed(char *path);
// create_case() with timeout of 30seconds (default)
void create_case(Suite *s, const char *name, TFun function);
// create_case_timeout() allow to specific a specific timeout - intended for breaking testcases which needs longer then 30seconds (default)
void create_case_timeout(Suite *s, const char *name, TFun function, int timeout);

void conflict_handler_random(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);

OSyncMappingTable *mappingtable_load(const char *path, const char *objtype, int num_mappings);
void mappingtable_close(OSyncMappingTable *maptable);

OSyncHashTable *hashtable_load(const char *path, const char *objtype, int entries);
void check_hash(OSyncHashTable *table, const char *cmpuid);
void check_mapping(OSyncMappingTable *table, int memberid, int mappingid, int numentries, const char *uid);

void create_random_file(const char *path);

void reset_counters();
osync_bool synchronize_once(OSyncEngine *engine, OSyncError **error);
void discover_all_once(OSyncEngine *engine, OSyncError **error);

/* Status callbacks */
void member_status(OSyncMemberUpdate *status, void *user_data);
void entry_status(OSyncChangeUpdate *status, void *user_data);
void engine_status(OSyncEngineUpdate *status, void *user_data);
void mapping_status(OSyncMappingUpdate *status, void *user_data);

/* Conflict handlers */
void conflict_handler_choose_first(OSyncEngine *engine, OSyncMappingEngine *mapping, void *user_data);
void conflict_handler_choose_deleted(OSyncEngine *engine, OSyncMappingEngine *mapping, void *user_data);
void conflict_handler_choose_modified(OSyncEngine *engine, OSyncMappingEngine *mapping, void *user_data);
void conflict_handler_ignore(OSyncEngine *engine, OSyncMappingEngine *mapping, void *user_data);
void conflict_handler_duplicate(OSyncEngine *engine, OSyncMappingEngine *mapping, void *user_data);
void conflict_handler_abort(OSyncEngine *engine, OSyncMappingEngine *mapping, void *user_data);
void solve_conflict(OSyncMappingEngine *mapping);
void conflict_handler_delay(OSyncEngine *engine, OSyncMappingEngine *mapping, void *user_data);


