#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>
#include "engine.h"
#include "engine_internals.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glib.h>
#include <sys/time.h>

GMutex *working;
GMutex *working2;
GList *changes;
GList *tests;
osync_bool alwaysempty;
osync_bool noaccess = FALSE;

typedef struct OSyncPluginTest {
	char *name;
	double (*test)(OSyncEngine *engine, OSyncMember *file, int num, const char *);
	double alltime;
	double connecttime;
	double readtime;
	double writetime;
	double othertime;
	int num;
} OSyncPluginTest;

static void usage (char *name, int ecode)
{
  fprintf (stderr, "Usage: %s <pluginname>\n", name);
  fprintf (stderr, "--config <filename>\tSet the config file to use\n");
  fprintf (stderr, "--type <object type>\tSets the objtype to test\n");
  fprintf (stderr, "--empty\tOnly deleta all data. Do not test\n");
  exit (ecode);
}

gboolean only_random = FALSE;
char *localdir = NULL;

static void sync_now(OSyncEngine *engine)
{
	OSyncError *error = NULL;
	printf(".");
	fflush(stdout);

	if (!osengine_sync_and_block(engine, &error)) {
		printf("Error while starting synchronization: %s\n", osync_error_print(&error));
		osync_error_unref(&error);
		exit(1);
	}

	printf(".");
	fflush(stdout);
}

static void check_empty(void)
{
	printf(".");
	fflush(stdout);
	char *command = g_strdup_printf("test \"x$(ls %s)\" = \"x\"", localdir);
	int ret = system(command);
	g_free(command);
	if (ret)
		abort();
	printf(".");
	fflush(stdout);
}

double starttime;
double currenttime;

double connecttime;
double readtime;
double writetime;

double _second()     /* note that some compilers like AIX xlf do not require the trailing  '_' */
{
    struct timeval tp;
    int rtn;
    rtn=gettimeofday(&tp, NULL);

    return ((double)tp.tv_sec+(1.e-6)*tp.tv_usec);
}

void engine_status(OSyncEngine *engine, OSyncEngineUpdate *status, void *user_data)
{
	switch (status->type) {
		case ENG_ENDPHASE_CON:
			osync_trace(TRACE_INTERNAL, "++++++++++++++++ Connection Phase ended ++++++++++++++");
			currenttime = _second();
			connecttime = currenttime - starttime;
			break;
		case ENG_ENDPHASE_READ:
			osync_trace(TRACE_INTERNAL, "++++++++++++++++ Read Phase ended ++++++++++++++");
			readtime = _second() - currenttime;
			currenttime = _second();
			break;
		case ENG_ENDPHASE_WRITE:
			osync_trace(TRACE_INTERNAL, "++++++++++++++++ Write Phase ended ++++++++++++++");
			writetime = _second() - currenttime;
			currenttime = _second();
			break;
		default:
			;
	}
}

double check_sync(OSyncEngine *engine, const char *name, int num)
{
	int ret;
	printf(".");
	fflush(stdout);
	starttime = _second();
	osync_trace(TRACE_INTERNAL, "++++++++++++++++ Test \"%s %i\" starting ++++++++++++++", name, num);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	sync_now(engine);
	osengine_set_enginestatus_callback(engine, NULL, NULL);
	int wasted = 0;
	int alldeciders = 0;
	osengine_get_wasted(engine, &alldeciders, &wasted);
	osync_trace(TRACE_INTERNAL, "++++++++++++++++ Test \"%s %i\" ended (%i / %i (%i%%)) ++++++++++++++", name, num, wasted, alldeciders, (int)(((float)wasted / (float)alldeciders) * 100));
	double thistime = _second() - starttime;

	printf(".");
	fflush(stdout);

	char *tempdir = g_strdup_printf("%s/plgtest.XXXXXX", g_get_tmp_dir());
	if (!mkdtemp(tempdir))
	{
		g_free(tempdir);
		osync_trace(TRACE_INTERNAL, "unable to create temporary dir: %s", g_strerror(errno));
		abort();
	}
	char *command = g_strdup_printf("mv %s/* %s &> /dev/null", localdir, tempdir);
	ret = system(command);
	if (ret)
	{
		g_free(tempdir);
		g_free(command);
		osync_trace(TRACE_INTERNAL, "Unable to move files to temporary dir: %d", ret);
		abort();
	}
	g_free(command);
	printf(".");
	fflush(stdout);

	check_empty();

	printf(".");
	fflush(stdout);

	osync_group_set_slow_sync(engine->group, "data", TRUE);

	sync_now(engine);
	printf(".");
	fflush(stdout);
	command = g_strdup_printf("test \"x$(diff -x \".*\" %s %s)\" = \"x\"", localdir, tempdir);
	int result = system(command);
	g_free(command);

	g_free(tempdir);
	if (result)
		abort();

	printf(" success\n");
	return thistime;
}

