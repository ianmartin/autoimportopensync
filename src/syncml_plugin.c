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

#include "syncml_plugin.h"
#include <string.h>
#include <time.h>

static void _verify_user(SmlAuthenticator *auth, const char *username, const char *password, void *userdata, SmlSessionAuthType *reply)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s, %p, %p)", __func__, auth, username, password, userdata, reply);
	SmlPluginEnv *env = userdata;
	
	osync_trace(TRACE_INTERNAL, "configured is %s, %s", env->username, env->password);
	if (env->username && (!env->password || !username || !password || strcmp(env->username, username) || strcmp(env->password, password))) {
		*reply = SML_AUTH_REJECTED;
	} else {
		*reply = SML_AUTH_ACCEPTED;
	}
	osync_trace(TRACE_EXIT, "%s: %i", __func__, *reply);
}

static void _send_success(void *userData, SmlTransportResult result, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, userData, result, error);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

const char *_contenttype_to_format(SmlContentType type)
{
	switch (type) {
		case SML_CONTENT_TYPE_VCARD:
			return "contact";
			break;
		default:
			;
	}
	return NULL;
}

OSyncChangeType _to_osync_changetype(SmlChangeType type)
{
	switch (type) {
		case SML_CHANGE_ADD:
			return CHANGE_ADDED;
		case SML_CHANGE_REPLACE:
			return CHANGE_MODIFIED;
		case SML_CHANGE_DELETE:
			return CHANGE_DELETED;
		default:
			;
	}
	return CHANGE_UNKNOWN;
}

static SmlBool _syncml_recv_change(SmlDsServer *server, SmlChangeType type, const char *uid, char *data, unsigned int size, SmlContentType contenttype, void *userdata, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %p, %i, %i, %p, %p)", __func__, server, type, uid, data, size, contenttype, userdata, error);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data((OSyncContext *)userdata);

	if (type) {
		OSyncChange *change = osync_change_new();
		if (!change) {
			smlErrorSet(error, SML_ERROR_GENERIC, "No change created");
			goto error;
		}
		
		osync_change_set_member(change, env->member);
		osync_change_set_uid(change, uid);
		
		switch (contenttype) {
			case SML_CONTENT_TYPE_VCARD:
				osync_change_set_objformat_string(change, "vcard21");
				break;
			default:
				;
		}
		
		osync_change_set_data(change, data, size, TRUE);
		osync_change_set_changetype(change, _to_osync_changetype(type));
		osync_context_report_change((OSyncContext *)userdata, change);
	} else
		osync_context_report_success((OSyncContext *)userdata);
		
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, smlErrorPrint(error));
	return FALSE;
}

static SmlBool _recv_sync(SmlDsServer *server, void *userdata, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, server, sync, error);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

static void _recv_init_alert(SmlDsServer *server, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, server, userdata);
	SmlPluginEnv *env = (SmlPluginEnv *)userdata;
	osync_member_request_synchronization(env->member);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static SmlBool _recv_alert(SmlDsServer *server, SmlAlertType type, const char *last, const char *next, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %s, %p)", __func__, server, type, last, next, userdata);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data((OSyncContext *)userdata);
	SmlBool ret = TRUE;
	
	char *key = g_strdup_printf("remoteanchor%s", smlDsServerGetLocation(server));
	
	if ((!last || !osync_anchor_compare(env->member, key, last)) && type == SML_ALERT_TWO_WAY)
		ret = FALSE;
	
	if (!ret || type != SML_ALERT_TWO_WAY)
		osync_member_set_slow_sync(env->member, _contenttype_to_format(smlDsServerGetContentType(server)), TRUE);
		
	
	osync_anchor_update(env->member, key, next);
	g_free(key);
	
	osync_context_report_success((OSyncContext *)userdata);
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

struct commitContext {
	OSyncContext *context;
	OSyncChange *change;
};

