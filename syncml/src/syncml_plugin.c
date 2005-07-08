/*
 * syncml plugin - A syncml plugin for OpenSync
 * Copyright (C) 2005  Armin Bauer <armin.bauer@opensync.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "syncml_plugin.h"


void _send_success(void *userData, SmlTransportResult result, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, userData, result, error);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

void _recv_final(SmlSession *session, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, session, userdata);
	
	SmlError *error = NULL;
	smlSessionEndPackage(session, _send_success, NULL, &error);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

SmlBool _new_session(SmlTransport *tsp, SmlSession *session, void *userdata, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, tsp, session, userdata, error);
	SmlPluginEnv *env = (SmlPluginEnv *)userdata;
	
	if (!smlDsServerRegister(env->contactserver, session, error))
		goto error;
	
	smlSessionRegisterFinalHandler(session, _recv_final, userdata);
		
	if (!smlAuthRegister(env->auth, session, error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, smlErrorPrint(error));
	return FALSE;
}

osync_bool syncml_http_server_parse_config(SmlTransportHttpServerConfig *env, const char *config, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p)", __func__, env, config, size, error);
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	env->port = 8080;

	if (!(doc = xmlParseMemory(config, size))) {
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
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"port")) {
				env->port = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"url")) {
				env->url = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"interface")) {
				env->interface = g_strdup(str);
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

static void *syncml_http_server_init(OSyncMember *member, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);
	char *configdata = NULL;
	int configsize = 0;
	SmlError *serror = NULL;
	SmlTransportHttpServerConfig serverConfig;
	
	SmlPluginEnv *env = osync_try_malloc0(sizeof(SmlPluginEnv), error);
	if (!env)
		goto error;
		
	if (!osync_member_get_config(member, &configdata, &configsize, error))
		goto error_free_env;
	
	env->tsp = smlTransportNew(SML_TRANSPORT_HTTP_SERVER, &serror);
	if (!env->tsp)
		goto error_free_config;
		
	smlTransportSetNewSessionCallback(env->tsp, _new_session, NULL);
	
	if (!syncml_http_server_parse_config(&serverConfig, configdata, configsize, error))
		goto error_free_transport;
	
	env->context = osync_member_get_loop(member);
	env->member = member;
	
	if (!smlTransportInitialize(env->tsp, &serverConfig, env->context, &serror))
		goto error_free_transport;
	
	SmlLocation *loc = smlLocationNew("/vcards", NULL, &serror);
	
	env->contactserver = smlDsServerNew(loc, &serror);
	if (!env->contactserver)
		goto error_free_loc;
	
	env->auth = smlAuthNew(&serror);
	if (!env->auth)
		goto error_free_contactserver;

	
	g_free(configdata);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;
error_free_contactserver:
	smlDsServerFree(env->contactserver);
error_free_loc:
	smlLocationFree(loc);
error_free_transport:
	smlTransportFree(env->tsp);
error_free_config:
	g_free(configdata);
error_free_env:
	g_free(env);
error:
	if (serror)
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&serror));
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);


	//you can also use the anchor system to detect a device reset
	//or some parameter change here. Check the docs to see how it works
	char *lanchor = NULL;
	//Now you get the last stored anchor from the device
	if (!osync_anchor_compare(env->member, "lanchor", lanchor))
		osync_member_set_slow_sync(env->member, "contact", TRUE);
		
	osync_context_report_success(ctx);
}

static void get_changeinfo(OSyncContext *ctx)
{
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	
	if (osync_member_get_slow_sync(env->member, "contact")) {
		;
	}

	/*
	 * Now you can get the changes.
	 * Loop over all changes you get and do the following:
	 */
	/*	char *data = NULL;
		//Now get the data of this change
		
		//Make the new change to report
		OSyncChange *change = osync_change_new();
		//Set the member
		osync_change_set_member(change, env->member);
		//Now set the uid of the object
		osync_change_set_uid(change, "<some uid>");
		//Set the object format
		osync_change_set_objformat_string(change, "<the format of the object>");
		//Set the hash of the object (optional, only required if you use hashtabled)
		osync_change_set_hash(change, "the calculated hash of the object");
		//Now you can set the data for the object
		//Set the last argument to FALSE if the real data
		//should be queried later in a "get_data" function
		
		osync_change_set_data(change, data, sizeof(data), TRUE);		*/	

		//otherwise just report the change via
		//osync_context_report_change(ctx, change);

	//Now we need to answer the call
	osync_context_report_success(ctx);
}

static void batch_commit(OSyncContext *ctx, OSyncContext **contexts, OSyncChange **changes)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, ctx, contexts, changes);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
}

static void sync_done(OSyncContext *ctx)
{
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	
	/*
	 * This function will only be called if the sync was successfull
	 */
	
	//If we use anchors we have to update it now.
	char *lanchor = NULL;
	//Now you get/calculate the current anchor of the device
	osync_anchor_update(env->member, "lanchor", lanchor);
	
	//Answer the call
	osync_context_report_success(ctx);
}

static void disconnect(OSyncContext *ctx)
{
	//SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	
	//Close all stuff you need to close
	
	//Answer the call
	osync_context_report_success(ctx);
}

static void finalize(void *data)
{
	SmlPluginEnv *env = (SmlPluginEnv *)data;
	g_free(env);
}

void get_info(OSyncEnv *env)
{
	OSyncPluginInfo *info = osync_plugin_new_info(env);
	
	info->name = "syncml-http-server";
	info->longname = "SyncML over HTTP Server";
	
	info->functions.initialize = syncml_http_server_init;
	info->functions.connect = connect;
	info->functions.sync_done = sync_done;
	info->functions.disconnect = disconnect;
	info->functions.finalize = finalize;
	info->functions.get_changeinfo = get_changeinfo;
	
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard21", NULL);
	osync_plugin_set_batch_commit_objformat(info, "contact", "vcard21", batch_commit);
}
