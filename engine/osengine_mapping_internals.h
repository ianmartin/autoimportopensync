OSyncMapping *osync_mapping_find(OSyncEngine *engine, OSyncChange *change);
void osync_mappingentry_all_deciders(OSyncEngine *engine, OSyncMapping *mapping);
void osync_mapping_calculate_flags(OSyncEngine *engine, OSyncMapping *mapping);
void osync_change_map(OSyncEngine *engine, OSyncChange *change);
void send_change_changed(OSyncChange *change);
void osync_mapping_duplicate(OSyncEngine *engine, OSyncMapping *dupe_mapping);
void osync_change_decider(OSyncEngine *engine, OSyncChange *change);
void osync_mapping_decider(OSyncEngine *engine, OSyncMapping *mapping);
void osync_mapping_all_deciders(OSyncEngine *engine);
