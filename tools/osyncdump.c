#include <src/opensync.h>
#include <src/opensync_internals.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <db.h>

static void usage (char *name, int ecode)
{
	fprintf (stderr, "Usage: %s <groupname>\n", name);
	fprintf (stderr, "[--mappings] \tDump all mappings. Default\n");
	fprintf (stderr, "[--hash <memberid>] \tDump hash table for member id\n");
	fprintf (stderr, "[--unmapped] \tAlso dumps changes which are unmapped\n");
	fprintf (stderr, "[--configdir] \tSet a different configdir then ~./opensync\n");
	exit(ecode);
}

typedef enum  {
	DUMPMAPS = 1,
	DUMPHASH = 2,
	DUMPUNMAPPED = 3
} ToolAction;

void dump_map(OSyncEnv *osync, char *groupname)
{
	OSyncGroup *group = osync_group_from_name(osync, groupname);
	
	if (!group) {
		printf("Unable to find group with name \"%s\"\n", groupname);
		return;
	}
	
	OSyncMappingTable *table = osync_mappingtable_new(group);
	osync_mappingtable_set_dbpath(table, osync_group_get_configdir(group));
	osync_mappingtable_load(table);
	
	int i, n;
	for (i = 0; i < osync_mappingtable_num_mappings(table); i++) {
		OSyncMapping *mapping = osync_mappingtable_nth_mapping(table, i);	
		printf("\nNEW MAPPING: %lu\n", osync_mapping_get_id(mapping));
		for (n = 0; n < osync_mapping_num_entries(mapping); n++) {
			OSyncChange *change = osync_mapping_nth_entry(mapping, n);
			OSyncMember *member = osync_change_get_member(change);
	    	int memberid = 0;
	    	if (member)
	    		memberid = osync_member_get_id(member);
	    	const char *formatname = NULL;
	    	if (osync_change_get_objformat(change))
	    		formatname = osync_objformat_get_name(osync_change_get_objformat(change));
	    	const char *objname = NULL;
	    	if (osync_change_get_objtype(change))
	    		objname = osync_objtype_get_name(osync_change_get_objtype(change));
			printf("ID: %lu UID: %s MEMBER: %i\n\tOBJTYPE: %s OBJFORMAT: %s\n", osync_change_get_id(change), osync_change_get_uid(change), memberid, objname, formatname);
		}
	}
    
	osync_mappingtable_close(table);
}

void dump_unmapped(OSyncEnv *osync, char *groupname)
{
	OSyncGroup *group = osync_group_from_name(osync, groupname);
	
	if (!group) {
		printf("Unable to find group with name \"%s\"\n", groupname);
		return;
	}
	
	OSyncMappingTable *table = osync_mappingtable_new(group);
	char *entrydb = g_strdup_printf("%s/change.db", osync_group_get_configdir(group)); //FIXME!!!
	DB *entrytable = osync_db_open(entrydb, "Entries", DB_BTREE, NULL);
	g_free(entrydb);
	if (!entrytable) {
		printf("Unable to open change database\n");
		return;
	}
	
	DBC *dbcp = osync_db_cursor_new(entrytable);

	void *entryid;
	void *data;
    
	OSyncChange *change = NULL;
	
	while (osync_db_cursor_next(dbcp, &entryid, &data)) {
		change = osync_change_new();
    	osync_change_unmarshal(table, change, data);
    	OSyncMember *member = osync_change_get_member(change);
    	int memberid = 0;
    	if (member)
    		memberid = osync_member_get_id(member);
    	printf("ID: %lu UID: %s MEMBER: %i\n", *(unsigned long *)entryid, osync_change_get_uid(change), memberid);
    }
    osync_db_cursor_close(dbcp);
    
    osync_db_close(entrytable);
}

void dump_hash(OSyncEnv *osync, char *groupname, char *memberid)
{
	unsigned int id = atoi(memberid);
	OSyncGroup *group = osync_group_from_name(osync, groupname);
	
	if (!group) {
		printf("Unable to find group with name %s\n", groupname);
		return;
	}
	
	OSyncMember *member = osync_member_from_id(group, id);
	if (!member) {
		printf("Unable to find member with id %s\n", memberid);
		return;
	}
	
	char *uid;
    char *hash;
    OSyncHashTable *table = osync_hashtable_new();
    osync_hashtable_load(table, member);
    int i;
	for (i = 0; i < osync_hashtable_num_entries(table); i++) {
		if (osync_hashtable_nth_entry(table, i, &uid, &hash))
			printf("UID: %s HASH: %s\n", uid, hash);
	}
	osync_hashtable_close(table);
}

int main (int argc, char *argv[])
{
	int i;
	char *groupname = NULL;
	char *membername = NULL;
	ToolAction action = DUMPMAPS;
	char *configdir = NULL;
	
	if (argc == 1)
		usage (argv[0], 1);

	groupname = argv[1];
	for (i = 2; i < argc; i++) {
		char *arg = argv[i];
		if (!strcmp (arg, "--mappings")) {
			action = DUMPMAPS;
		} else if (!strcmp (arg, "--hash")) {
			action = DUMPHASH;
			membername = argv[i + 1];
			i++;
			if (!membername)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--unmapped")) {
			action = DUMPUNMAPPED;
		} else if (!strcmp (arg, "--help")) {
			usage (argv[0], 0);
		} else if (!strcmp (arg, "--configdir")) {
			configdir = argv[i + 1];
			i++;
			if (!configdir)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--")) {
			break;
		} else if (arg[0] == '-') {
			usage (argv[0], 1);
		} else {
			usage (argv[0], 1);
		}
	}
	
	OSyncEnv *osync = osync_env_new();
	osync_init(osync);
	if (configdir)
		osync_env_set_configdir(osync, configdir);
	osync_env_load_groups_dir(osync);
	
	switch (action) {
		case DUMPMAPS:
			dump_map(osync, groupname);
			break;
		case DUMPHASH:
			dump_hash(osync, groupname, membername);
			break;
		case DUMPUNMAPPED:
			dump_unmapped(osync, groupname);
			break;
		default:
			printf("error\n");
	}
	
	return 0;
}
