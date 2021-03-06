#include <opensync/opensync.h>
#include "engine.h"
#include "engine_internals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glib.h>

static void usage (char *name, int ecode)
{
  fprintf (stderr, "Usage: %s <groupname>\n", name);
  fprintf (stderr, "[--configdir] \tSet a different configdir then ~./opensync\n");
  fprintf (stderr, "[--num] \tHow often do you want to synchronize?\n");
  fprintf (stderr, "[--tryrecover] \tIf something goes wrong try to recover via slow-sync\n");
  fprintf (stderr, "[--crash] \tRandomly crash the engine/plugins\n");
  exit (ecode);
}

GList *members = NULL;

typedef struct member_info {
	OSyncMember *member;
	GList *changes;
	osync_bool connected;
	osync_bool disconnected;
	osync_bool sent_changes;
} member_info;

typedef struct change_info {
	OSyncChange *change;
	char *uid;
	OSyncChangeType type;
	osync_bool sent;
	osync_bool received;
} change_info;

void update_change_list(OSyncEngine *engine)
{
	g_assert(engine);
	members = NULL;
	GList *v;
	GList *e;
	for (v = engine->maptable->views; v; v = v->next) {
		member_info *meminfo = g_malloc0(sizeof(member_info));
		members = g_list_append(members, meminfo);
		OSyncMappingView *view = v->data;
		meminfo->member = view->client->member;
		for (e = view->changes; e; e = e->next) {
			OSyncMappingEntry *entry = e->data;
			change_info *chinfo = g_malloc0(sizeof(change_info));
			meminfo->changes = g_list_append(meminfo->changes, chinfo);
			chinfo->change = entry->change;
			chinfo->type = CHANGE_UNMODIFIED;
			chinfo->uid = g_strdup(osync_change_get_uid(chinfo->change));
		}
	}
}

member_info *find_member_info(OSyncMember *member)
{
	GList *m;
	for (m = members; m; m = m->next) {
		member_info *meminfo = m->data;
		if (meminfo->member == member)
			return meminfo;
	}
	return NULL;
}

change_info *find_change_info(member_info *meminfo, OSyncChange *change)
{
	GList *c;
	for (c = meminfo->changes; c; c = c->next) {
		change_info *chinfo = c->data;
		if (chinfo->uid && !strcmp(chinfo->uid, osync_change_get_uid(change)))
			return chinfo;
	}
	return NULL;
}

void conflict_handler(OSyncEngine *engine, OSyncMapping *mapping)
{
	if (g_random_int_range(0, 3) == 0) {
		osengine_mapping_duplicate(engine, mapping);
		printf("Conflict for Mapping %p: Duplicating\n", mapping);
	} else {
		int num = osengine_mapping_num_changes(mapping);
		int choosen = g_random_int_range(0, num);
		OSyncChange *change = osengine_mapping_nth_change(mapping, choosen);
		osengine_mapping_solve(engine, mapping, change);
		printf("Conflict for Mapping %p: Choosing entry %i\n", mapping, choosen);
	}
}


void member_status(OSyncMemberUpdate *status)
{
	member_info *meminfo = find_member_info(status->member);
	if (!meminfo)
		return;
	switch (status->type) {
		case MEMBER_CONNECTED:
			meminfo->connected = TRUE;
			break;
		case MEMBER_DISCONNECTED:
			meminfo->disconnected = TRUE;
			break;
		default:
			printf("Unknown status\n");
	}
}

void mapping_status(OSyncMappingUpdate *status)
{
	switch (status->type) {
		case MAPPING_SOLVED:
			printf("Mapping solved\n");
			break;
		case MAPPING_SYNCED:
			printf("Mapping Synced\n");
			break;
		default:
			printf("errro\n");
	}
}

void entry_status(OSyncChangeUpdate *status)
{
	member_info *meminfo = find_member_info(osync_change_get_member(status->change));
	if (!meminfo)
		return;
	change_info *chinfo = find_change_info(meminfo, status->change);
	switch (status->type) {
		case CHANGE_RECEIVED:
			if (chinfo && (chinfo->type == osync_change_get_changetype(status->change)))
				chinfo->received = TRUE;
			break;
		case CHANGE_SENT:
			if (chinfo)
				chinfo->sent = TRUE;
			break;
		default:
			printf("Unknown status\n");
	}
}

GMutex *working;

GMainLoop *loop = NULL;
gboolean busy = FALSE;

void stress_message_callback(OSyncMember *member, void *user_data, OSyncError *error)
{
	g_main_loop_quit(loop);
	busy = FALSE;
}

