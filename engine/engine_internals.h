#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct ITMessage ITMessage;
typedef struct ITMQueue ITMQueue;

#include "osengine_message_internals.h"
#include "osengine_queue_internals.h"

/**
 * @defgroup MSyncInternals libmultisync Internals
 * @brief The internals of the multisync engine
 * 
 */

/**
 * @defgroup MSyncPublic libmultisync Public API
 * @brief The API available to everyone to (ab)use.
 * 
 */

typedef void (* MSyncFlagTriggerFunc) (void *user_data);

typedef struct MSyncFlag MSyncFlag;

struct MSyncFlag {
	osync_bool is_set;
	osync_bool is_changing;
	MSyncFlag *comb_flag;
	unsigned int num_not_set;
	unsigned int num_set;
	osync_bool is_comb;
	MSyncFlagTriggerFunc pos_trigger_func;
	void *pos_user_data;
	MSyncFlagTriggerFunc neg_trigger_func;
	void *neg_user_data;
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
	void (* mebstat_callback) (MSyncMemberUpdate *);
	void (* engstat_callback) (OSyncEngine *, OSyncEngineUpdate *, void *);
	void *engstat_userdata;
	void (* mapstat_callback) (MSyncMappingUpdate *);
	void *(* plgmsg_callback) (OSyncEngine *, OSyncClient *, const char *, void *, void *);
	void *plgmsg_userdata;
	/** A list of connected clients **/
	GList *clients;
	/** The g_main_loop of this engine **/
	GMainLoop *syncloop;
	/** The incoming queue of this engine **/
	ITMQueue *incoming;
	
	GCond* syncing;
	GMutex* syncing_mutex;
	
	GCond* info_received;
	GMutex* info_received_mutex;
	//GMutex *syncing;
	//GMutex *info_received;
	
	//The normal flags
	MSyncFlag *fl_running; //Is the syncengine running?
	MSyncFlag *fl_sync; //Do we want to sync data or do we just want info?
	MSyncFlag *fl_stop; //Do we want to stop the engine?
	
	//The combined flags
	MSyncFlag *cmb_sent_changes; //Did all clients sent changes?
	MSyncFlag *cmb_entries_mapped; //Do we have unmapped entries?
	MSyncFlag *cmb_synced; //Are all mappings synced?
	MSyncFlag *cmb_finished; //Are all clients done and disconnected?
	
	osync_bool man_dispatch;
	osync_bool allow_sync_alert;
	OSyncMappingTable *maptable;
};

typedef struct MSyncMappingFlags {
	MSyncFlag *fl_solved;
	//The combined flags
	MSyncFlag *cmb_synced;
	MSyncFlag *cmb_has_data;
	MSyncFlag *cmb_has_info;
	MSyncFlag *cmb_deleted;
} MSyncMappingFlags;

typedef struct MSyncChangeFlags {
	MSyncFlag *fl_has_data;
	MSyncFlag *fl_dirty;
	MSyncFlag *fl_mapped;
	MSyncFlag *fl_has_info;
	MSyncFlag *fl_synced;
	MSyncFlag *fl_deleted;
} MSyncChangeFlags;

typedef enum {
	CLIENT_ASYNC_OPS = 1,
	CLIENT_IS_SERVER = 2
} ClientCaps;

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
	unsigned long capabilities;
};

#include "osengine_debug.h"
#include "osengine_flags_internals.h"
#include "osengine_engine_internals.h"
#include "osengine_mapping_internals.h"
#include "osengine_client_internals.h"