void add_data(OSyncMember *member, const char *objtype)
{
	OSyncChange *change = osync_change_new();
	if (!osync_member_make_random_data(member, change, objtype)) {
		printf("Unable to create random data\n");
		abort();
	}

	char *filename = NULL;
	while (1) {
		char *randstr = osync_rand_str(8);
		filename = g_strdup_printf("%s/%s", localdir, randstr);
		g_free(randstr);
		char *command = g_strdup_printf("ls %s &> /dev/null", filename);
		int ret = system(command);
		g_free(command);
		if (ret)
			break;
		g_free(filename);
	}

	OSyncError *error = NULL;
	if (!osync_file_write(filename, osync_change_get_data(change), osync_change_get_datasize(change), 0700, &error)) {
		printf("Unable to write to file %s\n", osync_error_print(&error));
		abort();
	}
	g_free(filename);
}

void modify_data(OSyncMember *member, const char *objtype)
{
	GDir *dir;
	GError *gerror = NULL;

	dir = g_dir_open(localdir, 0, &gerror);
	if (!dir)
		abort();

	const char *de = NULL;
	while ((de = g_dir_read_name(dir))) {
		char *filename = g_build_filename(localdir, de, NULL);

		OSyncChange *change = osync_change_new();
		if (!osync_member_make_random_data(member, change, objtype)) {
			printf("Unable to create random data\n");
			abort();
		}

		OSyncError *error = NULL;
		if (!osync_file_write(filename, osync_change_get_data(change), osync_change_get_datasize(change), 0700, &error)) {
			printf("Unable to write to file %s\n", osync_error_print(&error));
			abort();
		}

		g_free(filename);
	}
	g_dir_close(dir);
}


void delete_data(OSyncMember *member, const char *objtype)
{
	char *command = g_strdup_printf("rm -f %s/*", localdir);
	int ret = system(command);
	if (ret)
	{
		osync_trace(TRACE_INTERNAL, "Unable to delete data: %d", ret);
		abort();
	}
	g_free(command);
}

static void empty_all(OSyncEngine *engine)
{
	printf(".");
	fflush(stdout);

	osync_group_set_slow_sync(engine->group, "data", TRUE);
	sync_now(engine);

	char *command = g_strdup_printf("rm -f %s/*", localdir);
	int ret = system(command);
	if (ret)
	{
		osync_trace(TRACE_INTERNAL, "Unable to delete data: %d", ret);
		abort();
	}
	g_free(command);
	sync_now(engine);

	printf(".");
	fflush(stdout);

	check_empty();

}

double add_test(OSyncEngine *engine, OSyncMember *member, int num, const char *objtype)
{
	printf("Test \"Add %i\" starting", num);
	fflush(stdout);

	empty_all(engine);

	printf(".");
	fflush(stdout);
	int i = 0;
	for (i = 0; i < num; i++)
		add_data(member, objtype);
	printf(".");
	fflush(stdout);

	return check_sync(engine, "Add", num);
}

double modify_test(OSyncEngine *engine, OSyncMember *member, int num, const char *objtype)
{
	printf("Test \"Modify %i\" starting", num);
	fflush(stdout);

	empty_all(engine);

	printf(".");
	fflush(stdout);
	int i = 0;
	for (i = 0; i < num; i++)
		add_data(member, objtype);
	printf(".");
	fflush(stdout);

	check_sync(engine, "None", num);

	printf(".");
	fflush(stdout);
	modify_data(member, objtype);
	printf(".");
	fflush(stdout);

	return check_sync(engine, "Modify", num);
}

double delete_test(OSyncEngine *engine, OSyncMember *member, int num, const char *objtype)
{
	printf("Test \"Delete %i\" starting", num);
	fflush(stdout);

	empty_all(engine);

	printf(".");
	fflush(stdout);
	int i = 0;
	for (i = 0; i < num; i++)
		add_data(member, objtype);
	printf(".");
	fflush(stdout);

	check_sync(engine, "None", num);

	printf(".");
	fflush(stdout);
	delete_data(member, objtype);
	printf(".");
	fflush(stdout);

	return check_sync(engine, "Delete", num);
}

