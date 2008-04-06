#include "support.h"

#include <opensync/opensync-plugin.h>
#include "opensync/plugin/opensync_plugin_config_internals.h"

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
	OSyncPluginConnection *conn = osync_plugin_connection_new(OSYNC_PLUGIN_CONNECTION_UNKNOWN, &error);
	fail_unless(error == NULL, NULL);
	fail_unless(conn != NULL, NULL);
	osync_plugin_config_set_connection(config, conn);
	fail_unless(osync_plugin_config_get_connection(config) == conn, NULL);
	fail_unless(osync_plugin_connection_ref(conn) != NULL, NULL);
	osync_plugin_connection_unref(conn);
	osync_plugin_connection_unref(conn);

	/* Localization */
	OSyncPluginLocalization *local = osync_plugin_localization_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(local != NULL, NULL);
	osync_plugin_config_set_localization(config, local);
	fail_unless(osync_plugin_config_get_localization(config) == local, NULL);
	fail_unless(osync_plugin_localization_ref(local) != NULL, NULL);
	osync_plugin_localization_unref(local);
	osync_plugin_localization_unref(local);

	/* Authentication */
	OSyncPluginAuthentication *auth = osync_plugin_authentication_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(auth != NULL, NULL);
	osync_plugin_config_set_authentication(config, auth);
	fail_unless(osync_plugin_config_get_authentication(config) == auth, NULL);
	fail_unless(osync_plugin_authentication_ref(auth) != NULL, NULL);
	osync_plugin_authentication_unref(auth);
	osync_plugin_authentication_unref(auth);


	/* Overwrite Subcomponents */
	/* Connection */
	OSyncPluginConnection *conn2 = osync_plugin_connection_new(OSYNC_PLUGIN_CONNECTION_UNKNOWN, &error);
	fail_unless(error == NULL, NULL);
	fail_unless(conn2 != NULL, NULL);
	osync_plugin_config_set_connection(config, conn2);
	fail_unless(osync_plugin_config_get_connection(config) == conn2, NULL);
	osync_plugin_connection_unref(conn2);

	/* Localization */
	OSyncPluginLocalization *local2 = osync_plugin_localization_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(local2 != NULL, NULL);
	osync_plugin_config_set_localization(config, local2);
	fail_unless(osync_plugin_config_get_localization(config) == local2, NULL);
	osync_plugin_localization_unref(local2);

	/* Authentication */
	OSyncPluginAuthentication *auth2 = osync_plugin_authentication_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(auth2 != NULL, NULL);
	osync_plugin_config_set_authentication(config, auth2);
	fail_unless(osync_plugin_config_get_authentication(config) == auth2, NULL);
	osync_plugin_authentication_unref(auth2);

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
	OSyncPluginConnection *conn = osync_plugin_connection_new(OSYNC_PLUGIN_CONNECTION_UNKNOWN, &error);
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

