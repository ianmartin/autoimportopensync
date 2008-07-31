#include "support.h"
#include <sys/wait.h>

#include <opensync/opensync-ipc.h>
#include <opensync/opensync-plugin.h>

static osync_bool _compare_string(const char *string1, const char *string2)
{
	if ((!!string1) != (!!string2))
		return FALSE;

	if (string1 && string2 && strcmp(string1, string2))
		return FALSE;

	return TRUE;
}

typedef osync_bool (*_compare_func)(void *a, void *b);

static osync_bool _compare_list(OSyncList *list1, OSyncList *list2, _compare_func cmpfunc)
{
	OSyncList *l1 = osync_list_copy(list1);
	OSyncList *l2 = osync_list_copy(list2);

	while (l1) {
		for (l2 = osync_list_first(l2); l2; l2 = l2->next) {
			if (!cmpfunc(l1->data, l2->data))
				continue;

			/* Bingo, match! */
			l1 = osync_list_remove(l1, l1->data); 
			l2 = osync_list_remove(l2, l2->data); 
			break;
		}
	}

	if (osync_list_length(l1) || osync_list_length(l2))
		return FALSE;

	return TRUE;
}

static osync_bool _compare_pluginconfig_connection(OSyncPluginConnection *conn1, OSyncPluginConnection *conn2)
{
	osync_assert(conn1);
	osync_assert(conn2);

	switch (osync_plugin_connection_get_type(conn1)) {
		case OSYNC_PLUGIN_CONNECTION_BLUETOOTH:
		/** Bluetooth */
		if (!_compare_string(osync_plugin_connection_bt_get_addr(conn1), osync_plugin_connection_bt_get_addr(conn2)))
			return FALSE;

		if (osync_plugin_connection_bt_get_channel(conn1) != osync_plugin_connection_bt_get_channel(conn2))
			return FALSE;

		if (!_compare_string(osync_plugin_connection_bt_get_sdpuuid(conn1), osync_plugin_connection_bt_get_sdpuuid(conn2)))
			return FALSE;
		break;
		case OSYNC_PLUGIN_CONNECTION_USB:
		/** USB */
		if (osync_plugin_connection_usb_get_vendorid(conn1) != osync_plugin_connection_usb_get_vendorid(conn2))
			return FALSE;

		if (osync_plugin_connection_usb_get_productid(conn1) != osync_plugin_connection_usb_get_productid(conn2))
			return FALSE;

		if (osync_plugin_connection_usb_get_interface(conn1) != osync_plugin_connection_usb_get_interface(conn2))
			return FALSE;
		break;

		case OSYNC_PLUGIN_CONNECTION_NETWORK:
		/** Network */
		if (!_compare_string(osync_plugin_connection_net_get_address(conn1), osync_plugin_connection_net_get_address(conn2)))
			return FALSE;

		if (osync_plugin_connection_net_get_port(conn1) != osync_plugin_connection_net_get_port(conn2))
			return FALSE;

		if (!_compare_string(osync_plugin_connection_net_get_protocol(conn1), osync_plugin_connection_net_get_protocol(conn2)))
			return FALSE;

		if (!_compare_string(osync_plugin_connection_net_get_dnssd(conn1), osync_plugin_connection_net_get_dnssd(conn2)))
			return FALSE;

		break;


		case OSYNC_PLUGIN_CONNECTION_SERIAL:
		/** Serial */
		if (osync_plugin_connection_serial_get_speed(conn1) != osync_plugin_connection_serial_get_speed(conn2))
			return FALSE;

		if (!_compare_string(osync_plugin_connection_serial_get_devicenode(conn1), osync_plugin_connection_serial_get_devicenode(conn2)))
			return FALSE;

		break;

		case OSYNC_PLUGIN_CONNECTION_IRDA:
		/** IrDA */
		if (!_compare_string(osync_plugin_connection_irda_get_service(conn1), osync_plugin_connection_irda_get_service(conn2)))
			return FALSE;

		break;
		case OSYNC_PLUGIN_CONNECTION_UNKNOWN:
		break;
	}

	return TRUE;
}

static osync_bool _compare_pluginconfig_authentication(OSyncPluginAuthentication *auth1, OSyncPluginAuthentication *auth2)
{
	osync_assert(auth1);
	osync_assert(auth2);

	const char *username1, *username2;
	const char *password1, *password2;
	const char *reference1, *reference2;

	username1 = osync_plugin_authentication_get_username(auth1);
	username2 = osync_plugin_authentication_get_username(auth2);

	if (!_compare_string(username1, username2))
		return FALSE;

	password1 = osync_plugin_authentication_get_password(auth1);
	password2 = osync_plugin_authentication_get_password(auth2);

	if (!_compare_string(password1, password2))
		return FALSE;
	
	reference1 = osync_plugin_authentication_get_reference(auth1);
	reference2 = osync_plugin_authentication_get_reference(auth2);

	if (!_compare_string(reference1, reference2))
		return FALSE;

	return TRUE;
}