static void run_all_tests(OSyncEngine *engine, OSyncMember *file, OSyncMember *target, const char *objtype)
{

	GList *t;
	for (t = tests; t; t = t->next) {
		OSyncPluginTest *test = t->data;
		test->alltime = test->test(engine, target, test->num, objtype);
		test->connecttime = connecttime;
		test->readtime = readtime;
		test->writetime = writetime;
		test->othertime  = test->alltime - test->connecttime - test->readtime - test->writetime;
	}

	printf("\nOutcome:\n");

	for (t = tests; t; t = t->next) {
		OSyncPluginTest *test = t->data;
		printf("Test \"%s\": All: %f Connect: %f(%i%%) Read: %f(%i%%) Write: %f(%i%%) Other: %f(%i%%)\n", test->name, test->alltime, test->connecttime, (int)((test->connecttime / test->alltime)* 100), test->readtime, (int)((test->readtime / test->alltime)* 100), test->writetime, (int)((test->writetime / test->alltime)* 100), test->othertime, (int)((test->othertime / test->alltime)* 100));
	}
}

static void register_test(const char *name, double test(OSyncEngine *engine, OSyncMember *file, int num, const char *), int num)
{
	OSyncPluginTest *newtest = g_malloc0(sizeof(OSyncPluginTest));
	newtest->name = g_strdup(name);
	newtest->test = test;
	newtest->num = num;
	tests = g_list_append(tests, newtest);
}

static void register_tests(void)
{
	tests = NULL;
	/*register_test("add_test1", add_test, 1);
	register_test("add_test5", add_test, 5);
	register_test("add_test10", add_test, 10);
	register_test("add_test20", add_test, 20);*/
	/*register_test("add_test50", add_test, 50);
	register_test("add_test100", add_test, 100);
	register_test("add_test200", add_test, 200);*/

	/*register_test("modify_test1", modify_test, 1);
	register_test("modify_test5", modify_test, 5);
	register_test("modify_test10", modify_test, 10);
	register_test("modify_test20", modify_test, 20);
	register_test("modify_test50", modify_test, 50);*/
	register_test("modify_test100", modify_test, 100);
	//register_test("modify_test200", modify_test, 200);

	/*register_test("delete_test1", delete_test, 1);
	register_test("delete_test5", delete_test, 5);
	register_test("delete_test10", delete_test, 10);
	register_test("delete_test20", delete_test, 20);
	register_test("delete_test50", delete_test, 50);*/
	//register_test("delete_test100", delete_test, 100);
	//register_test("delete_test200", delete_test, 200);
}

void change_content(void)
{
	printf("changing content\n");
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

	OSyncEnv *env = osync_env_new(NULL);
	osync_env_set_option(env, "LOAD_GROUPS", "FALSE");

	if (plugindir)
		osync_env_set_option(env, "PLUGINS_DIRECTORY", plugindir);

	if (!osync_env_initialize(env, &error)) {
		printf("Unable to initialize environment: %s\n", osync_error_print(&error));
		osync_error_unref(&error);
		return 1;
	}

	char *testdir = g_strdup_printf("%s/plgtest.XXXXXX", g_get_tmp_dir());
	char *result = mkdtemp(testdir);

	if (result == NULL)
	{
		osync_trace(TRACE_EXIT_ERROR, "unable to create temporary dir: %s", g_strerror(errno));
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
			osync_error_unref(&error);
			return 1;
		}
		osync_member_set_config(member, config, size);
	}

	osync_member_set_pluginname(member, pluginname);

	OSyncMember *file = osync_member_new(group);

	localdir = g_strdup_printf("%s/plgtest.XXXXXX", g_get_tmp_dir());
	result = mkdtemp(localdir);

	if (result == NULL)
	{
		osync_trace(TRACE_EXIT_ERROR, "unable to create temporary dir: %s",
			g_strerror(errno));
		return 1;
	}

	config = g_strdup_printf("<config><path>%s</path><recursive>0</recursive></config>", localdir);
	osync_member_set_config(file, config, strlen(config) + 1);
	osync_member_set_pluginname(file, "file-sync");

	if (!osync_group_save(group, &error)) {
		printf("Error while creating syncengine: %s\n", osync_error_print(&error));
		osync_error_unref(&error);
		goto error_free_env;
	}

	if (!g_thread_supported ()) g_thread_init (NULL);

	OSyncEngine *engine = osengine_new(group, &error);
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

			change_content();

			check_sync(engine, "Random", 1);
		} while (g_random_int_range(0, 3) != 0);

		printf("Finalizing engine\n");
		osengine_finalize(engine);
		osengine_free(engine);

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
	} else {
		register_tests();
		run_all_tests(engine, file, member, objtype);
	}

	printf("\nCompleted successfully\n");
	return 0;

error_free_engine:
	osengine_free(engine);
error_free_env:
	osync_group_free(group);
	osync_env_free(env);
	return 1;
}
