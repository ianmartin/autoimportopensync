#include "support.h"
#include "engine_support.h"

#include "opensync/engine/opensync_engine_internals.h"

#include <opensync/opensync-group.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-client.h>
#include <opensync/opensync-engine.h>
#include <opensync/opensync-context.h>

#include "../mock-plugin/mock_sync.h"

static void _member_add_objtype(OSyncMember *member, const char *objtype)
{
       OSyncObjTypeSink *sink = NULL;
       osync_assert(member);
       osync_assert(objtype);

       if (!osync_member_find_objtype_sink(member, objtype)) {
               sink = osync_objtype_sink_new(objtype, NULL);
	       osync_member_add_objtype_sink(member, sink);
       }
}

int num_connect = 0;
int num_disconnect = 0;
int num_get_changes = 0;

static void connect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	
	g_atomic_int_inc(&(num_connect));
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void connect_error(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	
	g_atomic_int_inc(&(num_connect));
	
	osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "connect error");
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	
	g_atomic_int_inc(&(num_disconnect));
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void get_changes(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	
	g_atomic_int_inc(&(num_get_changes));
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void *initialize_error(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);

	osync_error_set(error, OSYNC_ERROR_GENERIC, "init error");
	
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void *initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);

	mock_env *env = osync_try_malloc0(sizeof(mock_env), error);
	if (!env)
		goto error;

	OSyncObjTypeSink *sink = osync_objtype_sink_new("mockobjtype1", error);
	if (!sink)
		goto error;
	
	osync_objtype_sink_add_objformat(sink, "mockobjtype1");
	
	OSyncObjTypeSinkFunctions functions;
	memset(&functions, 0, sizeof(functions));
	functions.connect = connect;
	functions.disconnect = disconnect;
	functions.get_changes = get_changes;
	
	osync_objtype_sink_set_functions(sink, functions, NULL);
	osync_plugin_info_add_objtype(info, sink);
	osync_objtype_sink_unref(sink);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void *initialize_connect_error(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);

	mock_env *env = osync_try_malloc0(sizeof(mock_env), error);
	if (!env)
		goto error;

	OSyncObjTypeSink *sink = osync_objtype_sink_new("mockobjtype1", error);
	if (!sink)
		goto error;
	
	osync_objtype_sink_add_objformat(sink, "mockobjtype1");
	
	OSyncObjTypeSinkFunctions functions;
	memset(&functions, 0, sizeof(functions));
	functions.connect = connect_error;
	functions.disconnect = disconnect;
	functions.get_changes = get_changes;
	
	osync_objtype_sink_set_functions(sink, functions, NULL);
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
	char *path = g_strdup_printf("%s/configs/group", testbed);
	osync_group_set_configdir(debug->group, path);
	g_free(path);
	
	debug->member1 = osync_member_new(&error);
	fail_unless(debug->member1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(debug->group, debug->member1);
	osync_member_set_pluginname(debug->member1, "mock-sync-foo");
	path = g_strdup_printf("%s/configs/group/1", testbed);
	osync_member_set_configdir(debug->member1, path);
	g_free(path);
	_member_add_objtype(debug->member1, "mockobjtype1");
	osync_member_add_objformat(debug->member1, "mockobjtype1", "mockformat1");
	osync_member_set_config(debug->member1, "<config><directory><path>data1</path><objtype>mockobjtype1</objtype></directory></config>");
	
	debug->member2 = osync_member_new(&error);
	fail_unless(debug->member2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(debug->group, debug->member2);
	osync_member_set_pluginname(debug->member2, "mock-sync-foo");
	path = g_strdup_printf("%s/configs/group/2", testbed);
	osync_member_set_configdir(debug->member2, path);
	g_free(path);

	_member_add_objtype(debug->member2, "mockobjtype1");
	osync_member_add_objformat(debug->member2, "mockobjtype1", "mockformat1");
	osync_member_set_config(debug->member2, "<config><directory><path>data2</path><objtype>mockobjtype1</objtype></directory></config>");
	
	
	
	debug->plugin = osync_plugin_new(&error);
	fail_unless(debug->plugin != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_plugin_set_name(debug->plugin, "mock-sync-foo");
	osync_plugin_set_longname(debug->plugin, "Mock Sync Plugin");
	osync_plugin_set_description(debug->plugin, "This is a pseudo plugin");
	osync_plugin_set_start_type(debug->plugin, OSYNC_START_TYPE_EXTERNAL);
	osync_plugin_set_config_type(debug->plugin, OSYNC_PLUGIN_NO_CONFIGURATION);
	
	osync_plugin_set_initialize(debug->plugin, initialize);
	osync_plugin_set_finalize(debug->plugin, finalize);
	
	
	debug->plugin2 = osync_plugin_new(&error);
	fail_unless(debug->plugin2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_plugin_set_name(debug->plugin2, "mock-sync-foo");
	osync_plugin_set_longname(debug->plugin2, "Mock Sync Plugin");
	osync_plugin_set_description(debug->plugin2, "This is a pseudo plugin");
	osync_plugin_set_start_type(debug->plugin2, OSYNC_START_TYPE_EXTERNAL);
	
	osync_plugin_set_initialize(debug->plugin2, initialize_error);
	osync_plugin_set_finalize(debug->plugin2, finalize);
	
	
	
	
	debug->client1 = osync_client_new(&error);
	fail_unless(debug->client1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	char *pipe_path = g_strdup_printf("%s/configs/group/1/pluginpipe", testbed);
	osync_client_run_external(debug->client1, pipe_path, debug->plugin, &error);
	g_free(pipe_path);
	
	debug->client2 = osync_client_new(&error);
	fail_unless(debug->client2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	pipe_path = g_strdup_printf("%s/configs/group/2/pluginpipe", testbed);
	osync_client_run_external(debug->client2, pipe_path, debug->plugin2, &error);
	g_free(pipe_path);
	
	return debug;
}

static OSyncDebugGroup *_create_group2(char *testbed)
{
	OSyncDebugGroup *debug = g_malloc0(sizeof(OSyncDebugGroup));
	
	OSyncError *error = NULL;
	debug->group = osync_group_new(&error);
	fail_unless(debug->group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	char *path = g_strdup_printf("%s/configs/group", testbed);
	osync_group_set_configdir(debug->group, path);
	g_free(path);
	
	debug->member1 = osync_member_new(&error);
	fail_unless(debug->member1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(debug->group, debug->member1);
	osync_member_set_pluginname(debug->member1, "mock-sync-foo");
	path = g_strdup_printf("%s/configs/group/1", testbed);
	osync_member_set_configdir(debug->member1, path);
	g_free(path);

	_member_add_objtype(debug->member1, "mockobjtype1");
	osync_member_add_objformat(debug->member1, "mockobjtype1", "mockformat1");
	osync_member_set_config(debug->member1, "<config><directory><path>data1</path><objtype>mockobjtype1</objtype></directory></config>");
	
	debug->member2 = osync_member_new(&error);
	fail_unless(debug->member2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(debug->group, debug->member2);
	osync_member_set_pluginname(debug->member2, "mock-sync-foo");
	path = g_strdup_printf("%s/configs/group/2", testbed);
	osync_member_set_configdir(debug->member2, path);
	g_free(path);

	_member_add_objtype(debug->member2, "mockobjtype1");
	osync_member_add_objformat(debug->member2, "mockobjtype1", "mockformat1");
	osync_member_set_config(debug->member2, "<config><directory><path>data2</path><objtype>mockobjtype1</objtype></directory></config>");
	
	
	
	debug->plugin = osync_plugin_new(&error);
	fail_unless(debug->plugin != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_plugin_set_name(debug->plugin, "mock-sync-foo");
	osync_plugin_set_longname(debug->plugin, "Mock Sync Plugin");
	osync_plugin_set_description(debug->plugin, "This is a pseudo plugin");
	osync_plugin_set_start_type(debug->plugin, OSYNC_START_TYPE_EXTERNAL);
	osync_plugin_set_config_type(debug->plugin, OSYNC_PLUGIN_NO_CONFIGURATION);
	
	osync_plugin_set_initialize(debug->plugin, initialize_error);
	osync_plugin_set_finalize(debug->plugin, finalize);
	
	
	debug->plugin2 = osync_plugin_new(&error);
	fail_unless(debug->plugin2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_plugin_set_name(debug->plugin2, "mock-sync-foo");
	osync_plugin_set_longname(debug->plugin2, "Mock Sync Plugin");
	osync_plugin_set_description(debug->plugin2, "This is a pseudo plugin");
	osync_plugin_set_start_type(debug->plugin2, OSYNC_START_TYPE_EXTERNAL);
	
	osync_plugin_set_initialize(debug->plugin2, initialize_error);
	osync_plugin_set_finalize(debug->plugin2, finalize);
	
	
	
	
	debug->client1 = osync_client_new(&error);
	fail_unless(debug->client1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	char *pipe_path = g_strdup_printf("%s/configs/group/1/pluginpipe", testbed);
	osync_client_run_external(debug->client1, pipe_path, debug->plugin, &error);
	g_free(pipe_path);
	
	debug->client2 = osync_client_new(&error);
	fail_unless(debug->client2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	pipe_path = g_strdup_printf("%s/configs/group/2/pluginpipe", testbed);
	osync_client_run_external(debug->client2, pipe_path, debug->plugin2, &error);
	g_free(pipe_path);
	
	return debug;
}

static OSyncDebugGroup *_create_group3(char *testbed)
{
	OSyncDebugGroup *debug = g_malloc0(sizeof(OSyncDebugGroup));
	
	OSyncError *error = NULL;
	debug->group = osync_group_new(&error);
	fail_unless(debug->group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	char *path = g_strdup_printf("%s/configs/group", testbed);
	osync_group_set_configdir(debug->group, path);
	g_free(path);
	
	debug->member1 = osync_member_new(&error);
	fail_unless(debug->member1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(debug->group, debug->member1);
	osync_member_set_pluginname(debug->member1, "mock-sync-foo");
	path = g_strdup_printf("%s/configs/group/1", testbed);
	osync_member_set_configdir(debug->member1, path);
	g_free(path);

	_member_add_objtype(debug->member1, "mockobjtype1");
	osync_member_add_objformat(debug->member1, "mockobjtype1", "mockformat1");
	
	debug->member2 = osync_member_new(&error);
	fail_unless(debug->member2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(debug->group, debug->member2);
	osync_member_set_pluginname(debug->member2, "mock-sync-foo");
	path = g_strdup_printf("%s/configs/group/2", testbed);
	osync_member_set_configdir(debug->member2, path);
	g_free(path);

	_member_add_objtype(debug->member2, "mockobjtype1");
	osync_member_add_objformat(debug->member2, "mockobjtype1", "mockformat1");
	
	
	
	debug->plugin = osync_plugin_new(&error);
	fail_unless(debug->plugin != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_plugin_set_name(debug->plugin, "mock-sync-foo");
	osync_plugin_set_longname(debug->plugin, "Mock Sync Plugin");
	osync_plugin_set_description(debug->plugin, "This is a pseudo plugin");
	osync_plugin_set_start_type(debug->plugin, OSYNC_START_TYPE_EXTERNAL);
	osync_plugin_set_config_type(debug->plugin, OSYNC_PLUGIN_NO_CONFIGURATION);
	
	osync_plugin_set_initialize(debug->plugin, initialize_error);
	osync_plugin_set_finalize(debug->plugin, finalize);
	
	
	debug->plugin2 = osync_plugin_new(&error);
	fail_unless(debug->plugin2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_plugin_set_name(debug->plugin2, "mock-sync-foo");
	osync_plugin_set_longname(debug->plugin2, "Mock Sync Plugin");
	osync_plugin_set_description(debug->plugin2, "This is a pseudo plugin");
	osync_plugin_set_start_type(debug->plugin2, OSYNC_START_TYPE_EXTERNAL);
	
	osync_plugin_set_initialize(debug->plugin2, initialize_error);
	osync_plugin_set_finalize(debug->plugin2, finalize);
	
	
	
	
	debug->client1 = osync_client_new(&error);
	fail_unless(debug->client1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	char *pipe_path = g_strdup_printf("%s/configs/group/1/pluginpipe", testbed);
	osync_client_run_external(debug->client1, pipe_path, debug->plugin, &error);
	g_free(pipe_path);
	
	debug->client2 = osync_client_new(&error);
	fail_unless(debug->client2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	pipe_path = g_strdup_printf("%s/configs/group/2/pluginpipe", testbed);
	osync_client_run_external(debug->client2, pipe_path, debug->plugin2, &error);
	g_free(pipe_path);
	
	return debug;
}

static OSyncDebugGroup *_create_group4(char *testbed)
{
	OSyncDebugGroup *debug = g_malloc0(sizeof(OSyncDebugGroup));
	
	OSyncError *error = NULL;
	debug->group = osync_group_new(&error);
	fail_unless(debug->group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	char *path = g_strdup_printf("%s/configs/group", testbed);
	osync_group_set_configdir(debug->group, path);
	g_free(path);
	
	debug->member1 = osync_member_new(&error);
	fail_unless(debug->member1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(debug->group, debug->member1);
	osync_member_set_pluginname(debug->member1, "mock-sync-foo");
	path = g_strdup_printf("%s/configs/group/1", testbed);
	osync_member_set_configdir(debug->member1, path);
	g_free(path);

	_member_add_objtype(debug->member1, "mockobjtype1");
	osync_member_add_objformat(debug->member1, "mockobjtype1", "mockformat1");
	osync_member_set_config(debug->member1, "<config><directory><path>data1</path><objtype>mockobjtype1</objtype></directory></config>");
	
	debug->member2 = osync_member_new(&error);
	fail_unless(debug->member2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(debug->group, debug->member2);
	osync_member_set_pluginname(debug->member2, "mock-sync-foo");
	path = g_strdup_printf("%s/configs/group/2", testbed);
	osync_member_set_configdir(debug->member2, path);
	g_free(path);

	osync_member_set_config(debug->member2, "<config><directory><path>data2</path><objtype>mockobjtype1</objtype></directory></config>");
	
	
	
	debug->plugin = osync_plugin_new(&error);
	fail_unless(debug->plugin != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_plugin_set_name(debug->plugin, "mock-sync-foo");
	osync_plugin_set_longname(debug->plugin, "Mock Sync Plugin");
	osync_plugin_set_description(debug->plugin, "This is a pseudo plugin");
	osync_plugin_set_start_type(debug->plugin, OSYNC_START_TYPE_EXTERNAL);
	osync_plugin_set_config_type(debug->plugin, OSYNC_PLUGIN_NO_CONFIGURATION);
	
	osync_plugin_set_initialize(debug->plugin, initialize);
	osync_plugin_set_finalize(debug->plugin, finalize);
	
	
	debug->client1 = osync_client_new(&error);
	fail_unless(debug->client1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	char *pipe_path = g_strdup_printf("%s/configs/group/1/pluginpipe", testbed);
	osync_client_run_external(debug->client1, pipe_path, debug->plugin, &error);
	g_free(pipe_path);
	
	debug->client2 = osync_client_new(&error);
	fail_unless(debug->client2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	pipe_path = g_strdup_printf("%s/configs/group/2/pluginpipe", testbed);
	osync_client_run_external(debug->client2, pipe_path, debug->plugin, &error);
	g_free(pipe_path);
	
	return debug;
}

static OSyncDebugGroup *_create_group5(char *testbed)
{
	OSyncDebugGroup *debug = g_malloc0(sizeof(OSyncDebugGroup));
	
	OSyncError *error = NULL;
	debug->group = osync_group_new(&error);
	fail_unless(debug->group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	char *path = g_strdup_printf("%s/configs/group", testbed);
	osync_group_set_configdir(debug->group, path);
	g_free(path);
	
	debug->member1 = osync_member_new(&error);
	fail_unless(debug->member1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(debug->group, debug->member1);
	osync_member_set_pluginname(debug->member1, "mock-sync-foo");
	path = g_strdup_printf("%s/configs/group/1", testbed);
	osync_member_set_configdir(debug->member1, path);
	g_free(path);

	_member_add_objtype(debug->member1, "mockobjtype1");
	osync_member_add_objformat(debug->member1, "mockobjtype1", "mockformat1");
	osync_member_set_config(debug->member1, "<config><directory><path>data1</path><objtype>mockobjtype1</objtype></directory></config>");
	
	debug->member2 = osync_member_new(&error);
	fail_unless(debug->member2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_add_member(debug->group, debug->member2);
	osync_member_set_pluginname(debug->member2, "mock-sync-foo");
	path = g_strdup_printf("%s/configs/group/2", testbed);
	osync_member_set_configdir(debug->member2, path);
	g_free(path);
	_member_add_objtype(debug->member2, "mockobjtype1");
	osync_member_add_objformat(debug->member2, "mockobjtype1", "mockformat1");
	osync_member_set_config(debug->member2, "<config><directory><path>data2</path><objtype>mockobjtype1</objtype></directory></config>");
	
	
	debug->plugin = osync_plugin_new(&error);
	fail_unless(debug->plugin != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_plugin_set_name(debug->plugin, "mock-sync-foo");
	osync_plugin_set_longname(debug->plugin, "Mock Sync Plugin");
	osync_plugin_set_description(debug->plugin, "This is a pseudo plugin");
	osync_plugin_set_start_type(debug->plugin, OSYNC_START_TYPE_EXTERNAL);
	osync_plugin_set_config_type(debug->plugin, OSYNC_PLUGIN_NO_CONFIGURATION);
	
	osync_plugin_set_initialize(debug->plugin, initialize_connect_error);
	osync_plugin_set_finalize(debug->plugin, finalize);
	
	
	debug->plugin2 = osync_plugin_new(&error);
	fail_unless(debug->plugin2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_plugin_set_name(debug->plugin2, "mock-sync-foo");
	osync_plugin_set_longname(debug->plugin2, "Mock Sync Plugin");
	osync_plugin_set_description(debug->plugin2, "This is a pseudo plugin");
	osync_plugin_set_start_type(debug->plugin2, OSYNC_START_TYPE_EXTERNAL);
	
	osync_plugin_set_initialize(debug->plugin2, initialize_connect_error);
	osync_plugin_set_finalize(debug->plugin2, finalize);
	
	
	debug->client1 = osync_client_new(&error);
	fail_unless(debug->client1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	char *pipe_path = g_strdup_printf("%s/configs/group/1/pluginpipe", testbed);
	osync_client_run_external(debug->client1, pipe_path, debug->plugin, &error);
	g_free(pipe_path);
	
	debug->client2 = osync_client_new(&error);
	fail_unless(debug->client2 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	pipe_path = g_strdup_printf("%s/configs/group/2/pluginpipe", testbed);
	osync_client_run_external(debug->client2, pipe_path, debug->plugin2, &error);
	g_free(pipe_path);
	
	return debug;
}

static void _free_group(OSyncDebugGroup *debug)
{
	osync_client_unref(debug->client1);
	osync_client_unref(debug->client2);
	
	if (debug->plugin)
		osync_plugin_unref(debug->plugin);

	if (debug->plugin2)
		osync_plugin_unref(debug->plugin2);
	
	osync_member_unref(debug->member1);
	osync_member_unref(debug->member2);
	osync_group_unref(debug->group);
	
	g_free(debug);
}

static void _engine_instrument_pluginenv(OSyncEngine *engine, OSyncDebugGroup *debug)
{
	fail_unless(engine->pluginenv == NULL, NULL);
	engine->pluginenv = osync_plugin_env_new(NULL);

	if (debug->plugin)
		osync_plugin_env_register_plugin(engine->pluginenv, debug->plugin);

	if (debug->plugin2)
		osync_plugin_env_register_plugin(engine->pluginenv, debug->plugin2);
}

START_TEST (single_init_error)
{
	char *testbed = setup_testbed("sync_setup");
	
	OSyncError *error = NULL;
	OSyncDebugGroup *debug = _create_group(testbed);
	
	OSyncEngine *engine = osync_engine_new(debug->group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	_engine_instrument_pluginenv(engine, debug);
	
	fail_unless(!osync_engine_initialize(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	osync_error_unref(&error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	osync_error_unref(&error);
	
	_free_group(debug);
	
	osync_engine_unref(engine);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (double_init_error)
{
	char *testbed = setup_testbed("sync_setup");
	
	OSyncError *error = NULL;
	OSyncDebugGroup *debug = _create_group2(testbed);
	
	OSyncEngine *engine = osync_engine_new(debug->group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	_engine_instrument_pluginenv(engine, debug);

	fail_unless(!osync_engine_initialize(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	osync_error_unref(&error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	osync_error_unref(&error);
	
	_free_group(debug);
	
	osync_engine_unref(engine);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (no_config_error)
{
	char *testbed = setup_testbed("sync_setup");
	
	OSyncError *error = NULL;
	OSyncDebugGroup *debug = _create_group3(testbed);
	
	OSyncEngine *engine = osync_engine_new(debug->group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	_engine_instrument_pluginenv(engine, debug);
	
	fail_unless(!osync_engine_initialize(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	osync_error_unref(&error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	osync_error_unref(&error);
	
	_free_group(debug);
	
	osync_engine_unref(engine);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (no_objtype_error)
{
	char *testbed = setup_testbed("sync_setup");
	
	OSyncError *error = NULL;
	OSyncDebugGroup *debug = _create_group4(testbed);
	
	OSyncEngine *engine = osync_engine_new(debug->group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	_engine_instrument_pluginenv(engine, debug);

	fail_unless(!osync_engine_initialize(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	osync_error_unref(&error);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	osync_error_unref(&error);
	
	_free_group(debug);
	
	osync_engine_unref(engine);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_connect_error)
{
	char *testbed = setup_testbed("sync_setup");
	
	OSyncError *error = NULL;
	OSyncDebugGroup *debug = _create_group5(testbed);
	
	OSyncEngine *engine = osync_engine_new(debug->group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	_engine_instrument_pluginenv(engine, debug);

	osync_engine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	osync_error_unref(&error);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(num_connect == 2, NULL);
	fail_unless(num_disconnect == 0, NULL);
	fail_unless(num_get_changes == 0, NULL);
	
	/* Client checks */
	fail_unless(num_client_connected == 0, NULL);
	fail_unless(num_client_read == 0, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_sync_done == 0, NULL);
	fail_unless(num_client_discovered == 0, NULL);

	/* Main sink checks */

	fail_unless(num_client_main_disconnected == 2, NULL);
	fail_unless(num_client_main_connected == 2, NULL);

	fail_unless(num_client_main_read == 0, NULL);
	fail_unless(num_client_main_written == 0, NULL);
	fail_unless(num_client_main_sync_done == 0, NULL);
	
	/* Engine checks */
	fail_unless(num_engine_connected == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_read == 0, NULL);
	fail_unless(num_engine_written == 0, NULL);
	fail_unless(num_engine_sync_done == 0, NULL);
	fail_unless(num_engine_disconnected == 0, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);

	/* Change checks */
	fail_unless(num_change_read == 0, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_change_error == 0, NULL);

	/* Mapping checks */
	fail_unless(num_mapping_solved == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	
	_free_group(debug);
	
	osync_engine_unref(engine);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_two_connect_error)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	
	setenv("CONNECT_ERROR", "1", TRUE);

	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error); 
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);

	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 1, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);

	osync_error_unref(&error);
	
	osync_engine_unref(engine);

	osync_group_unref(group);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (two_of_three_connect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("CONNECT_ERROR", "5", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);

	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 1, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (two_of_three_connect_error2)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("CONNECT_ERROR", "6", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);

	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 1, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (three_of_three_connect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("CONNECT_ERROR", "7", TRUE);
	
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	fail_unless(num_client_errors == 3, NULL);
	fail_unless(num_client_connected == 0, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_connect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");

	setenv("CONNECT_ERROR", "2", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_client_disconnected == 2, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (no_connect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");

	setenv("CONNECT_ERROR", "0", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_connect_timeout)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	
	setenv("CONNECT_TIMEOUT", "2", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 1, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_connect_timeout)
{
	char *testbed = setup_testbed("sync");
	system("cp testdata data1/testdata");
	
	setenv("CONNECT_TIMEOUT", "3", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 0, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("CONNECT_TIMEOUT", "2", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (timeout_and_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("CONNECT_TIMEOUT", "2", TRUE);
	setenv("CONNECT_ERROR", "4", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 1, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_get_changes_error)
{
	char *testbed = setup_testbed("sync_easy_conflict");
	
	setenv("GET_CHANGES_ERROR", "2", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(num_engine_errors == 1, NULL);

	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_get_changes_error)
{
	char *testbed = setup_testbed("sync_easy_conflict");
	
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	setenv("GET_CHANGES_ERROR", "3", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 2, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_change_read == 0, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(num_engine_errors == 1, NULL);

	fail_unless(num_engine_successful == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (two_of_three_get_changes_error)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	setenv("GET_CHANGES_ERROR", "5", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(num_engine_errors == 1, NULL);

	fail_unless(num_engine_successful == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_get_changes_error)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	setenv("GET_CHANGES_ERROR", "1", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);

	fail_unless(num_engine_errors == 1, NULL);

	fail_unless(num_engine_successful == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (one_of_three_get_changes_timeout)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	setenv("GET_CHANGES_TIMEOUT", "1", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_read == 2, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_change_read == 2, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (get_changes_timeout_and_error)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	setenv("GET_CHANGES_TIMEOUT", "3", TRUE);
	setenv("GET_CHANGES_ERROR", "4", TRUE);

	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 3, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_read == 0, NULL);
	fail_unless(num_client_written == 0, NULL);
	fail_unless(num_change_read == 0, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

/* FIXME */
#if 0 
START_TEST (get_changes_timeout_sleep)
{
	char *testbed = setup_testbed("multisync_conflict_data_choose2");
	
	setenv("GET_CHANGES_TIMEOUT2", "7", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);

	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 3, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_read == 0, NULL);
	fail_unless(num_client_written == 0, NULL);

	// FIXME: If get_changes delays and get timed out .. set change_callback to NULL. To make sure changes got completely ignored by the engine
	fail_unless(num_change_read == 0, NULL);

	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST
#endif /* FIXME */

START_TEST (single_commit_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("COMMIT_ERROR", "4", TRUE);

	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 1, NULL);

	// TODO: Review, whats the diffent of mapping status emits and change status emits?! (dgollub)
	fail_unless(num_change_error == 1, NULL);
	// TODO: Review, whats the diffent of mapping status emits and change status emits?! (dgollub)
	fail_unless(num_mapping_errors == 1, NULL);

	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_commit_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("COMMIT_ERROR", "6", TRUE);

	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_change_error == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_commit_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("COMMIT_TIMEOUT", "4", TRUE);

	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 1, NULL);
	fail_unless(num_change_error == 1, NULL);
	fail_unless(num_mapping_errors == 1, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_commit_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("COMMIT_TIMEOUT", "6", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_change_error == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (commit_timeout_and_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("COMMIT_TIMEOUT", "4", TRUE);
	setenv("COMMIT_ERROR", "2", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_change_error == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (commit_timeout_and_error2)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("COMMIT_TIMEOUT", "2", TRUE);
	setenv("COMMIT_ERROR", "4", TRUE);
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(error != NULL, NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_change_error == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

/* FIXME: timeout */
START_TEST (commit_error_modify)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	setenv("COMMIT_TIMEOUT", "2", TRUE);
	setenv("COMMIT_ERROR", "4", TRUE);
	
	sleep(2);
	
	system("cp newdata2 data1/testdata");
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_change_error == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data2 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (commit_error_delete)
{
	char *testbed = setup_testbed("multisync_easy_new");

	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	setenv("COMMIT_TIMEOUT", "2", TRUE);
	setenv("COMMIT_ERROR", "4", TRUE);
	
	sleep(2);
	
	system("rm -f data1/testdata");
	
	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 0, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 0, NULL);
	fail_unless(num_change_error == 2, NULL);
	fail_unless(num_mapping_errors == 2, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data2 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (committed_all_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("COMMITTED_ALL_ERROR", "3", TRUE);

	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 1, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (committed_all_batch_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("BATCH_COMMIT", "7", TRUE);
	setenv("COMMITTED_ALL_ERROR", "3", TRUE);
	
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 1, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_sync_done_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("SYNC_DONE_ERROR", "4", TRUE);
	
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_sync_done_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("SYNC_DONE_ERROR", "6", TRUE);
	
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (triple_sync_done_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("SYNC_DONE_ERROR", "7", TRUE);
	
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 3, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_sync_done_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("SYNC_DONE_TIMEOUT", "4", TRUE);
	
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_sync_done_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("SYNC_DONE_TIMEOUT", "6", TRUE);
	
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (sync_done_timeout_and_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("SYNC_DONE_TIMEOUT", "5", TRUE);
	setenv("SYNC_DONE_ERROR", "2", TRUE);
	
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 3, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 3, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 1, NULL);
	fail_unless(num_engine_successful == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_disconnect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_ERROR", "4", TRUE);
	
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_disconnect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_ERROR", "6", TRUE);
	
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (triple_disconnect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_ERROR", "7", TRUE);

	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 3, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (single_disconnect_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_TIMEOUT", "4", TRUE);
	
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 1, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 2, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_disconnect_timeout)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_TIMEOUT", "6", TRUE);
	
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 2, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 1, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (disconnect_timeout_and_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_TIMEOUT", "5", TRUE);
	setenv("DISCONNECT_ERROR", "2", TRUE);

	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(num_client_errors == 3, NULL);
	fail_unless(num_client_connected == 3, NULL);
	fail_unless(num_client_disconnected == 0, NULL);
	fail_unless(num_client_written == 3, NULL);
	fail_unless(num_change_read == 1, NULL);
	fail_unless(num_change_written == 2, NULL);
	fail_unless(num_change_error == 0, NULL);
	fail_unless(num_mapping_errors == 0, NULL);
	fail_unless(num_mapping_conflicts == 0, NULL);
	fail_unless(num_engine_errors == 0, NULL);
	fail_unless(num_engine_successful == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (get_changes_disconnect_error)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	setenv("DISCONNECT_TIMEOUT", "1", TRUE);
	setenv("DISCONNECT_ERROR", "2", TRUE);
	setenv("GET_CHANGES_TIMEOUT", "6", TRUE);
	setenv("NO_COMMITTED_ALL_CHECK", "1", TRUE);
	
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	discover_all_once(engine, &error);

	osync_engine_set_memberstatus_callback(engine, member_status, GINT_TO_POINTER(1));
	osync_engine_set_enginestatus_callback(engine, engine_status, GINT_TO_POINTER(1));
	osync_engine_set_changestatus_callback(engine, entry_status, GINT_TO_POINTER(1));
	osync_engine_set_mappingstatus_callback(engine, mapping_status, GINT_TO_POINTER(1));
	osync_engine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(!synchronize_once(engine, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	
	osync_error_unref(&error);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *error_suite(void)
{
	Suite *s = suite_create("Engine Errors");
	//Suite *s2 = suite_create("Engine Errors");
	
	create_case(s, "single_init_error", single_init_error);
	create_case(s, "double_init_error", double_init_error);
	create_case(s, "no_config_error", no_config_error);
	create_case(s, "no_objtype_error", no_objtype_error);
	create_case(s, "dual_connect_error", dual_connect_error);
	create_case(s, "one_of_two_connect_error", one_of_two_connect_error);
	create_case(s, "two_of_three_connect_error", two_of_three_connect_error);
	create_case(s, "two_of_three_connect_error2", two_of_three_connect_error2);
	create_case(s, "three_of_three_connect_error", three_of_three_connect_error);

	create_case(s, "one_of_three_connect_error", one_of_three_connect_error);
	create_case(s, "no_connect_error", no_connect_error);

	create_case(s, "single_connect_timeout", single_connect_timeout);
	create_case(s, "dual_connect_timeout", dual_connect_timeout);
	create_case(s, "one_of_three_timeout", one_of_three_timeout);
	create_case(s, "timeout_and_error", timeout_and_error);

	create_case(s, "single_get_changes_error", single_get_changes_error);
	create_case(s, "dual_get_changes_error", dual_get_changes_error);
	create_case(s, "two_of_three_get_changes_error", two_of_three_get_changes_error);
	create_case(s, "one_of_three_get_changes_error", one_of_three_get_changes_error);

	create_case(s, "one_of_three_get_changes_timeout", one_of_three_get_changes_timeout);
	create_case(s, "get_changes_timeout_and_error", get_changes_timeout_and_error);

	/* FIXME: If get_changes delays and got timed out .. set change_callback to NULL.
	   Make sure changes from the plugin got completely ignored by the engine when the timout handler got called.
	   Even better would be to abort the get_changes call from the plugin process...

	create_case(s2, "get_changes_timeout_sleep", get_changes_timeout_sleep);
	*/

	create_case(s, "single_commit_error", single_commit_error);
	create_case(s, "dual_commit_error", dual_commit_error);

	create_case(s, "single_commit_timeout", single_commit_timeout);
	create_case(s, "dual_commit_timeout", dual_commit_timeout);
	create_case(s, "commit_timeout_and_error", commit_timeout_and_error);
	create_case(s, "commit_timeout_and_error2", commit_timeout_and_error2);

	create_case(s, "commit_error_modify", commit_error_modify);
	create_case(s, "commit_error_delete", commit_error_delete);

	create_case(s, "committed_all_error", committed_all_error);
	create_case(s, "committed_all_batch_error", committed_all_batch_error);

	create_case(s, "single_sync_done_error", single_sync_done_error);
	create_case(s, "dual_sync_done_error", dual_sync_done_error);
	create_case(s, "triple_sync_done_error", triple_sync_done_error);

	create_case(s, "single_sync_done_timeout", single_sync_done_timeout);
	create_case(s, "dual_sync_done_timeout", dual_sync_done_timeout);
	create_case(s, "sync_done_timeout_and_error", sync_done_timeout_and_error);

	create_case(s, "single_disconnect_error", single_disconnect_error);
	create_case(s, "dual_disconnect_error", dual_disconnect_error);
	create_case(s, "triple_disconnect_error", triple_disconnect_error);

	create_case(s, "single_disconnect_timeout", single_disconnect_timeout);
	create_case(s, "dual_disconnect_timeout", dual_disconnect_timeout);
	create_case(s, "disconnect_timeout_and_error", disconnect_timeout_and_error);

	create_case(s, "get_changes_disconnect_error", get_changes_disconnect_error);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = error_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
