#include "syncml_common.h"

static void connect_http_client(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	// WARNING: this is a synchronous function !!!
	// if the function returns then the client must be connected

	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)data;
	SmlError *error = NULL;
	OSyncError *oserror = NULL;

	if (env->isConnected) {
		osync_context_report_success(ctx);
		osync_trace(TRACE_EXIT, "%s", __func__);
		return;
	} else {
		if (!smlTransportHttpClientIsConnected(env->tsp, TRUE, &error))
			goto error;
	}

	env->tryDisconnect = FALSE;
	env->connectCtx = ctx;
	/* Create the alert for the remote device */
	if (!env->identifier)
		env->identifier = g_strdup("OpenSync SyncML Plugin");

	/* This ref counting is needed to avoid a segfault. TODO: review if this is really needed.
	   To reproduce the segfault - just remove the osync_context_ref() call in the next line. */ 
	osync_context_ref(env->connectCtx);
	osync_trace(TRACE_INTERNAL, "%s: environment ready", __func__);

	/* prepare session and notification */
	SmlDatabase *db = env->databases->data;
	SmlLocation *loc = smlLocationNew(db->url, NULL, &error);
	SmlLink *link = smlLinkNew(env->tsp, NULL, &error);
	SmlSession *session = smlSessionNew(SML_SESSION_TYPE_CLIENT,
                                            SML_MIMETYPE_XML,
                                            SML_VERSION_12,
                                            SML_PROTOCOL_SYNCML,
                                            loc, loc,
					    NULL, 0, &error);
	// FIXME: MD5 is more safe but the most systems does not support MD5
	// FIXME: auth is available in client config !!!
	if (env->username || env->password)
	{
		SmlCred *cred = smlCredNewAuth(SML_AUTH_TYPE_BASIC,
						 env->username,
						 env->password,
						 loc, &error);
		smlSessionSetCredential(session, cred);
	}
	if (!smlManagerSessionAdd(env->manager, session, link, &error))
		goto error_free_san;
	osync_trace(TRACE_INTERNAL, "%s: session ready", __func__);
	env->san = smlNotificationNew(SML_SAN_VERSION_11,
                                      SML_SAN_UIMODE_UNSPECIFIED,
                                      SML_SAN_INITIATOR_USER,
                                      atoi(smlSessionGetSessionID(session)),
                                      env->identifier, "/", SML_MIMETYPE_XML, &error);
	if (!env->san)
		goto error;
	gboolean sanUsed = FALSE;
	osync_trace(TRACE_INTERNAL, "%s: notification prepared", __func__);

	/* scan the available DsSmlSessions */
	GList *o = env->databases;
	for (; o; o = o->next) {
		SmlDatabase *database = o->data;
		if (database->session)
		{
			smlDsSessionGetAlert(database->session, _recv_alert, database);
		}
		if (!smlDsServerAddSan(database->server, env->san, &error))
			goto error_free_san;
		sanUsed = TRUE;
	}
	osync_trace(TRACE_INTERNAL, "%s: notifications added", __func__);
	if (sanUsed && !smlNotificationSend(env->san, env->tsp, &error))
		goto error_free_san;
	smlNotificationFree(env->san);
	env->san = NULL;
	osync_trace(TRACE_INTERNAL, "%s: notifications send", __func__);

	// do not move this to an upper location or
	// otherwise errors are not detected correctly
	env->isConnected = TRUE;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
error_free_san:
	smlNotificationFree(env->san);
	env->san = NULL;
	smlManagerSessionRemove(env->manager, session);
	// unref is already executed by the manager
	// smlSessionUnref(session);
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_context_report_osyncerror(ctx, oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

static osync_bool syncml_http_client_parse_config(SmlPluginEnv *env, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, env, config, error);
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	env->url = NULL;
	env->authType = SML_AUTH_TYPE_UNKNOWN;
	env->username = NULL;
	env->password = NULL;
	
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
			if (!xmlStrcmp(cur->name, (const xmlChar *)"auth")) {
				if (!strcmp(str, "BASIC")) {
					env->authType = SML_AUTH_TYPE_BASIC;
				} else if (!strcmp(str, "MD5")) {
					env->authType = SML_AUTH_TYPE_MD5;
				} else if (!strcmp(str, "NONE")) {
					env->authType = SML_AUTH_TYPE_UNKNOWN;
				} else {
					// this is an illegal keyword
					osync_error_set(error, OSYNC_ERROR_GENERIC,
						 "illegal authentication type - %s", str);
					goto error_free_doc;
				}
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"url")) {
				env->url = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"username")) {
				env->username = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"password")) {
				env->password = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"database")) {
				if (!syncml_config_parse_database(env, cur->xmlChildrenNode, error))
					goto error_free_doc;
			}

			xmlFree(str);
		}
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

