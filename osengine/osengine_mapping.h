/**
 * @defgroup OSEngineMapping OpenSync Mapping
 * @ingroup OSEnginePublic
 * @brief The commands to manipulate mappings
 * 
 */
/*@{*/

void osengine_mapping_duplicate(OSyncEngine *engine, OSyncMapping *dupe_mapping);
void osengine_mapping_solve(OSyncEngine *engine, OSyncMapping *mapping, OSyncChange *change);
int osengine_mapping_num_changes(OSyncMapping *mapping);
OSyncChange *osengine_mapping_nth_change(OSyncMapping *mapping, int nth);
long long osengine_mapping_get_id(OSyncMapping *mapping);

/*@}*/
