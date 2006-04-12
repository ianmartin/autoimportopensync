
#ifndef DOXYGEN_SHOULD_SKIP_THIS
struct OSyncEngine {
	/** The real opensync group **/
	OSyncGroup *group;
	void (* conflict_callback) (OSyncEngine *, OSyncMapping *, void *);
	void *conflict_userdata;
	void (* changestat_callback) (OSyncEngine *, OSyncChangeUpdate *, void *);
	void *changestat_userdata;
	void (* mebstat_callback) (OSyncMemberUpdate *, void *);
	void *mebstat_userdata;
	void (* engstat_callback) (OSyncEngine *, OSyncEngineUpdate *, void *);
	void *engstat_userdata;
	void (* mapstat_callback) (OSyncMappingUpdate *, void *);
	void *mapstat_userdata;
	void *(* plgmsg_callback) (OSyncEngine *, OSyncClient *, const char *, void *, void *);
	void *plgmsg_userdata;
	/** A list of connected clients **/
	GList *clients;
	/** The g_main_loop of this engine **/
	GMainLoop *syncloop;
	GMainContext *context;
	/** The incoming queue of this engine **/
	OSyncQueue *commands_from_self;
	OSyncQueue *commands_to_self;
	
	GCond* syncing;
	GMutex* syncing_mutex;
	
	GCond* info_received;
	GMutex* info_received_mutex;
	
	GCond* started;
	GMutex* started_mutex;
	
	//The normal flags
	OSyncFlag *fl_running; //Is the syncengine running?
	OSyncFlag *fl_sync; //Do we want to sync data or do we just want info?
	OSyncFlag *fl_stop; //Do we want to stop the engine?
	
	//The combined flags
	OSyncFlag *cmb_connected; //Did all client connect or error?
	OSyncFlag *cmb_sent_changes; //Did all clients sent changes?
	OSyncFlag *cmb_entries_mapped; //Do we have unmapped entries?
	OSyncFlag *cmb_synced; //Are all mappings synced?
	OSyncFlag *cmb_finished; //Are all clients done and disconnected?
	OSyncFlag *cmb_chkconflict;
	OSyncFlag *cmb_read_all;
	OSyncFlag *cmb_multiplied;
	OSyncFlag *cmb_committed_all;
	OSyncFlag *cmb_committed_all_sent;
	
	osync_bool man_dispatch;
	osync_bool allow_sync_alert;
	OSyncMappingTable *maptable;
	osync_bool is_initialized;
	osync_bool committed_all_sent;
	
	OSyncError *error;
	GThread *thread;
	
	int wasted;
	int alldeciders;
};
#endif

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
