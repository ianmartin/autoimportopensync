#include "support.h"

#include <opensync/opensync-ipc.h>
#include "opensync/ipc/opensync_serializer_internals.h"
#include <opensync/opensync-plugin.h>

static osync_bool _compare_string(const void *string1, const void *string2)
{
	if ((!!string1) != (!!string2))
		return FALSE;

	if (string1 && string2 && strcmp(string1, string2))
		return FALSE;

	return TRUE;
}

typedef osync_bool (*_compare_func)(const void *a, const void *b);

static osync_bool _compare_list(OSyncList *list1, OSyncList *list2, _compare_func cmpfunc)
{
	for (; list1; list1 = list1->next) {
		for (list2 = osync_list_first(list2); list2; list2 = list2->next) {
			if (cmpfunc(list1->data, list2->data))
				break;
		}

		if (!list2)
			return FALSE;
	}

	return TRUE;
}

static osync_bool _compare_pluginconfig_connection(OSyncPluginConnection *conn1, OSyncPluginConnection *conn2)
{

	if (!conn1 && !conn2)
		return TRUE;

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
	const char *username1, *username2;
	const char *password1, *password2;
	const char *reference1, *reference2;

	if (!auth1 && !auth2)
		return TRUE;

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
	const char *encoding1, *encoding2;
	const char *timezone1, *timezone2;
	const char *language1, *language2;

	if (!local1 && !local2)
		return TRUE;

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

static osync_bool _compare_pluginconfig_resource(const void *a, const void *b)
{
	OSyncPluginResource *res1 = (OSyncPluginResource *) a;
	OSyncPluginResource *res2 = (OSyncPluginResource *) b;

	if (osync_plugin_resource_is_enabled(res1) != osync_plugin_resource_is_enabled(res2))
		return FALSE;

	const char *name1 = osync_plugin_resource_get_name(res1);
	const char *name2 = osync_plugin_resource_get_name(res2);

	if (!_compare_string(name1, name2))
		return FALSE;

	const char *mime1 = osync_plugin_resource_get_mime(res1);
	const char *mime2 = osync_plugin_resource_get_mime(res2);

	if (!_compare_string(mime1, mime2))
		return FALSE;

	const char *objtype1 = osync_plugin_resource_get_objtype(res1);
	const char *objtype2 = osync_plugin_resource_get_objtype(res2);

	if (!_compare_string(objtype1, objtype2))
		return FALSE;

	return TRUE;
}

static osync_bool _compare_pluginconfig_advacedoption_parameters(const void *a, const void *b)
{
	OSyncPluginAdvancedOptionParameter *param1 = (OSyncPluginAdvancedOptionParameter *) a;
	OSyncPluginAdvancedOptionParameter *param2 = (OSyncPluginAdvancedOptionParameter *) b;

	osync_assert(param1);
	osync_assert(param2);

	const char *displayname1 = osync_plugin_advancedoption_param_get_displayname(param1);
	const char *displayname2 = osync_plugin_advancedoption_param_get_displayname(param2);

	if (!_compare_string(displayname1, displayname2))
		return FALSE;

	const char *name1 = osync_plugin_advancedoption_param_get_name(param1);
	const char *name2 = osync_plugin_advancedoption_param_get_name(param2);

	if (!_compare_string(name1, name2))
		return FALSE;

	OSyncPluginAdvancedOptionType type1 = osync_plugin_advancedoption_param_get_type(param1);
	OSyncPluginAdvancedOptionType type2 = osync_plugin_advancedoption_param_get_type(param2);

	if (type1 != type2)
		return FALSE;

	OSyncList *valenums1 = osync_plugin_advancedoption_param_get_valenums(param1); 
	OSyncList *valenums2 = osync_plugin_advancedoption_param_get_valenums(param2); 

	if (!_compare_list(valenums1, valenums2, _compare_string))
		return FALSE;

	const char *value1 = osync_plugin_advancedoption_param_get_value(param1);
	const char *value2 = osync_plugin_advancedoption_param_get_value(param2);

	if (!_compare_string(value1, value2))
		return FALSE;

	return TRUE;
}

static osync_bool _compare_pluginconfig_advancedoption(const void *a, const void *b)
{
	OSyncPluginAdvancedOption *opt1 = (OSyncPluginAdvancedOption *) a;
	OSyncPluginAdvancedOption *opt2 = (OSyncPluginAdvancedOption *) b;
	osync_assert(opt1);
	osync_assert(opt2);

	OSyncList *param_list1 = osync_plugin_advancedoption_get_parameters(opt1);
	OSyncList *param_list2 = osync_plugin_advancedoption_get_parameters(opt2);

	if (!_compare_list(param_list1, param_list2, _compare_pluginconfig_advacedoption_parameters))
		return FALSE;

	const char *displayname1 = osync_plugin_advancedoption_get_displayname(opt1);
	const char *displayname2 = osync_plugin_advancedoption_get_displayname(opt2);

	if (!_compare_string(displayname1, displayname2))
		return FALSE;

	const char *name1 = osync_plugin_advancedoption_get_name(opt1);
	const char *name2 = osync_plugin_advancedoption_get_name(opt2);

	if (!_compare_string(name1, name2))
		return FALSE;

	OSyncPluginAdvancedOptionType type1 = osync_plugin_advancedoption_get_type(opt1);
	OSyncPluginAdvancedOptionType type2 = osync_plugin_advancedoption_get_type(opt2);

	if (type1 != type2)
		return FALSE;

	OSyncList *valenums1 = osync_plugin_advancedoption_get_valenums(opt1); 
	OSyncList *valenums2 = osync_plugin_advancedoption_get_valenums(opt2); 

	if (!_compare_list(valenums1, valenums2, _compare_string))
		return FALSE;

	const char *value1 = osync_plugin_advancedoption_get_value(opt1);
	const char *value2 = osync_plugin_advancedoption_get_value(opt2);

	if (!_compare_string(value1, value2))
		return FALSE;

	unsigned int max1 = osync_plugin_advancedoption_get_max(opt1);
	unsigned int max2 = osync_plugin_advancedoption_get_max(opt2);

	if (max1 != max2)
		return FALSE;

	unsigned int maxoccurs1 = osync_plugin_advancedoption_get_maxoccurs(opt1);
	unsigned int maxoccurs2 = osync_plugin_advancedoption_get_maxoccurs(opt2);

	if (maxoccurs1 != maxoccurs2)
		return FALSE;

	return TRUE;
}

static osync_bool _compare_pluginconfig(OSyncPluginConfig *config1, OSyncPluginConfig *config2)
{
	OSyncPluginAuthentication *auth1 = osync_plugin_config_get_authentication(config1);
	OSyncPluginAuthentication *auth2 = osync_plugin_config_get_authentication(config2);
	fail_unless(_compare_pluginconfig_authentication(auth1, auth2), NULL);

	OSyncPluginLocalization *local1 = osync_plugin_config_get_localization(config1);
	OSyncPluginLocalization *local2 = osync_plugin_config_get_localization(config2);

	fail_unless(_compare_pluginconfig_localization(local1, local2), NULL);

	OSyncList *resources1 = osync_plugin_config_get_resources(config1);
	OSyncList *resources2 = osync_plugin_config_get_resources(config2);
	fail_unless(_compare_list(resources1, resources2, _compare_pluginconfig_resource), NULL);


	OSyncPluginConnection *conn1 = osync_plugin_config_get_connection(config1);
	OSyncPluginConnection *conn2 = osync_plugin_config_get_connection(config2);
	fail_unless(_compare_pluginconfig_connection(conn1, conn2), NULL);

	OSyncList *advancedopts1 = osync_plugin_config_get_advancedoptions(config1);
	OSyncList *advancedopts2 = osync_plugin_config_get_advancedoptions(config2);
	fail_unless(_compare_list(advancedopts1, advancedopts2, _compare_pluginconfig_advancedoption), NULL);


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
