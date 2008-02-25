#include "syncml_common.h"
#include "syncml_callbacks.h"
#include "syncml_devinf.h"
#include "syncml_ds_client.h"

/* Some informations about the SyncML protocol
 * 
 * A synchronization with SyncML which is initiated by the client
 * requires the following actions (two way sync):
 * 
 * 1. client sends sync alert
 * 2. server sends sync alert
 * 3. client sends changes
 * 4. server sends status and changes
 * 5. client sends status and map
 * 6. server sends status
 *
 * OpenSync uses the following communication pattern:
 *
 * 1. OpenSync requests all changes
 * 2. The other side sends all changes
 * 3. OpenSync performs all necessary operations
 * 4. OpenSync sends all necessary changes
 *
 * OpenSync'c behaviour is always the behaviour of an SyncML server
 * even if we use OpenSync as a SyncML client. This requires some special
 * use cases of SyncML (OpenSync as SyncML client):
 *
 * 1. OpenSync sends sync alert
 * 2. SyncML server sends sync alert
 * 3. OpenSync sends numberOfChanges 0 (no changes at client)
 * 4. SyncML server sends all of its changes
 * 5.1 OpenSync sends ok and the mapping entries of the changes
 * 5.2 OpenSync perform the necessary comparisons internally
 * 6.1 SyncML server sends ok (will be ignored/tolerated by OpenSync)
 * 6.2 OpenSync sends sync alert
 * 7. SyncML server sends sync alert
 * 8. OpenSync sends changes
 * 9. SyncML server sends changes
 * 10. OpenSync sends ok and mapping
 * 11. SyncML server sends ok
 *
 * The problem is that we do no like a complete second implementation
 * only for the client. So the solution is that we know states and events
 * in the SyncML communication. Clients and servers simply start in
 * different modes.
 *
 */

guint64 init_client_session (SmlPluginEnv *env, SmlError **error)
{
    g_assert(env->url);
    g_assert(env->identifier);
    g_assert(env->tsp);
    g_assert(env->manager);
    g_assert(env->agent);

    /* cleanup old session */
    if (env->session)
    {
        smlSessionUnref(env->session);
        env->session = NULL;
    }

    /* prepare credential */
    SmlCred *cred = NULL;
    if (env->username || env->password)
    {
        cred = smlCredNewAuth(env->authType,
                              env->username,
                              env->password,
                              error);
	if (!cred) return 0;
        osync_trace(TRACE_INTERNAL, "%s: credential initialized", __func__);
    }

    /* create random session ID - glib only supports 32 bit random numbers */
    guint64 sessionID = ((guint64) g_random_int ()) << 32 + g_random_int ();
    char *sessionString = g_strdup_printf("%llu", sessionID);
    osync_trace(TRACE_INTERNAL, "%s: new session ID is %llu (%s)",
                __func__, sessionID, sessionString);

    /* create session */
    SmlLocation *target = smlLocationNew(env->url, NULL, error);
    SmlLocation *source = smlLocationNew(env->identifier, NULL, error);
    SmlLink *link = smlLinkNew(env->tsp, NULL, error);
    env->session = smlSessionNew(SML_SESSION_TYPE_CLIENT,
                                 SML_MIMETYPE_XML,
                                 env->syncmlVersion,
                                 SML_PROTOCOL_SYNCML,
                                 target, source,
                                 sessionString, 0, error);
    safe_cfree(&sessionString);
    if (!env->session) return 0;

    /* register all the add-ons */
    if (cred)
        smlSessionSetCred(env->session, cred);
    if (!smlManagerSessionAdd(env->manager, env->session, link, error))
        return 0;
    if (!smlDevInfAgentRegisterSession(env->agent, env->manager, env->session, error))
        return 0;

    return sessionID;
}

