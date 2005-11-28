#include <opensync/opensync.h>
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
	void (*test)(OSyncMember *, const char *);
} OSyncPluginTest;

static void usage (char *name, int ecode)
{
  fprintf (stderr, "Usage: %s <pluginname>\n", name);
  fprintf (stderr, "--config <filename>\tSet the config file to use\n");
  fprintf (stderr, "--type <object type>\tSets the objtype to test\n");
  fprintf (stderr, "--empty\tOnly deleta all data. Do not test\n");
  exit (ecode);
}

GMainLoop *loop = NULL;
gboolean busy = FALSE;

static void stress_message_callback(OSyncMember *member, void *user_data, OSyncError *error)
{
	//g_mutex_unlock(working);
	g_main_loop_quit(loop);
	busy = FALSE;
}

static void stress_message_callback2(OSyncMember *member, void *user_data, OSyncError *error)
{
	g_mutex_unlock(working2);
}

static void changes_sink(OSyncMember *member, OSyncChange *change, void *user_data)
{
	if (!osync_change_has_data(change)) {
		g_mutex_lock(working2);
		osync_member_get_change_data(member, change, (OSyncEngCallback)stress_message_callback2, NULL);
		g_mutex_lock(working2);
		g_mutex_unlock(working2);
	}
	changes = g_list_append(changes, change);
}

static void connect(OSyncMember *member)
{
	//g_mutex_lock(working);
	osync_member_connect(member, (OSyncEngCallback)stress_message_callback, NULL);
	g_main_loop_run(loop);
	//g_mutex_lock(working);
	//g_mutex_unlock(working);
}

static void disconnect(OSyncMember *member)
{
	/*g_mutex_lock(working);
	osync_member_sync_done(member, (OSyncEngCallback)stress_message_callback, NULL);
	g_mutex_lock(working);
	g_mutex_unlock(working);*/
	
	//g_mutex_lock(working);
	busy = TRUE;
	
	osync_member_disconnect(member, (OSyncEngCallback)stress_message_callback, NULL);
	
	if (busy)
		g_main_loop_run(loop);
	
	//g_mutex_lock(working);
	//g_mutex_unlock(working);
}

static void sync_done(OSyncMember *member)
{
	//g_mutex_lock(working);
	osync_member_sync_done(member, (OSyncEngCallback)stress_message_callback, NULL);
	g_main_loop_run(loop);
	//g_mutex_lock(working);
	//g_mutex_unlock(working);
}

static void committed_all(OSyncMember *member)
{
	osync_member_committed_all(member, (OSyncEngCallback)stress_message_callback, NULL);
	g_main_loop_run(loop);
}

