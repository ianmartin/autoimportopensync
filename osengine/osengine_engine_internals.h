
/**
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
	MSyncFlag *cmb_read_all;
	MSyncFlag *cmb_multiplied;
	
	osync_bool man_dispatch;
	osync_bool allow_sync_alert;
	OSyncMappingTable *maptable;
	osync_bool is_initialized;
	
	OSyncError *error;
	GThread *thread;
};

void send_client_changed(OSyncEngine *engine, OSyncClient *client);
void send_mappingentry_changed(OSyncEngine *engine, OSyncMappingEntry *entry);
void send_mapping_changed(OSyncEngine *engine, OSyncMapping *mapping);
void send_get_change_data(OSyncEngine *sender, OSyncMappingEntry *entry);
void send_commit_change(OSyncEngine *sender, OSyncMappingEntry *entry);
void send_connect(OSyncClient *target, OSyncEngine *sender);
void send_get_changes(OSyncClient *target, OSyncEngine *sender, osync_bool data);
void send_sync_done(OSyncClient *target, OSyncEngine *sender);
void send_disconnect(OSyncClient *target, OSyncEngine *sender);
void send_read_change(OSyncEngine *sender, OSyncMappingEntry *entry);
void send_engine_changed(OSyncEngine *engine);
void send_committed_all(OSyncClient *target, OSyncEngine *sender);
