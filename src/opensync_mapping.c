#include <opensync.h>
#include "opensync_internals.h"

OSyncMappingTable *osync_mappingtable_new(OSyncGroup *group)
{
	OSyncMappingTable *table = g_malloc0(sizeof(OSyncMappingTable));
	table->group = group;
	return table;
}

void osync_mappingtable_free(OSyncMappingTable *table)
{
	GList *c = NULL;
	GList *m = NULL;
	GList *mappings = g_list_copy(table->mappings);
	GList *unmapped = g_list_copy(table->unmapped);

	for (m = mappings; m; m = m->next) {
		OSyncMapping *mapping = m->data;
		osync_mapping_free(mapping);
	}
	for (c = unmapped; c; c = c->next) {
		OSyncChange *change = c->data;
		osync_change_free(change);
	}
	g_list_free(mappings);
	g_list_free(unmapped);
	g_free(table->db_path);
	g_free(table);
}

void osync_mappingtable_add_mapping(OSyncMappingTable *table, OSyncMapping *mapping)
{
	table->mappings = g_list_append(table->mappings, mapping);
	mapping->table = table;
}

void osync_mappingtable_remove_mapping(OSyncMappingTable *table, OSyncMapping *mapping)
{
	table->mappings = g_list_remove(table->mappings, mapping);
}

int osync_mappingtable_num_mappings(OSyncMappingTable *table)
{
	return g_list_length(table->mappings);
}

void osync_mapping_create_changeid(OSyncMappingTable *table, OSyncChange *entry)
{
	g_assert(table != NULL);
	g_assert(entry != NULL);
	
	void *entryidp;
	char *anchorstr = "ID";
	unsigned long entryid = 1;

	if (osync_db_get(table->entryidtable, anchorstr, strlen(anchorstr) + 1, &entryidp)) {
		entryid = *((unsigned long *)entryidp) + 1;
	}

	osync_db_put(table->entryidtable, anchorstr, strlen(anchorstr) + 1, &(entryid), sizeof(unsigned long));
	entry->id = entryid;
}

void osync_mapping_create_id(OSyncMappingTable *table, OSyncMapping *mapping)
{
	g_assert(table != NULL);
	g_assert(mapping != NULL);
	
	void *mapidp;
	char *anchorstr = "ID";
	unsigned long mapid = 1;

	if (osync_db_get(table->mapidtable, anchorstr, strlen(anchorstr) + 1, &mapidp)) {
		mapid = *((unsigned long *)mapidp) + 1;
	}
	osync_db_put(table->mapidtable, anchorstr, strlen(anchorstr) + 1, &(mapid), sizeof(unsigned long));
	mapping->id = mapid;
}

OSyncGroup *osync_mapping_get_group(OSyncMapping *mapping)
{
	OSyncMappingTable *table = mapping->table;
	g_assert(table);
	return table->group;
}

void osync_mappingtable_save_change(OSyncMappingTable *table, OSyncChange *change)
{
	g_assert(table->entrytable != NULL);
	
	DBT key, data;
	memset(&data, 0, sizeof(data));
	memset(&key, 0, sizeof(key));
	key.data = &(change->id);
	key.size = sizeof(unsigned long);
	if (!change->id)
		osync_mapping_create_changeid(table, change);
	if (change->mapping) {
		if (!change->mapping->id) {
			osync_mapping_create_id(table, change->mapping);
		}
	}
	osync_change_marshal(change, &data);
	osync_db_put_dbt(table->entrytable, &key, &data);
	osync_db_sync(table->maptable);
}

void osync_mappingtable_delete_change(OSyncMappingTable *table, OSyncChange *change)
{
	g_assert(table->entrytable != NULL);
	osync_db_del(table->entrytable, &(change->id), sizeof(unsigned long));
	osync_db_sync(table->maptable);
}

OSyncMapping *osync_mappingtable_nth_mapping(OSyncMappingTable *table, int num)
{
	return g_list_nth_data(table->mappings, num);
}

void *osync_mapping_get_engine_data(OSyncMapping *mapping)
{
	g_assert(mapping);
	return mapping->engine_data;
}

void osync_mapping_set_engine_data(OSyncMapping *mapping, void *engine_data)
{
	g_assert(mapping);
	mapping->engine_data = engine_data;
}

void osync_mapping_set_masterentry(OSyncMapping *mapping, OSyncChange *master)
{
	g_assert(mapping);
	mapping->master = master;
}

OSyncChange *osync_mapping_get_masterentry(OSyncMapping *mapping)
{
	g_assert(mapping);
	return mapping->master;
}

void osync_mapping_add_entry(OSyncMapping *mapping, OSyncChange *entry)
{
	g_assert(mapping);
	g_assert(entry);
	osync_assert(!osync_mapping_get_entry_by_owner(mapping, entry->member), "WTF?")
	mapping->entries = g_list_append(mapping->entries, entry);
	entry->mapping = mapping;
}

void osync_mapping_remove_entry(OSyncMapping *mapping, OSyncChange *entry)
{
	g_assert(mapping);
	g_assert(entry);
	mapping->entries = g_list_remove(mapping->entries, entry);
	entry->mapping = NULL;
}

OSyncMapping *osync_mapping_new(OSyncMappingTable *table)
{
	g_assert(table);
	OSyncMapping *mapping = g_malloc0(sizeof(OSyncMapping));
	osync_mappingtable_add_mapping(table, mapping);
	return mapping;
}