static osync_bool _compare_pluginconfig_localization(OSyncPluginLocalization *local1, OSyncPluginLocalization *local2)
{
	osync_assert(local1);
	osync_assert(local2);

	const char *encoding1, *encoding2;
	const char *timezone1, *timezone2;
	const char *language1, *language2;

	encoding1 = osync_plugin_localization_get_encoding(local1);
	encoding2 = osync_plugin_localization_get_encoding(local2);

	if (!_compare_string(encoding1, encoding2))
		return FALSE;

	timezone1 = osync_plugin_localization_get_timezone(local1);
	timezone2 = osync_plugin_localization_get_timezone(local2);

	if (!_compare_string(timezone1, timezone2))
		return FALSE;

	language1 = osync_plugin_localization_get_language(local1);
	language2 = osync_plugin_localization_get_language(local2);

	if (!_compare_string(language1, language2))
		return FALSE;
	
	return TRUE;
}

static osync_bool _compare_pluginconfig_ressource(void *a, void *b)
{
	OSyncPluginRessource *res1 = a;
	OSyncPluginRessource *res2 = b;
	if (osync_plugin_ressource_is_enabled(res1) != osync_plugin_ressource_is_enabled(res2))
		return FALSE;

	const char *name1 = osync_plugin_ressource_get_name(res1);
	const char *name2 = osync_plugin_ressource_get_name(res2);

	if (!_compare_string(name1, name2))
		return FALSE;

	const char *mime1 = osync_plugin_ressource_get_mime(res1);
	const char *mime2 = osync_plugin_ressource_get_mime(res2);

	if (!_compare_string(mime1, mime2))
		return FALSE;

	const char *objtype1 = osync_plugin_ressource_get_objtype(res1);
	const char *objtype2 = osync_plugin_ressource_get_objtype(res2);

	if (!_compare_string(objtype1, objtype2))
		return FALSE;

	return TRUE;
}

static osync_bool _compare_pluginconfig_advacedoption_parameters(void *a, void *b)
{
	OSyncPluginAdvancedOptionParameter *param1 = a;
	OSyncPluginAdvancedOptionParameter *param2 = b;

	osync_assert(param1);
	osync_assert(param2);

	/* TODO compare fields */

	return TRUE;
}

static osync_bool _compare_pluginconfig_advancedoption(void *a, void *b)
{
	OSyncPluginAdvancedOption *opt1 = a;
	OSyncPluginAdvancedOption *opt2 = b;
	osync_assert(opt1);
	osync_assert(opt2);

	OSyncList *param_list1 = osync_plugin_advancedoption_get_parameters(opt1);
	OSyncList *param_list2 = osync_plugin_advancedoption_get_parameters(opt2);

	if (!_compare_list(param_list1, param_list2, _compare_pluginconfig_advacedoption_parameters))
		return FALSE;

	/* TODO compare fields */

	return TRUE;
}

static osync_bool _compare_pluginconfig(OSyncPluginConfig *config1, OSyncPluginConfig *config2)
{
	OSyncPluginAuthentication *auth1 = osync_plugin_config_get_authentication(config1);
	OSyncPluginAuthentication *auth2 = osync_plugin_config_get_authentication(config2);
	if (auth1 && auth2 && !_compare_pluginconfig_authentication(auth1, auth2))
		return FALSE;

	OSyncPluginLocalization *local1 = osync_plugin_config_get_localization(config1);
	OSyncPluginLocalization *local2 = osync_plugin_config_get_localization(config2);

	if (local1 && local2 && !_compare_pluginconfig_localization(local1, local2))
		return FALSE;

	OSyncList *ressources1 = osync_plugin_config_get_ressources(config1);
	OSyncList *ressources2 = osync_plugin_config_get_ressources(config2);
	if (!_compare_list(ressources1, ressources2, _compare_pluginconfig_ressource))
		return FALSE;


	OSyncPluginConnection *conn1 = osync_plugin_config_get_connection(config1);
	OSyncPluginConnection *conn2 = osync_plugin_config_get_connection(config2);
	if (conn1 && conn2 && !_compare_pluginconfig_connection(conn1, conn2))
			return FALSE;

	OSyncList *advancedopts1 = osync_plugin_config_get_advancedoptions(config1);
	OSyncList *advancedopts2 = osync_plugin_config_get_advancedoptions(config2);
	if (!_compare_list(advancedopts1, advancedopts2, _compare_pluginconfig_advancedoption))
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

	osync_message_unref(message);

	fail_unless(_compare_pluginconfig(config1, config2), NULL);

	osync_plugin_config_unref(config1);
	osync_plugin_config_unref(config2);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (serializer_pluginconfig_full)
{
	char *testbed = setup_testbed("serializer_pluginconfig_full");
	
	OSyncError *error = NULL;
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
	fail_unless(message != NULL, NULL);
	fail_unless(error == NULL, NULL);

	OSyncPluginConfig *config2;
	OSyncPluginConfig *config1 = osync_plugin_config_new(&error);
	fail_unless(config1 != NULL, NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(osync_plugin_config_file_load(config1, "config1.xml", testbed, &error));

	fail_unless(osync_marshal_pluginconfig(message, config1, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(osync_demarshal_pluginconfig(message, &config2, &error), NULL);
	fail_unless(error == NULL, NULL);

	osync_message_unref(message);

	fail_unless(_compare_pluginconfig(config1, config2), NULL);

	osync_plugin_config_unref(config1);
	osync_plugin_config_unref(config2);
	
	destroy_testbed(testbed);
}
END_TEST


Suite *ipc_suite(void)
{
	Suite *s = suite_create("Serializer");
//	Suite *s2 = suite_create("Serializer");
	
	create_case(s, "serializer_pluginconfig", serializer_pluginconfig);
	create_case(s, "serializer_pluginconfig_full", serializer_pluginconfig_full);
	
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