static GList *get_changes(OSyncMember *member)
{
	changes = NULL;
	//g_mutex_lock(working);
	osync_member_get_changeinfo(member, (OSyncEngCallback)stress_message_callback, NULL);
	g_main_loop_run(loop);
	//g_mutex_lock(working);
	//g_mutex_unlock(working);
	printf("Number of changes %i\n", g_list_length(changes));
	return changes;
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

static void modify_data(OSyncMember *member, OSyncChange *change)
{
	sleep(2);
	if (!osync_member_modify_random_data(member, change)) {
		printf("unable to modify data\n");
		g_assert_not_reached();
	}
	printf("Modified change with uid %s\n", osync_change_get_uid(change));
	return;
}

static void delete_data(OSyncMember *member, OSyncChange *change)
{
	if (!osync_member_delete_data(member, change)) {
		printf("unable to delete data\n");
		g_assert_not_reached();
	}
	
	printf("Deleted change with uid %s\n", osync_change_get_uid(change));
	return;
}

static void multi_init(OSyncMember *member, const char *objtype)
{
	printf("initializing multiple times\n");
	connect(member);
	disconnect(member);
	
	osync_member_finalize(member);
	
	OSyncError *error = NULL;
	if (!osync_member_initialize(member, &error)) {
		osync_trace(TRACE_EXIT_ERROR, "unable to initialize: %s", osync_error_print(&error));
		printf("Unable to initialize\n");
		exit(1);
	}
	
	if (objtype) {
		osync_member_set_objtype_enabled(member, "data", FALSE);
		osync_member_set_objtype_enabled(member, objtype, TRUE);
	}
	
	connect(member);
	disconnect(member);
	
	osync_member_finalize(member);
	
	if (!osync_member_initialize(member, &error)) {
		osync_trace(TRACE_EXIT_ERROR, "unable to initialize: %s", osync_error_print(&error));
		printf("Unable to initialize\n");
		exit(1);
	}
	
	if (objtype) {
		osync_member_set_objtype_enabled(member, "data", FALSE);
		osync_member_set_objtype_enabled(member, objtype, TRUE);
	}
}

static void add_test1(OSyncMember *member, const char *objtype)
{
	printf("Test \"Add1\" starting\n");
	printf("Adding data... ");
	fflush(stdout);
	connect(member);
	get_changes(member);
	OSyncChange *change = add_data(member, objtype);
	committed_all(member);
	if (noaccess)
		sync_done(member);
	disconnect(member);
	printf("success\n");
	
	if (noaccess)
		osync_member_set_slow_sync(member, "data", TRUE);
	
	
	printf("Reading data... \r");
	connect(member);
	GList *chg = get_changes(member);
	
	g_assert(g_list_length(chg) == 1);
	OSyncChange *newchange = g_list_nth_data(chg, 0);
	g_assert(osync_change_compare(newchange, osync_change_copy(change, NULL)) == CONV_DATA_SAME);
	committed_all(member);
	sync_done(member);
	disconnect(member);
	printf("success\n");
	
	printf("Deleting data... \r");
	connect(member);
	delete_data(member, change);
	committed_all(member);
	if (noaccess)
		sync_done(member);
	disconnect(member);
	printf("success\n");
	
	if (noaccess)
		osync_member_set_slow_sync(member, "data", TRUE);
	
	
	printf("Reading remaining data... \r");
	connect(member);
	chg = get_changes(member);
	if (noaccess) {
		g_assert(g_list_length(chg) == 0);
	} else {
		g_assert(g_list_length(chg) == 1);
		newchange = g_list_nth_data(chg, 0);
		g_assert(osync_change_get_changetype(newchange) == CHANGE_DELETED);
	}
	disconnect(member);
	printf("success\n");
	
	printf("Test \"Add1\" ended\n");
}

static void add_test2(OSyncMember *member, const char *objtype)
{
	printf("Test \"Add2\" starting\n");
	connect(member);
	OSyncChange *change1 = add_data(member, objtype);
	OSyncChange *change2 = add_data(member, objtype);
	OSyncChange *change3 = add_data(member, objtype);
	disconnect(member);
	
	connect(member);
	GList *chg = get_changes(member);
	g_assert(g_list_length(chg) == 3);
	
	OSyncChange *cpychange1 = osync_change_copy(change1, NULL);
	OSyncChange *cpychange2 = osync_change_copy(change2, NULL);
	OSyncChange *cpychange3 = osync_change_copy(change3, NULL);
	
	
	OSyncChange *newchange = g_list_nth_data(chg, 0);
	g_assert(osync_change_compare(newchange, cpychange1) == CONV_DATA_SAME || osync_change_compare(newchange, cpychange2) == CONV_DATA_SAME || osync_change_compare(newchange, cpychange3) == CONV_DATA_SAME);
	
	newchange = g_list_nth_data(chg, 1);
	g_assert(osync_change_compare(newchange, cpychange1) == CONV_DATA_SAME || osync_change_compare(newchange, cpychange2) == CONV_DATA_SAME || osync_change_compare(newchange, cpychange3) == CONV_DATA_SAME);
	
	newchange = g_list_nth_data(chg, 2);
	g_assert(osync_change_compare(newchange, cpychange1) == CONV_DATA_SAME || osync_change_compare(newchange, cpychange2) == CONV_DATA_SAME || osync_change_compare(newchange, cpychange3) == CONV_DATA_SAME);
	sync_done(member);
	disconnect(member);
	
	connect(member);
	delete_data(member, change1);
	delete_data(member, change2);
	delete_data(member, change3);
	disconnect(member);
	
	connect(member);
	chg = get_changes(member);
	g_assert(g_list_length(chg) == 3);
	newchange = g_list_nth_data(chg, 0);
	g_assert(osync_change_get_changetype(newchange) == CHANGE_DELETED);
	newchange = g_list_nth_data(chg, 1);
	g_assert(osync_change_get_changetype(newchange) == CHANGE_DELETED);
	newchange = g_list_nth_data(chg, 2);
	g_assert(osync_change_get_changetype(newchange) == CHANGE_DELETED);
	disconnect(member);
	
	connect(member);
	chg = get_changes(member);
	g_assert(g_list_length(chg) == 0);
	disconnect(member);
	printf("Test \"Add3\" ended\n");
}

static void modify_test1(OSyncMember *member, const char *objtype)
{
	printf("Test \"Modify1\" starting\n");
	
	connect(member);
	OSyncChange *change = add_data(member, objtype);
	disconnect(member);
	
	connect(member);
	GList *chg = get_changes(member);
	g_assert(g_list_length(chg) == 1);
	OSyncChange *newchange = g_list_nth_data(chg, 0);
	g_assert(osync_change_compare(newchange, osync_change_copy(change, NULL)) == CONV_DATA_SAME);
	disconnect(member);
	
	connect(member);
	modify_data(member, change);
	disconnect(member);
	
	connect(member);
	chg = get_changes(member);
	g_assert(g_list_length(chg) == 1);
	newchange = g_list_nth_data(chg, 0);
	g_assert(osync_change_compare(newchange, osync_change_copy(change, NULL)) == CONV_DATA_SAME);
	disconnect(member);
	
	connect(member);
	delete_data(member, change);
	disconnect(member);
	
	connect(member);
	chg = get_changes(member);
	g_assert(g_list_length(chg) == 1);
	newchange = g_list_nth_data(chg, 0);
	g_assert(osync_change_get_changetype(newchange) == CHANGE_DELETED);
	disconnect(member);
	
	connect(member);
	chg = get_changes(member);
	g_assert(g_list_length(chg) == 0);
	disconnect(member);
	printf("Test \"Modify1\" ended\n");
}

static void empty_all(OSyncMember *member)
{
	printf("Emptying requested sources (Access available: %s)\n", noaccess == TRUE ? "No" : "Yes");
	//connect(member);
	//sync_done(member);
	//disconnect(member);
	
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
	
	if (!alwaysempty && !noaccess) {
		connect(member);
		chg = get_changes(member);
		g_assert(g_list_length(chg) == num_del);
		disconnect(member);
		
		connect(member);
		chg = get_changes(member);
		g_assert(g_list_length(chg) == 0);
		disconnect(member);
	}
	printf("Done emptying\n");
}

static void run_all_tests(OSyncMember *member, const char *objtype)
{
	empty_all(member);
	GList *t;
	for (t = tests; t; t = t->next) {
		OSyncPluginTest *test = t->data;
		test->test(member, objtype);
	}
}

static void run_test(const char *name, OSyncMember *member, const char *objtype)
{
	empty_all(member);
	GList *t;
	for (t = tests; t; t = t->next) {
		OSyncPluginTest *test = t->data;
		if (!strcmp(name, test->name))
			test->test(member, objtype);
	}
}

static void register_test(const char *name, void test(OSyncMember *, const char *))
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
	register_test("add_test1", add_test1);
	register_test("add_test2", add_test2);
	register_test("modify_test1", modify_test1);
	register_test("multi_init", multi_init);
}

