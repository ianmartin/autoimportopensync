#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>
#include <osengine/engine.h>
#include <osengine/engine_internals.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sqlite3.h>

static void usage (char *name, int ecode)
{
	fprintf (stderr, "Usage: %s <groupname>\n", name);
	fprintf (stderr, "[--mappings] \tDump all mappings. Default\n");
	fprintf (stderr, "[--hash <memberid>] \tDump hash table for member id\n");
	fprintf (stderr, "[--unmapped] \tAlso dumps changes which are unmapped\n");
	fprintf (stderr, "[--configdir] \tSet a different configdir then ~./opensync\n");
	fprintf (stderr, "[--reset] \tReset the database for this group\n");
	exit(ecode);
}

typedef enum  {
	DUMPMAPS = 1,
	DUMPHASH = 2,
	DUMPUNMAPPED = 3,
	RESET = 4
} ToolAction;

static void dump_map(OSyncEnv *osync, char *groupname)
{
	OSyncGroup *group = osync_env_find_group(osync, groupname);
	
	if (!group) {
		printf("Unable to find group with name \"%s\"\n", groupname);
		return;
	}
	
	
	OSyncChange **changes = NULL;
	OSyncError *error = NULL;
	if (!osync_changes_load(group, &changes, &error)) {
		printf("Unable to load changes: %s\n", osync_error_print(&error));
		return;
	}
	
	int i = 0;
	OSyncChange *change = NULL;
	while ((change = changes[i])) {
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
    	printf("ID: %lli UID: %s MEMBER: %i\n\tOBJTYPE: %s OBJFORMAT: %s MAPPINGID: %lli\n", osync_change_get_id(change), osync_change_get_uid(change), memberid, objname, formatname, osync_change_get_mappingid(change));
		i++;
	}

	osync_changes_close(group);
}

static void dump_unmapped(OSyncEnv *osync, char *groupname)
{
	OSyncGroup *group = osync_env_find_group(osync, groupname);
	
	if (!group) {
		printf("Unable to find group with name \"%s\"\n", groupname);
		return;
	}
	
	char *filename = g_strdup_printf("%s/change.db", osync_group_get_configdir(group));
	OSyncDB *db = osync_db_open(filename, NULL);
	g_free(filename);
	
	sqlite3 *sdb = db->db;
	
	sqlite3_stmt *ppStmt = NULL;
	sqlite3_prepare(sdb, "SELECT id, uid, objtype, format, memberid FROM tbl_changes WHERE mappingid=0", -1, &ppStmt, NULL);
	while (sqlite3_step(ppStmt) == SQLITE_ROW) {
		long long int entryid = sqlite3_column_int64(ppStmt, 0);
		char *uid = g_strdup((gchar*)sqlite3_column_text(ppStmt, 1));
		char *objtype = g_strdup((gchar*)sqlite3_column_text(ppStmt, 2));
		char *objformat = g_strdup((gchar*)sqlite3_column_text(ppStmt, 3));
		long long int memberid = sqlite3_column_int64(ppStmt, 4);
		
    	printf("ID: %lli UID: %s MEMBER: %lli, TYPE %s, FORMAT %s\n", entryid, uid, memberid, objtype, objformat);
	}
	sqlite3_finalize(ppStmt);
	
	osync_db_close(db);
	
	
	/*
	
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
    	
    }
    osync_db_cursor_close(dbcp);
    
    osync_db_close(entrytable);*/
}

static void dump_hash(OSyncEnv *osync, char *groupname, char *memberid)
{
	long long int id = atoi(memberid);
	OSyncGroup *group = osync_env_find_group(osync, groupname);
	
	if (!group) {
		printf("Unable to find group with name %s\n", groupname);
		return;
	}
	
	OSyncMember *member = osync_member_from_id(group, id);
	if (!member) {
		printf("Unable to find member with id %s\n", memberid);
		return;
	}
	
    OSyncHashTable *table = osync_hashtable_new();
    osync_db_open_hashtable(table, member, NULL);
    
    sqlite3 *sdb = table->dbhandle->db;
	
	sqlite3_stmt *ppStmt = NULL;
	sqlite3_prepare(sdb, "SELECT uid, hash FROM tbl_hash", -1, &ppStmt, NULL);
	while (sqlite3_step(ppStmt) == SQLITE_ROW) {
		char *uid = g_strdup((gchar*)sqlite3_column_text(ppStmt, 0));
		char *hash = g_strdup((gchar*)sqlite3_column_text(ppStmt, 1));
    	printf("UID: %s HASH: %s\n", uid, hash);
	}
	sqlite3_finalize(ppStmt);

	osync_db_close_hashtable(table);
}

static void reset(OSyncEnv *osync, char *groupname)
{
	OSyncGroup *group = osync_env_find_group(osync, groupname);
	
	if (!group) {
		printf("Unable to find group with name \"%s\"\n", groupname);
		return;
	}
	
	osync_group_reset(group);
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
		} else if (!strcmp (arg, "--reset")) {
			action = RESET;
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
	osync_env_set_option(osync, "GROUPS_DIRECTORY", configdir);
	
	OSyncError *error = NULL;
	if (!osync_env_initialize(osync, &error)) {
		printf("Unable to initialize environment: %s\n", error->message);
		osync_error_free(&error);
		return 1;
	}
	
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
		case RESET:
			reset(osync, groupname);
			break;
		default:
			printf("error\n");
	}
	
	return 0;
}