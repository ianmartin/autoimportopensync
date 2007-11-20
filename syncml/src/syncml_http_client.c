#include "syncml_common.h"

static SmlBool _recv_server_alert(SmlDsSession *dsession, SmlAlertType type, const char *last, const char *next, void *userdata)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %s, %p)", __func__, dsession, type, last, next, userdata);
        SmlDatabase *database = (SmlDatabase*) userdata;
        SmlPluginEnv *env = database->env;
        SmlBool ret = TRUE;
	SmlError *error = NULL;
	OSyncError *oserror = NULL;

	// this is for client which receive alerts from servers
	// _recv_alert is for servers only
       	char *key = g_strdup_printf("remoteanchor%s", smlDsSessionGetLocation(dsession));

	if ((!last || !osync_anchor_compare(env->anchor_path, key, last)) && type == SML_ALERT_TWO_WAY)
		ret = FALSE;

	osync_bool ans = osync_objtype_sink_get_slowsync(database->sink);
	if (ans && type != SML_ALERT_SLOW_SYNC)
		ret = FALSE;

	if (!ret || type != SML_ALERT_TWO_WAY)
		osync_objtype_sink_set_slowsync(database->sink, TRUE);

       osync_anchor_update(env->anchor_path, key, next);
       	g_free(key);

	// this is a client
	// so we have to initiate the sync
	if (!smlDsSessionSendSync(database->session, 0, _recv_sync_reply, database, &error))
		goto error;
	if (!smlDsSessionCloseSync(database->session, &error))
		goto error;
	if (!flush_session_for_all_databases(env, &error))
		goto error;

        osync_trace(TRACE_EXIT, "%s: %i", __func__, TRUE);
        return ret;
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	// osync_context_report_osyncerror(env->connectCtx, oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

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

	/* This ref counting is needed to avoid a segfault. TODO: review if this is really needed.
	   To reproduce the segfault - just remove the osync_context_ref() call in the next line. */ 
	/* the context can be null if the function is called from discover */
	if (env->connectCtx)
		osync_context_ref(env->connectCtx);
	osync_trace(TRACE_INTERNAL, "%s: environment ready", __func__);

	/* prepare credential */
	SmlCred *cred = NULL;
	if (env->username || env->password)
	{
		cred = smlCredNewAuth(env->authType,
				 env->username,
				 env->password,
				 &error);
		osync_trace(TRACE_INTERNAL, "%s: credential initialized", __func__);
	}

	/* prepare session */
	SmlDatabase *db = env->databases->data;
	SmlLocation *target = smlLocationNew(env->url, NULL, &error);
	SmlLocation *source = smlLocationNew(env->identifier, NULL, &error);
	SmlLink *link = smlLinkNew(env->tsp, NULL, &error);
	env->session = smlSessionNew(SML_SESSION_TYPE_CLIENT,
                                    SML_MIMETYPE_XML,
                                    env->syncmlVersion,
                                    SML_PROTOCOL_SYNCML,
                                    target, source,
	 			    "1", // this is the first session
				    0, &error); 
	if (cred)
		smlSessionSetCred(env->session, cred);
	if (!smlManagerSessionAdd(env->manager, env->session, link, &error))
		goto error;

	/* send the device information */
	if (!smlDevInfAgentRegisterSession(env->agent, env->manager, env->session, &error))
		goto error;
	smlDevInfAgentSendDevInf(env->agent, env->session, &error);
	smlDevInfAgentRequestDevInf(env->agent, env->session, &error);
	smlSessionFlush(env->session, TRUE, &error);
	osync_trace(TRACE_INTERNAL, "%s: session ready", __func__);

	/* I think we can remove notification code because */
	/* this is a client and not a server (SAN - server alerted notification :) */
	/* DO NOT ACTIVATE THE NOTIFICATION CODE */
	/* The notification damages the devinf handling */
	/* looks like a problem with the status or results handling */

	/* send an syncml alert notification */
	//env->san = smlNotificationNew(SML_SAN_VERSION_11,
        //                              SML_SAN_UIMODE_UNSPECIFIED,
        //                              SML_SAN_INITIATOR_USER,
        //                              2, // sessionID 1 is the normal session
        //                              env->identifier, "/", SML_MIMETYPE_XML, &error);
	//if (!env->san)
	//	goto error;
	//osync_trace(TRACE_INTERNAL, "%s: notification created", __func__);
	//if (cred)
	//	smlNotificationSetCred(env->san, cred);
	//smlNotificationSetManager(env->san, env->manager);
	//gboolean sanUsed = FALSE;
	//osync_trace(TRACE_INTERNAL, "%s: notification prepared", __func__);

	GList *o = env->databases;
	for (; o; o = o->next) {
		SmlDatabase *database = o->data;
		if (database->session)
		{
			smlDsSessionGetAlert(database->session, _recv_alert, database);
		}
		//if (!smlDsServerAddSan(database->server, env->san, &error))
		//	goto error_free_san;
		//sanUsed = TRUE;
	}
	//osync_trace(TRACE_INTERNAL, "%s: notifications added", __func__);
	//if (sanUsed && !smlNotificationSend(env->san, env->tsp, &error))
	//	goto error_free_san;
	//smlNotificationFree(env->san);
	//env->san = NULL;
	//osync_trace(TRACE_INTERNAL, "%s: notifications send", __func__);

	// do not move this to an upper location or
	// otherwise errors are not detected correctly
	env->isConnected = TRUE;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
