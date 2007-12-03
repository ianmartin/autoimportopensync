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
#include "syncml_devinf.h"

static void connect_http_server(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)data;

	env->tryDisconnect = FALSE;

	/* For the obex client, we will store the context at this point since
	 * we can only answer it as soon as the device returned an answer to our san */
	env->connectCtx = ctx;

	/* This ref counting is needed to avoid a segfault. TODO: review if this is really needed.
	   To reproduce the segfault - just remove the osync_context_ref() call in the next line. */ 
	osync_context_ref(env->connectCtx);

	/* For the http server we can report success right away since we know
	 * that we already received an alert (otherwise we could not have triggered
	 * the synchronization) */
	GList *o = env->databases;
	for (; o; o = o->next) {
		SmlDatabase *database = o->data;

		if (database->session)
			smlDsSessionGetAlert(database->session, _recv_alert, database);
	}
	
	/* If we already received the final, we just report success. otherwise
	 * we let the final report success */
	if (env->gotFinal)
		osync_context_report_success(ctx);
	else
		env->connectCtx = ctx;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static osync_bool syncml_http_server_parse_config(SmlPluginEnv *env, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, env, config, error);
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	env->port = 8080;
	env->url = NULL;
	env->username = NULL;
	env->recvLimit = 0;
	env->password = NULL;
	env->useStringtable = TRUE;
	env->onlyReplace = FALSE;
	
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
			if (!xmlStrcmp(cur->name, (const xmlChar *)"port")) {
				env->port = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"url")) {
				env->url = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"username")) {
				env->username = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"recvLimit")) {
				env->recvLimit = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"password")) {
				env->password = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"usestringtable")) {
				env->useStringtable = atoi(str);
			}

			/* XXX Workaround for mobiles which only handle localtime! */
			if (!xmlStrcmp(cur->name, (const xmlChar *)"onlyLocaltime")) {
				env->onlyLocaltime = atoi(str);
			}
		
			if (!xmlStrcmp(cur->name, (const xmlChar *)"onlyreplace")) {
				env->onlyReplace = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"maxObjSize")) {
				env->maxObjSize = atoi(str);
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

extern void *syncml_http_server_init(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);
	SmlError *serror = NULL;
	
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	SmlPluginEnv *env = osync_try_malloc0(sizeof(SmlPluginEnv), error);
	if (!env)
		goto error;

	const char *configdata = osync_plugin_info_get_config(info);
        osync_trace(TRACE_INTERNAL, "The config: %s", configdata);
		
	if (!syncml_http_server_parse_config(env, configdata, error))
		goto error_free_env;

	env->num = 0;	

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
                functions.connect = connect_http_server;
                functions.disconnect = disconnect;
                functions.get_changes = get_changeinfo;
                functions.sync_done = sync_done;
		functions.batch_commit = batch_commit;
                
                osync_objtype_sink_set_functions(sink, functions, database);
                database->sink = sink;
                osync_plugin_info_add_objtype(info, sink);
	}
	
	env->context = osync_plugin_info_get_loop(info); 
	
	/* The transport needed to transport the data */
	env->tsp = smlTransportNew(SML_TRANSPORT_HTTP_SERVER, &serror);
	if (!env->tsp)
		goto error;
	
	/* The manager responsible for handling the other objects */
	env->manager = smlManagerNew(env->tsp, &serror);
	if (!env->manager)
		goto error;
	smlManagerSetEventCallback(env->manager, _manager_event, env);
	
	/* The authenticator */
	env->auth = smlAuthNew(&serror);
	if (!env->auth)
		goto error;
	smlAuthSetVerifyCallback(env->auth, _verify_user, env);
	
	if (!env->username)
		smlAuthSetEnable(env->auth, FALSE);
	
	if (!smlAuthRegister(env->auth, env->manager, &serror))
		goto error_free_auth;
	
	
	/* Now create the devinf handler */
	SmlDevInf *devinf = smlDevInfNew("libsyncml", SML_DEVINF_DEVTYPE_SERVER, &serror);
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
		
		database->server = smlDsServerNew(
					get_database_pref_content_type(database, error),
                                	loc, &serror);
		if (!database->server)
			goto error;
			
		if (!smlDsServerRegister(database->server, env->manager, &serror))
			goto error;
		
		smlDsServerSetConnectCallback(database->server, _ds_alert, database);
		
		/* And we also add the devinfo to the devinf agent */
		if (!add_dev_inf_datastore(devinf, database, error))
			goto error_free_auth;
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

	SmlTransportHttpServerConfig config;
	config.port = env->port;
	config.url = env->url;
	config.interface = NULL;
	config.ssl_key = NULL;
	config.ssl_crt = NULL;
	
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

extern osync_bool syncml_http_server_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);
        
        SmlPluginEnv *env = (SmlPluginEnv *)data;
        GList *o = env->databases;
        for (; o; o = o->next) {
                SmlDatabase *database = o->data;
                osync_objtype_sink_set_available(database->sink, TRUE);
        }
        
        OSyncVersion *version = osync_version_new(error);
        osync_version_set_plugin(version, "syncml-http-server");
        //osync_version_set_modelversion(version, "version");
        //osync_version_set_firmwareversion(version, "firmwareversion");
        //osync_version_set_softwareversion(version, "softwareversion");
        //osync_version_set_hardwareversion(version, "hardwareversion");
        osync_plugin_info_set_version(info, version);
        osync_version_unref(version);

        osync_trace(TRACE_EXIT, "%s", __func__);
        return TRUE;
}

