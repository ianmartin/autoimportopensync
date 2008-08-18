/*
 * syncml plugin - A syncml plugin for OpenSync
 * Copyright (C) 2005  Armin Bauer <armin.bauer@opensync.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

#include "syncml_common.h"
#include "syncml_callbacks.h"
#include "syncml_devinf.h"
#include "syncml_ds_server.h"

void connect_obex_client(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);

	SmlPluginEnv *env = (SmlPluginEnv *)data;
	SmlError *error = NULL;
	OSyncError *oserror = NULL;

	/* For the obex client, we will store the context at this point since
	 * we can only answer it as soon as the device returned an answer to our san */
	env->connectCtx = ctx;

	/* This ref counting is needed to avoid a segfault. TODO: review if this is really needed.
	   To reproduce the segfault - just remove the osync_context_ref() call in the next line. */ 
	osync_context_ref(env->connectCtx);
	
	if (!smlTransportConnect(env->tsp, &error))
		goto error;

	env->isConnected = TRUE;

	if (!smlNotificationSend(env->san, env->tsp, &error))
		goto error_free_san;

	smlNotificationFree(env->san);
	env->san = NULL;

	/* Context get replied in manager_event() callback function. */

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_san:
	smlNotificationFree(env->san);
	env->san = NULL;
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
	report_error_on_context(&(env->connectCtx), &oserror, TRUE);
}

osync_bool syncml_obex_client_parse_config(SmlPluginEnv *env, OSyncPluginConfig *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, config, error);
	OSyncPluginConnection *conn;

	env->useTimestampAnchor = TRUE;
        env->maxObjSize = OSYNC_PLUGIN_SYNCML_MAX_OBJ_SIZE;
        env->maxMsgSize = OSYNC_PLUGIN_SYNCML_MAX_MSG_SIZE;

        conn = osync_plugin_config_get_connection(config);
        if (!conn) {
                osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "No configuration of connection available.");
                goto error;
        }
	
	switch(osync_plugin_connection_get_type(conn)) {
		case OSYNC_PLUGIN_CONNECTION_BLUETOOTH:
			env->bluetoothAddress = osync_plugin_connection_bt_get_addr(conn);
			if (osync_plugin_connection_bt_get_channel(conn))
				env->bluetoothChannel = g_strdup_printf("%u", osync_plugin_connection_bt_get_channel(conn));

			env->type = SML_TRANSPORT_CONNECTION_TYPE_BLUETOOTH;
			break;
                case OSYNC_PLUGIN_CONNECTION_USB:
			/* TODO
			osync_plugin_connection_usb_get_interface(conn);
			*/
			env->type = SML_TRANSPORT_CONNECTION_TYPE_USB;
                        break;
                case OSYNC_PLUGIN_CONNECTION_SERIAL:
			/* TODO serial */
			env->type = SML_TRANSPORT_CONNECTION_TYPE_SERIAL;
                        break;
                case OSYNC_PLUGIN_CONNECTION_IRDA:
			/* TODO IRDA */
			env->type = SML_TRANSPORT_CONNECTION_TYPE_IRDA;
                        break;
                case OSYNC_PLUGIN_CONNECTION_NETWORK:
			/* TODO NEtwork */
			env->type = SML_TRANSPORT_CONNECTION_TYPE_NET;
                        break;
                case OSYNC_PLUGIN_CONNECTION_UNKNOWN:
		default:
			osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unsupported connection type configured.");
			break;
	}

	OSyncPluginAuthentication *auth = osync_plugin_config_get_authentication(config);
	if (auth) {
		env->username = osync_plugin_authentication_get_username(auth);
		/* TODO: Make use of password keyrings/password wallets! */
		env->password = osync_plugin_authentication_get_password(auth);
	}

	OSyncList *optslist = osync_plugin_config_get_advancedoptions(config);
	for (; optslist; optslist = optslist->next) {
		OSyncPluginAdvancedOption *option = optslist->data;

		const char *val = osync_plugin_advancedoption_get_value(option);
		const char *name = osync_plugin_advancedoption_get_name(option);
		g_assert(name);
		g_assert(val);

		if (!strcmp(SYNCML_PLUGIN_CONFIG_SANVERSION, name)) {

			if (!strcmp(val, "1.2")) {
				env->version = SML_SAN_VERSION_12;
			} else if (!strcmp(val, "1.1")) {
				env->version = SML_SAN_VERSION_11;
			} else if (!strcmp(val, "1.0")) {
				env->version = SML_SAN_VERSION_10;
			} else {
				osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Invalid SyncML SAN version.");
				goto error;
			}

		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_WBXML, name)) {
			env->useWbxml = atoi(val);
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_ATCOMMAND, name)) {
			env->atCommand = val;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_ATMANUFACTURER, name)) {
			env->atManufacturer = val;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_ATMODEL, name)) {
			env->atModel = val;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_IDENTIFIER, name)) {
			env->identifier = val;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_USESTRINGTABLE, name)) {
			env->useStringtable = atoi(val);
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_USETIMEANCHOR, name)) {
			env->useTimestampAnchor = atoi(val);
		/* TODO: Dead option? */
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_ONLYREPLACE, name)) {
			env->onlyReplace = atoi(val);
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_MAXMSGSIZE, name)) {
			if (atoi(val))
				env->maxMsgSize = atoi(val);
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_MAXOBJSIZE, name)) {
			if (atoi(val))
				env->maxObjSize = atoi(val);
		/* XXX Workaround for mobiles which only handle localtime! */
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_ONLYLOCALTIME, name)) {
			env->onlyLocaltime = atoi(val);
		}
	

	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