void connect_http_client(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)data;
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	g_assert(env);
	g_assert(env->agent);
	g_assert(env->connectMutex);
	g_mutex_lock(env->connectMutex);

	/* we need to ref the context here because we signal the success later
	 * later means we signal the success in another function
	 */
	env->tryDisconnect = FALSE;
	if (ctx)
	{
		/* There is no context if this is the second connect to handle
		 * an OMA DS client - and this is an OMA DS client. OMA DS
		 * clients need a second session for batch_commit.
		 */
		env->connectCtx = ctx;
		osync_context_ref(env->connectCtx);
	}
	osync_trace(TRACE_INTERNAL, "%s: environment ready", __func__);

	/* init new session */
	guint64 sessionID = init_client_session(env, &error);
	if (sessionID == 0 || error != NULL)
		goto error;

	/* If there is not connect context then this is the second connect from
	 * from the synchronization. Two OMA DS sessions are required because
	 * OpenSync wants to have the last word but according to OMA DS specs
	 * the OMA DS servers have always the last word. So a second session is
	 * necessary two talk to the server for a second time.
	 *
	 * If we talk to a server for the second time then there is already a
	 * TCP/IP connection and the device information was already sent.
	 * Therefore the device information will only be send for the first
	 * connection attempt.
	 */
	if (ctx)
	{
		/* send the device information */
		if (!smlDevInfAgentSendDevInf(env->agent, env->session, &error))
			goto error;
		if (!smlDevInfAgentRequestDevInf(env->agent, env->session, &error))
			goto error;
		if (!smlSessionFlush(env->session, TRUE, &error))
			goto error;
		osync_trace(TRACE_INTERNAL, "%s: sent devinf", __func__);

		/* If we receive a device information from the server
		 * then we can be sure that the connect was successful.
		 * "Unfortunately" such a message is handled automatically
		 * by the devinf agent. Nevertheless the transport layer
		 * sends a CONNECT_DONE event which will be managed by
		 * _manage_event.
		 */
	}
	
	g_mutex_unlock(env->connectMutex);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
	if (ctx)
		report_error_on_context(&(env->connectCtx), &oserror, TRUE);
	else
		osync_error_unref(&oserror);
	g_mutex_unlock(env->connectMutex);
}

osync_bool syncml_http_client_parse_config(SmlPluginEnv *env, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, env, config, error);
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	env->url = NULL;
	env->proxy = NULL;
	env->cafile = NULL;
	env->authType = SML_AUTH_TYPE_UNKNOWN;
	env->username = NULL;
	env->password = NULL;
	env->syncmlVersion = SML_VERSION_12;
	env->fakeDevice = FALSE;
	env->fakeManufacturer = "NOKIA";
	env->fakeModel = "E60";
	env->fakeSoftwareVersion = "1.0";
        env->onlyLocaltime = FALSE;
        env->maxObjSize = 0;
        env->recvLimit = 0;
	
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
			if (!xmlStrcmp(cur->name, (const xmlChar *)"proxy")) {
				env->proxy = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"cafile")) {
				env->cafile = g_strdup(str);
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

			/* XXX Workaround for mobiles which only handle localtime! */
			if (!xmlStrcmp(cur->name, (const xmlChar *)"onlyLocaltime")) {
				env->onlyLocaltime = atoi(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"maxObjSize")) {
				env->maxObjSize = atoi(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"recvLimit")) {
				env->recvLimit = atoi(str);
			}

			/* this is necessary to work together with such servers like OCS */
			/* yes, we fake another vendor here ;-) */
			if (!xmlStrcmp(cur->name, (const xmlChar *)"fake_device")) {
				if (str && atoi(str))
					env->fakeDevice = TRUE;
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"syncml_version")) {
				if (!strcmp(str, "1.0")) {
					env->syncmlVersion = SML_VERSION_10;
				} else if (!strcmp(str, "1.1")) {
					env->syncmlVersion = SML_VERSION_11;
				} else if (!strcmp(str, "1.2")) {
					env->syncmlVersion = SML_VERSION_12;
				} else {
					// this is an illegal version
					osync_error_set(error, OSYNC_ERROR_GENERIC,
						 "illegal SyncML version - %s", str);
					goto error_free_doc;
				}
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"fake_manufacturer")) {
				env->fakeManufacturer = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"fake_model")) {
				env->fakeModel = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"fake_software_version")) {
				env->fakeSoftwareVersion = g_strdup(str);
			}

			xmlFree(str);
		}
		cur = cur->next;
	}

	// the default syncml:auth-basic
	if ((env->username || env->password) && env->authType == SML_AUTH_TYPE_UNKNOWN)
		env->authType = SML_AUTH_TYPE_BASIC;
	osync_trace(TRACE_INTERNAL, "%s: username with auth type %d", env->username, env->authType);

	xmlFreeDoc(doc);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_doc:
	xmlFreeDoc(doc);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

