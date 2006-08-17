/**
 * @ingroup OSEngineMappingPrivate
 * 
 */
/*@{*/

void osengine_change_map(OSyncEngine *engine, OSyncMappingEntry *entry);
void osengine_mapping_check_conflict(OSyncEngine *engine, OSyncMapping *mapping);
void osengine_mapping_multiply_master(OSyncEngine *engine, OSyncMapping *mapping);
void osengine_mapping_ignore_conflict(OSyncEngine *engine, OSyncMapping *mapping);
osync_bool osengine_mapping_ignore_supported(OSyncEngine *engine, OSyncMapping *mapping);

osync_bool osync_change_check_level(OSyncEngine *engine, OSyncMappingEntry *entry);
osync_bool osync_change_elevate(OSyncEngine *engine, OSyncChange *change, int level);

/*@}*/
