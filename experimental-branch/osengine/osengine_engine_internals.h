void send_client_changed(OSyncEngine *engine, OSyncClient *client);
void send_mappingentry_changed(OSyncEngine *engine, OSyncMappingEntry *entry);
void send_mapping_changed(OSyncEngine *engine, OSyncMapping *mapping);
void send_get_change_data(OSyncEngine *sender, OSyncMappingEntry *entry);
void send_commit_change(OSyncEngine *sender, OSyncMappingEntry *entry);
void send_connect(OSyncClient *target, OSyncEngine *sender);
void send_get_changes(OSyncClient *target, OSyncEngine *sender, osync_bool data);
void send_sync_done(OSyncClient *target, OSyncEngine *sender);
void send_disconnect(OSyncClient *target, OSyncEngine *sender);
