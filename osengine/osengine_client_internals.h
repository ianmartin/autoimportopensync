
#ifndef DOXYGEN_SHOULD_SKIP_THIS
struct OSyncClient {
	OSyncMember *member;
	OSyncQueue *incoming;
	OSyncEngine *engine;

	OSyncFlag *fl_connected;
	OSyncFlag *fl_sent_changes;
	OSyncFlag *fl_done;
	OSyncFlag *fl_finished;
	OSyncFlag *fl_committed_all;
	
	//GList *changes;
};
#endif

typedef void (* OSyncPluginReplyHandler) (void *, void *, OSyncError *);

typedef struct OSyncPluginCallContext {
	OSyncPluginReplyHandler handler;
	void *userdata;
} OSyncPluginCallContext;

OSyncClient *osync_client_new(OSyncEngine *engine, OSyncMember *member, OSyncError **error);
void osync_client_free(OSyncClient *client);

osync_bool osync_client_spawn(OSyncClient *client, OSyncEngine *engine, OSyncError **error);
OSyncEngine *osync_client_get_engine(OSyncClient *client);
void osync_client_call_plugin(OSyncClient *client, char *function, void *data, OSyncPluginReplyHandler replyhandler, void *userdata);

osync_bool osync_client_init(OSyncClient *client, OSyncEngine *engine, OSyncError **error);
void osync_client_finalize(OSyncClient *client);
OSyncPluginTimeouts osync_client_get_timeouts(OSyncClient *client);
void osync_client_reset(OSyncClient *client);

osync_bool osync_client_connect(OSyncClient *target, OSyncEngine *sender, OSyncError **error);
osync_bool osync_client_get_changes(OSyncClient *target, OSyncEngine *sender, OSyncError **error);
osync_bool osync_client_committed_all(OSyncClient *target, OSyncEngine *sender, OSyncError **error);
osync_bool osync_client_sync_done(OSyncClient *target, OSyncEngine *sender, OSyncError **error);
osync_bool osync_client_disconnect(OSyncClient *target, OSyncEngine *sender, OSyncError **error);
