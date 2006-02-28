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

static const char *_contenttype_to_format(SmlContentType type)
{
	switch (type) {
		case SML_CONTENT_TYPE_VCARD:
			return "contact";
			break;
		case SML_CONTENT_TYPE_VCAL:
			return "event";
			break;
		default:
			;
	}
	return NULL;
}


static OSyncChangeType _to_osync_changetype(SmlChangeType type)
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

static SmlBool _recv_change(SmlDsSession *dsession, SmlChangeType type, const char *uid, char *data, unsigned int size, SmlContentType contenttype, void *userdata, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %p, %i, %i, %p, %p)", __func__, dsession, type, uid, data, size, contenttype, userdata, error);
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
			case SML_CONTENT_TYPE_VCAL:
			case SML_CONTENT_TYPE_VTODO:
				/* Most devices cannot separate events and todos. so we
				 * set the format to plain and let opensync decide */
				osync_change_set_objformat_string(change, "plain");
				break;
			case SML_CONTENT_TYPE_PLAIN:
				osync_change_set_objformat_string(change, "note");
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

static void _recv_change_reply(SmlDsSession *dsession, SmlStatus *status, const char *newuid, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, %p)", __func__, dsession, status, newuid, userdata);
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

static void _recv_sync_reply(SmlSession *session, SmlStatus *status, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, session, status, userdata);
	
	printf("Received an reply to our sync\n");
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _recv_sync(SmlDsSession *dsession, unsigned int numchanges, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, dsession, numchanges, userdata);
	
	printf("Going to receive %i changes\n", numchanges);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _recv_alert_reply(SmlSession *session, SmlStatus *status, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, session, status, userdata);
	
	printf("Received an reply to our Alert\n");
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static SmlBool _recv_alert(SmlDsSession *dsession, SmlAlertType type, const char *last, const char *next, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %s, %p)", __func__, dsession, type, last, next, userdata);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data((OSyncContext *)userdata);
	SmlBool ret = TRUE;
	
	char *key = g_strdup_printf("remoteanchor%s", smlDsSessionGetLocation(dsession));
	
	if ((!last || !osync_anchor_compare(env->member, key, last)) && type == SML_ALERT_TWO_WAY)
		ret = FALSE;
	
	osync_bool ans = osync_member_get_slow_sync(env->member, _contenttype_to_format(smlDsSessionGetContentType(dsession)));
	if (ans)
		ret = FALSE;
	
	if (!ret || type != SML_ALERT_TWO_WAY)
		osync_member_set_slow_sync(env->member, _contenttype_to_format(smlDsSessionGetContentType(dsession)), TRUE);
	
	osync_anchor_update(env->member, key, next);
	g_free(key);
	
	if (!ret) {
		smlDsSessionSendAlert(dsession, SML_ALERT_SLOW_SYNC, last, next, _recv_alert_reply, NULL, NULL);
	} else {
		smlDsSessionSendAlert(dsession, SML_ALERT_TWO_WAY, last, next, _recv_alert_reply, NULL, NULL);
	}
	
	smlDevInfAgentGetDevInf(env->agent, NULL);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static void _ds_alert(SmlDsSession *dsession, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, dsession, userdata);

	SmlPluginEnv *env = (SmlPluginEnv *)userdata;
	osync_member_request_synchronization(env->member);
	
	if (smlDsSessionGetContentType(dsession) == SML_CONTENT_TYPE_VCARD) {
		printf("received contact dsession\n");
		env->contactSession = dsession;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _manager_event(SmlManager *manager, SmlManagerEventType type, SmlSession *session, SmlError *error, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p)", __func__, manager, type, session, error, userdata);
	SmlPluginEnv *env = userdata;

	switch (type) {
		case SML_MANAGER_SESSION_NEW:
			osync_trace(TRACE_INTERNAL, "Just received a new session with ID %s\n", smlSessionGetSessionID(session));
			smlSessionUseStringTable(session, env->useStringtable);
			smlSessionUseOnlyReplace(session, env->onlyReplace);
			smlSessionSetAllowLateStatus(session, TRUE);
					
			env->session = session;
			break;
		case SML_MANAGER_SESSION_FINAL:
			osync_trace(TRACE_INTERNAL, "Session %s reported final\n", smlSessionGetSessionID(session));
			
			if (env->connectCtx) {
				osync_context_report_success(env->connectCtx);
				env->connectCtx = NULL;
			}
			
			if (env->getChangesCtx) {
				osync_context_report_success(env->getChangesCtx);
				env->getChangesCtx = NULL;
			}
			
			if (env->commitCtx) {
				osync_context_report_success(env->commitCtx);
				env->commitCtx = NULL;
			}
			
			if (env->disconnectCtx) {
				osync_context_report_success(env->commitCtx);
				env->commitCtx = NULL;
			}
			break;
		case SML_MANAGER_SESSION_END:
			osync_trace(TRACE_INTERNAL, "Session %s has ended\n", smlSessionGetSessionID(session));
			break;
		case SML_MANAGER_SESSION_ERROR:
			osync_trace(TRACE_INTERNAL, "There was an error in the session %s: %s", smlSessionGetSessionID(session), smlErrorPrint(&error));
			break;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static gboolean _sessions_prepare(GSource *source, gint *timeout_)
{
	*timeout_ = 50;
	return FALSE;
}

static gboolean _sessions_check(GSource *source)
{
	SmlPluginEnv *env = *((SmlPluginEnv **)(source + 1));
	
	if (env->contactSession && smlDsSessionCheck(env->contactSession))
		return TRUE;
	
	if (smlManagerCheck(env->manager))
		return TRUE;
		
	return FALSE;
}

static gboolean _sessions_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
	SmlPluginEnv *env = user_data;
	
	if (env->contactSession && smlDsSessionCheck(env->contactSession))
		smlDsSessionDispatch(env->contactSession);
	else
		smlManagerDispatch(env->manager);
	
	return TRUE;
}

static void _verify_user(SmlAuthenticator *auth, const char *username, const char *password, void *userdata, SmlErrorType *reply)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s, %p, %p)", __func__, auth, username, password, userdata, reply);
	SmlPluginEnv *env = userdata;
	
	osync_trace(TRACE_INTERNAL, "configured is %s, %s", env->username, env->password);
	if (env->username && (!env->password || !username || !password || strcmp(env->username, username) || strcmp(env->password, password))) {
		*reply = SML_ERROR_AUTH_REJECTED;
	} else {
		*reply = SML_AUTH_ACCEPTED;
	}
	osync_trace(TRACE_EXIT, "%s: %i", __func__, *reply);
}

