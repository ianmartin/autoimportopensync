#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>
#include <opensync/opensync-group.h>
#include <opensync/opensync-archive.h>
#include <opensync/opensync-mapping.h>
#include <opensync/opensync-helper.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
//#include <sys/wait.h>
#include <sqlite3.h>

static void usage (char *name, int ecode)
{
	fprintf (stderr, "Usage: %s <groupname>\n", name);
	fprintf (stderr, "[--mappings <objtype>] \tDump all mappings. Default\n");
	fprintf (stderr, "[--hash <objtype> <memberid>] \tDump hash table for member id\n");
	fprintf (stderr, "[--configdir] \tSet a different configdir then ~./opensync\n");
	fprintf (stderr, "[--reset] \tReset the database for this group\n");
	exit(ecode);
}

typedef enum  {
	DUMPMAPS = 1,
	DUMPHASH = 2,
	RESET = 4
} ToolAction;

static void dump_map_objtype(OSyncGroupEnv *env, const char *objtype, const char *groupname)
{
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_env_find_group(env, groupname);
	
	char *path = g_strdup_printf("%s/archive.db", osync_group_get_configdir(group));
	OSyncArchive *archive = osync_archive_new(path, &error);
	if (!archive)
		goto error;
	g_free(path);
	
	OSyncList *ids = NULL;
	OSyncList *uids = NULL;
	OSyncList *mappingids = NULL;
	OSyncList *memberids = NULL;
	
	if (!osync_archive_load_changes(archive, objtype, &ids, &uids, &mappingids, &memberids, &error))
		goto error;
	
	OSyncList *d = ids;
	OSyncList *u = uids;
	OSyncList *m = mappingids;
	OSyncList *i = memberids;
	
	for (; u; u = u->next) {
		long long int id = (long long int)GPOINTER_TO_INT(d->data);
		char *uid = u->data;
		long long int memberid = (long long int)GPOINTER_TO_INT(i->data);
		long long int mappingid = (long long int)GPOINTER_TO_INT(m->data);
		
		printf("ID: %lli UID: %s MEMBER: %lli MAPPINGID: %lli\n", id, uid, memberid, mappingid);
		
		m = m->next;
		d = d->next;
		i = i->next;
	}
	
	osync_list_free(ids);
	osync_list_free(uids);
	osync_list_free(mappingids);
	osync_list_free(memberids);
	
	osync_archive_unref(archive);
	return;

error:
	printf("ERROR: %s", osync_error_print(&error));
	osync_error_unref(&error);
}

static void dump_map(OSyncGroupEnv *env, const char *groupname)
{

	OSyncGroup *group = osync_group_env_find_group(env, groupname);
	
	if (!group) {
		printf("Unable to find group with name \"%s\"\n", groupname);
		return;
	}

        int i, num_objtypes = osync_group_num_objtypes(group); 
        if (num_objtypes == 0) { 
		printf("Group has no objtypes. Have the objtypes already been discovered?\n"); 
		return;
        }

        for (i = 0; i < num_objtypes; i++) {
		const char *objtype = osync_group_nth_objtype(group, i);
		printf("Mappings for objtype \"%s\":\n", objtype);
		dump_map_objtype(env, objtype, groupname);
	}

}

static void print_hashtable(const char *uid, const char *hash, void *user_data)
{
	printf("UID: %s\tHASH:%s\n", uid, hash);
}

static void dump_hash(OSyncGroupEnv *env, const char *objtype, const char *groupname, char *memberid)
{
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_env_find_group(env, groupname);
	
	if (!group) {
		printf("Unable to find group with name \"%s\"\n", groupname);
		return;
	}
	
	long long int id = atoi(memberid);
	OSyncMember *member = osync_group_find_member(group, id);
	if (!member) {
		printf("Unable to find member with id %s\n", memberid);
		return;
	}
	
	char *path = g_strdup_printf("%s/hashtable.db", osync_member_get_configdir(member));
	OSyncHashTable *table = osync_hashtable_new(path, objtype, &error);
	if (!table)
		goto error;
	g_free(path);
	
	osync_hashtable_foreach(table, print_hashtable, NULL);

	osync_hashtable_unref(table);
	
	return;

error:
	printf("ERROR: %s", osync_error_print(&error));
	osync_error_unref(&error);
}

static void reset(OSyncGroupEnv *osync, char *groupname)
{
	OSyncGroup *group = osync_group_env_find_group(osync, groupname);
	
	if (!group) {
		printf("Unable to find group with name \"%s\"\n", groupname);
		return;
	}
	
	osync_group_reset(group, NULL);
}

int main (int argc, char *argv[])
{
	int i;
	char *groupname = NULL;
	char *membername = NULL;
	ToolAction action = DUMPMAPS;
	char *configdir = NULL;
	char *objtype = NULL;
	
	if (argc == 1)
		usage (argv[0], 1);

	groupname = argv[1];
	for (i = 2; i < argc; i++) {
		char *arg = argv[i];
		if (!strcmp (arg, "--mappings")) {
			action = DUMPMAPS;
			objtype = argv[i + 1];
			i++;
			if (!objtype)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--hash")) {
			action = DUMPHASH;
			objtype = argv[i + 1];
			membername = argv[i + 2];
			i += 2;
			if (!objtype || !membername)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--reset")) {
			action = RESET;
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
	
	OSyncError *error = NULL;
	
	OSyncGroupEnv *env = osync_group_env_new(&error);
	if (!env)
		goto error;
	
	if (!osync_group_env_load_groups(env, configdir, &error))
		goto error;
	
	switch (action) {
		case DUMPMAPS:
			if (objtype)
				dump_map_objtype(env, objtype, groupname);
			else
				dump_map(env, groupname);
			break;
		case DUMPHASH:
			dump_hash(env, objtype, groupname, membername);
			break;
		case RESET:
			reset(env, groupname);
			break;
	}
	
	return 0;

error:
	printf("ERROR: %s", osync_error_print(&error));
	osync_error_unref(&error);
	return 1;
}