void change_content(OSyncEngine *engine)
{
	GList *m;
	for (m = members; m; m = m->next) {
		member_info *meminfo = m->data;
		
		busy = TRUE;
		osync_member_connect(meminfo->member, (OSyncEngCallback)stress_message_callback, NULL);
		if (busy)
			g_main_loop_run(loop);
		
		GList *c = NULL;
		for (c = meminfo->changes; c; c = c->next) {
			change_info *chinfo = c->data;
			if (g_random_int_range(0, 3) == 0) {
				switch (g_random_int_range(1, 6)) {
					case 1:
					case 5:
						if (osync_member_modify_random_data(meminfo->member, chinfo->change)) {
							printf("Modifying data %s for member %lli. Objtype: %s Format: %s\n", osync_change_get_uid(chinfo->change), osync_member_get_id(meminfo->member), osync_objtype_get_name(osync_change_get_objtype(chinfo->change)), osync_objformat_get_name(osync_change_get_objformat(chinfo->change)));
							chinfo->type = CHANGE_MODIFIED;
						}
						break;
					case 2:
						if (osync_member_delete_data(meminfo->member, chinfo->change)) {
							printf("Deleting data %s for member %lli. Objtype: %s Format: %s\n", osync_change_get_uid(chinfo->change), osync_member_get_id(meminfo->member), osync_objtype_get_name(osync_change_get_objtype(chinfo->change)), osync_objformat_get_name(osync_change_get_objformat(chinfo->change)));
							if (!osync_group_get_slow_sync(engine->group, osync_objtype_get_name(osync_change_get_objtype(chinfo->change))));
								chinfo->type = CHANGE_DELETED;
						}
						break;
					case 3:
						//printf("Modifying all for %s\n", osync_change_get_uid(change));
						break;
					case 4:
						//printf("Deleting all for %s\n", osync_change_get_uid(change));
						break;
					default:
						printf("error\n");
				}
			}
		}
		
		int num_new = g_random_int_range(0, 8);
		int n = 0;;
		for (n = 0; n < num_new; n++) {
			change_info *chinfo = g_malloc0(sizeof(change_info));
			if ((chinfo->change = osync_member_add_random_data(meminfo->member, NULL))) {
				if (find_change_info(meminfo, chinfo->change))
					continue;
				meminfo->changes = g_list_append(meminfo->changes, chinfo);
				chinfo->type = CHANGE_ADDED;
				chinfo->uid = g_strdup(osync_change_get_uid(chinfo->change));
				printf("Adding new data %s for member %lli. Objtype: %s Format: %s\n", osync_change_get_uid(chinfo->change), osync_member_get_id(meminfo->member), osync_objtype_get_name(osync_change_get_objtype(chinfo->change)), osync_objformat_get_name(osync_change_get_objformat(chinfo->change)));
			}
		}
		
		busy = TRUE;
		osync_member_disconnect(meminfo->member, (OSyncEngCallback)stress_message_callback, NULL);
		if (busy)
			g_main_loop_run(loop);
	}
}

static osync_bool check_mappings(OSyncEngine *engine)
{
	member_info *meminfo = NULL;
	GList *m;
	for (m = members; m; m = m->next) {
		meminfo = m->data;

		if (g_list_length(engine->maptable->mappings) != g_list_length(meminfo->changes)) {
			printf("Number of mappings do not match for member %lli, %i compared to %i\n", osync_member_get_id(meminfo->member), g_list_length(engine->maptable->mappings), g_list_length(meminfo->changes));
			return FALSE;
		}
	}
	
	return TRUE;
}

static osync_bool check_hashtables(OSyncEngine *engine)
{
	GList *m;
	for (m = members; m; m = m->next) {
		member_info *meminfo = m->data;
		OSyncHashTable *table = osync_hashtable_new();
		osync_hashtable_load(table, meminfo->member, NULL);
		/*if (osync_hashtable_num_entries(table) != g_list_length(meminfo->changes)) {
			printf("Hashtable for member %i has wrong number %i compared to %i\n", osync_member_get_id(meminfo->member), osync_hashtable_num_entries(table), g_list_length(meminfo->changes));
			abort();
		}*/
		printf("hashtable %p\n", table);
		if (osync_hashtable_num_entries(table) && g_list_length(engine->maptable->mappings) != osync_hashtable_num_entries(table)) {
			printf("Number of mappings do not match hastable for member %lli, %i compared to %i\n", osync_member_get_id(meminfo->member), g_list_length(engine->maptable->mappings), osync_hashtable_num_entries(table));
			return FALSE;
		}
		
		osync_hashtable_close(table);
	}
	return TRUE;
}

/*static osync_bool compare_updates(OSyncEngine *engine)
{
	GList *m;
	for (m = members; m; m = m->next) {
		member_info *meminfo = m->data;
		
		if (!meminfo->connected) {
			printf("Member %lli never connected\n", osync_member_get_id(meminfo->member));
			return FALSE;
		}
		if (!meminfo->disconnected) {
			printf("Member %lli never connected\n", osync_member_get_id(meminfo->member));
			return FALSE;
		}
		
		GList *c = NULL;
		for (c = meminfo->changes; c; c = c->next) {
			change_info *chinfo = c->data;
			if (chinfo->type != CHANGE_UNMODIFIED) {
				if (chinfo->received != TRUE) {
					printf("Never received change %s from member %lli\n", chinfo->uid, osync_member_get_id(meminfo->member));
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}*/

