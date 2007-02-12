
void osengine_mapping_duplicate(OSyncEngine *engine, OSyncMapping *dupe_mapping);
void osengine_mapping_solve(OSyncEngine *engine, OSyncMapping *mapping, OSyncChange *change);
int osengine_mapping_num_changes(OSyncMapping *mapping);
OSyncChange *osengine_mapping_nth_change(OSyncMapping *mapping, int nth);
long long osengine_mapping_get_id(OSyncMapping *mapping);
void osengine_mapping_solve_updated(OSyncEngine *engine, OSyncMapping *mapping, OSyncChange *change);
osync_bool osengine_mapping_solve_latest(OSyncEngine *engine, OSyncMapping *mapping, OSyncError **error);

osync_bool osengine_mapping_ignore_conflict(OSyncEngine *engine, OSyncMapping *mapping, OSyncError **error);
osync_bool osengine_mapping_ignore_supported(OSyncEngine *engine, OSyncMapping *mapping);
osync_bool osengine_mapping_check_timestamps(OSyncEngine *engine, OSyncMapping *mapping, OSyncError **error);
