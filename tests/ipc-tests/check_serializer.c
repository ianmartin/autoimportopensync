#include "support.h"
#include <sys/wait.h>

#include <opensync/opensync-ipc.h>
#include <opensync/opensync-plugin.h>


static osync_bool _compare_pluginconfig_connection(OSyncPluginConnection *conn1, OSyncPluginConnection *conn2)
{
	osync_assert(conn1);
	osync_assert(conn2);

	switch (osync_plugin_connection_get_type(conn1)) {
		case OSYNC_PLUGIN_CONNECTION_BLUETOOTH:
		/** Bluetooth */
		if (strcmp(osync_plugin_connection_bt_get_addr(conn1), osync_plugin_connection_bt_get_addr(conn2)))
			return FALSE;

		if (osync_plugin_connection_bt_get_channel(conn1) == osync_plugin_connection_bt_get_channel(conn2))
			return FALSE;

		if (strcmp(osync_plugin_connection_bt_get_sdpuuid(conn1), osync_plugin_connection_bt_get_sdpuuid(conn2)))
			return FALSE;
		break;
		case OSYNC_PLUGIN_CONNECTION_USB:
		/** USB */
		if (osync_plugin_connection_usb_get_vendorid(conn1) == osync_plugin_connection_usb_get_vendorid(conn2))
			return FALSE;

		if (osync_plugin_connection_usb_get_productid(conn1) == osync_plugin_connection_usb_get_productid(conn2))
			return FALSE;

		if (osync_plugin_connection_usb_get_interface(conn1) == osync_plugin_connection_usb_get_interface(conn2))
			return FALSE;
		break;

		case OSYNC_PLUGIN_CONNECTION_NETWORK:
		/** Network */
		if (strcmp(osync_plugin_connection_net_get_address(conn1), osync_plugin_connection_net_get_address(conn2)))
			return FALSE;

		if (osync_plugin_connection_net_get_port(conn1) == osync_plugin_connection_net_get_port(conn2))
			return FALSE;

		if (strcmp(osync_plugin_connection_net_get_protocol(conn1), osync_plugin_connection_net_get_protocol(conn2)))
			return FALSE;

		if (strcmp(osync_plugin_connection_net_get_dnssd(conn1), osync_plugin_connection_net_get_dnssd(conn2)))
			return FALSE;

		break;


		case OSYNC_PLUGIN_CONNECTION_SERIAL:
		/** Serial */
		if (osync_plugin_connection_serial_get_speed(conn1) == osync_plugin_connection_serial_get_speed(conn2))
			return FALSE;

		if (strcmp(osync_plugin_connection_serial_get_devicenode(conn1), osync_plugin_connection_serial_get_devicenode(conn2)))
			return FALSE;

		break;

		case OSYNC_PLUGIN_CONNECTION_IRDA:
		/** IrDA */
		if (strcmp(osync_plugin_connection_irda_get_service(conn1), osync_plugin_connection_irda_get_service(conn2)))
			return FALSE;

		break;
		case OSYNC_PLUGIN_CONNECTION_UNKNOWN:
		break;
	}

	return TRUE;
}

static osync_bool _compare_pluginconfig(OSyncPluginConfig *config1, OSyncPluginConfig *config2)
{
	/*
	if (!_compare_pluginconfig_authentication(config1, config2))
		return FALSE;

	if (!_compare_pluginconfig_localization(config1, config2))
		return FALSE;

	if (!_compare_pluginconfig_ressources(config1, config2))
		return FALSE;
	*/

	OSyncPluginConnection *conn1 = osync_plugin_config_get_connection(config1);
	OSyncPluginConnection *conn2 = osync_plugin_config_get_connection(config2);
	if (conn1 && conn2)
		if (!_compare_pluginconfig_connection(conn1, conn2))
			return FALSE;

	return TRUE;
}

START_TEST (serializer_pluginconfig)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
	fail_unless(message != NULL, NULL);
	fail_unless(error == NULL, NULL);

	OSyncPluginConfig *config2;
	OSyncPluginConfig *config1 = osync_plugin_config_new(&error);
	fail_unless(config1 != NULL, NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(osync_marshal_pluginconfig(message, config1, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(osync_demarshal_pluginconfig(message, &config2, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(_compare_pluginconfig(config1, config2), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *ipc_suite(void)
{
	Suite *s = suite_create("Serializer");
//	Suite *s2 = suite_create("Serializer");
	
	create_case(s, "serializer_pluginconfig", serializer_pluginconfig);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = ipc_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