static osync_bool compare_content(OSyncEngine *engine)
{
	/*if (system("test \"x$(diff -x \".*\" r1 r2)\" = \"x\"")) { //FIXME
		printf("Content did not match!\n");
		return FALSE;
	}*/
	return TRUE;
}

int main (int argc, char *argv[])
{
	int i;
	char *groupname = NULL;
	char *configdir = NULL;
	int num = -1;
	osync_bool failed = FALSE;
	osync_bool tryrecover = FALSE;
	OSyncError *error = NULL;
	OSyncEngine *engine = NULL;
	
	if (argc <= 1)
		usage (argv[0], 1);

	groupname = argv[1];
	for (i = 2; i < argc; i++) {
		char *arg = argv[i];
		if (!strcmp (arg, "--configdir")) {
			configdir = argv[i + 1];
			i++;
			if (!configdir)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--num")) {
			num = atoi(argv[i + 1]);
			i++;
			if (num <= 0)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--help")) {
			usage (argv[0], 0);
		} else if (!strcmp (arg, "--tryrecover")) {
			tryrecover = TRUE;
		} else if (!strcmp (arg, "--")) {
			break;
		} else if (arg[0] == '-') {
			usage (argv[0], 1);
		} else {
			usage (argv[0], 1);
		}
	}
	
	loop = g_main_loop_new(NULL, TRUE);	
	if (!g_thread_supported ()) g_thread_init (NULL);
	
	osync_trace(TRACE_ENTRY, "++++ Started the sync stress test +++");
	OSyncEnv *osync = osync_env_new(NULL);
	osync_env_set_option(osync, "GROUPS_DIRECTORY", configdir);
	
	if (!osync_env_initialize(osync, &error)) {
		printf("Unable to initialize environment: %s\n", osync_error_print(&error));
		osync_error_unref(&error);
		goto error_free_env;
	}
	
	OSyncGroup *group = osync_env_find_group(osync, groupname);
	
	if (!group) {
		printf("Unable to find group with name \"%s\"\n", groupname);
		goto error_free_env;
	}
	
	if (!g_thread_supported ()) g_thread_init (NULL);
	working = g_mutex_new();
	int count = 0;
	while (1) {
		engine = osengine_new(group, &error);
		if (!engine) {
			printf("Error while creating syncengine: %s\n", osync_error_print(&error));
			osync_error_unref(&error);
			goto error_free_env;
		}
		
		if (!osengine_init(engine, &error)) {
			printf("Error while initializing syncengine: %s\n", osync_error_print(&error));
			osync_error_unref(&error);
			goto error_free_engine;
		}
		
		do {
			count++;
			printf("++++++++++++++++++++++++++++++\n");
			printf("Initializing new round #%i!\n", count);
			
			if (g_random_int_range(0, 5) == 0) {
				int i;
				OSyncFormatEnv *env = osync_group_get_format_env(group);
				for (i = 0; i < osync_conv_num_objtypes(env); i++) {
					if (g_random_int_range(0, 5) == 0) {
						OSyncObjType *type = osync_conv_nth_objtype(env, i);
						osync_group_set_slow_sync(group, osync_objtype_get_name(type), TRUE);
						printf("Requesting slow-sync for: %s\n", osync_objtype_get_name(type));
					}
				}
				osync_conv_env_free(env);
			}
			
			update_change_list(engine);
			
			if (!check_mappings(engine) || !check_hashtables(engine) || !compare_content(engine)) {
				if (failed) {
					printf("already failed last round...\n");
					goto error_free_engine;
				}
				failed = TRUE;
				if (!tryrecover) {
					printf("Failed. Not trying to recover\n");
					goto error_free_engine;
				} else {
					printf("Failed. Trying to recover!\n");
					osync_group_set_slow_sync(group, "data", TRUE);
				}
			} else {
				failed = FALSE;
			}
			
			change_content(engine);

			printf("Starting to synchronize\n");
			if (!osengine_sync_and_block(engine, &error)) {
				printf("Error while starting synchronization: %s\n", osync_error_print(&error));
				osync_error_unref(&error);
				goto error_free_engine;
			}
			
			if (!compare_content(engine))
				goto error_free_engine;
			
			sleep(2);
			printf("End of synchronization\n");
			
			if (count == num)
				goto out;
		} while (g_random_int_range(0, 3) != 0);
		
		printf("Finalizing engine\n");
		osengine_finalize(engine);
		osengine_free(engine);
	}
	
out:
	osync_trace(TRACE_EXIT, "Stress test successful");
	printf("Stress test successful\n");
	return 0;
	
error_free_engine:
	osengine_free(engine);
error_free_env:
	osync_env_free(osync);
	g_main_loop_unref(loop);
	osync_trace(TRACE_EXIT_ERROR, "Stress test failed");
	printf("ERROR: Stress test failed\n");
	return 1;
}