#if 0
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

#endif

static osync_bool syncml_obex_client_parse_config(SmlPluginEnv *env, const char *config, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %i, %p)", __func__, env, config, size, error);
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	
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
		if (str && strlen(str)) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"interface")) {
				env->interface = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"identifier")) {
				env->identifier = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"type")) {
				env->type = atoi(str);
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
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"contact_db")) {
				env->contact_url = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"calendar_db")) {
				env->calendar_url = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"task_db")) {
				env->task_url = g_strdup(str);
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

static void *syncml_obex_client_init(OSyncMember *member, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);
	char *configdata = NULL;
	int configsize = 0;
	SmlError *serror = NULL;
	
	SmlPluginEnv *env = osync_try_malloc0(sizeof(SmlPluginEnv), error);
	if (!env)
		goto error;
		
	if (!osync_member_get_config(member, &configdata, &configsize, error))
		goto error_free_env;
	
	if (!syncml_obex_client_parse_config(env, configdata, configsize, error))
		goto error_free_transport;
	
	env->context = osync_member_get_loop(member);
	env->member = member;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;

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

static void client_connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	
	env->connectCtx = ctx;
	
	/* The transport needed to transport the data */
	env->tsp = smlTransportNew(SML_TRANSPORT_OBEX_CLIENT, &error);
	if (!env->tsp)
		goto error;
	
	/* The manager responsible for handling the other objects */
	env->manager = smlManagerNew(env->tsp, &error);
	if (!env->manager)
		goto error;
	smlManagerSetEventCallback(env->manager, _manager_event, env);
	
	/* The authenticator */
	env->auth = smlAuthNew(&error);
	if (!env->auth)
		goto error;
	smlAuthSetVerifyCallback(env->auth, _verify_user, env);
	
	if (!env->username)
		smlAuthSetEnable(env->auth, FALSE);
	
	if (!smlAuthRegister(env->auth, env->manager, &error))
		goto error_free_auth;
	
	
	/* Now create the devinf handler */
	SmlDevInf *devinf = smlDevInfNew(&error);
	if (!devinf)
		goto error_free_manager;
	
	env->agent = smlDevInfAgentNew(devinf, SML_DEVINF_VERSION_11, &error);
	if (!env->agent)
		goto error_free_manager;
	
	if (!smlDevInfAgentRegister(env->agent, env->manager, &error))
		goto error_free_manager;
	
	
	/* We now create the ds server hat the given location */
	SmlLocation *loc = smlLocationNew(env->contact_url, NULL, &error);
	if (!loc)
		goto error;
	
	env->contactserver = smlDsServerNew(SML_CONTENT_TYPE_VCARD, loc, &error);
	if (!env->contactserver)
		goto error_free_manager;
		
	if (!smlDsServerRegister(env->contactserver, env->manager, &error))
		goto error_free_auth;
	
	smlDsServerSetConnectCallback(env->contactserver, _ds_alert, env);
		
	/* And we also add the devinfo to the devinf agent */
	SmlDevInfDataStore *datastore = smlDevInfDataStoreNew(loc, SML_DEVINF_VERSION_11, &error);
	if (!datastore)
		goto error;
	
	smlDevInfAddDataStore(devinf, datastore);
	
	/* Create the alert for the remote device */
	if (!env->identifier)
		env->identifier = g_strdup("LibSyncML Test Suite");
		
	SmlNotification *san = smlNotificationNew(env->version, SML_SAN_UIMODE_UNSPECIFIED, SML_SAN_INITIATOR_USER, 1, env->identifier, env->useWbxml ? SML_MIMETYPE_WBXML : SML_MIMETYPE_XML, &error);
	if (!san)
		goto error;
	
	/* Then we add the alert to the SAN */
	if (!smlDsServerAddSan(env->contactserver, san, &error))
		goto error_free_san;
	
	GMainContext *context = osync_member_get_loop(env->member);
	
	GSourceFuncs *functions = g_malloc0(sizeof(GSourceFuncs));
	functions->prepare = _sessions_prepare;
	functions->check = _sessions_check;
	functions->dispatch = _sessions_dispatch;
	functions->finalize = NULL;

	GSource *source = g_source_new(functions, sizeof(GSource) + sizeof(SmlPluginEnv *));
	SmlPluginEnv **envptr = (SmlPluginEnv **)(source + 1);
	*envptr = env;
	g_source_set_callback(source, NULL, env, NULL);
	g_source_attach(source, context);


	SmlTransportObexClientConfig config;
	config.type = SML_OBEX_TYPE_USB;
	config.port = env->interface;

	/* Run the manager */
	if (!smlManagerStart(env->manager, &error))
		goto error;
	
	/* Initialize the Transport */
	if (!smlTransportInitialize(env->tsp, &config, &error))
		goto error;
	
	if (!smlTransportConnect(env->tsp, &error))
		goto error;
	
	if (!smlNotificationSend(san, env->tsp, &error))
		goto error_free_san;
	
	smlNotificationFree(san);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_san:
	smlNotificationFree(san);
error_free_auth:
	smlAuthFree(env->auth);
error_free_manager:
	smlManagerFree(env->manager);
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_context_report_osyncerror(ctx, &oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

/*static void connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	
	if (!smlDsServerRequestAlert(env->contactserver, _recv_alert, ctx, &error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	//osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorFree(&error);
	osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "You cannot start the syncml server directly. plaese use msynctool --sync name --wait");
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}*/

static void get_changeinfo(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	env->getChangesCtx = ctx;
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	
	if (env->contactSession) {
		smlDsSessionGetAlert(env->contactSession, _recv_alert, ctx);
		smlDsSessionGetSync(env->contactSession, _recv_sync, ctx);
		smlDsSessionGetChanges(env->contactSession, _recv_change, ctx);
	}
	
	if (!smlSessionFlush(env->session, TRUE, &error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_context_report_osyncerror(ctx, &oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

static void batch_commit_vcard(OSyncContext *ctx, OSyncContext **contexts, OSyncChange **changes)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, ctx, contexts, changes);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	
	env->commitCtx = ctx;
	
	int i = 0;
	for (i = 0; changes[i]; i++);
		
	if (!smlDsSessionSendSync(env->contactSession, i, _recv_sync_reply, NULL, &error))
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

		if (!smlDsSessionQueueChange(env->contactSession, _get_changetype(change), osync_change_get_uid(change), osync_change_get_data(change), osync_change_get_datasize(change), _format_to_contenttype(change), _recv_change_reply, tracer, &error))
			goto error;
	}
	
	if (!smlDsSessionCloseSync(env->contactSession, &error))
		goto error;

	if (!smlSessionFlush(env->session, TRUE, &error))
		goto error;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
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
	
	if (!smlSessionEnd(env->session, &error))
		goto error;
	
	if (!smlTransportDisconnect(env->tsp, NULL, &error))
		goto error;
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_context_report_osyncerror(ctx, &oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

static void finalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	SmlPluginEnv *env = (SmlPluginEnv *)data;
	
	/* Stop the manager */
	smlManagerStop(env->manager);
	
	smlTransportFinalize(env->tsp, NULL);
	
	smlTransportFree(env->tsp);
	
	g_free(env);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void get_info(OSyncEnv *env)
{
	OSyncPluginInfo *info = osync_plugin_new_info(env);
	
	/*info->name = "syncml-http-server";
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
	
	info = osync_plugin_new_info(env);*/
	
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
	osync_plugin_set_batch_commit_objformat(info, "contact", "vcard21", batch_commit_vcard);
}
