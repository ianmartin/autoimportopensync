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

osync_bool syncml_obex_client_parse_config(SmlPluginEnv *env, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, config, error);
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	
	if (!(doc = xmlParseMemory(config, strlen(config)))) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Could not parse config");
		goto error;
	}

	if (!(cur = xmlDocGetRootElement(doc))) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "config seems to be empty");
		goto error_free_doc;
	}
	
	if (xmlStrcmp(cur->name, (xmlChar*)"config")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "config does not seem to be valid");
		goto error_free_doc;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		char *str = (char*)xmlNodeGetContent(cur);
		if (str && strlen(str)) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"bluetooth_address")) {
				env->bluetoothAddress = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"bluetooth_channel")) {
				env->bluetoothChannel = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"interface")) {
				env->interface = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"identifier") && strlen(str)) {
				env->identifier = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"type")) {
				env->type = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"recvLimit")) {
				env->recvLimit = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"version")) {
				switch (atoi(str)) {
					case 0:
						env->version = SML_SAN_VERSION_10;
						break;
					case 1:
						env->version = SML_SAN_VERSION_11;
						break;
					case 2:
						env->version = SML_SAN_VERSION_12;
						break;
					default:
						xmlFree(str);
						osync_error_set(error, OSYNC_ERROR_GENERIC, "config does not seem to be valid2");
						goto error_free_doc;
				}
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"wbxml")) {
				env->useWbxml = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"username")) {
				env->username = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"password")) {
				env->password = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"usestringtable")) {
				env->useStringtable = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"onlyreplace")) {
				env->onlyReplace = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"maxObjSize")) {
				env->maxObjSize = atoi(str);
			}

			/* XXX Workaround for mobiles which only handle localtime! */
			if (!xmlStrcmp(cur->name, (const xmlChar *)"onlyLocaltime")) {
				env->onlyLocaltime = atoi(str);
			}

			if (!xmlStrcmp(cur->name, (const xmlChar *)"database")) {
				if (!syncml_config_parse_database(env, cur->xmlChildrenNode, error))
					goto error_free_doc;
			}


		}
		if (str)
			xmlFree(str);

		cur = cur->next;
	}
	
	xmlFreeDoc(doc);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_doc:
	xmlFreeDoc(doc);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

void *syncml_obex_client_init(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);
	SmlError *serror = NULL;
	
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	SmlPluginEnv *env = osync_try_malloc0(sizeof(SmlPluginEnv), error);
	if (!env)
		goto error;
	env->pluginInfo = info;

	const char *configdata = osync_plugin_info_get_config(info);
        osync_trace(TRACE_INTERNAL, "The config: %s", configdata);
	
	if (!syncml_obex_client_parse_config(env, configdata, error))
		goto error_free_env;

	env->num = 0;	
	env->isConnected = FALSE;

	/* Create the alert for the remote device */
	if (!env->identifier)
		env->identifier = get_devinf_identifier();

	/* Register main sink for connect and disconnect functions */
	env->mainsink = osync_objtype_main_sink_new(error);
	if (!env->mainsink)
		goto error_free_env;

	OSyncObjTypeSinkFunctions main_functions;
	memset(&main_functions, 0, sizeof(main_functions));
	main_functions.connect = connect_obex_client;
	main_functions.disconnect = disconnect;

	osync_objtype_sink_set_functions(env->mainsink, main_functions, NULL);
	osync_plugin_info_set_main_sink(info, env->mainsink);

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
	switch(env->type) {
		case SML_OBEX_TYPE_USB:
			if (!smlTransportSetConfigOption(env->tsp, "PORT", env->interface, &serror))
				goto error_free_env;
			break;

		case SML_OBEX_TYPE_BLUETOOTH:
			if (!env->bluetoothAddress) {
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Bluetooth selected but no bluetooth address given");
				goto error_free_env;
			}
			if (!smlTransportSetConfigOption(env->tsp, "URL", env->bluetoothAddress, &serror) ||
			    !smlTransportSetConfigOption(env->tsp, "PORT", env->bluetoothChannel, &serror))
				goto error_free_env;
			break;

		case SML_OBEX_TYPE_SERIAL:
		case SML_OBEX_TYPE_IRDA: 
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
		
		database->server = smlDsServerNew(
					get_database_pref_content_type(database, error),
                                	loc, &serror);
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
	if (env->san) {
		smlNotificationFree(env->san);
		env->san = NULL;
	}
	if (env->auth)
		smlAuthFree(env->auth);
	if (env->manager)
		smlManagerFree(env->manager);
	if (env->tsp)
		smlTransportFree(env->tsp);
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