void *syncml_obex_client_init(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);
	SmlError *serror = NULL;
	
	SmlPluginEnv *env = osync_try_malloc0(sizeof(SmlPluginEnv), error);
	if (!env)
		goto error;
	env->pluginInfo = info;
	osync_plugin_info_ref(env->pluginInfo);

	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
        osync_trace(TRACE_INTERNAL, "The config: %p", config);
	
	if (!syncml_obex_client_parse_config(env, config, error))
		goto error_free_env;

	env->num = 0;	
	env->isConnected = FALSE;

	/* Create the alert for the remote device */
	if (!env->identifier)
		env->identifier = get_devinf_identifier();

	/* Register main sink for connect and disconnect functions */
	OSyncObjTypeSink *mainsink = osync_objtype_main_sink_new(error);
	if (!mainsink)
		goto error_free_env;

	OSyncObjTypeSinkFunctions main_functions;
	memset(&main_functions, 0, sizeof(main_functions));
	main_functions.connect = connect_obex_client;
	main_functions.disconnect = disconnect;

	osync_objtype_sink_set_functions(mainsink, main_functions, NULL);
	osync_plugin_info_set_main_sink(info, mainsink);
	osync_objtype_sink_unref(mainsink);

	if (!ds_server_init_databases(env, info, error))
		goto error_free_env;


	env->context = osync_plugin_info_get_loop(info); 

	env->anchor_path = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
	env->devinf_path = g_strdup_printf("%s/devinf.db", osync_plugin_info_get_configdir(info));
	env->connectMutex = g_mutex_new();
	env->managerMutex = g_mutex_new();

	/* Create the SAN */
	env->san = smlNotificationNew(env->version, SML_SAN_UIMODE_UNSPECIFIED, SML_SAN_INITIATOR_USER, 1, env->identifier, "/", env->useWbxml ? SML_MIMETYPE_WBXML : SML_MIMETYPE_XML, &serror);
	if (!env->san)
		goto error;

	/* The transport needed to transport the data */
	env->tsp = smlTransportNew(SML_TRANSPORT_OBEX_CLIENT, &serror);
	if (!env->tsp)
		goto error_free_env;
	if (!smlTransportSetConnectionType(env->tsp, env->type, &serror))
		goto error_free_env;
	if (env->atCommand &&
	    !smlTransportSetConfigOption(env->tsp, "AT_COMMAND", env->atCommand, &serror))
		goto error_free_env;
	if (env->atManufacturer &&
	    !smlTransportSetConfigOption(env->tsp, "AT_MANUFACTURER", env->atManufacturer, &serror))
		goto error_free_env;
	if (env->atModel &&
	    !smlTransportSetConfigOption(env->tsp, "AT_MODEL", env->atModel, &serror))
		goto error_free_env;
	switch(env->type) {
		case SML_TRANSPORT_CONNECTION_TYPE_USB:
			if (!smlTransportSetConfigOption(env->tsp, "PORT", env->port, &serror))
				goto error_free_env;
			break;

		case SML_TRANSPORT_CONNECTION_TYPE_BLUETOOTH:
			if (!env->bluetoothAddress) {
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Bluetooth selected but no bluetooth address given");
				goto error_free_env;
			}
			if (!smlTransportSetConfigOption(env->tsp, "URL", env->bluetoothAddress, &serror) ||
			    !smlTransportSetConfigOption(env->tsp, "PORT", env->bluetoothChannel, &serror))
				goto error_free_env;
			break;

		case SML_TRANSPORT_CONNECTION_TYPE_SERIAL:
		case SML_TRANSPORT_CONNECTION_TYPE_IRDA: 
			if (!smlTransportSetConfigOption(env->tsp, "URL", env->bluetoothAddress, &serror))
				goto error_free_env;
			break;

		default:
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong obex type specified");
			goto error_free_env;
	}

	
	/* The manager responsible for handling the other objects */
	env->manager = smlManagerNew(env->tsp, &serror);
	if (!env->manager)
		goto error_free_env;
	smlManagerSetEventCallback(env->manager, _manager_event, env);
	smlManagerSetLocalMaxMsgSize(env->manager, env->maxMsgSize);
	smlManagerSetLocalMaxObjSize(env->manager, env->maxObjSize);
	smlNotificationSetManager(env->san, env->manager);
	
	/* The authenticator */
	env->auth = smlAuthNew(&serror);
	if (!env->auth)
		goto error_free_env;
	smlAuthSetVerifyCallback(env->auth, _verify_user, env);
	
	if (!env->username)
		smlAuthSetEnable(env->auth, FALSE);
	
	if (!smlAuthRegister(env->auth, env->manager, &serror))
		goto error_free_env;

	if (!init_env_devinf(env, SML_DEVINF_DEVTYPE_SERVER, &serror))
		goto error_free_env;
	
	GList *o = env->databases;
	for (; o; o = o->next) { 
		SmlDatabase *database = o->data;

		/* We now create the ds server hat the given location */
		SmlLocation *loc = smlLocationNew(database->url, NULL, &serror);
		if (!loc)
			goto error_free_env;
		
		const char *ct = get_database_pref_content_type(database, error);
		if (!ct)
			goto error_free_env;

		database->server = smlDsServerNew(ct, loc, &serror);
		if (!database->server)
			goto error_free_env;

		if (!smlDsServerAddSan(database->server, env->san, &serror))
			goto error_free_env;

		if (!smlDsServerRegister(database->server, env->manager, &serror))
			goto error_free_env;
		
		smlDsServerSetConnectCallback(database->server, _ds_alert, database);
		
		/* And we also add the devinfo to the devinf agent */
		if (!add_devinf_datastore(env->devinf, database, error))
			goto error_free_env;

		/* Add the datastore the transport layer.
		 * This is necessary for devices which need a special
		 * AT command like some Samsung devices.
		 */
		char *objtype = g_ascii_strup(osync_objformat_get_objtype(database->objformat), -1);
		if (!smlTransportSetConfigOption(env->tsp, "DATASTORE", objtype, &serror))
		{
			safe_cfree(&objtype);
			goto error_free_env;
		}
		safe_cfree(&objtype);
	}

	GSourceFuncs *functions = g_malloc0(sizeof(GSourceFuncs));
	functions->prepare = _sessions_prepare;
	functions->check = _sessions_check;
	functions->dispatch = _sessions_dispatch;
	functions->finalize = NULL;

	GSource *source = g_source_new(functions, sizeof(GSource) + sizeof(SmlPluginEnv *));
	SmlPluginEnv **envptr = (SmlPluginEnv **)(source + 1);
	*envptr = env;
	g_source_set_callback(source, NULL, env, NULL);
	g_source_attach(source, env->context);

	env->source = source;
	env->source_functions = functions;

	/* Run the manager */
	if (!smlManagerStart(env->manager, &serror))
		goto error_free_env;
	
	/* Initialize the Transport */
	if (!smlTransportInitialize(env->tsp, &serror))
		goto error_free_env;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;

error_free_env:
	finalize(env);
error:
	if (serror)
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&serror));
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

osync_bool syncml_obex_client_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);
        
        SmlPluginEnv *env = (SmlPluginEnv *)data;
        GList *o = env->databases;
        for (; o; o = o->next) {
                SmlDatabase *database = o->data;
                osync_objtype_sink_set_available(database->sink, TRUE);
        }
        
        OSyncVersion *version = osync_version_new(error);
        osync_version_set_plugin(version, "syncml-obex-client");
        //osync_version_set_modelversion(version, "version");
        //osync_version_set_firmwareversion(version, "firmwareversion");
        //osync_version_set_softwareversion(version, "softwareversion");
        //osync_version_set_hardwareversion(version, "hardwareversion");
        osync_plugin_info_set_version(info, version);
        osync_version_unref(version);

        osync_trace(TRACE_EXIT, "%s", __func__);
        return TRUE;
}
