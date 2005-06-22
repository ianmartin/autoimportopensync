#include <opensync/opensync.h>
#include "engine.h"
#include "engine_internals.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glib.h>

GMutex *working;
GMutex *working2;
GList *changes;
GList *tests;
osync_bool alwaysempty;
osync_bool noaccess = FALSE;
	
typedef struct OSyncPluginTest {
	char *name;
	void (*test)(OSyncEngine *engine, OSyncMember *file, const char *);
} OSyncPluginTest;

static void usage (char *name, int ecode)
{
  fprintf (stderr, "Usage: %s <pluginname>\n", name);
  fprintf (stderr, "--config <filename>\tSet the config file to use\n");
  fprintf (stderr, "--type <object type>\tSets the objtype to test\n");
  fprintf (stderr, "--empty\tOnly deleta all data. Do not test\n");
  exit (ecode);
}

gboolean busy = FALSE;
gboolean only_random = FALSE;

static void sync_now(OSyncEngine *engine)
{
	OSyncError *error = NULL;
	printf("Starting to synchronize\n");
	if (!osengine_sync_and_block(engine, &error)) {
		printf("Error while starting synchronization: %s\n", osync_error_print(&error));
		osync_error_free(&error);
		exit(1);
	}
	printf("Done synchronizing\n");
}

static void stress_message_callback(OSyncMember *member, void *user_data, OSyncError *error)
{
	busy = FALSE;
}

static void connect(OSyncMember *member)
{
	osync_member_connect(member, (OSyncEngCallback)stress_message_callback, NULL);
}

static void disconnect(OSyncMember *member)
{
	osync_member_disconnect(member, (OSyncEngCallback)stress_message_callback, NULL);
}

static void committed_all(OSyncMember *member)
{
	osync_member_committed_all(member, (OSyncEngCallback)stress_message_callback, NULL);
}

static OSyncChange *add_data(OSyncMember *member, const char *objtype)
{
	OSyncChange *change = NULL;
	if (!(change = osync_member_add_random_data(member, objtype))) {
		printf("unable to add data\n");
		g_assert_not_reached();
	}
	printf("Added change with uid %s\n", osync_change_get_uid(change));
	return change;
}

/*static void modify_data(OSyncMember *member, OSyncChange *change)
{
	sleep(2);
	if (!osync_member_modify_random_data(member, change)) {
		printf("unable to modify data\n");
		g_assert_not_reached();
	}
	printf("Modified change with uid %s\n", osync_change_get_uid(change));
	return;
}*/

static void delete_data(OSyncMember *member, OSyncChange *change)
{
	if (!osync_member_delete_data(member, change)) {
		printf("unable to delete data\n");
		g_assert_not_reached();
	}
	
	printf("Deleted change with uid %s\n", osync_change_get_uid(change));
	return;
}

static GList *get_changes(OSyncMember *member)
{
	changes = NULL;
	
	osync_member_get_changeinfo(member, (OSyncEngCallback)stress_message_callback, NULL);

	printf("Number of changes %i\n", g_list_length(changes));
	return changes;
}

void change_content(OSyncMember *member)
{
	OSyncChange *change = NULL;
	osync_member_set_slow_sync(member, "data", TRUE);
	
	osync_member_connect(member, (OSyncEngCallback)stress_message_callback, NULL);
	
	GList *c = get_changes(member);
	for (; c; c = c->next) {
		change = c->data;
		if (g_random_int_range(0, 3) == 0) {
			switch (g_random_int_range(1, 6)) {
				case 1:
				case 5:
					if (osync_member_modify_random_data(member, change))
						printf("Modifying data %s. Objtype: %s Format: %s\n", osync_change_get_uid(change), osync_objtype_get_name(osync_change_get_objtype(change)), osync_objformat_get_name(osync_change_get_objformat(change)));
					break;
				case 2:
					if (osync_member_delete_data(member, change))
						printf("Deleting data %s. Objtype: %s Format: %s\n", osync_change_get_uid(change), osync_objtype_get_name(osync_change_get_objtype(change)), osync_objformat_get_name(osync_change_get_objformat(change)));
					break;
				default:
					break;
			}
		}
	}
	
	int num_new = g_random_int_range(0, 8);
	int n = 0;
	for (n = 0; n < num_new; n++) {
		if ((change = osync_member_add_random_data(member, NULL)))
			printf("Adding new data %s. Objtype: %s Format: %s\n", osync_change_get_uid(change), osync_objtype_get_name(osync_change_get_objtype(change)), osync_objformat_get_name(osync_change_get_objformat(change)));
	}
	
	osync_member_disconnect(member, (OSyncEngCallback)stress_message_callback, NULL);
}

void check_sync(OSyncEngine *engine)
{
	printf("Synchronizing... ");
	fflush(stdout);
	sync_now(engine);
	printf("success\n");
	
	printf("Checking source... ");
	//Move testdir
	fflush(stdout);
	sync_now(engine);
	//Check empty
	printf("success\n");
	
	printf("Getting data... \r");
	osync_group_set_slow_sync(engine->group, "data", TRUE);
	fflush(stdout);
	sync_now(engine);
	printf("success\n");
	
	printf("Comparing data... \r");
	//diff dir
	printf("success\n");
}

static void add_test1(OSyncEngine *engine, OSyncMember *file, const char *objtype)
{
	printf("Test \"Add1\" starting\n");
	printf("Adding data... ");
	fflush(stdout);
	
	connect(file);
	add_data(file, objtype);
	disconnect(file);
	printf("success\n");

	check_sync(engine);
	
	printf("Test \"Add1\" ended\n");
}

