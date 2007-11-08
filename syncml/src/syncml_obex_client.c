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

static void connect_obex_client(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);

	SmlPluginEnv *env = (SmlPluginEnv *)data;

	SmlError *error = NULL;
	OSyncError *oserror = NULL;

	if (env->isConnected) {
		osync_context_report_success(ctx);
		osync_trace(TRACE_EXIT, "%s", __func__);
		return;
	}

	env->tryDisconnect = FALSE;

	/* For the obex client, we will store the context at this point since
	 * we can only answer it as soon as the device returned an answer to our san */
	env->connectCtx = ctx;

	/* This ref counting is needed to avoid a segfault. TODO: review if this is really needed.
	   To reproduce the segfault - just remove the osync_context_ref() call in the next line. */ 
	osync_context_ref(env->connectCtx);
	
	if (!smlTransportConnect(env->tsp, &error))
		goto error;
	else
		env->isConnected = TRUE;

	if (!smlNotificationSend(env->san, env->tsp, &error))
		goto error_free_san;

	smlNotificationFree(env->san);
	env->san = NULL;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_san:
	smlNotificationFree(env->san);
	env->san = NULL;
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_context_report_osyncerror(ctx, oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

static osync_bool syncml_obex_client_parse_config(SmlPluginEnv *env, const char *config, OSyncError **error)
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
				env->bluetoothChannel = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"interface")) {
				env->interface = atoi(str);
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

extern void *syncml_obex_client_init(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);
	SmlError *serror = NULL;
	
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	SmlPluginEnv *env = osync_try_malloc0(sizeof(SmlPluginEnv), error);
	if (!env)
		goto error;

	const char *configdata = osync_plugin_info_get_config(info);
        osync_trace(TRACE_INTERNAL, "The config: %s", configdata);
	
	if (!syncml_obex_client_parse_config(env, configdata, error))
		goto error_free_env;

	env->num = 0;	
	env->isConnected = FALSE;

	SmlTransportObexClientConfig config;
	config.type = env->type;
	switch(config.type) {
		case SML_OBEX_TYPE_USB:
			config.url = NULL;
			config.port = env->interface;
			break;

		case SML_OBEX_TYPE_BLUETOOTH:
			if (!env->bluetoothAddress) {
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Bluetooth selected but no bluetooth address given");
				goto error_free_auth;
			}
			config.url = g_strdup(env->bluetoothAddress);
			config.port = env->bluetoothChannel;
			break;

		case SML_OBEX_TYPE_SERIAL:
		case SML_OBEX_TYPE_IRDA: 
			config.url = env->bluetoothAddress;
			break;

		default:
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong obex type specified");
			goto error_free_auth;
	}

	/* Create the alert for the remote device */
	if (!env->identifier)
		env->identifier = g_strdup("OpenSync SyncML Plugin");

	GList *o = env->databases;
	for (; o; o = o->next) {
                SmlDatabase *database = o->data;
		database->gotChanges = FALSE;
		database->finalChanges = FALSE;
                OSyncObjTypeSink *sink = osync_objtype_sink_new(database->objtype, error);
                if (!sink)
                        goto error_free_env;

		database->objformat = osync_format_env_find_objformat(formatenv, database->objformat_name);
		if (!database->objformat) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find \"%s\" object format. Are format plugins correctly installed?", database->objformat_name);
			return FALSE;
		}

		// TODO:... in case of maemo ("plain text") we have to set "memo"...
		osync_objtype_sink_add_objformat(sink, database->objformat_name);
                
                OSyncObjTypeSinkFunctions functions;
                memset(&functions, 0, sizeof(functions));
                functions.connect = connect_obex_client;
                functions.disconnect = disconnect;
                functions.get_changes = get_changeinfo;
                functions.sync_done = sync_done;
		functions.batch_commit = batch_commit;
                
                osync_objtype_sink_set_functions(sink, functions, database);
                database->sink = sink;
                osync_plugin_info_add_objtype(info, sink);
	}

	env->context = osync_plugin_info_get_loop(info); 

	env->anchor_path = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));

	/* Create the SAN */
	env->san = smlNotificationNew(env->version, SML_SAN_UIMODE_UNSPECIFIED, SML_SAN_INITIATOR_USER, 1, env->identifier, "/", env->useWbxml ? SML_MIMETYPE_WBXML : SML_MIMETYPE_XML, &serror);
	if (!env->san)
		goto error;

	/* The transport needed to transport the data */
	env->tsp = smlTransportNew(SML_TRANSPORT_OBEX_CLIENT, &serror);
	if (!env->tsp)
		goto error_free_env;
	
	/* The manager responsible for handling the other objects */
	env->manager = smlManagerNew(env->tsp, &serror);
	if (!env->manager)
		goto error_free_transport;
	smlManagerSetEventCallback(env->manager, _manager_event, env);
	
	/* The authenticator */
	env->auth = smlAuthNew(&serror);
	if (!env->auth)
		goto error_free_manager;
	smlAuthSetVerifyCallback(env->auth, _verify_user, env);
	
	if (!env->username)
		smlAuthSetEnable(env->auth, FALSE);
	
	if (!smlAuthRegister(env->auth, env->manager, &serror))
		goto error_free_auth;
	
	/* Now create the devinf handler */
	SmlDevInf *devinf = smlDevInfNew("libsyncml", SML_DEVINF_DEVTYPE_SERVER, &serror);
	if (!devinf)
		goto error_free_auth;
	
	smlDevInfSetSupportsNumberOfChanges(devinf, TRUE);
	
	env->agent = smlDevInfAgentNew(devinf, &serror);
	if (!env->agent)
		goto error_free_auth;
	
	if (!smlDevInfAgentRegister(env->agent, env->manager, &serror))
		goto error_free_auth;
	
	o = env->databases;
	for (; o; o = o->next) { 
		SmlDatabase *database = o->data;

		/* We now create the ds server hat the given location */
		SmlLocation *loc = smlLocationNew(database->url, NULL, &serror);
		if (!loc)
			goto error_free_auth;
		
		database->server = smlDsServerNew(_objtype_to_contenttype(database->objtype), loc, &serror);
		if (!database->server)
			goto error_free_auth;

		if (!smlDsServerAddSan(database->server, env->san, &serror))
			goto error_free_san;

		if (!smlDsServerRegister(database->server, env->manager, &serror))
			goto error_free_auth;
		
		smlDsServerSetConnectCallback(database->server, _ds_alert, database);
		
		/* And we also add the devinfo to the devinf agent */
		SmlDevInfDataStore *datastore = smlDevInfDataStoreNew(smlLocationGetURI(loc), &serror);
		if (!datastore)
			goto error_free_auth;
		
		if (!strcmp(_objtype_to_contenttype(database->objtype), SML_ELEMENT_TEXT_VCARD)) {
			smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_VCARD, "2.1");
			smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_VCARD, "2.1");
		} else if (!strcmp(_objtype_to_contenttype(database->objtype), SML_ELEMENT_TEXT_VCAL)) {
			smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_VCAL, "2.0");
			smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_VCAL, "2.0");
		} else if (!strcmp(_objtype_to_contenttype(database->objtype), SML_ELEMENT_TEXT_PLAIN)) {
			smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_PLAIN, "1.0");
			smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_PLAIN, "1.0");
		}

		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_TWO_WAY, TRUE);
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_SLOW_SYNC, TRUE);
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_SERVER_ALERTED_SYNC, TRUE);
		
		smlDevInfAddDataStore(devinf, datastore);
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
		goto error_free_auth;
	
	/* Initialize the Transport */
	if (!smlTransportInitialize(env->tsp, &config, &serror))
		goto error_free_auth;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;

error_free_san:
	smlNotificationFree(env->san);
	env->san = NULL;
error_free_auth:
	smlAuthFree(env->auth);
error_free_manager:
	smlManagerFree(env->manager);
error_free_transport:
	smlTransportFree(env->tsp);
error_free_env:
	finalize(env);
error:
	if (serror)
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&serror));
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

extern osync_bool syncml_obex_client_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
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
