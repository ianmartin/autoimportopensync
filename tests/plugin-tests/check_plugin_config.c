#include "support.h"

#include <opensync/opensync-plugin.h>

START_TEST (plugin_config_new)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	fail_unless(config != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	/* add 2nd reference, and ... */
	fail_unless(osync_plugin_config_ref(config) != NULL, NULL);

	/* All sub components should be NULL in the new config object */
	fail_unless(osync_plugin_config_get_authentication(config) == NULL, NULL);
	fail_unless(osync_plugin_config_get_localization(config) == NULL, NULL);
	fail_unless(osync_plugin_config_get_connection(config) == NULL, NULL);

	/* ... unref twice. */
	osync_plugin_config_unref(config);
	osync_plugin_config_unref(config);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (plugin_config_new_nomemory)
{
	char *testbed = setup_testbed(NULL);

	setenv("OSYNC_NOMEMORY", "1", TRUE);
	
	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	fail_unless(error != NULL, NULL);
	fail_unless(config == NULL, NULL);
	osync_error_unref(&error);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (plugin_config_subcomponents)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	/* Adding Subcomponents: */
	/* Connection */
	OSyncPluginConnection *conn = osync_plugin_connection_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(conn != NULL, NULL);
	osync_plugin_config_set_connection(config, conn);
	fail_unless(osync_plugin_config_get_connection(config) != NULL, NULL);
	fail_unless(osync_plugin_connection_ref(conn) != NULL, NULL);
	osync_plugin_connection_unref(conn);
	osync_plugin_connection_unref(conn);

	/* Localization */
	OSyncPluginLocalization *local = osync_plugin_localization_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(local != NULL, NULL);
	osync_plugin_config_set_localization(config, local);
	fail_unless(osync_plugin_config_get_localization(config) != NULL, NULL);
	fail_unless(osync_plugin_localization_ref(local) != NULL, NULL);
	osync_plugin_localization_unref(local);
	osync_plugin_localization_unref(local);

	/* Authentication */
	OSyncPluginAuthentication *auth = osync_plugin_authentication_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(auth != NULL, NULL);
	osync_plugin_config_set_authentication(config, auth);
	fail_unless(osync_plugin_config_get_authentication(config) != NULL, NULL);
	fail_unless(osync_plugin_authentication_ref(auth) != NULL, NULL);
	osync_plugin_authentication_unref(auth);
	osync_plugin_authentication_unref(auth);

	osync_plugin_config_unref(config);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (plugin_config_subcomponents_nomemory)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	setenv("OSYNC_NOMEMORY", "1", TRUE);

	/* Adding Subcomponents (without memory -> booooooom!): */
	/* Connection */
	OSyncPluginConnection *conn = osync_plugin_connection_new(&error);
	fail_unless(error != NULL, NULL);
	fail_unless(conn == NULL, NULL);
	osync_error_unref(&error);

	/* Localization */
	OSyncPluginLocalization *local = osync_plugin_localization_new(&error);
	fail_unless(error != NULL, NULL);
	fail_unless(local == NULL, NULL);
	osync_error_unref(&error);

	/* Authentication */
	OSyncPluginAuthentication *auth = osync_plugin_authentication_new(&error);
	fail_unless(error != NULL, NULL);
	fail_unless(auth == NULL, NULL);
	osync_error_unref(&error);

	osync_plugin_config_unref(config);

	destroy_testbed(testbed);
}
END_TEST


Suite *client_suite(void)
{
	Suite *s = suite_create("PluginConfig");
//	Suite *s2 = suite_create("PluginConfig");
	
	create_case(s, "plugin_config_new", plugin_config_new);
	create_case(s, "plugin_config_new_nomemory", plugin_config_new_nomemory);
	create_case(s, "plugin_config_subcomponents", plugin_config_subcomponents);
	create_case(s, "plugin_config_subcomponents_nomemory", plugin_config_subcomponents_nomemory);

	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = client_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