static void empty_all(OSyncMember *member)
{
	printf("Emptying requested sources\n");
	
	osync_member_set_slow_sync(member, "data", TRUE);
	connect(member);
	GList *chg = get_changes(member);
	GList *i = NULL;
	int num_del = 0;
	for (i = chg; i; i = i->next) {
		OSyncChange *change = i->data;
		delete_data(member, change);
		num_del++;
	}
	committed_all(member);
	disconnect(member);

	printf("Done emptying\n");
}

static void run_all_tests(OSyncEngine *engine, OSyncMember *file, OSyncMember *target, const char *objtype)
{
	empty_all(file);
	empty_all(target);
	
	GList *t;
	for (t = tests; t; t = t->next) {
		OSyncPluginTest *test = t->data;
		test->test(engine, file, objtype);
	}
}

static void register_test(const char *name, void test(OSyncEngine *engine, OSyncMember *file, const char *))
{
	OSyncPluginTest *newtest = g_malloc0(sizeof(OSyncPluginTest));
	newtest->name = g_strdup(name);
	newtest->test = test;
	tests = g_list_append(tests, newtest);
}

static void register_tests(void)
{
	tests = NULL;
	register_test("add_test1", add_test1);
}

int main (int argc, char *argv[])
{
	int i;
	char *pluginname = NULL;
	char *plugindir = NULL;
	char *configfile = NULL;
	char *objtype = NULL;
	OSyncError *error = NULL;
	
	if (argc < 2)
		usage (argv[0], 1);

	pluginname = argv[1];
	for (i = 2; i < argc; i++) {
		char *arg = argv[i];
		if (!strcmp (arg, "--config")) {
			configfile = argv[i + 1];
			i++;
			if (!configfile)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--type")) {
			objtype = argv[i + 1];
			i++;
			if (!objtype)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--plugindir")) {
			printf("plugindir %s\n", argv[i + 1]);
			plugindir = argv[i + 1];
			i++;
			if (!plugindir)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--random")) {
			only_random = TRUE;
		} else if (!strcmp (arg, "--help")) {
			usage (argv[0], 0);
		} else {
			usage (argv[0], 1);
		}
	}
	
	OSyncEnv *env = osync_env_new();
	osync_env_set_option(env, "LOAD_GROUPS", "FALSE");
	
	if (plugindir)
		osync_env_set_option(env, "PLUGINS_DIRECTORY", plugindir);
	
	if (!osync_env_initialize(env, &error)) {
		printf("Unable to initialize environment: %s\n", osync_error_print(&error));
		osync_error_free(&error);
		return 1;
	}
	
	char *testdir = g_strdup_printf("%s/plgtest.XXXXXX", g_get_tmp_dir());
	char *result = mkdtemp(testdir);
	
	if (result == NULL)
	{
		osync_trace(TRACE_EXIT_ERROR, "unable to create temporary dir: %s", strerror(errno));
		return 1;
	}
	
	OSyncGroup *group = osync_group_new(env);
	osync_group_set_name(group, osync_rand_str(8));
	osync_group_set_configdir(group, testdir);
	OSyncMember *member = osync_member_new(group);
	
	char *config = NULL;
	int size = 0;
	if (configfile) {
		if (!osync_file_read(configfile, &config, &size, &error)) {
			fprintf(stderr, "Unable to read config: %s\n", osync_error_print(&error));
			osync_error_free(&error);
			return 1;
		}
		osync_member_set_config(member, config, size);
	}
	
	osync_member_set_pluginname(member, pluginname);
	
	OSyncMember *file = osync_member_new(group);
	
	testdir = g_strdup_printf("%s/plgtest.XXXXXX", g_get_tmp_dir());
	result = mkdtemp(testdir);
	
	if (result == NULL)
	{
		osync_trace(TRACE_EXIT_ERROR, "unable to create temporary dir: %s",
			strerror(errno));
		return 1;
	}
	
	config = g_strdup_printf("<config><path>%s</path><recursive>0</recursive></config>", testdir);
	osync_member_set_config(file, config, strlen(config) + 1);
	osync_member_set_pluginname(file, "file-sync");
	
	if (!osync_group_save(group, &error)) {
		printf("Error while creating syncengine: %s\n", osync_error_print(&error));
		osync_error_free(&error);
		goto error_free_env;
	}
	
	if (!g_thread_supported ()) g_thread_init (NULL);
	
	OSyncEngine *engine = osengine_new(group, &error);
	if (!engine) {
		printf("Error while creating syncengine: %s\n", osync_error_print(&error));
		osync_error_free(&error);
		goto error_free_env;
	}
	
	if (!osengine_init(engine, &error)) {
		printf("Error while initializing syncengine: %s\n", osync_error_print(&error));
		osync_error_free(&error);
		goto error_free_engine;
	}
	
	int count = 0;
	if (only_random) {
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
			
			change_content(file);
			
			check_sync(engine);
		} while (g_random_int_range(0, 3) != 0);
		
		printf("Finalizing engine\n");
		osengine_finalize(engine);
		osengine_free(engine);
		
		engine = osengine_new(group, &error);
		if (!engine) {
			printf("Error while creating syncengine: %s\n", osync_error_print(&error));
			osync_error_free(&error);
			goto error_free_env;
		}
		
		if (!osengine_init(engine, &error)) {
			printf("Error while initializing syncengine: %s\n", osync_error_print(&error));
			osync_error_free(&error);
			goto error_free_engine;
		}
	} else {
		register_tests();
		run_all_tests(engine, file, member, objtype);
	}
	
	return 0;

error_free_engine:
	osengine_free(engine);
error_free_env:
	osync_group_free(group);
	osync_env_free(env);
	return 1;
}
