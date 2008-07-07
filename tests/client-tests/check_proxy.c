#include "support.h"
#include <sys/wait.h>

#include <opensync/opensync-client.h>
#include <opensync/opensync-ipc.h>
#include <opensync/opensync-plugin.h>

START_TEST (proxy_new)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncClientProxy *proxy = osync_client_proxy_new(NULL, NULL, &error);
	fail_unless(proxy != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_client_proxy_ref(proxy);
	osync_client_proxy_unref(proxy);
	osync_client_proxy_unref(proxy);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (proxy_spawn)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncClientProxy *proxy = osync_client_proxy_new(NULL, NULL, &error);
	fail_unless(proxy != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_client_proxy_spawn(proxy, OSYNC_START_TYPE_THREAD, NULL, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_client_proxy_shutdown(proxy, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_client_proxy_unref(proxy);
	
	destroy_testbed(testbed);
}
END_TEST

int init_replies = 0;
int fin_replies = 0;
int discover_replies = 0;
int connect_replies = 0;
int disconnect_replies = 0;

static void initialize_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	fail_unless(userdata == GINT_TO_POINTER(1), NULL);
	fail_unless(error == NULL, NULL);
	init_replies++;
}

static void finalize_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	fail_unless(userdata == GINT_TO_POINTER(1), NULL);
	fail_unless(error == NULL, NULL);
	fin_replies++;
}

static void discover_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	fail_unless(userdata == GINT_TO_POINTER(1), NULL);
	fail_unless(error == NULL, NULL);
	discover_replies++;
}

static void connect_callback(OSyncClientProxy *proxy, void *userdata, osync_bool slowsync, OSyncError *error)
{
	fail_unless(userdata == GINT_TO_POINTER(1), NULL);
	fail_unless(error == NULL, NULL);
	connect_replies++;
}

static void disconnect_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	fail_unless(userdata == GINT_TO_POINTER(1), NULL);
	fail_unless(error == NULL, NULL);
	disconnect_replies++;
}

