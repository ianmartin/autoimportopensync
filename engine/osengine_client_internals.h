OSyncClient *osync_client_new(OSyncEngine *engine, OSyncMember *member);
OSyncEngine *osync_client_get_engine(OSyncClient *client);
void osync_client_call_plugin(OSyncClient *client, char *function, void *data);
void osync_client_free(OSyncClient *client);
osync_bool osync_client_init(OSyncClient *client, OSyncError **error);
void osync_client_finalize(OSyncClient *client);
