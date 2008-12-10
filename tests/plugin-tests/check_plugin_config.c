#include "support.h"

#include <opensync/opensync-plugin.h>
#include <opensync/opensync-format.h>

#include "opensync/plugin/opensync_plugin_config_private.h"

static const char *_format_sink_get_objformat(OSyncPluginResource *res) {

	OSyncObjFormatSink *format_sink;
	format_sink = (OSyncObjFormatSink *) osync_list_nth_data(osync_plugin_resource_get_objformat_sinks(res), 0);
	fail_unless(!!format_sink, NULL);
	return osync_objformat_sink_get_objformat(format_sink);
}

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

	g_setenv("OSYNC_NOMEMORY", "1", TRUE);
	
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
	OSyncPluginConnection *conn2 = osync_plugin_connection_new(&error);
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

	g_setenv("OSYNC_NOMEMORY", "1", TRUE);

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

	/* Resource(s) */
	OSyncPluginResource *res = osync_plugin_resource_new(&error);
	fail_unless(error != NULL, NULL);
	fail_unless(res == NULL, NULL);
	osync_error_unref(&error);

	osync_plugin_config_unref(config);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (plugin_config_advancedoption)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	/* Advanced Option */
	OSyncPluginAdvancedOption *option = osync_plugin_advancedoption_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(option != NULL, NULL);

	osync_plugin_advancedoption_set_max(option, 2323);
	fail_unless(osync_plugin_advancedoption_get_max(option) == 2323, NULL);

	osync_plugin_advancedoption_set_min(option, 0);
	fail_unless(osync_plugin_advancedoption_get_min(option) == 0, NULL);

	osync_plugin_advancedoption_set_maxoccurs(option, 3232);
	fail_unless(osync_plugin_advancedoption_get_maxoccurs(option) == 3232, NULL);

	osync_plugin_advancedoption_set_type(option, OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_BOOL);
	fail_unless(osync_plugin_advancedoption_get_type(option) == OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_BOOL, NULL);


	/* Check for uniq number of VALENUMs */
	osync_plugin_advancedoption_add_valenum(option, "bar");
	osync_plugin_advancedoption_add_valenum(option, "foo");
	osync_plugin_advancedoption_add_valenum(option, "foo");
	osync_plugin_advancedoption_add_valenum(option, "foo");

	/* Expected result: 2 - 1x foo, 1x bar */
	fail_unless(osync_list_length(osync_plugin_advancedoption_get_valenums(option)) == 2, NULL);

	/* Expected resut: 1 - 1x bar */
	osync_plugin_advancedoption_remove_valenum(option, "bar");
	fail_unless(osync_list_length(osync_plugin_advancedoption_get_valenums(option)) == 1, NULL);

	/*****/

	osync_plugin_advancedoption_set_displayname(option, "foo");
	fail_unless(!strcmp(osync_plugin_advancedoption_get_displayname(option), "foo"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_advancedoption_set_displayname(option, "bar");
	fail_unless(!strcmp(osync_plugin_advancedoption_get_displayname(option), "bar"), NULL);

	osync_plugin_advancedoption_set_name(option, "foo");
	fail_unless(!strcmp(osync_plugin_advancedoption_get_name(option), "foo"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_advancedoption_set_name(option, "bar");
	fail_unless(!strcmp(osync_plugin_advancedoption_get_name(option), "bar"), NULL);

	fail_unless(osync_plugin_advancedoption_ref(option) != NULL, NULL);
	osync_plugin_advancedoption_unref(option);
	osync_plugin_advancedoption_unref(option);

	osync_plugin_config_unref(config);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (plugin_config_advancedoption_param)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	/* Advanced Option */
	OSyncPluginAdvancedOptionParameter *param = osync_plugin_advancedoption_param_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(param != NULL, NULL);

	osync_plugin_advancedoption_param_set_type(param, OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_BOOL);
	fail_unless(osync_plugin_advancedoption_param_get_type(param) == OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_BOOL, NULL);


	/* Check for uniq number of VALENUMs */
	osync_plugin_advancedoption_param_add_valenum(param, "bar");
	osync_plugin_advancedoption_param_add_valenum(param, "foo");
	osync_plugin_advancedoption_param_add_valenum(param, "foo");
	osync_plugin_advancedoption_param_add_valenum(param, "foo");

	/* Expected result: 2 - 1x foo, 1x bar */
	fail_unless(osync_list_length(osync_plugin_advancedoption_param_get_valenums(param)) == 2, NULL);

	/* Expected resut: 1 - 1x bar */
	osync_plugin_advancedoption_param_remove_valenum(param, "bar");
	fail_unless(osync_list_length(osync_plugin_advancedoption_param_get_valenums(param)) == 1, NULL);

	/*****/

	osync_plugin_advancedoption_param_set_displayname(param, "foo");
	fail_unless(!strcmp(osync_plugin_advancedoption_param_get_displayname(param), "foo"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_advancedoption_param_set_displayname(param, "bar");
	fail_unless(!strcmp(osync_plugin_advancedoption_param_get_displayname(param), "bar"), NULL);

	osync_plugin_advancedoption_param_set_name(param, "foo");
	fail_unless(!strcmp(osync_plugin_advancedoption_param_get_name(param), "foo"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_advancedoption_param_set_name(param, "bar");
	fail_unless(!strcmp(osync_plugin_advancedoption_param_get_name(param), "bar"), NULL);

	fail_unless(osync_plugin_advancedoption_param_ref(param) != NULL, NULL);
	osync_plugin_advancedoption_param_unref(param);
	osync_plugin_advancedoption_param_unref(param);

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
	OSyncPluginConnection *conn = osync_plugin_connection_new(&error);
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
	osync_plugin_connection_usb_set_vendorid(conn, "0xffff");
	fail_unless(!strcmp(osync_plugin_connection_usb_get_vendorid(conn), "0xffff"), NULL);

	osync_plugin_connection_usb_set_vendorid(conn, "0xaffe");
	fail_unless(!strcmp(osync_plugin_connection_usb_get_vendorid(conn), "0xaffe"), NULL);

	osync_plugin_connection_usb_set_vendorid(conn, "0x0ffe");
	fail_unless(!strcmp(osync_plugin_connection_usb_get_vendorid(conn), "0x0ffe"), NULL);

	/* USB Product ID */
	osync_plugin_connection_usb_set_productid(conn, "0xffff");
	fail_unless(!strcmp(osync_plugin_connection_usb_get_productid(conn), "0xffff"), NULL);

	osync_plugin_connection_usb_set_productid(conn, "0xaffe");
	fail_unless(!strcmp(osync_plugin_connection_usb_get_productid(conn), "0xaffe"), NULL);

	osync_plugin_connection_usb_set_productid(conn, "0x0ffe");
	fail_unless(!strcmp(osync_plugin_connection_usb_get_productid(conn), "0x0ffe"), NULL);

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

START_TEST (plugin_config_resources)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	/* Resources */
	OSyncList *resources = NULL;

	/* Resource */
	OSyncPluginResource *resource = osync_plugin_resource_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(resource != NULL, NULL);

	/* Name */
	osync_plugin_resource_set_name(resource, "foobar");
	fail_unless(!strcmp(osync_plugin_resource_get_name(resource), "foobar"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_resource_set_name(resource, "barfoo");
	fail_unless(!strcmp(osync_plugin_resource_get_name(resource), "barfoo"), NULL);

	/* MIME */
	osync_plugin_resource_set_mime(resource, "foobar");
	fail_unless(!strcmp(osync_plugin_resource_get_mime(resource), "foobar"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_resource_set_mime(resource, "barfoo");
	fail_unless(!strcmp(osync_plugin_resource_get_mime(resource), "barfoo"), NULL);

	/* ObjFormat */
	OSyncObjFormatSink *format_sink = osync_objformat_sink_new("foobar", &error);
	osync_assert(format_sink);
	osync_objformat_sink_set_config(format_sink, "random");
	fail_unless(!strcmp(osync_objformat_sink_get_config(format_sink), "random"), NULL);
	osync_plugin_resource_add_objformat_sink(resource, format_sink);
	osync_objformat_sink_unref(format_sink);

	fail_unless(!strcmp(_format_sink_get_objformat(resource), "foobar"), NULL);

	/* Overwrite (leak check) */
	OSyncObjFormatSink *format_sink2 = osync_objformat_sink_new("barfoo", &error);
	osync_assert(format_sink2);
	osync_objformat_sink_set_config(format_sink2, "random1");
	osync_objformat_sink_set_config(format_sink2, "random2");
	fail_unless(!strcmp(osync_objformat_sink_get_config(format_sink2), "random2"), NULL);
	osync_plugin_resource_add_objformat_sink(resource, format_sink2);
	osync_objformat_sink_unref(format_sink2);

	fail_unless(!strcmp(_format_sink_get_objformat(resource), "barfoo"), NULL);

	/* Check for correct number objformat sinks: 2 sinks! */
	fail_unless(osync_list_length(osync_plugin_resource_get_objformat_sinks(resource)) == 2, NULL);

	/* Check if removing of an objformat sink work at all.. */
	/** reference format_sink2, otherwise refcount gets 0 */
	osync_objformat_sink_ref(format_sink2);
	osync_plugin_resource_remove_objformat_sink(resource, format_sink2);
	fail_unless(osync_list_length(osync_plugin_resource_get_objformat_sinks(resource)) == 1, NULL);
	osync_plugin_resource_add_objformat_sink(resource, format_sink2);
	fail_unless(osync_list_length(osync_plugin_resource_get_objformat_sinks(resource)) == 2, NULL);
	osync_objformat_sink_unref(format_sink2);

	/* ObjType */
	osync_plugin_resource_set_objtype(resource, "foobar");
	fail_unless(!strcmp(osync_plugin_resource_get_objtype(resource), "foobar"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_resource_set_objtype(resource, "barfoo");
	fail_unless(!strcmp(osync_plugin_resource_get_objtype(resource), "barfoo"), NULL);

	/* Path */
	osync_plugin_resource_set_path(resource, "foobar");
	fail_unless(!strcmp(osync_plugin_resource_get_path(resource), "foobar"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_resource_set_path(resource, "barfoo");
	fail_unless(!strcmp(osync_plugin_resource_get_path(resource), "barfoo"), NULL);

	/* URL */
	osync_plugin_resource_set_url(resource, "foobar");
	fail_unless(!strcmp(osync_plugin_resource_get_url(resource), "foobar"), NULL);

	/* Overwrite (leak check) */
	osync_plugin_resource_set_url(resource, "barfoo");
	fail_unless(!strcmp(osync_plugin_resource_get_url(resource), "barfoo"), NULL);

	/* Invoke OSyncPluginConfig */
	osync_plugin_config_add_resource(config, resource);

	/* Check for correct amount of resources */
	fail_unless(osync_list_length(osync_plugin_config_get_resources(config)) == 1, NULL);
	osync_plugin_config_remove_resource(config, resource);
	fail_unless(osync_list_length(osync_plugin_config_get_resources(config)) == 0, NULL);
	osync_plugin_config_add_resource(config, resource);
	osync_plugin_config_add_resource(config, resource);
	fail_unless(osync_list_length(osync_plugin_config_get_resources(config)) == 1, NULL);

	/* Twice to check for correct order of ref/unref calls in set_resources() */
	resources = osync_plugin_config_get_resources(config);
	fail_unless(!strcmp(osync_plugin_resource_get_url(resources->data), "barfoo"), NULL);
	osync_plugin_resource_unref(resources->data);

	fail_unless(osync_plugin_resource_ref(resource) != NULL, NULL);
	osync_plugin_resource_unref(resource);

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

	/* Advanced Options */
	OSyncPluginAdvancedOption *option = osync_plugin_advancedoption_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(option != NULL, NULL);

	osync_plugin_advancedoption_set_max(option, 1);
	osync_plugin_advancedoption_set_min(option, 1);
	osync_plugin_advancedoption_set_maxoccurs(option, 1);
	osync_plugin_advancedoption_set_displayname(option, "foobar1");
	osync_plugin_advancedoption_set_name(option, "foobar1");
	osync_plugin_advancedoption_set_type(option, OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_CHAR);
	osync_plugin_advancedoption_set_value(option, "FOOBAR1");

	osync_plugin_advancedoption_add_valenum(option, "FOOBAR1");
	osync_plugin_advancedoption_add_valenum(option, "FOOBAR2");
	fail_unless(osync_list_length(osync_plugin_advancedoption_get_valenums(option)) == 2, NULL);

	/* Advanced Parameter */
	OSyncPluginAdvancedOptionParameter *param = osync_plugin_advancedoption_param_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(param != NULL, NULL);

	osync_plugin_advancedoption_param_set_displayname(param, "bar1");
	osync_plugin_advancedoption_param_set_name(param, "bar1");
	osync_plugin_advancedoption_param_set_type(param, OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_INT);

	osync_plugin_advancedoption_param_set_value(param, "BAR1");

	osync_plugin_advancedoption_param_add_valenum(param, "BAR1");
	osync_plugin_advancedoption_param_add_valenum(param, "BAR2");
	fail_unless(osync_list_length(osync_plugin_advancedoption_param_get_valenums(param)) == 2, NULL);
	osync_plugin_advancedoption_add_parameter(option, param);
	osync_plugin_advancedoption_param_unref(param);

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

	/* Authentication */
	OSyncPluginAuthentication *auth = osync_plugin_authentication_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(auth != NULL, NULL);

	osync_plugin_authentication_set_username(auth, "foo");
	osync_plugin_authentication_set_password(auth, "bar");
	osync_plugin_authentication_set_reference(auth, "ref");

	/* Resource #1 */
	OSyncPluginResource *resource1 = osync_plugin_resource_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(resource1 != NULL, NULL);

	/* OSyncObjFormatSink */
	OSyncObjFormatSink *format_sink1 = osync_objformat_sink_new("foobar1", &error);
	fail_unless(format_sink1 != NULL, NULL);
	osync_objformat_sink_set_config(format_sink1, "random1");

	/* Name */
	osync_plugin_resource_set_name(resource1, "foobar1");
	osync_plugin_resource_set_mime(resource1, "foobar1");
	osync_plugin_resource_add_objformat_sink(resource1, format_sink1);
	osync_objformat_sink_unref(format_sink1);
	osync_plugin_resource_set_objtype(resource1, "foobar1");
	osync_plugin_resource_set_path(resource1, "foobar1");
	osync_plugin_resource_set_url(resource1, "foobar1");

	/* Resource #2 */
	OSyncPluginResource *resource2 = osync_plugin_resource_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(resource2 != NULL, NULL);

	/* OSyncObjFormatSink */
	OSyncObjFormatSink *format_sink2 = osync_objformat_sink_new("foobar2", &error);
	fail_unless(format_sink2 != NULL, NULL);
	osync_objformat_sink_set_config(format_sink2, "random2");

	osync_plugin_resource_set_name(resource2, "foobar2");
	osync_plugin_resource_set_mime(resource2, "foobar2");
	osync_plugin_resource_add_objformat_sink(resource2, format_sink2);
	osync_objformat_sink_unref(format_sink2);
	osync_plugin_resource_set_objtype(resource2, "foobar2");
	osync_plugin_resource_set_path(resource2, "foobar2");
	osync_plugin_resource_set_url(resource2, "foobar2");

	/* Set subcomponents */
	osync_plugin_config_add_advancedoption(config, option);
	osync_plugin_advancedoption_unref(option);
	osync_plugin_config_set_authentication(config, auth);
	osync_plugin_authentication_unref(auth);
	osync_plugin_config_set_localization(config, local);
	osync_plugin_localization_unref(local);
	osync_plugin_config_add_resource(config, resource1);
	osync_plugin_config_add_resource(config, resource2);
	osync_plugin_resource_unref(resource1);
	osync_plugin_resource_unref(resource2);

	char *config_file = g_strdup_printf("%s/dummy_config.xml", testbed);
	fail_unless(osync_plugin_config_file_save(config, config_file, &error), "%s", osync_error_print(&error));
	fail_unless(osync_plugin_config_file_load(reloaded_config, config_file, testbed, &error), NULL);
	g_free(config_file);

	/* Compare stored config with original config */

	OSyncPluginLocalization *reloaded_local = osync_plugin_config_get_localization(reloaded_config);
	OSyncPluginAuthentication *reloaded_auth = osync_plugin_config_get_authentication(reloaded_config);
	OSyncList *reloaded_resources = osync_plugin_config_get_resources(reloaded_config);
	OSyncList *reloaded_advancedoptions = osync_plugin_config_get_advancedoptions(reloaded_config);
	OSyncList *reloaded_advancedoption_parameters = NULL;
	OSyncList *reloaded_advancedoption_valenums = NULL;
	OSyncList *reloaded_advancedoption_param_valenums = NULL;

	fail_unless(reloaded_local != NULL, NULL);
	fail_unless(reloaded_auth != NULL, NULL);
	fail_unless(reloaded_resources != NULL, NULL);
	fail_unless(reloaded_advancedoptions != NULL, NULL);

	fail_unless(!strcmp(osync_plugin_localization_get_language(reloaded_local), "de_DE"), NULL);
	fail_unless(!strcmp(osync_plugin_localization_get_encoding(reloaded_local), "cp1222"), NULL);
	fail_unless(!strcmp(osync_plugin_localization_get_timezone(reloaded_local), "Europe/Berlin"), NULL);

	fail_unless(!strcmp(osync_plugin_authentication_get_username(reloaded_auth), "foo"), NULL);
	fail_unless(!strcmp(osync_plugin_authentication_get_password(reloaded_auth), "bar"), NULL);
	fail_unless(!strcmp(osync_plugin_authentication_get_reference(reloaded_auth), "ref"), NULL);

	OSyncList *r, *p, *v;
	int i, j, k;
	for (i = 1, r = reloaded_resources; r; r = r->next, i++) {
		char *value = g_strdup_printf("foobar%i", i);
		fail_unless(!strcmp(osync_plugin_resource_get_name(r->data), value), NULL);
		fail_unless(!strcmp(osync_plugin_resource_get_mime(r->data), value), NULL);
		fail_unless(!strcmp(_format_sink_get_objformat(r->data), value), NULL);
		fail_unless(!strcmp(osync_plugin_resource_get_objtype(r->data), value), NULL);

		fail_unless(!strcmp(osync_plugin_resource_get_path(r->data), value), NULL);
		fail_unless(!strcmp(osync_plugin_resource_get_url(r->data), value), NULL);
		g_free(value);
	}

	for (i = 1, r = reloaded_advancedoptions; r; r = r->next, i++) {
		char *value = g_strdup_printf("foobar%i", i);
		fail_unless(!strcmp(osync_plugin_advancedoption_get_name(r->data), value), NULL);
		fail_unless(!strcmp(osync_plugin_advancedoption_get_displayname(r->data), value), NULL);
		fail_unless(osync_plugin_advancedoption_get_max(r->data) == i, NULL);
		fail_unless(osync_plugin_advancedoption_get_min(r->data) == i, NULL);
		fail_unless(osync_plugin_advancedoption_get_maxoccurs(r->data) == i, NULL);
		fail_unless(osync_plugin_advancedoption_get_type(r->data) == OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_CHAR, NULL);
		g_free(value);

		reloaded_advancedoption_valenums = osync_plugin_advancedoption_get_valenums(r->data);
		for (j = 1, v = reloaded_advancedoption_valenums; v; v = v->next, j++) {
			char *value = g_strdup_printf("FOOBAR%i", j);
			fail_unless(!strcmp(v->data, value), NULL);
			g_free(value);
		}

		fail_unless(!strcmp(osync_plugin_advancedoption_get_value(r->data), "FOOBAR1"));

		reloaded_advancedoption_parameters = osync_plugin_advancedoption_get_parameters(r->data);
		for (j = 1, p = reloaded_advancedoption_parameters; p; p = p->next, j++) {
			char *value = g_strdup_printf("bar%i", j);
			fail_unless(!strcmp(osync_plugin_advancedoption_param_get_name(p->data), value), NULL);
			fail_unless(!strcmp(osync_plugin_advancedoption_param_get_displayname(p->data), value), NULL);
			g_free(value);

			reloaded_advancedoption_param_valenums = osync_plugin_advancedoption_param_get_valenums(p->data);
			for (k = 1, v = reloaded_advancedoption_param_valenums; v; v = v->next, k++) {
				char *value = g_strdup_printf("BAR%i", k);
				fail_unless(!strcmp(v->data, value), NULL);
				g_free(value);

			}

			fail_unless(!strcmp(osync_plugin_advancedoption_param_get_value(p->data), "BAR1"));

		}


	}


	osync_plugin_config_unref(config);
	osync_plugin_config_unref(reloaded_config);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (plugin_config_save_and_load_connection_bluetooth)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	OSyncPluginConfig *reloaded_config = osync_plugin_config_new(&error); 
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	/* Connection */
	OSyncPluginConnection *conn = osync_plugin_connection_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(conn != NULL, NULL);

	osync_plugin_connection_set_supported(conn, OSYNC_PLUGIN_CONNECTION_BLUETOOTH);
	osync_plugin_connection_set_type(conn, OSYNC_PLUGIN_CONNECTION_BLUETOOTH);

	/* Bluetooth Address */
	osync_plugin_connection_bt_set_addr(conn, "FF:FF:FF:FF:FF:FF");

	/* Bluetooth RFCOMM */
	osync_plugin_connection_bt_set_channel(conn, 11);

	/* Bluetooth SDP UUID */
	osync_plugin_connection_bt_set_sdpuuid(conn, "00000001-0000-1000-8000-0002EE000002");


	/* Set subcomponents */
	osync_plugin_config_set_connection(config, conn);
	osync_plugin_connection_unref(conn);

	char *config_file = g_strdup_printf("%s/dummy_config.xml", testbed);
	fail_unless(osync_plugin_config_file_save(config, config_file, &error), "%s", osync_error_print(&error));
	fail_unless(osync_plugin_config_file_load(reloaded_config, config_file, testbed, &error), NULL);
	g_free(config_file);

	/* Compare stored config with original config */

	OSyncPluginConnection *reloaded_conn = osync_plugin_config_get_connection(reloaded_config);

	fail_unless(reloaded_conn != NULL, NULL);

	fail_unless(!strcmp(osync_plugin_connection_bt_get_sdpuuid(reloaded_conn), "00000001-0000-1000-8000-0002EE000002"), NULL);
	fail_unless(osync_plugin_connection_bt_get_channel(reloaded_conn) == 11, NULL);
	fail_unless(!strcmp(osync_plugin_connection_bt_get_addr(reloaded_conn), "FF:FF:FF:FF:FF:FF"), NULL);

	osync_plugin_config_unref(config);
	osync_plugin_config_unref(reloaded_config);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (plugin_config_save_and_load_connection_irda)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	OSyncPluginConfig *reloaded_config = osync_plugin_config_new(&error); 
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	/* Connection */
	OSyncPluginConnection *conn = osync_plugin_connection_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(conn != NULL, NULL);

	osync_plugin_connection_set_supported(conn, OSYNC_PLUGIN_CONNECTION_IRDA);
	osync_plugin_connection_set_type(conn, OSYNC_PLUGIN_CONNECTION_IRDA);

	osync_plugin_connection_irda_set_service(conn, "FancyIR Mobile");

	/* Set subcomponents */
	osync_plugin_config_set_connection(config, conn);
	osync_plugin_connection_unref(conn);

	char *config_file = g_strdup_printf("%s/dummy_config.xml", testbed);
	fail_unless(osync_plugin_config_file_save(config, config_file, &error), "%s", osync_error_print(&error));
	fail_unless(osync_plugin_config_file_load(reloaded_config, config_file, testbed, &error), NULL);
	g_free(config_file);

	/* Compare stored config with original config */

	OSyncPluginConnection *reloaded_conn = osync_plugin_config_get_connection(reloaded_config);

	fail_unless(reloaded_conn != NULL, NULL);

	fail_unless(!strcmp(osync_plugin_connection_irda_get_service(reloaded_conn), "FancyIR Mobile"), NULL);

	osync_plugin_config_unref(config);
	osync_plugin_config_unref(reloaded_config);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (plugin_config_save_and_load_connection_usb)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	OSyncPluginConfig *reloaded_config = osync_plugin_config_new(&error); 
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	/* Connection */
	OSyncPluginConnection *conn = osync_plugin_connection_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(conn != NULL, NULL);

	osync_plugin_connection_set_supported(conn, OSYNC_PLUGIN_CONNECTION_USB);
	osync_plugin_connection_set_type(conn, OSYNC_PLUGIN_CONNECTION_USB);

	/* USB Vendor ID */
	osync_plugin_connection_usb_set_vendorid(conn, "0xaffe");

	/* USB Product ID */
	osync_plugin_connection_usb_set_productid(conn, "0xffff");

	/* USB Interface*/
	osync_plugin_connection_usb_set_interface(conn, 2);

	/* Set subcomponents */
	osync_plugin_config_set_connection(config, conn);
	osync_plugin_connection_unref(conn);

	char *config_file = g_strdup_printf("%s/dummy_config.xml", testbed);
	fail_unless(osync_plugin_config_file_save(config, config_file, &error), "%s", osync_error_print(&error));
	fail_unless(osync_plugin_config_file_load(reloaded_config, config_file, testbed, &error), NULL);
	g_free(config_file);

	/* Compare stored config with original config */
	OSyncPluginConnection *reloaded_conn = osync_plugin_config_get_connection(reloaded_config);

	fail_unless(reloaded_conn != NULL, NULL);

	fail_unless(!strcmp(osync_plugin_connection_usb_get_vendorid(reloaded_conn), "0xaffe"), NULL);
	fail_unless(!strcmp(osync_plugin_connection_usb_get_productid(reloaded_conn), "0xffff"), NULL);
	fail_unless(osync_plugin_connection_usb_get_interface(reloaded_conn) == 2, NULL);

	osync_plugin_config_unref(config);
	osync_plugin_config_unref(reloaded_config);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (plugin_config_save_and_load_connection_network)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	OSyncPluginConfig *reloaded_config = osync_plugin_config_new(&error); 
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	/* Connection */
	OSyncPluginConnection *conn = osync_plugin_connection_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(conn != NULL, NULL);

	osync_plugin_connection_set_supported(conn, OSYNC_PLUGIN_CONNECTION_NETWORK);
	osync_plugin_connection_set_type(conn, OSYNC_PLUGIN_CONNECTION_NETWORK);


	/* Network Address */
	osync_plugin_connection_net_set_address(conn, "opensync.org");

	/* Network Port */
	osync_plugin_connection_net_set_port(conn, 8888);

	/* Network Protocol */
	osync_plugin_connection_net_set_protocol(conn, "http://");

	/* Network DNSSD */
	osync_plugin_connection_net_set_dnssd(conn, "_syncml-http._tcp");

	/* Set subcomponents */
	osync_plugin_config_set_connection(config, conn);
	osync_plugin_connection_unref(conn);

	char *config_file = g_strdup_printf("%s/dummy_config.xml", testbed);
	fail_unless(osync_plugin_config_file_save(config, config_file, &error), "%s", osync_error_print(&error));
	fail_unless(osync_plugin_config_file_load(reloaded_config, config_file, testbed, &error), NULL);
	g_free(config_file);

	/* Compare stored config with original config */
	OSyncPluginConnection *reloaded_conn = osync_plugin_config_get_connection(reloaded_config);

	fail_unless(reloaded_conn != NULL, NULL);

	fail_unless(!strcmp(osync_plugin_connection_net_get_dnssd(reloaded_conn), "_syncml-http._tcp"), NULL);
	fail_unless(!strcmp(osync_plugin_connection_net_get_protocol(reloaded_conn), "http://"), NULL);
	fail_unless(!strcmp(osync_plugin_connection_net_get_address(reloaded_conn), "opensync.org"), NULL);
	fail_unless(osync_plugin_connection_net_get_port(reloaded_conn) == 8888, NULL);

	osync_plugin_config_unref(config);
	osync_plugin_config_unref(reloaded_config);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (plugin_config_save_and_load_connection_serial)
{
	char *testbed = setup_testbed(NULL);

	OSyncError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_config_new(&error);
	OSyncPluginConfig *reloaded_config = osync_plugin_config_new(&error); 
	fail_unless(error == NULL, NULL);
	fail_unless(config != NULL, NULL);

	/* Connection */
	OSyncPluginConnection *conn = osync_plugin_connection_new(&error);
	fail_unless(error == NULL, NULL);
	fail_unless(conn != NULL, NULL);

	osync_plugin_connection_set_supported(conn, OSYNC_PLUGIN_CONNECTION_SERIAL);
	osync_plugin_connection_set_type(conn, OSYNC_PLUGIN_CONNECTION_SERIAL);

	/* Serial Speed */
	osync_plugin_connection_serial_set_speed(conn, 1234);

	/* Serial Devicenode */
	osync_plugin_connection_serial_set_devicenode(conn, "/dev/ttyS0");

	/* Set subcomponents */
	osync_plugin_config_set_connection(config, conn);
	osync_plugin_connection_unref(conn);

	char *config_file = g_strdup_printf("%s/dummy_config.xml", testbed);
	fail_unless(osync_plugin_config_file_save(config, config_file, &error), "%s", osync_error_print(&error));
	fail_unless(osync_plugin_config_file_load(reloaded_config, config_file, testbed, &error), NULL);
	g_free(config_file);

	/* Compare stored config with original config */
	OSyncPluginConnection *reloaded_conn = osync_plugin_config_get_connection(reloaded_config);

	fail_unless(reloaded_conn != NULL, NULL);

	fail_unless(osync_plugin_connection_serial_get_speed(conn) == 1234, NULL);
	fail_unless(!strcmp(osync_plugin_connection_serial_get_devicenode(conn), "/dev/ttyS0"), NULL);

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
	create_case(s, "plugin_config_advancedoption", plugin_config_advancedoption);
	create_case(s, "plugin_config_advancedoption_param", plugin_config_advancedoption_param);
	create_case(s, "plugin_config_authentication", plugin_config_authentication);
	create_case(s, "plugin_config_connection", plugin_config_connection);
	create_case(s, "plugin_config_localization", plugin_config_localization);
	create_case(s, "plugin_config_resources", plugin_config_resources);
	create_case(s, "plugin_config_save_and_load", plugin_config_save_and_load);
	create_case(s, "plugin_config_save_and_load_connection_bluetooth", plugin_config_save_and_load_connection_bluetooth);
	create_case(s, "plugin_config_save_and_load_connection_usb", plugin_config_save_and_load_connection_usb);
	create_case(s, "plugin_config_save_and_load_connection_irda", plugin_config_save_and_load_connection_irda);
	create_case(s, "plugin_config_save_and_load_connection_network", plugin_config_save_and_load_connection_network);
	create_case(s, "plugin_config_save_and_load_connection_serial", plugin_config_save_and_load_connection_serial);
	
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
