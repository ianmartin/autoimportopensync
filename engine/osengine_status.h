void osync_status_conflict(OSyncEngine *engine, OSyncMapping *mapping);
void osync_status_update_member(OSyncEngine *engine, OSyncClient *client, memberupdatetype type, OSyncError **error);
void osync_status_update_change(OSyncEngine *engine, OSyncChange *change, changeupdatetype type, OSyncError **error);
void osync_status_update_mapping(OSyncEngine *engine, OSyncMapping *mapping, mappingupdatetype type, OSyncError **error);
void osync_status_update_engine(OSyncEngine *engine, engineupdatetype type, OSyncError **error);
