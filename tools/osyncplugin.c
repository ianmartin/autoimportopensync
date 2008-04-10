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


		/* TODO Porting - get_changes is method of OSyncObjTypeSink
		osync_member_get_change_data(member, change, stress_message_callback2, NULL);
		*/

		g_mutex_lock(working2);
		g_mutex_unlock(working2);
	}
	changes = g_list_append(changes, change);
}

static void connect(OSyncMember *member)
{
	//g_mutex_lock(working);

	/* TODO Porting - connect is method of OSyncObjTypeSink
	osync_member_connect(member, stress_message_callback, NULL);
	*/

	g_main_loop_run(loop);
	//g_mutex_lock(working);
	//g_mutex_unlock(working);
}

static void disconnect(OSyncMember *member)
{
	/*g_mutex_lock(working);
	osync_member_sync_done(member, stress_message_callback, NULL);
	g_mutex_lock(working);
	g_mutex_unlock(working);*/
	
	//g_mutex_lock(working);
	busy = TRUE;
	

	/* TODO Porting - sync_done is method of OSyncObjTypeSink
	osync_member_disconnect(member, stress_message_callback, NULL);
	*/
	
	if (busy)
		g_main_loop_run(loop);
	
	//g_mutex_lock(working);
	//g_mutex_unlock(working);
}

static void sync_done(OSyncMember *member)
{
	//g_mutex_lock(working);

	/* TODO Porting - sync_done is method of OSyncObjTypeSink
	osync_member_sync_done(member, stress_message_callback, NULL);
	*/

	g_main_loop_run(loop);
	//g_mutex_lock(working);
	//g_mutex_unlock(working);
}

static void committed_all(OSyncMember *member)
{
	/* TODO Porting - committed_all is method of OSyncObjTypeSink
	osync_member_committed_all(member, stress_message_callback, NULL);
	*/

	g_main_loop_run(loop);
}

static GList *get_changes(OSyncMember *member)
{
	changes = NULL;
	//g_mutex_lock(working);


	/* TODO Porting - get_changes is method of OSyncObjTypeSink
	osync_member_get_changeinfo(member, stress_message_callback, NULL);
	*/

	g_main_loop_run(loop);
	//g_mutex_lock(working);
	//g_mutex_unlock(working);
	printf("Number of changes %i\n", g_list_length(changes));
	return changes;
}

static OSyncChange *add_data(OSyncMember *member, const char *objtype)
{
	OSyncChange *change = NULL;
	/* TODO Porting: This function doesn't exist anymore. Introduce this for testing only....
	if (!(change = osync_member_add_random_data(member, objtype))) {
		printf("unable to add data\n");
		g_assert_not_reached();
	}
	*/
	printf("Added change with uid %s\n", osync_change_get_uid(change));
	return change;
}

static void modify_data(OSyncMember *member, OSyncChange *change)
{
	sleep(2);

	/* TODO Porting: This function doesn't exist anymore. Introduce this for testing only....
	if (!osync_member_modify_random_data(member, change)) {
		printf("unable to modify data\n");
		g_assert_not_reached();
	}
	*/
	printf("Modified change with uid %s\n", osync_change_get_uid(change));
	return;
}

static void delete_data(OSyncMember *member, OSyncChange *change)
{
	/* TODO Porting: OSyncChagne/OSyncData is not an attribute of OSyncMember - DROP?
	if (!osync_member_delete_data(member, change)) {
		printf("unable to delete data\n");
		g_assert_not_reached();
	}
	*/
	
	printf("Deleted change with uid %s\n", osync_change_get_uid(change));
	return;
}

