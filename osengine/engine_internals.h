#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

typedef struct ITMessage ITMessage;
typedef struct ITMQueue ITMQueue;

#define segfault_me char **blablabla = NULL; *blablabla = "test";

/**
 * @defgroup PrivateAPI Private APIs
 * @brief Available private APIs
 * 
 */

/**
 * @defgroup OSEnginePrivate OpenSync Engine Private API
 * @ingroup PrivateAPI
 * @brief The internals of the multisync engine
 * 
 */

typedef void (* MSyncFlagTriggerFunc) (gpointer user_data1, gpointer user_data2);

typedef struct MSyncFlag MSyncFlag;
typedef struct OSyncMappingTable OSyncMappingTable;
typedef struct OSyncMappingView OSyncMappingView;
typedef struct OSyncMappingEntry OSyncMappingEntry;
typedef struct timeout_info timeout_info;

struct MSyncFlag {
	osync_bool is_set;
	osync_bool is_changing;
	MSyncFlag *comb_flag;
	unsigned int num_not_set;
	unsigned int num_set;
	osync_bool is_comb;
	MSyncFlagTriggerFunc pos_trigger_func;
	void *pos_user_data1;
	void *pos_user_data2;
	MSyncFlagTriggerFunc neg_trigger_func;
	void *neg_user_data1;
	void *neg_user_data2;
	osync_bool is_any;
};

/**
 * @ingroup OSyncEnginePrivate
 * @brief Represents a SyncEngine used to sync a group
 * 
 */
struct OSyncEngine {
	/** The real opensync group **/
	OSyncGroup *group;
	void (* conflict_callback) (OSyncEngine *, OSyncMapping *, void *);
	void *conflict_userdata;
	void (* changestat_callback) (OSyncEngine *, MSyncChangeUpdate *, void *);
	void *changestat_userdata;
	void (* mebstat_callback) (MSyncMemberUpdate *, void *);
	void *mebstat_userdata;
	void (* engstat_callback) (OSyncEngine *, OSyncEngineUpdate *, void *);
	void *engstat_userdata;
	void (* mapstat_callback) (MSyncMappingUpdate *, void *);
	void *mapstat_userdata;
	void *(* plgmsg_callback) (OSyncEngine *, OSyncClient *, const char *, void *, void *);
	void *plgmsg_userdata;
	/** A list of connected clients **/
	GList *clients;
	/** The g_main_loop of this engine **/
	GMainLoop *syncloop;
	GMainContext *context;
	/** The incoming queue of this engine **/
	ITMQueue *incoming;
	
	GCond* syncing;
	GMutex* syncing_mutex;
	
	GCond* info_received;
	GMutex* info_received_mutex;
	
	GCond* started;
	GMutex* started_mutex;
	
	//The normal flags
	MSyncFlag *fl_running; //Is the syncengine running?
	MSyncFlag *fl_sync; //Do we want to sync data or do we just want info?
	MSyncFlag *fl_stop; //Do we want to stop the engine?
	
	//The combined flags
	MSyncFlag *cmb_connected; //Did all client connect or error?
	MSyncFlag *cmb_sent_changes; //Did all clients sent changes?
	MSyncFlag *cmb_entries_mapped; //Do we have unmapped entries?
	MSyncFlag *cmb_synced; //Are all mappings synced?
	MSyncFlag *cmb_finished; //Are all clients done and disconnected?
	MSyncFlag *cmb_chkconflict;
	
	osync_bool man_dispatch;
	osync_bool allow_sync_alert;
	OSyncMappingTable *maptable;
	osync_bool is_initialized;
	
	OSyncError *error;
	GThread *thread;
};

/**
 * @ingroup OSyncClientPrivate
 * @brief Represents a SyncClient
 * 
 */
struct OSyncClient {
	OSyncMember *member;
	ITMQueue *incoming;
	GMainLoop *memberloop;
	OSyncEngine *engine;

	MSyncFlag *fl_connected;
	MSyncFlag *fl_sent_changes;
	MSyncFlag *fl_done;
	MSyncFlag *fl_finished;
	GThread *thread;
	GMainContext *context;
	
	GCond* started;
	GMutex* started_mutex;
	
	osync_bool is_initialized;
	
	GList *changes;
};

#include "osengine_deciders_internals.h"
#include "osengine_message_internals.h"
#include "osengine_queue_internals.h"
#include "osengine_debug.h"
#include "osengine_flags_internals.h"
#include "osengine_engine_internals.h"
#include "osengine_mapping_internals.h"
#include "osengine_mapcmds_internals.h"
#include "osengine_client_internals.h"
