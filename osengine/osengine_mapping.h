void osengine_mapping_duplicate(OSyncEngine *engine, OSyncMapping *dupe_mapping);
void osengine_mapping_solve(OSyncEngine *engine, OSyncMapping *mapping, OSyncChange *change);
int osengine_mapping_num_changes(OSyncMapping *mapping);
OSyncChange *osengine_mapping_nth_change(OSyncMapping *mapping, int nth);
