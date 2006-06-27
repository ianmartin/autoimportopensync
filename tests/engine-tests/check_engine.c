#include "support.h"

#include <opensync/opensync-group.h>
#include <opensync/opensync-client.h>
#include <opensync/opensync-engine.h>
#include <opensync/opensync-context.h>

#include "../mock-plugin/mock_sync.h"

START_TEST (engine_new)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncMember *member = osync_member_new(&error);
	fail_unless(member != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(group, member);
	
	member = osync_member_new(&error);
	fail_unless(member != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(group, member);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_ref(engine);
	osync_engine_unref(engine);
	osync_engine_unref(engine);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (engine_init)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncMember *member = osync_member_new(&error);
	fail_unless(member != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(group, member);
	osync_member_set_pluginname(member, "mock-sync");
	
	member = osync_member_new(&error);
	fail_unless(member != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(group, member);
	osync_member_set_pluginname(member, "mock-sync");
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	destroy_testbed(testbed);
}
END_TEST

typedef struct OSyncDebugGroup {
	OSyncGroup *group;
	OSyncMember *member1;
	OSyncClient *client1;
	
	OSyncMember *member2;
	OSyncClient *client2;
	
	OSyncPlugin *plugin;
} OSyncDebugGroup;


static void connect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	//mock_env *env = (mock_env *)data;
	//GError *direrror = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);

	/*env->dir = g_dir_open(env->path, 0, &direrror);
	if (direrror) {
		//Unable to open directory
		osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to open directory %s", env->path);
		g_error_free(direrror);
	} else {
		osync_context_report_success(ctx);
	}*/
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void *initialize(OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);

	mock_env *env = osync_try_malloc0(sizeof(mock_env), error);
	if (!env)
		goto error;

	OSyncObjTypeSink *sink = osync_objtype_sink_new("file", error);
	if (!sink)
		goto error;
	
	osync_objtype_sink_add_objformat(sink, "file");
	
	OSyncObjTypeSinkFunctions functions;
	memset(&functions, 0, sizeof(functions));
	functions.connect = connect;
	functions.disconnect = disconnect;
	
	osync_objtype_sink_set_functions(sink, functions);
	osync_plugin_info_add_objtype(info, sink);
	osync_objtype_sink_unref(sink);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void finalize(void *data)
{
	mock_env *env = data;
	g_free(env);
}

static OSyncDebugGroup *_create_group(char *testbed)
{
	OSyncDebugGroup *debug = g_malloc0(sizeof(OSyncDebugGroup));
	
	OSyncError *error = NULL;
	debug->group = osync_group_new(&error);
	fail_unless(debug->group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	debug->member1 = osync_member_new(&error);
	fail_unless(debug->member1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(debug->group, debug->member1);
	osync_member_set_pluginname(debug->member1, "mock-sync");
	char *path = g_strdup_printf("%s/configs/group/1", testbed);
	osync_member_set_configdir(debug->member1, path);
	g_free(path);
	osync_member_set_start_type(debug->member1, OSYNC_START_TYPE_EXTERNAL);
	
	debug->member2 = osync_member_new(&error);
	fail_unless(debug->member2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(debug->group, debug->member2);
	osync_member_set_pluginname(debug->member2, "mock-sync");
	path = g_strdup_printf("%s/configs/group/2", testbed);
	osync_member_set_configdir(debug->member2, path);
	g_free(path);
	osync_member_set_start_type(debug->member2, OSYNC_START_TYPE_EXTERNAL);
	
	debug->plugin = osync_plugin_new(&error);
	fail_unless(debug->plugin != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_plugin_set_name(debug->plugin, "mock-sync");
	osync_plugin_set_longname(debug->plugin, "Mock Sync Plugin");
	osync_plugin_set_description(debug->plugin, "This is a pseudo plugin");
	
	osync_plugin_set_initialize(debug->plugin, initialize);
	osync_plugin_set_finalize(debug->plugin, finalize);
	
	debug->client1 = osync_client_new(&error);
	fail_unless(debug->client1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	char *pipe_path = g_strdup_printf("%s/configs/group/1/pluginpipe", testbed);
	osync_client_run_external(debug->client1, pipe_path, &error);
	g_free(pipe_path);
	
	debug->client2 = osync_client_new(&error);
	fail_unless(debug->client2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	pipe_path = g_strdup_printf("%s/configs/group/2/pluginpipe", testbed);
	osync_client_run_external(debug->client2, pipe_path, &error);
	g_free(pipe_path);
	
	return debug;
}

START_TEST (engine_sync)
{
	char *testbed = setup_testbed("sync_setup");
	
	OSyncError *error = NULL;
	OSyncDebugGroup *debug = _create_group(testbed);
	
	OSyncEngine *engine = osync_engine_new(debug->group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *engine_suite(void)
{
	Suite *s = suite_create("Engine");
	Suite *s2 = suite_create("Engine");
	
	create_case(s, "engine_new", engine_new);
	create_case(s, "engine_init", engine_init);
	create_case(s2, "engine_sync", engine_sync);
	
	return s2;
}

int main(void)
{
	int nf;

	Suite *s = engine_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
