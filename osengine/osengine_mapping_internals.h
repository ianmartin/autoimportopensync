/**
 * @defgroup OSEngineMappingPrivate OpenSync Mapping Internals
 * @ingroup OSEnginePrivate
 * @brief The internals the mappings
 * 
 */
/*@{*/

/*! @brief A table which holds the mappings
 */
struct OSyncMappingTable {
	GList *mappings;
	OSyncEngine *engine;
	GList *unmapped;
	OSyncGroup *group;
	GList *views;
	GList *entries;
};

/*! @brief A view to the mappingtable, represents one source
 */
struct OSyncMappingView {
	OSyncClient *client;
	GList *changes;
	OSyncMappingTable *table;
	long long int memberid;
};

/*! @brief A mapping of changes
 */
struct OSyncMapping {
	GList *entries;
	OSyncMappingEntry *master;
	void *engine_data;
	long long int id;
	OSyncMappingTable *table;
	MSyncFlag *fl_solved;
	MSyncFlag *fl_chkconflict;
	MSyncFlag *fl_multiplied;
	//The combined flags
	MSyncFlag *cmb_synced;
	MSyncFlag *cmb_has_data;
	MSyncFlag *cmb_has_info;
	MSyncFlag *cmb_deleted;
	OSyncEngine *engine;
};

/*! @brief Represent one change in the mapping
 */
struct OSyncMappingEntry {
	OSyncMappingView *view;
	OSyncClient *client;
	OSyncMapping *mapping;
	OSyncChange *change;
	MSyncFlag *fl_has_data;
	MSyncFlag *fl_dirty;
	MSyncFlag *fl_mapped;
	MSyncFlag *fl_has_info;
	MSyncFlag *fl_synced;
	MSyncFlag *fl_deleted;
};

OSyncMappingTable *osengine_mappingtable_new(OSyncEngine *engine);
void osengine_mappingtable_free(OSyncMappingTable *table);
OSyncMappingEntry *osengine_mappingtable_store_change(OSyncMappingTable *table, OSyncChange *change);
OSyncMapping *osengine_mappingtable_find_mapping(OSyncMappingTable *table, OSyncChange *change);
OSyncMappingView *osengine_mappingtable_find_view(OSyncMappingTable *table, OSyncMember *member);
void osengine_mappingtable_add_mapping(OSyncMappingTable *table, OSyncMapping *mapping);
osync_bool osengine_mappingtable_load(OSyncMappingTable *table, OSyncError **error);
OSyncMappingTable *_osengine_mappingtable_load_group(OSyncGroup *group);
void osengine_mappingtable_close(OSyncMappingTable *table);
long long int osengine_mappingtable_get_next_id(OSyncMappingTable *table);
void osengine_mappingtable_reset(OSyncMappingTable *table);

OSyncMapping *osengine_mapping_new(OSyncMappingTable *table);
void osengine_mapping_free(OSyncMapping *mapping);
void osengine_mapping_add_entry(OSyncMapping *mapping, OSyncMappingEntry *entry);
void osengine_mapping_remove_entry(OSyncMapping *mapping, OSyncMappingEntry *entry);
OSyncMappingEntry *osengine_mapping_find_entry(OSyncMapping *mapping, OSyncChange *change, OSyncMappingView *view);
OSyncMappingEntry *osengine_mapping_nth_entry(OSyncMapping *mapping, int nth);
void osengine_mapping_reset(OSyncMapping *mapping);
void osengine_mapping_delete(OSyncMapping *mapping);

OSyncMappingView *osengine_mappingview_new(OSyncMappingTable *table, OSyncClient *client);
OSyncMappingEntry *osengine_mappingview_store_change(OSyncMappingView *view, OSyncChange *change);
osync_bool osengine_mappingview_uid_is_unique(OSyncMappingView *view, OSyncMappingEntry *entry, osync_bool spare_deleted);
void osengine_mappingview_add_entry(OSyncMappingView *view, OSyncMappingEntry *entry);
void osengine_mappingview_reset(OSyncMappingView *view);
void osengine_mappingview_free(OSyncMappingView *view);

void osengine_mappingentry_update(OSyncMappingEntry *entry, OSyncChange *change);
OSyncMappingEntry *osengine_mappingentry_new(OSyncMapping *mapping);
void osengine_mappingentry_reset(OSyncMappingEntry *entry);
void osengine_mappingentry_free(OSyncMappingEntry *entry);
OSyncMapping *osengine_mappingtable_mapping_from_id(OSyncMappingTable *table, long long id);
OSyncMappingEntry *osengine_mappingentry_copy(OSyncMappingEntry *entry);

/*@}*/
