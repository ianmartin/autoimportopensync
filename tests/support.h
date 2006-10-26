#ifndef SUPPORT_H
#define SUPPORT_H

#include <check.h>

#ifndef CHECK_VERSION
#define CHECK_VERSION (CHECK_MAJOR_VERSION * 10000 + CHECK_MINOR_VERSION *  100 + CHECK_MICRO_VERSION)
#endif /*CHECK_VERSION*/

#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>
#include "engine.h"
#include "engine_internals.h"

char *setup_testbed(char *fkt_name);
void destroy_testbed(char *path);

int num_conflicts;
int num_written;
int num_read;
int num_connected;
int num_disconnected;
int num_read_info;

int num_member_connect_errors;
int num_member_sent_changes;
int num_member_get_changes_errors;
int num_member_sync_done_errors;
int num_member_disconnect_errors;
int num_member_comitted_all_errors;
int num_member_comitted_all;

int num_written_errors;
int num_mapping_errors;
int num_recv_errors;

int num_engine_errors;
int num_engine_successfull;
int num_engine_prev_unclean;
int num_engine_end_conflicts;
int num_engine_connected;
int num_engine_read;
int num_engine_wrote;
int num_engine_disconnected;
	
void conflict_handler_choose_first(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void conflict_handler_choose_modified(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void conflict_handler_choose_deleted(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void conflict_handler_duplication(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void conflict_handler_delay(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
void conflict_handler_ignore(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);

void entry_status(OSyncEngine *engine, OSyncChangeUpdate *status, void *user_data);
void member_status(OSyncMemberUpdate *status, void *user_data);
void engine_status(OSyncEngine *engine, OSyncEngineUpdate *status, void *user_data);
void mapping_status(OSyncMappingUpdate *status, void *user_data);

OSyncEngine *init_engine(OSyncGroup *group);
void conflict_handler_random(OSyncEngine *engine, OSyncMapping *mapping, void *user_data);
osync_bool synchronize_once(OSyncEngine *engine, OSyncError **error);

/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
void create_case(Suite *s, const char *name, void (*function)(void));
#else /*CHECK_VERSION > 903*/
void create_case(Suite *s, const char *name, void (*function)(int));
#endif /*CHECK_VERSION*/

OSyncMappingTable *mappingtable_load(OSyncGroup *group, int num_mappings, int num_unmapped);
void mappingtable_close(OSyncMappingTable *maptable);

OSyncHashTable *hashtable_load(OSyncGroup *group, int member, int entries);
void check_hash(OSyncHashTable *table, const char *cmpuid);
void check_mapping(OSyncMappingTable *table, int memberid, int mappingid, int numentries, const char *uid, const char *format, const char *objecttype);
OSyncEnv *init_env(void);
OSyncEnv *init_env_none(void);

#endif /*SUPPORT_H*/
