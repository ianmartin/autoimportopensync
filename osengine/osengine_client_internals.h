
#ifndef DOXYGEN_SHOULD_SKIP_THIS
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
#endif

OSyncClient *osync_client_new(OSyncEngine *engine, OSyncMember *member);
OSyncEngine *osync_client_get_engine(OSyncClient *client);
void osync_client_call_plugin(OSyncClient *client, char *function, void *data);
void osync_client_free(OSyncClient *client);
osync_bool osync_client_init(OSyncClient *client, OSyncError **error);
void osync_client_finalize(OSyncClient *client);
OSyncPluginTimeouts osync_client_get_timeouts(OSyncClient *client);