error_free_san:
	//smlNotificationFree(env->san);
	//env->san = NULL;
	// smlManagerSessionRemove(env->manager, session);
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
	env->syncmlVersion = SML_VERSION_12;
	env->fakeDevice = FALSE;
	env->fakeManufacturer = "NOKIA";
	env->fakeModel = "E60";
	env->fakeSoftwareVersion = "1.0";
        env->onlyLocaltime = FALSE;
        env->maxObjSize = 0;
        env->recvLimit = 0;
	char *identifier = "OpenSync SyncML Plugin";
 	env->identifier = g_base64_encode(identifier, strlen(identifier));
	
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

extern void syncml_http_client_get_changeinfo(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
        SmlPluginEnv *env = (SmlPluginEnv *)data;

        OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
        SmlDatabase *database = osync_objtype_sink_get_userdata(sink);

        database->getChangesCtx = ctx;
        osync_context_ref(database->getChangesCtx);

        SmlError *error = NULL;
        OSyncError *oserror = NULL;

	/* let's wait for the device info of the server */
	while (!smlDevInfAgentGetDevInf(env->agent) && !smlSessionCheck(env->session))
	{
		printf("SyncML HTTP client is waiting for server's device info.\n");
		sleep(10);
	}
	SmlDevInf *devinf = smlDevInfAgentGetDevInf(env->agent);
	unsigned int stores = smlDevInfNumDataStores(devinf);
	unsigned int i;
	SmlBool supportedDatabase = FALSE;
	for (i=0; i < stores; i++)
	{
		SmlDevInfDataStore *datastore = smlDevInfGetNthDataStore(devinf, i);
		// if (!strcmp(smlDevInfDataStoreGetSourceRef(datastore), database->objtype))
		if (!strcmp(smlDevInfDataStoreGetSourceRef(datastore),
			    database->url))
			supportedDatabase = TRUE;
	}
	if (!supportedDatabase)
	{
		printf("SyncML HTTP client uses unsupported objtype (%s) ...\n", database->objtype);
		for (i=0; i < stores; i++)
		{
			SmlDevInfDataStore *datastore = smlDevInfGetNthDataStore(devinf, i);
			printf("\t%s (supported)\n", smlDevInfDataStoreGetSourceRef(datastore));
		}
	} else {
		printf("SyncML HTTP client uses supported objtype (%s: %s).\n",
			 database->objtype, database->url);
	}

        if (!database->session &&
            osync_objtype_sink_get_slowsync(database->sink))
        {
            // if there is no DsSession and we have a slowsync
            // then we MUST initiate a sync which forces us
            // to create a new DsSession
            // const char *last = "000000T000000Z"; // perhaps NULL is better
            const char *last = "0"; // perhaps NULL is better
            char *next = malloc(sizeof(char)*17);
	    time_t htime = time(NULL);
	    if (env->onlyLocaltime)
	        strftime(next, 17, "%Y%m%dT%H%M%SZ", localtime(&htime));
	    else
	        strftime(next, 17, "%Y%m%dT%H%M%SZ", gmtime(&htime));
            database->session = smlDsServerSendAlert(
					database->server,
					env->session,
					SML_ALERT_SLOW_SYNC,
					NULL, next,
					_recv_alert_reply, database,
					&error);
		if (!database->session)
			goto error;
	}

	if (database->session)
	{
		// this is for server's alerts
		smlDsSessionGetAlert(database->session, _recv_server_alert, database);
		smlDsSessionGetEvent(database->session, _ds_event, database);
		smlDsSessionGetSync(database->session, _recv_sync, database);
		smlDsSessionGetChanges(database->session, _recv_change, database);
	}

	if (!flush_session_for_all_databases(env, &error))
		goto error;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_context_report_osyncerror(ctx, oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
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

		if (!init_objformat(info, database, error))
			goto error_free_env;
                
                OSyncObjTypeSinkFunctions functions;
                memset(&functions, 0, sizeof(functions));
                functions.connect = connect_http_client;
                functions.disconnect = disconnect;
                functions.get_changes = syncml_http_client_get_changeinfo;
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
	SmlDevInf *devinf;

	/* basic devinf setup incl. faling the device */
	if (env->fakeDevice)
	{
		osync_trace(TRACE_INTERNAL, "%s: faking devinf", __func__);
		devinf = smlDevInfNew(env->identifier, SML_DEVINF_DEVTYPE_SMARTPHONE, &serror);
		smlDevInfSetManufacturer(devinf, env->fakeManufacturer);
		smlDevInfSetModel(devinf, env->fakeModel);
		smlDevInfSetSoftwareVersion(devinf, env->fakeSoftwareVersion);
	} else {
		osync_trace(TRACE_INTERNAL, "%s: not faking devinf", __func__);
		devinf = smlDevInfNew(env->identifier, SML_DEVINF_DEVTYPE_WORKSTATION, &serror);
	}
	if (!devinf)
		goto error_free_manager;

	/* activation of number of changes support if requested */
	smlDevInfSetSupportsNumberOfChanges(devinf, TRUE);
	if (env->recvLimit && env->maxObjSize)
		smlDevInfSetSupportsLargeObjs(devinf, TRUE);
	else
		smlDevInfSetSupportsLargeObjs(devinf, TRUE);
	
	env->agent = smlDevInfAgentNew(devinf, &serror);
	if (!env->agent)
		goto error_free_manager;

	/* capability specification */
	/* FIXME: this is really poor because of devinf interface of libsyncml */
	/* FIXME: libsyncml only supports SyncML 1.0 and 1.1 !!! DevInf Spec */
	/* add VCard 2.1 - e.g. vcard21 */
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_CTTYPE, "text/x-vcard");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "BEGIN");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VCARD");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "END");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VCARD");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "VERSION");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "2.1");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "REV");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "N");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "TITLE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "CATEGORIES");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "CLASS");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "ORG");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "EMAIL");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "URL");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "TEL");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PARAMNAME, "CELL");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PARAMNAME, "HOME");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PARAMNAME, "WORK");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PARAMNAME, "FAX");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PARAMNAME, "MODEM");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PARAMNAME, "VOICE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "ADR");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PARAMNAME, "HOME");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PARAMNAME, "WORK");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "BDAY");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "NOTE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "PHOTO");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PARAMNAME, "TYPE");
	/* add VCard 3.0 - e.g. vcard30 */
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_CTTYPE, "text/vcard");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "BEGIN");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VCARD");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "END");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VCARD");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "VERSION");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "3.0");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "REV");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "N");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "TITLE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "CATEGORIES");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "CLASS");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "ORG");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "URL");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "EMAIL");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "URL");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "TEL");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PARAMNAME, "TYPE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "CELL");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "HOME");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "WORK");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "FAX");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "MODEM");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VOICE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "ADR");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PARAMNAME, "TYPE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "HOME");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "WORK");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "BDAY");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "NOTE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "PHOTO");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PARAMNAME, "TYPE");
	/* Oracle collaboration Suite uses the contant type to distinguish */
	/* the versions of VCalendar                                       */
	/* text/x-vcalendar --> VERSION 1.0                                */
	/* text/calendar    --> VERSION 2.0                                */
	/* SyncML specifies VERSION 1.0 as VCalendar 2.0 and VERSION 2.0   */
	/* as VCalendar 3.0                                                */
	/* So be VERY VERY CAREFUL if you change something here.           */
	/* VCalendar 2.0 - e.g. vevent10 (NO TYPO) */
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_CTTYPE, "text/vcalendar");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "BEGIN");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VCALENDAR");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VEVENT");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VTODO");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "END");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VCALENDAR");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VEVENT");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VTODO");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "VERSION");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "1.0");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "TZ");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "LAST-MODIFIED");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "DCREATED");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "CATEGORIES");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "CLASS");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "SUMMARY");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "DESCRIPTION");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "LOCATION");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "DTSTART");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "DTEND");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "ATTENDEE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "RRULE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "EXDATE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "AALARAM");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "DALARM");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "DUE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "PRIORITY");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "STATUS");
	/* VCalendar 3.0 - e.g. vevent20 (NO TYPO) */
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_CTTYPE, "text/calendar");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "BEGIN");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VCALENDAR");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VEVENT");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VTODO");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "END");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VCALENDAR");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VEVENT");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "VTODO");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "VERSION");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_VALENUM, "2.0");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "TZ");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "LAST-MODIFIED");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "DCREATED");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "CATEGORIES");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "CLASS");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "SUMMARY");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "DESCRIPTION");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "LOCATION");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "DTSTART");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "DTEND");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "ATTENDEE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "RRULE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "EXDATE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "AALARAM");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "DALARM");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "DUE");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "PRIORITY");
	smlDevInfAddCTCap(devinf, SML_DEVINF_CTCAP_PROPNAME, "STATUS");

	// this is important because the tranport sends it during init
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
		// but the callback initializes only database->session (DsSession)
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

	if (!env->session)
	{
		connect_http_client(data, info, NULL);
	}

	/* let's wait for the device info of the server */
	while (!smlDevInfAgentGetDevInf(env->agent))
	{
		unsigned int sleeping = 5;
		printf("SyncML HTTP client is waiting for server's device info (%d seconds).\n", sleeping);
		sleep(sleeping);
	}
	SmlDevInf *devinf = smlDevInfAgentGetDevInf(env->agent);
	unsigned int stores = smlDevInfNumDataStores(devinf);
	unsigned int i;
	for (i=0; i < stores; i++)
	{
		SmlDevInfDataStore *datastore = smlDevInfGetNthDataStore(devinf, i);
		char *version;
		char *contentType;
		smlDevInfDataStoreGetRxPref(datastore, &contentType, &version);
		printf("\t%s (%s %s)\n",
			smlDevInfDataStoreGetSourceRef(datastore),
			contentType, version);
	}

        osync_trace(TRACE_EXIT, "%s", __func__);
        return TRUE;
}