static void _sent_change(SmlDsServer *server, SmlStatus *status, const char *newuid, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, %p)", __func__, server, status, newuid, userdata);
	struct commitContext *ctx = userdata;
	OSyncContext *context = ctx->context;
	
	if (smlStatusGetClass(status) != SML_ERRORCLASS_SUCCESS) {
		osync_context_report_error(context, OSYNC_ERROR_GENERIC, "Unable to commit change. Error %i", smlStatusGetCode(status));
	} else {
		if (newuid)
			osync_change_set_uid(ctx->change, newuid);
		g_free(ctx);
		
		osync_context_report_success(context);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _sent_sync(SmlSession *session, SmlStatus *status, void *userdata, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, session, userdata);
	osync_context_report_success((OSyncContext *)userdata);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _recv_final(SmlSession *session, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, session, userdata);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _recv_final_flush(SmlSession *session, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, session, userdata);
	
	smlSessionRegisterFinalHandler(session, _recv_final, userdata);
	SmlError *error = NULL;
	smlSessionFlush(session, _send_success, NULL, TRUE, &error);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _recv_end(SmlSession *session, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, session, userdata);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data((OSyncContext *)userdata);
	
	
	smlDsServerReset(env->contactserver);
	
	osync_context_report_success((OSyncContext *)userdata);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static SmlBool _new_session(SmlTransport *tsp, SmlSession *session, void *userdata, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, tsp, session, userdata, error);
	SmlPluginEnv *env = (SmlPluginEnv *)userdata;
	
	if (!smlDsServerRegister(env->contactserver, session, error))
		goto error;
	
	smlSessionRegisterFinalHandler(session, _recv_final, userdata);
	smlDsServerSetConnectCallback(env->contactserver, _recv_init_alert, env);
	
	if (!smlAuthRegister(env->auth, session, error))
		goto error;
	
	env->session = session;
	smlSessionUseStringTable(session, env->useStringtable);
	smlSessionUseOnlyReplace(session, env->onlyReplace);
	smlSessionSetAllowLateStatus(session, TRUE);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, smlErrorPrint(error));
	return FALSE;
}

static SmlBool _new_san_session(SmlTransport *tsp, SmlSession *session, void *userdata, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, tsp, session, userdata, error);
	OSyncContext *ctx = userdata;
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	
	if (!smlDsServerRegister(env->contactserver, session, error))
		goto error;
	
	smlSessionRegisterFinalHandler(session, _recv_final, userdata);
	smlDsServerSetConnectCallback(env->contactserver, _recv_init_alert, env);
	
	if (!smlAuthRegister(env->auth, session, error))
		goto error;
	
	env->session = session;
	smlSessionUseStringTable(session, env->useStringtable);
	smlSessionUseOnlyReplace(session, env->onlyReplace);
	smlSessionSetAllowLateStatus(session, TRUE);
	
	/* Tell the DsServer to notify us as soon as we receive the alert */
	if (!smlDsServerRequestAlert(env->contactserver, _recv_alert, ctx, error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, smlErrorPrint(error));
	return FALSE;
}

static osync_bool syncml_http_server_parse_config(SmlPluginEnv *plugin, SmlTransportHttpServerConfig *env, const char *config, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %i, %p)", __func__, plugin, env, config, size, error);
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	env->port = 8080;
	env->interface = NULL;
	env->url = NULL;
	plugin->contact_url = NULL;
	
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
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"username")) {
				plugin->username = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"password")) {
				plugin->password = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"usestringtable")) {
				plugin->useStringtable = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"onlyreplace")) {
				plugin->onlyReplace = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"contact_db")) {
				plugin->contact_url = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"calendar_db")) {
				plugin->calendar_url = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"task_db")) {
				plugin->task_url = g_strdup(str);
			}
			xmlFree(str);
		}
		cur = cur->next;
	}
	
	xmlFreeDoc(doc);

	if (!plugin->contact_url) {
		osync_error_set(error, SML_ERROR_GENERIC, "Missing database name");
		goto error;
	}

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
		
	smlTransportSetNewSessionCallback(env->tsp, _new_session, env);
	
	if (!syncml_http_server_parse_config(env, &serverConfig, configdata, configsize, error))
		goto error_free_transport;
	
	env->context = osync_member_get_loop(member);
	env->member = member;
	
	if (!smlTransportInitialize(env->tsp, &serverConfig, env->context, &serror))
		goto error_free_transport;
	
	SmlLocation *loc = smlLocationNew(env->contact_url, NULL, &serror);
	
	env->contactserver = smlDsServerNew(SML_CONTENT_TYPE_VCARD, loc, &serror);
	if (!env->contactserver)
		goto error_free_loc;
	
	env->auth = smlAuthNew(&serror);
	if (!env->auth)
		goto error_free_contactserver;

	smlAuthSetVerifyCallback(env->auth, _verify_user, env);
	//smlAuthSetEnable(env->auth, FALSE);
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