START_TEST (plugin_config_authentication)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	/* Authentication */
	OSyncPluginAuthentication *auth = osync_plugin_authentication_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(auth != NULL, NULL);

	osync_plugin_authentication_set_username(auth, "foo");
	fail_unless(!strcmp(osync_plugin_authentication_get_username(auth), "foo"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_authentication_set_username(auth, "bar");
	fail_unless(!strcmp(osync_plugin_authentication_get_username(auth), "bar"), NULL);

	osync_plugin_authentication_set_password(auth, "foo");
	fail_unless(!strcmp(osync_plugin_authentication_get_password(auth), "foo"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_authentication_set_password(auth, "bar");
	fail_unless(!strcmp(osync_plugin_authentication_get_password(auth), "bar"), NULL);

	osync_plugin_authentication_set_reference(auth, "foo");
	fail_unless(!strcmp(osync_plugin_authentication_get_reference(auth), "foo"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_authentication_set_reference(auth, "bar");
	fail_unless(!strcmp(osync_plugin_authentication_get_reference(auth), "bar"), NULL);

	fail_unless(osync_plugin_authentication_ref(auth) != NULL, NULL);
	osync_plugin_authentication_unref(auth);
	osync_plugin_authentication_unref(auth);

	osync_plugin_config_unref(config);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (plugin_config_connection)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	/* Connection */
	OSyncPluginConnection *conn = osync_plugin_connection_new(OSYNC_PLUGIN_CONNECTION_UNKNOWN, &error);
	fail_unless(error == NULL, NULL);
	fail_unless(conn != NULL, NULL);

	/* Bluetooth Address */
	osync_plugin_connection_bt_set_addr(conn, "FF:FF:FF:FF:FF:FF");
	fail_unless(!strcmp(osync_plugin_connection_bt_get_addr(conn), "FF:FF:FF:FF:FF:FF"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_connection_bt_set_addr(conn, "AA:FF:EE:AA:FF:EE");
	fail_unless(!strcmp(osync_plugin_connection_bt_get_addr(conn), "AA:FF:EE:AA:FF:EE"), NULL);

	/* Bluetooth Channel */
	osync_plugin_connection_bt_set_channel(conn, 11);
	fail_unless(osync_plugin_connection_bt_get_channel(conn) == 11, NULL);

	/* Overwrite */
	osync_plugin_connection_bt_set_channel(conn, 2);
	fail_unless(osync_plugin_connection_bt_get_channel(conn) == 2, NULL);

	/* Bluetooth SDP UUID */
	osync_plugin_connection_bt_set_sdpuuid(conn, "00000001-0000-1000-8000-0002EE000002");
	fail_unless(!strcmp(osync_plugin_connection_bt_get_sdpuuid(conn), "00000001-0000-1000-8000-0002EE000002"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_connection_bt_set_sdpuuid(conn, "AAFFEE");
	fail_unless(!strcmp(osync_plugin_connection_bt_get_sdpuuid(conn), "AAFFEE"), NULL);

	/* Bluetooth SDP UUID */
	osync_plugin_connection_bt_set_sdpuuid(conn, "00000001-0000-1000-8000-0002EE000002");
	fail_unless(!strcmp(osync_plugin_connection_bt_get_sdpuuid(conn), "00000001-0000-1000-8000-0002EE000002"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_connection_bt_set_sdpuuid(conn, "AAFFEE");
	fail_unless(!strcmp(osync_plugin_connection_bt_get_sdpuuid(conn), "AAFFEE"), NULL);

	/* USB Vendor ID */
	osync_plugin_connection_usb_set_vendorid(conn, 0xffff);
	fail_unless(osync_plugin_connection_usb_get_vendorid(conn) == 0xffff, NULL);

	osync_plugin_connection_usb_set_vendorid(conn, 0xaffe);
	fail_unless(osync_plugin_connection_usb_get_vendorid(conn) == 0xaffe, NULL);

	/* USB Product ID */
	osync_plugin_connection_usb_set_productid(conn, 0xffff);
	fail_unless(osync_plugin_connection_usb_get_productid(conn) == 0xffff, NULL);

	osync_plugin_connection_usb_set_productid(conn, 0xaffe);
	fail_unless(osync_plugin_connection_usb_get_productid(conn) == 0xaffe, NULL);

	/* USB Interface*/
	osync_plugin_connection_usb_set_interface(conn, 2);
	fail_unless(osync_plugin_connection_usb_get_interface(conn) == 2, NULL);

	/* Network Address */
	osync_plugin_connection_net_set_address(conn, "opensync.org");
	fail_unless(!strcmp(osync_plugin_connection_net_get_address(conn), "opensync.org"), NULL);

	osync_plugin_connection_net_set_address(conn, "libsyncml.opensync.org");
	fail_unless(!strcmp(osync_plugin_connection_net_get_address(conn), "libsyncml.opensync.org"), NULL);

	/* Network Port */
	osync_plugin_connection_net_set_port(conn, 8888);
	fail_unless(osync_plugin_connection_net_get_port(conn) == 8888, NULL);

	osync_plugin_connection_net_set_port(conn, 9999);
	fail_unless(osync_plugin_connection_net_get_port(conn) == 9999, NULL);

	/* Network Protocol */
	osync_plugin_connection_net_set_protocol(conn, "http://");
	fail_unless(!strcmp(osync_plugin_connection_net_get_protocol(conn), "http://"), NULL);

	osync_plugin_connection_net_set_protocol(conn, "ssh://");
	fail_unless(!strcmp(osync_plugin_connection_net_get_protocol(conn), "ssh://"), NULL);

	/* Network DNSSD */
	osync_plugin_connection_net_set_dnssd(conn, "_syncml-http._tcp");
	fail_unless(!strcmp(osync_plugin_connection_net_get_dnssd(conn), "_syncml-http._tcp"), NULL);

	osync_plugin_connection_net_set_dnssd(conn, "_syncml-obex._tcp");
	fail_unless(!strcmp(osync_plugin_connection_net_get_dnssd(conn), "_syncml-obex._tcp"), NULL);

	/* Serial Speed */
	osync_plugin_connection_serial_set_speed(conn, 1234);
	fail_unless(osync_plugin_connection_serial_get_speed(conn) == 1234, NULL);

	osync_plugin_connection_serial_set_speed(conn, 4321);
	fail_unless(osync_plugin_connection_serial_get_speed(conn) == 4321, NULL);

	/* Serial Devicenode */
	osync_plugin_connection_serial_set_devicenode(conn, "/dev/ttyS0");
	fail_unless(!strcmp(osync_plugin_connection_serial_get_devicenode(conn), "/dev/ttyS0"), NULL);

	osync_plugin_connection_serial_set_devicenode(conn, "/dev/ttyUSB0");
	fail_unless(!strcmp(osync_plugin_connection_serial_get_devicenode(conn), "/dev/ttyUSB0"), NULL);

	/* IrDA Service */
	osync_plugin_connection_irda_set_service(conn, "FancyIR Mobile");
	fail_unless(!strcmp(osync_plugin_connection_irda_get_service(conn), "FancyIR Mobile"), NULL);

	osync_plugin_connection_irda_set_service(conn, "IRMobile");
	fail_unless(!strcmp(osync_plugin_connection_irda_get_service(conn), "IRMobile"), NULL);

	fail_unless(osync_plugin_connection_ref(conn) != NULL, NULL);
	osync_plugin_connection_unref(conn);
	osync_plugin_connection_unref(conn);

	osync_plugin_config_unref(config);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (plugin_config_localization)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	/* Localization */
	OSyncPluginLocalization *local = osync_plugin_localization_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(local != NULL, NULL);

	/* Encoding */
	osync_plugin_localization_set_encoding(local, "cp1222");
	fail_unless(!strcmp(osync_plugin_localization_get_encoding(local), "cp1222"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_localization_set_encoding(local, "cp2221");
	fail_unless(!strcmp(osync_plugin_localization_get_encoding(local), "cp2221"), NULL);

	/* Timezone */
	osync_plugin_localization_set_timezone(local, "Europe/Berlin");
	fail_unless(!strcmp(osync_plugin_localization_get_timezone(local), "Europe/Berlin"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_localization_set_timezone(local, "Europe/Munich");
	fail_unless(!strcmp(osync_plugin_localization_get_timezone(local), "Europe/Munich"), NULL);

	/* Language */
	osync_plugin_localization_set_language(local, "de_DE");
	fail_unless(!strcmp(osync_plugin_localization_get_language(local), "de_DE"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_localization_set_language(local, "en_US");
	fail_unless(!strcmp(osync_plugin_localization_get_language(local), "en_US"), NULL);

	fail_unless(osync_plugin_localization_ref(local) != NULL, NULL);
	osync_plugin_localization_unref(local);
	osync_plugin_localization_unref(local);

	osync_plugin_config_unref(config);

	destroy_testbed(testbed);
}
END_TEST


START_TEST (plugin_config_save_and_load)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	OSyncPluginConfig *reloaded_config = osync_plugin_config_new(&error); 
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	/* Localization */
	OSyncPluginLocalization *local = osync_plugin_localization_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(local != NULL, NULL);

	/* Encoding */
	osync_plugin_localization_set_encoding(local, "cp1222");

	/* Timezone */
	osync_plugin_localization_set_timezone(local, "Europe/Berlin");

	/* Language */
	osync_plugin_localization_set_language(local, "de_DE");

	/* Connection */
	OSyncPluginConnection *conn = osync_plugin_connection_new(OSYNC_PLUGIN_CONNECTION_BLUETOOTH, &error);
	fail_unless(error == NULL, NULL);
	fail_unless(conn != NULL, NULL);

	/* Bluetooth Address */
	osync_plugin_connection_bt_set_addr(conn, "FF:FF:FF:FF:FF:FF");

	/* Bluetooth SDP UUID */
	osync_plugin_connection_bt_set_sdpuuid(conn, "00000001-0000-1000-8000-0002EE000002");

	/* Set subcomponents */
	osync_plugin_config_set_connection(config, conn);
	osync_plugin_connection_unref(conn);
	osync_plugin_config_set_localization(config, local);
	osync_plugin_localization_unref(local);

	char *config_file = g_strdup_printf("%s/dummy_config.xml", testbed);
	fail_unless(osync_plugin_config_file_save(config, config_file, &error), "%s", osync_error_print(&error));
	fail_unless(_osync_plugin_config_file_load(reloaded_config, config_file, testbed, &error), NULL);
	g_free(config_file);

	/* Compare stored config with original config */

	OSyncPluginConnection *reloaded_conn = osync_plugin_config_get_connection(reloaded_config);
	OSyncPluginLocalization *reloaded_local = osync_plugin_config_get_localization(reloaded_config);

	fail_unless(reloaded_conn != NULL, NULL);
	fail_unless(reloaded_local != NULL, NULL);

	fail_unless(!strcmp(osync_plugin_localization_get_language(reloaded_local), "de_DE"), NULL);
	fail_unless(!strcmp(osync_plugin_localization_get_encoding(reloaded_local), "cp1222"), NULL);
	fail_unless(!strcmp(osync_plugin_localization_get_timezone(reloaded_local), "Europe/Berlin"), NULL);

	fail_unless(!strcmp(osync_plugin_connection_bt_get_sdpuuid(reloaded_conn), "00000001-0000-1000-8000-0002EE000002"), NULL);
	fail_unless(!strcmp(osync_plugin_connection_bt_get_addr(reloaded_conn), "FF:FF:FF:FF:FF:FF"), NULL);

	osync_plugin_config_unref(config);
	osync_plugin_config_unref(reloaded_config);

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
	create_case(s, "plugin_config_authentication", plugin_config_authentication);
	create_case(s, "plugin_config_connection", plugin_config_connection);
	create_case(s, "plugin_config_localization", plugin_config_localization);
	create_case(s, "plugin_config_save_and_load", plugin_config_save_and_load);
	
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