void *syncml_http_client_init(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);
	SmlError *serror = NULL;
	
	SmlPluginEnv *env = osync_try_malloc0(sizeof(SmlPluginEnv), error);
	if (!env)
		goto error;
	env->pluginInfo = info;

	const char *configdata = osync_plugin_info_get_config(info);
        osync_trace(TRACE_INTERNAL, "The config: %s", configdata);
	
	if (!syncml_http_client_parse_config(env, configdata, error))
		goto error_free_env;

	env->num = 0;	
	env->anchor_path = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
	env->devinf_path = g_strdup_printf("%s/devinf.db", osync_plugin_info_get_configdir(info));
	env->connectMutex = g_mutex_new();
	env->managerMutex = g_mutex_new();

	/* Register main sink for connect and disconnect functions */
	env->mainsink = osync_objtype_main_sink_new(error);
	if (!env->mainsink)
		goto error_free_env;

	OSyncObjTypeSinkFunctions main_functions;
	memset(&main_functions, 0, sizeof(main_functions));
	main_functions.connect = connect_http_client;
	main_functions.disconnect = disconnect;
	env->connectFunction = connect_http_client;

	osync_objtype_sink_set_functions(env->mainsink, main_functions, NULL);
	osync_plugin_info_set_main_sink(info, env->mainsink);

	if (!ds_client_init_databases(env, info, error))
		goto error_free_env;
	
	env->context = osync_plugin_info_get_loop(info); 
	
	/* The transport needed to transport the data */
	env->tsp = smlTransportNew(SML_TRANSPORT_HTTP_CLIENT, &serror);
	if (!env->tsp)
		goto error_free_env;
	
	/* The manager responsible for handling the other objects */
	env->manager = smlManagerNew(env->tsp, &serror);
	if (!env->manager)
		goto error_free_env;
	smlManagerSetEventCallback(env->manager, _manager_event, env);

	/* The authenticator is only used here to get a handler for the header.
	 * This is not really elegant nor clean but we started as server ;)
	 */
	env->auth = smlAuthNew(&serror);
	if (!env->auth)
		goto error;
	smlAuthSetEnable(env->auth, FALSE);
	if (!smlAuthRegister(env->auth, env->manager, &serror))
		goto error_free_env;
	
	///* The authenticator */
	///* disabled because the transport layer handles the authentication */
	//env->auth = smlAuthNew(&serror);
	//if (!env->auth)
	//	goto error;
	//smlAuthSetEnable(env->auth, FALSE);
	//if (!smlAuthRegister(env->auth, env->manager, &serror))
	//	goto error_free_auth;

	if (!init_env_devinf(env, SML_DEVINF_DEVTYPE_WORKSTATION, &serror))
		goto error_free_env;

	GList *o = env->databases;
	for (; o; o = o->next) { 
		SmlDatabase *database = o->data;
		osync_trace(TRACE_INTERNAL, "preparing DsServer %s", database->url);

		/* We now create the ds server at the given location */
		SmlLocation *loc = smlLocationNew(database->url, NULL, &serror);
		if (!loc)
			goto error_free_env;
		
		database->server = smlDsClientNew(
					get_database_pref_content_type(database, error),
                                	loc, loc, &serror);
		if (!database->server)
			goto error_free_env;
			
		if (!smlDsServerRegister(database->server, env->manager, &serror))
			goto error_free_env;
	
		// this is a client and not a server
		// but the callback initializes only database->session (DsSession)
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

   	env->isConnected = FALSE;
	
	/* Run the manager */
	if (!smlManagerStart(env->manager, &serror))
		goto error_free_env;
	
	/* Initialize the Transport */
	if (!smlTransportSetConfigOption(env->tsp, "URL", env->url, &serror) ||
            !smlTransportSetConfigOption(env->tsp, "SSL_CA_FILE", env->cafile, &serror) ||
            !smlTransportSetConfigOption(env->tsp, "PROXY", env->proxy, &serror) ||
            !smlTransportSetConfigOption(env->tsp, "USERNAME", env->username, &serror) ||
            !smlTransportSetConfigOption(env->tsp, "PASSWORD", env->password, &serror) ||
	    !smlTransportInitialize(env->tsp, &serror))
		goto error_free_env;

	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;

error_free_env:
	if (env->auth)
		smlAuthFree(env->auth);
	if (env->manager)
		smlManagerFree(env->manager);
	if (env->tsp)
		smlTransportFree(env->tsp);
	safe_free((gpointer *)&env);
error:
	if (serror)
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&serror));
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void _publish_osync_error(void *publicError, OSyncError *error)
{
    osync_trace(TRACE_ENTRY, "%s", __func__);
    if (error)
        osync_trace(TRACE_INTERNAL, "%s - failed", __func__);
    else
        osync_trace(TRACE_INTERNAL, "%s - succeeded", __func__);
    OSyncError **destError = publicError;
    *destError = error;
    osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool syncml_http_client_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);
        
        SmlPluginEnv *env = (SmlPluginEnv *)data;

	// first check if the server is available
	if (!env->session)
	{
		OSyncContext *ctx = osync_context_new(error);
		osync_context_set_callback(ctx, &_publish_osync_error, error);
        	osync_trace(TRACE_INTERNAL, "%s- create a fresh connection with a new context (%p)", __func__, ctx);
		connect_http_client(data, info, ctx);
	}

        GList *o = env->databases;
        for (; o; o = o->next) {
                SmlDatabase *database = o->data;
                osync_objtype_sink_set_available(database->sink, TRUE);

		int num = osync_objtype_sink_num_objformats(database->sink);
		osync_trace(TRACE_INTERNAL, "%s: register %i sink objformats", __func__, num);
		int i;
		for (i=0; i<num; i++)
		{
			osync_trace(TRACE_INTERNAL, "%s: sink objformat is %s", __func__,
					osync_objtype_sink_nth_objformat(database->sink, i));
		}
        }
        
        OSyncVersion *version = osync_version_new(error);
        osync_version_set_plugin(version, "syncml-http-client");
        //osync_version_set_modelversion(version, "version");
        //osync_version_set_firmwareversion(version, "firmwareversion");
        //osync_version_set_softwareversion(version, "softwareversion");
        //osync_version_set_hardwareversion(version, "hardwareversion");
        osync_plugin_info_set_version(info, version);
        osync_version_unref(version);

	/* let's wait for the device info of the server */
	while (!smlDevInfAgentGetDevInf(env->agent) && !*error)
	{
		unsigned int sleeping = 5;
		osync_trace(TRACE_INTERNAL,
			"%s: SyncML HTTP client waiting for device info (%d seconds) ...",
			__func__, sleeping);
		sleep(sleeping);
	}

	/* check for error */
	if (*error != NULL)
	{
		osync_trace(TRACE_INTERNAL, "%s - connect failed in some way", __func__);
	}

	/* print the device information */
	SmlDevInf *devinf = smlDevInfAgentGetDevInf(env->agent);
	unsigned int stores = smlDevInfNumDataStores(devinf);
	unsigned int i;
	for (i=0; i < stores; i++)
	{
		const SmlDevInfDataStore *datastore = smlDevInfGetNthDataStore(devinf, i);
		char *version;
		char *contentType;
		smlDevInfDataStoreGetRxPref(datastore, &contentType, &version);
		printf("\t%s (%s %s)\n",
			smlDevInfDataStoreGetSourceRef(datastore),
			contentType, version);
	}

	/* disconnect from the syncml server */
	OSyncContext *ctx = osync_context_new(error);
	osync_context_set_callback(ctx, &_publish_osync_error, error);
	osync_trace(TRACE_INTERNAL, "%s- close the connection with a new context (%p)", __func__, ctx);
	disconnect(data, info, ctx);

        osync_trace(TRACE_EXIT, "%s", __func__);
        return TRUE;
}

