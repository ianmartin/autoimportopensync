
#ifndef DOXYGEN_SHOULD_SKIP_THIS
struct OSyncClient {
	OSyncMember *member;
	ITMQueue *incoming;
	GMainLoop *memberloop;
	OSyncEngine *engine;

	OSyncFlag *fl_connected;
	OSyncFlag *fl_sent_changes;
	OSyncFlag *fl_done;
	OSyncFlag *fl_finished;
	OSyncFlag *fl_committed_all;
	GThread *thread;
	GMainContext *context;
	
	GCond* started;
	GMutex* started_mutex;
	
	osync_bool is_initialized;
	
	GList *changes;
};
#endif

typedef void (* OSyncPluginReplyHandler) (void *, void *, OSyncError *);

typedef struct OSyncPluginCallContext {
	OSyncPluginReplyHandler handler;
	void *userdata;
} OSyncPluginCallContext;

OSyncClient *osync_client_new(OSyncEngine *engine, OSyncMember *member);
OSyncEngine *osync_client_get_engine(OSyncClient *client);
void osync_client_call_plugin(OSyncClient *client, char *function, void *data, OSyncPluginReplyHandler replyhandler, void *userdata);
void osync_client_free(OSyncClient *client);
osync_bool osync_client_init(OSyncClient *client, OSyncError **error);
void osync_client_finalize(OSyncClient *client);
OSyncPluginTimeouts osync_client_get_timeouts(OSyncClient *client);
void osync_client_reset(OSyncClient *client);