START_TEST (proxy_init)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncThread *thread = osync_thread_new(NULL, &error);
	fail_unless(thread != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_thread_start(thread);
	
	OSyncClientProxy *proxy = osync_client_proxy_new(NULL, NULL, &error);
	fail_unless(proxy != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_client_proxy_spawn(proxy, OSYNC_START_TYPE_THREAD, NULL, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncPluginConfig *config = simple_plugin_config(NULL, "data1", "mockobjtype1", "mockformat1", NULL);
	fail_unless(osync_client_proxy_initialize(proxy, initialize_callback, GINT_TO_POINTER(1), testbed, testbed, "mock-sync", "test", testbed, config, &error), NULL);
	osync_plugin_config_unref(config);

	fail_unless(error == NULL, NULL);
	
	while (init_replies != 1) { g_usleep(100); }
	
	fail_unless(osync_client_proxy_finalize(proxy, finalize_callback, GINT_TO_POINTER(1), &error), NULL);
	fail_unless(error == NULL, NULL);
	
	while (fin_replies != 1) { g_usleep(100); }
	
	fail_unless(osync_client_proxy_shutdown(proxy, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_client_proxy_unref(proxy);
	
	osync_thread_stop(thread);
	osync_thread_free(thread);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (proxy_discover)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncThread *thread = osync_thread_new(NULL, &error);
	fail_unless(thread != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_thread_start(thread);
	
	OSyncClientProxy *proxy = osync_client_proxy_new(NULL, NULL, &error);
	fail_unless(proxy != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_client_proxy_spawn(proxy, OSYNC_START_TYPE_THREAD, NULL, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncPluginConfig *config = simple_plugin_config(NULL, "data1", "mockobjtype1", "mockformat1", NULL);
	fail_unless(osync_client_proxy_initialize(proxy, initialize_callback, GINT_TO_POINTER(1), testbed, testbed, "mock-sync", "test", testbed, config, &error), NULL);
	osync_plugin_config_unref(config);

	fail_unless(error == NULL, NULL);
	
	while (init_replies != 1) { g_usleep(100); }
	
	fail_unless(osync_client_proxy_num_objtypes(proxy) == 0, NULL);
	
	fail_unless(osync_client_proxy_discover(proxy, discover_callback, GINT_TO_POINTER(1), &error), NULL);
	fail_unless(error == NULL, NULL);
	
	while (discover_replies != 1) { g_usleep(100); }
	
	fail_unless(osync_client_proxy_num_objtypes(proxy) == 1, NULL);
	OSyncObjTypeSink *sink = osync_client_proxy_nth_objtype(proxy, 0);
	fail_unless(sink != NULL, NULL);
	fail_unless(!strcmp(osync_objtype_sink_get_name(sink), "file"), NULL);
	
	fail_unless(osync_objtype_sink_num_objformat_sinks(sink) == 1, NULL);
	OSyncObjFormatSink *format_sink = osync_objtype_sink_nth_objformat_sink(sink, 0);
	const char *objformat = osync_objformat_sink_get_objformat(format_sink);
	fail_unless(!strcmp(objformat, "mockformat1"), NULL);
	
	fail_unless(osync_client_proxy_finalize(proxy, finalize_callback, GINT_TO_POINTER(1), &error), NULL);
	fail_unless(error == NULL, NULL);
	
	while (fin_replies != 1) { g_usleep(100); }
	
	fail_unless(osync_client_proxy_shutdown(proxy, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_client_proxy_unref(proxy);
	
	osync_thread_stop(thread);
	osync_thread_free(thread);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (proxy_connect)
{
	char *testbed = setup_testbed("sync");
	
	OSyncError *error = NULL;
	OSyncThread *thread = osync_thread_new(NULL, &error);
	fail_unless(thread != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_thread_start(thread);
	
	OSyncClientProxy *proxy = osync_client_proxy_new(NULL, NULL, &error);
	fail_unless(proxy != NULL, NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(osync_client_proxy_spawn(proxy, OSYNC_START_TYPE_THREAD, NULL, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncPluginConfig *config = simple_plugin_config(NULL, "data1", "mockobjtype1", "mockformat1", NULL);
	fail_unless(osync_client_proxy_initialize(proxy, initialize_callback, GINT_TO_POINTER(1), testbed, testbed, "mock-sync", "test", testbed, config, &error), NULL);
	osync_plugin_config_unref(config);
	fail_unless(error == NULL, NULL);
	
	while (init_replies != 1) { g_usleep(100); }
	
	fail_unless(osync_client_proxy_connect(proxy, connect_callback, GINT_TO_POINTER(1), "file", FALSE, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	while (connect_replies != 1) { g_usleep(100); }
	
	fail_unless(osync_client_proxy_disconnect(proxy, disconnect_callback, GINT_TO_POINTER(1), "file", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	while (disconnect_replies != 1) { g_usleep(100); }
	
	fail_unless(osync_client_proxy_finalize(proxy, finalize_callback, GINT_TO_POINTER(1), &error), NULL);
	fail_unless(error == NULL, NULL);
	
	while (fin_replies != 1) { g_usleep(100); }
	
	fail_unless(osync_client_proxy_shutdown(proxy, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_client_proxy_unref(proxy);
	
	osync_thread_stop(thread);
	osync_thread_free(thread);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *proxy_suite(void)
{
	Suite *s = suite_create("Proxy");
//	Suite *s2 = suite_create("Proxy");
	
	create_case(s, "proxy_new", proxy_new);
	create_case(s, "proxy_spawn", proxy_spawn);
	create_case(s, "proxy_init", proxy_init);
	create_case(s, "proxy_discover", proxy_discover);
	create_case(s, "proxy_connect", proxy_connect);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = proxy_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