static osync_bool syncml_obex_client_parse_config(SmlPluginEnv *plugin, SmlTransportObexClientConfig *env, const char *config, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %i, %p)", __func__, plugin, env, config, size, error);
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	env->path = NULL;
	env->type = 1;
	plugin->contact_url = NULL;
	
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
			if (!xmlStrcmp(cur->name, (const xmlChar *)"path")) {
				env->path = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"type")) {
				env->type = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"username")) {
				plugin->username = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"password")) {
				plugin->password = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"usestringtable")) {
				plugin->useStringtable = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"onlyreplace")) {
				plugin->onlyReplace = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"contact_db")) {
				plugin->contact_url = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"calendar_db")) {
				plugin->calendar_url = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"task_db")) {
				plugin->task_url = g_strdup(str);
			}
			xmlFree(str);
		}
		cur = cur->next;
	}
	
	xmlFreeDoc(doc);

	if (!env->path) {
		osync_error_set(error, SML_ERROR_GENERIC, "Missing path name");
		goto error;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_doc:
	xmlFreeDoc(doc);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static void *syncml_obex_client_init(OSyncMember *member, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);
	char *configdata = NULL;
	int configsize = 0;
	SmlError *serror = NULL;
	SmlTransportObexClientConfig clientConfig;
	
	SmlPluginEnv *env = osync_try_malloc0(sizeof(SmlPluginEnv), error);
	if (!env)
		goto error;
		
	if (!osync_member_get_config(member, &configdata, &configsize, error))
		goto error_free_env;
	
	env->tsp = smlTransportNew(SML_TRANSPORT_OBEX_CLIENT, &serror);
	if (!env->tsp)
		goto error_free_config;
	
	if (!syncml_obex_client_parse_config(env, &clientConfig, configdata, configsize, error))
		goto error_free_transport;
	
	env->context = osync_member_get_loop(member);
	env->member = member;
	
	if (!smlTransportInitialize(env->tsp, &clientConfig, env->context, &serror))
		goto error_free_transport;
	
	SmlLocation *loc = smlLocationNew(env->contact_url, NULL, &serror);
	
	env->contactserver = smlDsServerNew(SML_CONTENT_TYPE_VCARD, loc, &serror);
	if (!env->contactserver)
		goto error_free_loc;
	
	env->auth = smlAuthNew(&serror);
	if (!env->auth)
		goto error_free_contactserver;

	smlAuthSetVerifyCallback(env->auth, _verify_user, env);
	//smlAuthSetEnable(env->auth, FALSE);
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

static void client_connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	SmlError *error = NULL;
	SmlNotification *san = NULL;
	OSyncError *oserror = NULL;
	
	smlTransportSetNewSessionCallback(env->tsp, _new_san_session, ctx);
	
	if (!smlTransportConnect(env->tsp, &error))
		goto error;
	
	/* Check if we need to alert the other side (SAN)
	 * or if we already received an alert */
	if (smlTransportGetType(env->tsp) == SML_TRANSPORT_OBEX_CLIENT) {
		san = smlNotificationNew(SML_SAN_VERSION_12, SML_SAN_UIMODE_USER, SML_SAN_INITIATOR_SERVER, 1, "opensync", &error);
		if (!san)
			goto error;
		
		if (!smlDsServerSendSan(env->contactserver, san, &error))
			goto error_free_san;
		
		if (!smlNotificationSend(san, env->tsp, _send_success, NULL, &error))
			goto error_free_san;
	}
	
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

static void connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	
	/* Tell the DsServer to notify us as soon as we receive the alert */
	if (!smlDsServerRequestAlert(env->contactserver, _recv_alert, ctx, &error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	//osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorFree(&error);
	osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "You cannot start the syncml server directly. plaese use msynctool --sync name --wait");
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

static void _alert_reply(SmlSession *session, SmlStatus *status, void *userdata, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, session, status, userdata, error);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void get_changeinfo(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	OSyncError *oserror = NULL;
	SmlAlertType type = SML_ALERT_TWO_WAY;
	
	SmlError *error = NULL;
	if (osync_member_get_slow_sync(env->member, "contact"))
		type = SML_ALERT_SLOW_SYNC;
	
	if (!smlDsServerRequestChanges(env->contactserver, _syncml_recv_change, _recv_sync, ctx, &error))
		goto error;
	
	char *key = g_strdup_printf("local%s", smlDsServerGetLocation(env->contactserver));
	
	char *next = g_strdup_printf("%i", (int)time(NULL));
	char *last = osync_anchor_retrieve(env->member, key);
	osync_anchor_update(env->member, key, next);
	g_free(key);
	
	if (!smlDsServerSendAlert(env->contactserver, type, last, next, _alert_reply, NULL, &error)) {
		g_free(next);
		g_free(last);
		goto error;
	}
	g_free(next);
	g_free(last);
	
	if (!smlSessionFlush(env->session, _send_success, ctx, TRUE, &error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorFree(&error);
	osync_context_report_osyncerror(ctx, &oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

SmlChangeType _get_changetype(OSyncChange *change)
{
	switch (osync_change_get_changetype(change)) {
		case CHANGE_ADDED:
			return SML_CHANGE_ADD;
		case CHANGE_MODIFIED:
			return SML_CHANGE_REPLACE;
		case CHANGE_DELETED:
			return SML_CHANGE_DELETE;
		default:
			;
	}
	return SML_CHANGE_UNKNOWN;
}

SmlContentType _format_to_contenttype(OSyncChange *change)
{
	if (!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "contact")) {
		return SML_CONTENT_TYPE_VCARD;
	}
	return SML_CONTENT_TYPE_UNKNOWN;
}

static void batch_commit(OSyncContext *ctx, OSyncContext **contexts, OSyncChange **changes)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, ctx, contexts, changes);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	
	int i = 0;
	for (i = 0; changes[i]; i++);
		
	if (!smlDsServerSendSync(env->contactserver, i, _sent_sync, ctx, &error))
		goto error;
	
	for (i = 0; changes[i] && contexts[i]; i++) {
		OSyncChange *change = changes[i];
		OSyncContext *context = contexts[i];
		osync_trace(TRACE_INTERNAL, "Uid: \"%s\", Format: \"%s\", Changetype: \"%i\"", osync_change_get_uid(change), osync_objtype_get_name(osync_change_get_objtype(change)), osync_change_get_changetype(change));
		
		struct commitContext *tracer = osync_try_malloc0(sizeof(struct commitContext), &oserror);
		if (!tracer)
			goto oserror;
		
		tracer->change = change;
		tracer->context = context;

		if (!smlDsServerQueueChange(env->contactserver, _get_changetype(change), osync_change_get_uid(change), osync_change_get_data(change), osync_change_get_datasize(change), _format_to_contenttype(change), _sent_change, tracer, &error))
			goto error;
	}
	
	if (!smlDsServerCloseSync(env->contactserver, &error))
		goto error;
	
	smlSessionRegisterFinalHandler(env->session, _recv_final_flush, NULL);
	if (!smlSessionFlush(env->session, _send_success, NULL, TRUE, &error))
		goto error;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorFree(&error);
oserror:
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
	osync_plugin_accept_objformat(info, "contact", "vcard21", "clean");
	osync_plugin_set_batch_commit_objformat(info, "contact", "vcard21", batch_commit);
	
	info = osync_plugin_new_info(env);
	
	info->name = "syncml-obex-client";
	info->longname = "SyncML over OBEX Client";
	
	info->functions.initialize = syncml_obex_client_init;
	info->functions.connect = client_connect;
	info->functions.sync_done = sync_done;
	info->functions.disconnect = disconnect;
	info->functions.finalize = finalize;
	info->functions.get_changeinfo = get_changeinfo;
	
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard21", "clean");
	osync_plugin_set_batch_commit_objformat(info, "contact", "vcard21", batch_commit);
}
