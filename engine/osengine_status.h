void osync_status_conflict(OSyncEngine *engine, OSyncMapping *mapping);
void osync_status_update_member(OSyncEngine *engine, MSyncClient *client, memberupdatetype type);
void osync_status_update_change(OSyncEngine *engine, OSyncChange *change, changeupdatetype type);
void osync_status_update_mapping(OSyncEngine *engine, OSyncMapping *mapping, mappingupdatetype type);
void osync_status_update_engine(OSyncEngine *engine, engineupdatetype type);
