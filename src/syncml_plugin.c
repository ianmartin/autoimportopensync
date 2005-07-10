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

SmlBool _recv_change(SmlDsServer *server, SmlDsChange *change, void *userdata, SmlError **error)
{
	
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
	return TRUE;
}

SmlBool _recv_sync(SmlDsServer *server, void *userdata, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, server, sync, error);
	osync_context_report_success((OSyncContext *)userdata);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

SmlBool _recv_alert(SmlDsServer *server, void *userdata, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, server, userdata, error);
	osync_context_report_success((OSyncContext *)userdata);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

SmlBool _sent_change(SmlDsServer *server, void *userdata, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, server, userdata, error);
	osync_context_report_success((OSyncContext *)userdata);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

void _recv_final(SmlSession *session, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, session, userdata);
	
	SmlError *error = NULL;
	smlSessionEndPackage(session, _send_success, NULL, &error);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void _recv_end(SmlSession *session, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, session, userdata);
	osync_context_report_success((OSyncContext *)userdata);
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
	
	env->session = session;
	
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
	SmlError *error = NULL;
	SmlNotification *san = NULL;
	OSyncError *oserror = NULL;
	
	/* Check if we need to alert the other side (SAN)
	 * or if we already received an alert */
	if (!smlDsServerReceivedAlert(env->contactserver)) {
		san = smlNotificationNew(&error);
		if (!san)
			goto error;
		
		if (!smlDsServerSendSan(env->contactserver, san, &error))
			goto error_free_san;
		
		if (!smlNotificationSend(san, env->tsp, &error))
			goto error_free_san;
	}
	
	/* Tell the DsServer to notify us as soon as we receive the alert */
	if (!smlDsServerRequestAlert(env->contactserver, _recv_alert, ctx, &error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_san:
	smlNotificationFree(san);
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorFree(&error);
	osync_context_report_osyncerror(ctx, &oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

static void get_changeinfo(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	OSyncError *oserror = NULL;
	
	SmlError *error = NULL;
	if (osync_member_get_slow_sync(env->member, "contact")) {
		smlDsServerSetSyncType(env->contactserver, SML_ALERT_TWO_WAY);
	}

	if (!smlDsServerRequestChanges(env->contactserver, _recv_change, _recv_sync, ctx, &error))
		goto error;
	
	if (!smlSessionEndPackage(env->session, _send_success, ctx, &error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorFree(&error);
	osync_context_report_osyncerror(ctx, &oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

static void batch_commit(OSyncContext *ctx, OSyncContext **contexts, OSyncChange **changes)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, ctx, contexts, changes);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	SmlError *error = NULL;
	SmlDsChange *smlchange = NULL;
	OSyncError *oserror = NULL;
	
	int i = 0;
	for (i = 0; changes[i] && contexts[i]; i++) {
		OSyncChange *change = changes[i];
		OSyncContext *context = contexts[i];
		osync_trace(TRACE_INTERNAL, "Uid: \"%s\", Format: \"%s\", Changetype: \"%i\"", osync_change_get_uid(change), osync_objtype_get_name(osync_change_get_objtype(change)), osync_change_get_changetype(change));
		
		smlchange = smlDsChangeNew(osync_change_get_uid(change), osync_change_get_data(change), osync_change_get_datasize(change), 0, &error);
		if (!smlchange)
			goto error;
		
		if (!smlDsServerSendChange(env->contactserver, smlchange, _sent_change, context, &error))
			goto error_free_change;
	}
	
	if (!smlSessionEndPackage(env->session, _send_success, ctx, &error))
		goto error;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error_free_change:
	smlDsChangeFree(smlchange);
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorFree(&error);
	osync_context_report_osyncerror(ctx, &oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

static void sync_done(OSyncContext *ctx)
{
	osync_context_report_success(ctx);
}

static void disconnect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	OSyncError *oserror = NULL;
	SmlError *error = NULL;
	
	if (!smlSessionEnd(env->session, _recv_end, ctx, &error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorFree(&error);
	osync_context_report_osyncerror(ctx, &oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
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