extern void *syncml_http_client_init(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);
	SmlError *serror = NULL;
	
	SmlPluginEnv *env = osync_try_malloc0(sizeof(SmlPluginEnv), error);
	if (!env)
		goto error;

	const char *configdata = osync_plugin_info_get_config(info);
        osync_trace(TRACE_INTERNAL, "The config: %s", configdata);
	
	if (!syncml_http_client_parse_config(env, configdata, error))
		goto error_free_transport;

	env->num = 0;	
	env->anchor_path = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));

	GList *o = env->databases;
	for (; o; o = o->next) {
                SmlDatabase *database = o->data;
		database->gotChanges = FALSE;
		database->finalChanges = FALSE;

                OSyncObjTypeSink *sink = osync_objtype_sink_new(database->objtype, error);
                if (!sink)
                        goto error_free_env;
                
                database->sink = sink;
                
		// TODO:... in case of maemo ("plain text") we have to set "memo"...
                osync_objtype_sink_add_objformat(sink, "plain");
                
                OSyncObjTypeSinkFunctions functions;
                memset(&functions, 0, sizeof(functions));
                functions.connect = connect_http_client;
                functions.disconnect = disconnect;
                functions.get_changes = get_changeinfo;
                functions.sync_done = sync_done;
		functions.batch_commit = batch_commit;
                
                osync_objtype_sink_set_functions(sink, functions, database);
                osync_plugin_info_add_objtype(info, sink);
	}
	
	env->context = osync_plugin_info_get_loop(info); 
	
	/* The transport needed to transport the data */
	env->tsp = smlTransportNew(SML_TRANSPORT_HTTP_CLIENT, &serror);
	if (!env->tsp)
		goto error;
	
	/* The manager responsible for handling the other objects */
	env->manager = smlManagerNew(env->tsp, &serror);
	if (!env->manager)
		goto error;
	smlManagerSetEventCallback(env->manager, _manager_event, env);
	
	/* The authenticator */
	/* disabled because the transport layer handles the authentication */
	env->auth = smlAuthNew(&serror);
	if (!env->auth)
		goto error;
	smlAuthSetEnable(env->auth, FALSE);
	if (!smlAuthRegister(env->auth, env->manager, &serror))
		goto error_free_auth;
	
	/* Now create the devinf handler */
	SmlDevInf *devinf = smlDevInfNew("libsyncml", SML_DEVINF_DEVTYPE_WORKSTATION, &serror);
	if (!devinf)
		goto error_free_manager;
	
	smlDevInfSetSupportsNumberOfChanges(devinf, TRUE);
	
	env->agent = smlDevInfAgentNew(devinf, &serror);
	if (!env->agent)
		goto error_free_manager;
	
	if (!smlDevInfAgentRegister(env->agent, env->manager, &serror))
		goto error_free_manager;

	o = env->databases;
	for (; o; o = o->next) { 
		SmlDatabase *database = o->data;

		/* We now create the ds server at the given location */
		SmlLocation *loc = smlLocationNew(database->url, NULL, &serror);
		if (!loc)
			goto error;
		
		database->server = smlDsClientNew(_objtype_to_contenttype(database->objtype), loc, loc, &serror);
		if (!database->server)
			goto error;
			
		if (!smlDsServerRegister(database->server, env->manager, &serror))
			goto error;
	
		// this is a client and not a server	
		// smlDsServerSetConnectCallback(database->server, _ds_alert, database);
		
		/* And we also add the devinfo to the devinf agent */
		SmlDevInfDataStore *datastore = smlDevInfDataStoreNew(smlLocationGetURI(loc), &serror);
		if (!datastore)
			goto error;
		
		smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_VCARD, "2.1");
		smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_VCARD, "2.1");
		
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

	SmlTransportHttpClientConfig config;
	config.url = env->url;
	// FIXME: actually we do not parse the proxy variable
	// FIXME: libsyncml supports proxy (only the parser is to stupid)
	config.proxy = NULL;
	config.username = env->username;
	config.password = env->password;
   	env->isConnected = FALSE;
	
	/* Run the manager */
	if (!smlManagerStart(env->manager, &serror))
		goto error;
	
	/* Initialize the Transport */
	if (!smlTransportInitialize(env->tsp, &config, &serror))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;

error_free_auth:
	smlAuthFree(env->auth);
error_free_manager:
	smlManagerFree(env->manager);
error_free_transport:
	smlTransportFree(env->tsp);
error_free_env:
	g_free(env);
error:
	if (serror)
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&serror));
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

extern osync_bool syncml_http_client_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);
        
        SmlPluginEnv *env = (SmlPluginEnv *)data;
        GList *o = env->databases;
        for (; o; o = o->next) {
                SmlDatabase *database = o->data;
                osync_objtype_sink_set_available(database->sink, TRUE);
        }
        
        OSyncVersion *version = osync_version_new(error);
        osync_version_set_plugin(version, "syncml-http-client");
        //osync_version_set_modelversion(version, "version");
        //osync_version_set_firmwareversion(version, "firmwareversion");
        //osync_version_set_softwareversion(version, "softwareversion");
        //osync_version_set_hardwareversion(version, "hardwareversion");
        osync_plugin_info_set_version(info, version);
        osync_version_unref(version);

        osync_trace(TRACE_EXIT, "%s", __func__);
        return TRUE;
}