void osync_mapping_free(OSyncMapping *mapping)
{
	GList *c = NULL;
	GList *entries = g_list_copy(mapping->entries);
	for (c = entries; c; c = c->next) {
		OSyncChange *change = c->data;
		osync_change_free(change);
	}
	g_list_free(entries);
	osync_mappingtable_remove_mapping(mapping->table, mapping);
	g_free(mapping);
}

int osync_mapping_num_entries(OSyncMapping *mapping)
{
	g_assert(mapping);
	return g_list_length(mapping->entries);
}

int osync_mappingtable_num_unmapped(OSyncMappingTable *table)
{
	g_assert(table);
	return g_list_length(table->unmapped);
}

OSyncChange *osync_mappingtable_nth_unmapped(OSyncMappingTable *table, int i)
{
	g_assert(table);
	return g_list_nth_data(table->unmapped, i);
}

void osync_mappingtable_add_unmapped(OSyncMappingTable *table, OSyncChange *change)
{
	g_assert(table);
	table->unmapped = g_list_append(table->unmapped, change);
}

void osync_mappingtable_remove_unmapped(OSyncMappingTable *table, OSyncChange *change)
{
	g_assert(table);
	table->unmapped = g_list_remove(table->unmapped, change);
}

OSyncChange *osync_mapping_nth_entry(OSyncMapping *mapping, int nth)
{
	g_assert(mapping);
	return g_list_nth_data(mapping->entries, nth);
}

//FIXME Do we need this function, or is there a more elegant way?
OSyncChange *osync_mapping_get_entry_by_owner(OSyncMapping *mapping, OSyncMember *member)
{
	g_assert(mapping);
	int i;
	for (i = 0; i < g_list_length(mapping->entries); i++) {
		OSyncChange *change = g_list_nth_data(mapping->entries, i);
		if (osync_member_get_id(change->member) == osync_member_get_id(member))
			return change;
	}
	return NULL;
}

void osync_mappingtable_set_dbpath(OSyncMappingTable *table, char *path)
{
	g_assert(table);
	//FIXME Free previous path
	table->db_path = g_strdup(path);
}

int getmapid(DB *sdbp, const DBT *key, const DBT *data, DBT *res)
{
	OSyncChange *change = osync_change_new();
	osync_change_unmarshal(NULL, change, data->data);
	unsigned long mapid = 0;
	memcpy(&mapid, data->data, sizeof(unsigned long));
	
	if (mapid) {
		res->data = data->data;
		res->size = sizeof(unsigned long);
		return 0;
	} else {
		return DB_DONOTINDEX;
	}
}

void osync_mappingtable_load(OSyncMappingTable *table)
{
	g_assert(table != NULL);
	g_assert(table->db_path != NULL);
	char *filename = g_strdup_printf("%s/change.db", table->db_path);
	table->entrytable = osync_db_open(filename, "Entries", DB_BTREE, table->group->dbenv);
	table->entryidtable = osync_db_open(filename, "ID", DB_BTREE, table->group->dbenv);
	g_free(filename);
	filename = g_strdup_printf("%s/mapping.db", table->db_path);
	table->maptable = osync_db_open_secondary(table->entrytable, filename, "Mappings", getmapid, table->group->dbenv);
	table->mapidtable = osync_db_open(filename, "ID", DB_BTREE, table->group->dbenv);
	g_free(filename);

	g_assert(table->entrytable);
	g_assert(table->maptable);
	
	DBC *dbcp = osync_db_cursor_new(table->maptable);

    void *mapidp;
    void *entryidp;
    void *data;
    
    OSyncMapping *mapping = NULL;
	OSyncChange *change = NULL;
	
	while (osync_db_cursor_next_sec(dbcp, &entryidp, &mapidp, &data)) {
		unsigned long mapid = *(unsigned long *)mapidp;
		unsigned long entryid = *(unsigned long *)entryidp;
		change = osync_change_new();
    	osync_change_unmarshal(table, change, data);
		if (!(mapid)) {
    		printf("Got change without mapping\n");
    		osync_mappingtable_add_unmapped(table, change);
    	} else {
    		if (!mapping || mapping->id != mapid) {
				mapping = osync_mapping_new(table);
				mapping->id = mapid;
    		}
    		osync_mapping_add_entry(mapping, change);
    	}
    	change->id = entryid;
    	osync_member_add_changeentry(change->member, change);
    }
    osync_db_cursor_close(dbcp);
}

void osync_mappingtable_close(OSyncMappingTable *table)
{
	osync_db_close(table->entrytable);
	osync_db_close(table->entryidtable);
	osync_db_close(table->maptable);
	osync_db_close(table->mapidtable);
}

void osync_mapping_delete(OSyncMapping *mapping)
{
	int i;
	OSyncMappingTable *table = mapping->table;
	for (i = 0; i < osync_mapping_num_entries(mapping); i++) {
		OSyncChange *change = osync_mapping_nth_entry(mapping, i);
		osync_mappingtable_delete_change(table, change);
	}
}

unsigned long osync_mapping_get_id(OSyncMapping *mapping)
{
	return mapping->id;
}

void osync_mapping_set_id(OSyncMapping *mapping, unsigned long id)
{
	mapping->id = id;
}