static void multi_init(OSyncMember *member, const char *objtype)
{
	printf("initializing multiple times\n");
	connect(member);
	disconnect(member);
	
	/* TODO Porting - finalize is method of OSyncObjTypeSink
	osync_member_finalize(member);
	*/
	
	OSyncError *error = NULL;

	/* TODO Porting - initiailze is method of OSyncObjTypeSink
	if (!osync_member_initialize(member, &error)) {
		osync_trace(TRACE_EXIT_ERROR, "unable to initialize: %s", osync_error_print(&error));
		printf("Unable to initialize\n");
		exit(1);
	}
	*/
	
	if (objtype) {
		osync_member_set_objtype_enabled(member, "data", FALSE);
		osync_member_set_objtype_enabled(member, objtype, TRUE);
	}
	
	connect(member);
	disconnect(member);
	

	/* TODO Porting - finalize is method of OSyncObjTypeSink
	osync_member_finalize(member);
	*/
	

	/* TODO Porting - initiailze is method of OSyncObjTypeSink
	if (!osync_member_initialize(member, &error)) {
		osync_trace(TRACE_EXIT_ERROR, "unable to initialize: %s", osync_error_print(&error));
		printf("Unable to initialize\n");
		exit(1);
	}
	*/
	
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
	

	/* TODO Porting - set_slowsync is method of OSyncObjTypeSink
	if (noaccess)
		osync_member_set_slow_sync(member, "data", TRUE);
	*/
	
	
	printf("Reading data... \r");
	connect(member);
	GList *chg = get_changes(member);
	
	g_assert(g_list_length(chg) == 1);
	OSyncChange *newchange = g_list_nth_data(chg, 0);
	g_assert(osync_change_compare(newchange, osync_change_clone(change, NULL)) == OSYNC_CONV_DATA_SAME);
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
	

	/* TODO Porting - set_slowsync is method of OSyncObjTypeSink
	if (noaccess)
		osync_member_set_slow_sync(member, "data", TRUE);
	*/
	
	
	printf("Reading remaining data... \r");
	connect(member);
	chg = get_changes(member);
	if (noaccess) {
		g_assert(g_list_length(chg) == 0);
	} else {
		g_assert(g_list_length(chg) == 1);
		newchange = g_list_nth_data(chg, 0);
		g_assert(osync_change_get_changetype(newchange) == OSYNC_CHANGE_TYPE_DELETED);
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
	
	OSyncChange *cpychange1 = osync_change_clone(change1, NULL);
	OSyncChange *cpychange2 = osync_change_clone(change2, NULL);
	OSyncChange *cpychange3 = osync_change_clone(change3, NULL);
	
	
	OSyncChange *newchange = g_list_nth_data(chg, 0);
	g_assert(osync_change_compare(newchange, cpychange1) == OSYNC_CONV_DATA_SAME || osync_change_compare(newchange, cpychange2) == OSYNC_CONV_DATA_SAME || osync_change_compare(newchange, cpychange3) == OSYNC_CONV_DATA_SAME);
	
	newchange = g_list_nth_data(chg, 1);
	g_assert(osync_change_compare(newchange, cpychange1) == OSYNC_CONV_DATA_SAME || osync_change_compare(newchange, cpychange2) == OSYNC_CONV_DATA_SAME || osync_change_compare(newchange, cpychange3) == OSYNC_CONV_DATA_SAME);
	
	newchange = g_list_nth_data(chg, 2);
	g_assert(osync_change_compare(newchange, cpychange1) == OSYNC_CONV_DATA_SAME || osync_change_compare(newchange, cpychange2) == OSYNC_CONV_DATA_SAME || osync_change_compare(newchange, cpychange3) == OSYNC_CONV_DATA_SAME);
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
	g_assert(osync_change_get_changetype(newchange) == OSYNC_CHANGE_TYPE_DELETED);
	newchange = g_list_nth_data(chg, 1);
	g_assert(osync_change_get_changetype(newchange) == OSYNC_CHANGE_TYPE_DELETED);
	newchange = g_list_nth_data(chg, 2);
	g_assert(osync_change_get_changetype(newchange) == OSYNC_CHANGE_TYPE_DELETED);
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
	g_assert(osync_change_compare(newchange, osync_change_clone(change, NULL)) == OSYNC_CONV_DATA_SAME);
	disconnect(member);
	
	connect(member);
	modify_data(member, change);
	disconnect(member);
	
	connect(member);
	chg = get_changes(member);
	g_assert(g_list_length(chg) == 1);
	newchange = g_list_nth_data(chg, 0);
	g_assert(osync_change_compare(newchange, osync_change_clone(change, NULL)) == OSYNC_CONV_DATA_SAME);
	disconnect(member);
	
	connect(member);
	delete_data(member, change);
	disconnect(member);
	
	connect(member);
	chg = get_changes(member);
	g_assert(g_list_length(chg) == 1);
	newchange = g_list_nth_data(chg, 0);
	g_assert(osync_change_get_changetype(newchange) == OSYNC_CHANGE_TYPE_DELETED);
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
	

	/* TODO Porting - set_slowsync is method of OSyncObjTypeSink
	osync_member_set_slow_sync(member, "data", TRUE);
	*/

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
	char *formatdir = NULL;
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
		} else if (!strcmp (arg, "--formatdir")) {
			printf("formatdir %s\n", argv[i + 1]);
			formatdir = argv[i + 1];
			i++;
			if (!formatdir)
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

	OSyncPluginEnv *plugin_env = osync_plugin_env_new(&error);
	if (!plugin_env) {
		printf("Unable to allocate Plugin Environment: %s\n", osync_error_print(&error));
		return 1;
	}

	if (!osync_plugin_env_load(plugin_env, plugindir, &error)) {
		printf("Unable to load plugins: %s\n", osync_error_print(&error));
		osync_error_unref(&error);
		return 1;
	}

	if (plugin) {
		/* TODO get certain plugin */
	}
	
	OSyncFormatEnv *format_env = osync_format_env_new(&error);
	if (!format_env) {
		printf("Unable to load formats: %s\n", osync_error_print(&error));
		osync_error_unref(&error);
	}

	if (!osync_format_env_load_plugins(format_env, formatdir, &error)) {
		printf("Unable to load format plugins: %s\n", osync_error_print(&error));
		osync_error_unref(&error);
		return 1;
	}

	if (format) {
		/* TODO get certain format plugin */
	}

	
	OSyncGroup *group = osync_group_new(&error);
	if (!group) {
		fprintf(stderr, "Unable to allocate a OSyncGroup: %s\n", osync_error_print(&error));
		osync_error_unref(&error);
		return 1;
	}	

	osync_group_set_name(group, osync_rand_str(8));

	OSyncMember *member = osync_member_new(&error);
	if (!member) {
		fprintf(stderr, "Unable to allocate a OSyncMember: %s\n", osync_error_print(&error));
		osync_error_unref(&error);
		return 1;
	}	

	osync_group_add_member(group, member);
	
	char *testdir = g_strdup_printf("%s/plgtest.XXXXXX", g_get_tmp_dir());
	char *result = mkdtemp(testdir);
	
	if (result == NULL)
	{
		osync_trace(TRACE_EXIT_ERROR, "unable to create temporary dir: %s", g_strerror(errno));
		return 1;
	}
	
	char *config = NULL;
	int size = 0;
	if (configfile) {
		if (!osync_file_read(configfile, &config, &size, &error)) {
			fprintf(stderr, "Unable to read config: %s\n", osync_error_print(&error));
			osync_error_unref(&error);
			return 1;
		}
		osync_member_set_config(member, config, size);
	}
	
	osync_member_set_pluginname(member, pluginname);
	osync_member_set_configdir(member, testdir);


	/* TODO Porting: OSyncPluginInfo -> OSyncObjTypeSink -> Overwrite Sink Function

	OSyncMemberFunctions *functions = osync_member_get_memberfunctions(member);
	functions->rf_change = changes_sink;
	*/
	
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
	

	/* TODO Porting - initiailze is method of OSyncObjTypeSink
	if (!osync_member_initialize(member, &error)) {
		printf("unable to initialize: %s\n", osync_error_print(&error));
		return 1;
	}
	*/
	
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
	

	/* TODO Porting - finalize is method of OSyncObjTypeSink
	osync_member_finalize(member);
	*/
	
	return 0;
}
