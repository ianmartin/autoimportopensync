MSyncClient *osync_client_new(OSyncEngine *engine, OSyncMember *member);
OSyncEngine *osync_client_get_engine(MSyncClient *client);
void osync_client_call_plugin(MSyncClient *client, char *function, void *data);
void osync_client_free(MSyncClient *client);
osync_bool osync_client_init(MSyncClient *client, OSyncError **error);
void osync_client_finalize(MSyncClient *client);