int main (int argc, char *argv[])
{
	int i;
	char *pluginname = NULL;
	char *plugindir = NULL;
	char *plugin = NULL;
	char *format = NULL;
	char *configfile = NULL;
	char *objtype = NULL;
	char *testname = NULL;
	OSyncError *error = NULL;
	alwaysempty = FALSE;
	gboolean emptyonly = FALSE;
	noaccess = TRUE;
	
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
		} else if (!strcmp (arg, "--plugin")) {
			plugin = argv[i + 1];
			i++;
			if (!plugin)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--format")) {
			format = argv[i + 1];
			i++;
			if (!format)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--help")) {
			usage (argv[0], 0);
		} else if (!strcmp (arg, "--alwaysempty")) {
			alwaysempty = TRUE;
		} else if (!strcmp (arg, "--noaccess")) {
			noaccess = TRUE;
		} else if (!strcmp (arg, "--empty")) {
			emptyonly = TRUE;
		} else {
			if (testname)
				usage (argv[0], 1);
			testname = argv[i + 1];
		}
	}
	
	OSyncEnv *env = osync_env_new();
	osync_env_set_option(env, "LOAD_GROUPS", "FALSE");
	
	if (plugin) {
		osync_env_set_option(env, "LOAD_PLUGINS", "FALSE");
		if (!osync_module_load(env, plugin, &error)) {
			printf("Unable to load plugin: %s\n", osync_error_print(&error));
			osync_error_free(&error);
			return 1;
		}
	} else {
		if (plugindir)
			osync_env_set_option(env, "PLUGINS_DIRECTORY", plugindir);
	}
	
	if (format) {
		osync_env_set_option(env, "LOAD_FORMATS", "FALSE");
		if (!osync_module_load(env, format, &error)) {
			printf("Unable to load format: %s\n", osync_error_print(&error));
			osync_error_free(&error);
			return 1;
		}
	}
	
	if (!osync_env_initialize(env, &error)) {
		printf("Unable to initialize environment: %s\n", osync_error_print(&error));
		osync_error_free(&error);
		return 1;
	}
	
	OSyncGroup *group = osync_group_new(env);
	osync_group_set_name(group, osync_rand_str(8));
	OSyncMember *member = osync_member_new(group);
	
	char *testdir = g_strdup_printf("%s/plgtest.XXXXXX", g_get_tmp_dir());
	char *result = mkdtemp(testdir);
	
	if (result == NULL)
	{
		osync_trace(TRACE_EXIT_ERROR, "unable to create temporary dir: %s",
			strerror(errno));
		return 1;
	}
	
	osync_member_set_pluginname(member, pluginname);
	osync_member_set_configdir(member, testdir);
	OSyncMemberFunctions *functions = osync_member_get_memberfunctions(member);
	functions->rf_change = changes_sink;
	
	//started_mutex = g_mutex_new();
	//started = g_cond_new();
	//GMainContext *context = g_main_context_new();
	loop = g_main_loop_new(NULL, TRUE);
	//g_mutex_lock(started_mutex);
	//GSource *idle = g_idle_source_new();
	//g_source_set_callback(idle, startupfunc, NULL, NULL);
   // g_source_attach(idle, context);
	//g_thread_create ((GThreadFunc)g_main_loop_run, loop, TRUE, NULL);
	//g_cond_wait(started, started_mutex);
	//g_mutex_unlock(started_mutex);
	
	//osync_member_set_loop(member, context);
	
	if (!osync_member_initialize(member, &error)) {
		printf("unable to initialize: %s\n", osync_error_print(&error));
		return 1;
	}
	
	if (objtype) {
		osync_member_set_objtype_enabled(member, "data", FALSE);
		osync_member_set_objtype_enabled(member, objtype, TRUE);
	}
	
	if (!g_thread_supported ()) g_thread_init (NULL);
	working = g_mutex_new();
	working2 = g_mutex_new();
	
	if (emptyonly) {
		empty_all(member);
	} else {
		register_tests();
		
		if (testname)
			run_test(testname, member, objtype);
		else
			run_all_tests(member, objtype);
	}
	
	osync_member_finalize(member);
	
	return 0;
}
