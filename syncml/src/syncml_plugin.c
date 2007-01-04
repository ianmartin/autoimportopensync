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

static SmlChangeType _get_changetype(OSyncChange *change)
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

static const char *_format_to_contenttype(OSyncChange *change)
{
	if (!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "contact")) {
		return SML_ELEMENT_TEXT_VCARD;
	}
	if (!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "event")) {
		return SML_ELEMENT_TEXT_VCAL;
	}
	if (!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "todo")) {
		return SML_ELEMENT_TEXT_VCAL;
	}
	if (!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "note")) {
		return SML_ELEMENT_TEXT_PLAIN;
	}
	if (!strcmp(osync_objtype_get_name(osync_change_get_objtype(change)), "data")) {
		return SML_ELEMENT_TEXT_PLAIN;
	}
	return NULL;
}

static const char *_contenttype_to_format(const char *contenttype)
{
	if (!strcmp(contenttype, SML_ELEMENT_TEXT_VCARD)) {
		return "contact";
	}
	if (!strcmp(contenttype, SML_ELEMENT_TEXT_VCAL)) {
		return "data";
	}
	if (!strcmp(contenttype, SML_ELEMENT_TEXT_PLAIN)) {
		return "note";
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

static SmlBool _recv_change(SmlDsSession *dsession, SmlChangeType type, const char *uid, char *data, unsigned int size, const char *contenttype, void *userdata, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %p, %i, %s, %p, %p)", __func__, dsession, type, uid, data, size, contenttype, userdata, error);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data((OSyncContext *)userdata);

	if (type) {
		OSyncChange *change = osync_change_new();
		if (!change) {
			smlErrorSet(error, SML_ERROR_GENERIC, "No change created");
			goto error;
		}
		
		osync_change_set_member(change, env->member);
		osync_change_set_uid(change, uid);
		
		if (contenttype != NULL) {
			/* We specify the objformat plain for vcard and vcal
			 * since we cannot be really sure what the device sends
			 * us, without looking at the devinf. Since we dont use
			 * the devinf yet, we let the opensync detector decide
			 * what format the item is.
			 * 
			 * For text/plain (notes) we specify the memo format, 
			 * so that it does not get detected */
			if (!strcmp(contenttype, SML_ELEMENT_TEXT_VCARD))
				osync_change_set_objformat_string(change, "plain");
			else if (!strcmp(contenttype, SML_ELEMENT_TEXT_VCAL))
				osync_change_set_objformat_string(change, "plain");
			else if (!strcmp(contenttype, SML_ELEMENT_TEXT_PLAIN))
				osync_change_set_objformat_string(change, "memo");
		}

		/* XXX Workaround for mobiles which only handle localtime! */
		if (!strcmp(contenttype, SML_ELEMENT_TEXT_VCAL) && env->onlyLocaltime && type != SML_CHANGE_DELETE) {
			char *_data = osync_time_vcal2utc(data);
			g_free(data);
			data = _data;
			size = strlen(data);
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
	SmlPluginEnv *env = userdata;
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
	
	smlDevInfAgentGetDevInf(env->agent);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

static void _ds_alert(SmlDsSession *dsession, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, dsession, userdata);

	SmlPluginEnv *env = (SmlPluginEnv *)userdata;
	osync_member_request_synchronization(env->member);
	
	if (!strcmp(smlDsSessionGetContentType(dsession), SML_ELEMENT_TEXT_VCARD)) {
		printf("received contact dsession\n");
		env->contactSession = dsession;
		smlDsSessionRef(dsession);
	} else if (!strcmp(smlDsSessionGetContentType(dsession), SML_ELEMENT_TEXT_VCAL)) {
		printf("received event dsession\n");
		env->calendarSession = dsession;
		smlDsSessionRef(dsession);
	} else if (!strcmp(smlDsSessionGetContentType(dsession), SML_ELEMENT_TEXT_PLAIN)) {
		printf("received note dsession\n");
		env->noteSession = dsession;
		smlDsSessionRef(dsession);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _manager_event(SmlManager *manager, SmlManagerEventType type, SmlSession *session, SmlError *error, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p)", __func__, manager, type, session, error, userdata);
	SmlPluginEnv *env = userdata;

	switch (type) {
		case SML_MANAGER_SESSION_FLUSH:
		case SML_MANAGER_CONNECT_DONE:
			env->gotDisconnect = FALSE;
			break;
		case SML_MANAGER_DISCONNECT_DONE:
			osync_trace(TRACE_INTERNAL, "connection with device has ended");
			env->gotDisconnect = TRUE;
			if (env->disconnectCtx) {
				osync_context_report_success(env->disconnectCtx);
				env->disconnectCtx = NULL;
			}
			break;
		case SML_MANAGER_TRANSPORT_ERROR:
			osync_trace(TRACE_INTERNAL, "There was an error in the transport: %s", smlErrorPrint(&error));
			if (!env->gotDisconnect) {
				if (env->tryDisconnect == FALSE) {
					env->tryDisconnect = TRUE;
					smlTransportDisconnect(env->tsp, NULL, NULL);
					while (!env->gotDisconnect) {
						smlManagerDispatch(manager);
					}
				} else {
					env->gotDisconnect = TRUE;
					osync_trace(TRACE_EXIT_ERROR, "%s: error while disconnecting: %s", __func__, smlErrorPrint(&error));
					return;
				}
			}
			goto error;
			break;
		case SML_MANAGER_SESSION_NEW:
			osync_trace(TRACE_INTERNAL, "Just received a new session with ID %s\n", smlSessionGetSessionID(session));
			smlSessionUseStringTable(session, env->useStringtable);
			smlSessionUseOnlyReplace(session, env->onlyReplace);
			
			if (env->recvLimit)
				smlSessionSetReceivingLimit(session, env->recvLimit);
				
			if (env->maxObjSize)
				smlSessionSetReceivingMaxObjSize(session, env->maxObjSize);
			
			env->session = session;
			smlSessionRef(session);
			break;
		case SML_MANAGER_SESSION_FINAL:
			osync_trace(TRACE_INTERNAL, "Session %s reported final\n", smlSessionGetSessionID(session));
			env->gotFinal = TRUE;
			
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
			break;
		case SML_MANAGER_SESSION_END:
			osync_trace(TRACE_INTERNAL, "Session %s has ended\n", smlSessionGetSessionID(session));
			if (!smlTransportDisconnect(env->tsp, NULL, &error))
				goto error;
			
			break;
		case SML_MANAGER_SESSION_ERROR:
			osync_trace(TRACE_INTERNAL, "There was an error in the session %s: %s", smlSessionGetSessionID(session), smlErrorPrint(&error));
			goto error;
			break;
		case SML_MANAGER_SESSION_WARNING:
			printf("WARNING: %s\n", smlErrorPrint(&error));
			break;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:;
	OSyncError *oserror = NULL;
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, smlErrorPrint(&error));
	
	if (env->connectCtx) {
		osync_context_report_osyncerror(env->connectCtx, &oserror);
		env->connectCtx = NULL;
	}
	
	if (env->getChangesCtx) {
		osync_context_report_osyncerror(env->getChangesCtx, &oserror);
		env->getChangesCtx = NULL;
	}
	
	if (env->disconnectCtx) {
		osync_context_report_osyncerror(env->disconnectCtx, &oserror);
		env->disconnectCtx = NULL;
	}
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
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
		
	if (env->calendarSession && smlDsSessionCheck(env->calendarSession))
		return TRUE;
		
	if (env->noteSession && smlDsSessionCheck(env->noteSession))
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
	else if (env->calendarSession && smlDsSessionCheck(env->calendarSession))
		smlDsSessionDispatch(env->calendarSession);
	else if (env->noteSession && smlDsSessionCheck(env->noteSession))
		smlDsSessionDispatch(env->noteSession);
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

static osync_bool syncml_http_server_parse_config(SmlPluginEnv *env, const char *config, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %i, %p)", __func__, env, config, size, error);
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	env->port = 8080;
	env->url = NULL;
	env->username = NULL;
	env->recvLimit = 0;
	env->password = NULL;
	env->useStringtable = TRUE;
	env->onlyReplace = FALSE;
	env->contact_url = NULL;
	env->calendar_url = NULL;
	env->note_url = NULL;
	
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
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"contact_db")) {
				env->contact_url = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"calendar_db")) {
				env->calendar_url = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"note_db")) {
				env->note_url = g_strdup(str);
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
	
	SmlPluginEnv *env = osync_try_malloc0(sizeof(SmlPluginEnv), error);
	if (!env)
		goto error;
		
	if (!osync_member_get_config(member, &configdata, &configsize, error))
		goto error_free_env;
	
	if (!syncml_http_server_parse_config(env, configdata, configsize, error))
		goto error_free_transport;
	
	env->context = osync_member_get_loop(member);
	env->member = member;
	
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
	
	if (env->contact_url) {
		/* We now create the ds server hat the given location */
		SmlLocation *loc = smlLocationNew(env->contact_url, NULL, &serror);
		if (!loc)
			goto error;
		
		env->contactserver = smlDsServerNew(SML_ELEMENT_TEXT_VCARD, loc, &serror);
		if (!env->contactserver)
			goto error_free_manager;
			
		if (!smlDsServerRegister(env->contactserver, env->manager, &serror))
			goto error_free_auth;
		
		smlDsServerSetConnectCallback(env->contactserver, _ds_alert, env);
		
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
	
	if (env->calendar_url) {
		/* We now create the ds server hat the given location */
		SmlLocation *loc = smlLocationNew(env->calendar_url, NULL, &serror);
		if (!loc)
			goto error;
		
		env->calendarserver = smlDsServerNew(SML_ELEMENT_TEXT_VCAL, loc, &serror);
		if (!env->calendarserver)
			goto error_free_manager;
			
		if (!smlDsServerRegister(env->calendarserver, env->manager, &serror))
			goto error_free_auth;
		
		smlDsServerSetConnectCallback(env->calendarserver, _ds_alert, env);
		
		/* And we also add the devinfo to the devinf agent */
		SmlDevInfDataStore *datastore = smlDevInfDataStoreNew(smlLocationGetURI(loc), &serror);
		if (!datastore)
			goto error;
		
		smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_VCAL, "1.0");
		smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_VCAL, "1.0");
		
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_TWO_WAY, TRUE);
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_SLOW_SYNC, TRUE);
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_SERVER_ALERTED_SYNC, TRUE);
		
		smlDevInfAddDataStore(devinf, datastore);
	}
	
	if (env->note_url) {
		/* We now create the ds server hat the given location */
		SmlLocation *loc = smlLocationNew(env->note_url, NULL, &serror);
		if (!loc)
			goto error;
		
		env->noteserver = smlDsServerNew(SML_ELEMENT_TEXT_PLAIN, loc, &serror);
		if (!env->noteserver)
			goto error_free_manager;
			
		if (!smlDsServerRegister(env->noteserver, env->manager, &serror))
			goto error_free_auth;
		
		smlDsServerSetConnectCallback(env->noteserver, _ds_alert, env);
		
		/* And we also add the devinfo to the devinf agent */
		SmlDevInfDataStore *datastore = smlDevInfDataStoreNew(smlLocationGetURI(loc), &serror);
		if (!datastore)
			goto error;
		
		smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_PLAIN, "1.0");
		smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_PLAIN, "1.0");
		
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


	SmlTransportHttpServerConfig config;
	config.port = env->port;
	config.url = env->url;
	config.interface = NULL;
	
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
			if (!xmlStrcmp(cur->name, (const xmlChar *)"bluetooth_address")) {
				env->bluetoothAddress = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"bluetooth_channel")) {
				env->bluetoothChannel = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"interface")) {
				env->interface = atoi(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"identifier")) {
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
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"contact_db")) {
				env->contact_url = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"calendar_db")) {
				env->calendar_url = g_strdup(str);
			}
			
			if (!xmlStrcmp(cur->name, (const xmlChar *)"note_db")) {
				env->note_url = g_strdup(str);
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
		goto error_free_env;
	
	env->context = osync_member_get_loop(member);
	env->member = member;
	
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
	
	
	if (env->contact_url) {
		/* We now create the ds server hat the given location */
		SmlLocation *loc = smlLocationNew(env->contact_url, NULL, &serror);
		if (!loc)
			goto error_free_auth;
		
		env->contactserver = smlDsServerNew(SML_ELEMENT_TEXT_VCARD, loc, &serror);
		if (!env->contactserver)
			goto error_free_auth;
			
		if (!smlDsServerRegister(env->contactserver, env->manager, &serror))
			goto error_free_auth;
		
		smlDsServerSetConnectCallback(env->contactserver, _ds_alert, env);
		
		/* And we also add the devinfo to the devinf agent */
		SmlDevInfDataStore *datastore = smlDevInfDataStoreNew(smlLocationGetURI(loc), &serror);
		if (!datastore)
			goto error_free_auth;
		
		smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_VCARD, "2.1");
		smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_VCARD, "2.1");
		
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_TWO_WAY, TRUE);
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_SLOW_SYNC, TRUE);
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_SERVER_ALERTED_SYNC, TRUE);
		
		smlDevInfAddDataStore(devinf, datastore);
	}
	
	if (env->calendar_url) {
		/* We now create the ds server hat the given location */
		SmlLocation *loc = smlLocationNew(env->calendar_url, NULL, &serror);
		if (!loc)
			goto error_free_auth;
		
		env->calendarserver = smlDsServerNew(SML_ELEMENT_TEXT_VCAL, loc, &serror);
		if (!env->calendarserver)
			goto error_free_auth;
			
		if (!smlDsServerRegister(env->calendarserver, env->manager, &serror))
			goto error_free_auth;
		
		smlDsServerSetConnectCallback(env->calendarserver, _ds_alert, env);
		
		/* And we also add the devinfo to the devinf agent */
		SmlDevInfDataStore *datastore = smlDevInfDataStoreNew(smlLocationGetURI(loc), &serror);
		if (!datastore)
			goto error_free_auth;
		
		smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_VCAL, "1.0");
		smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_VCAL, "1.0");
		
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_TWO_WAY, TRUE);
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_SLOW_SYNC, TRUE);
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_SERVER_ALERTED_SYNC, TRUE);
		
		smlDevInfAddDataStore(devinf, datastore);
	}
	
	if (env->note_url) {
		/* We now create the ds server hat the given location */
		SmlLocation *loc = smlLocationNew(env->note_url, NULL, &serror);
		if (!loc)
			goto error_free_auth;
		
		env->noteserver = smlDsServerNew(SML_ELEMENT_TEXT_PLAIN, loc, &serror);
		if (!env->noteserver)
			goto error_free_auth;
			
		if (!smlDsServerRegister(env->noteserver, env->manager, &serror))
			goto error_free_auth;
		
		smlDsServerSetConnectCallback(env->noteserver, _ds_alert, env);
		
		/* And we also add the devinfo to the devinf agent */
		SmlDevInfDataStore *datastore = smlDevInfDataStoreNew(smlLocationGetURI(loc), &serror);
		if (!datastore)
			goto error_free_auth;
		
		smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_PLAIN, "1.0");
		smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_PLAIN, "1.0");
		
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_TWO_WAY, TRUE);
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_SLOW_SYNC, TRUE);
		smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_SERVER_ALERTED_SYNC, TRUE);
		
		smlDevInfAddDataStore(devinf, datastore);
	}
	
	/* Create the alert for the remote device */
	if (!env->identifier)
		env->identifier = g_strdup("LibSyncML Test Suite");
	
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


	SmlTransportObexClientConfig config;
	config.type = env->type;
	if (config.type == SML_OBEX_TYPE_BLUETOOTH) {
		if (!env->bluetoothAddress) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Bluetooth selected but no bluetooth address given");
			goto error_free_auth;
		}
		config.url = g_strdup(env->bluetoothAddress);
		config.port = env->bluetoothChannel;
	} else if (config.type == SML_OBEX_TYPE_USB)
		config.port = env->interface;
	else {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong obex type specified");
		goto error_free_auth;
	}
	
	/* Run the manager */
	if (!smlManagerStart(env->manager, &serror))
		goto error_free_auth;
	
	/* Initialize the Transport */
	if (!smlTransportInitialize(env->tsp, &config, &serror))
		goto error_free_auth;
	
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

static void client_connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	SmlNotification *san = NULL;
	
	env->tryDisconnect = FALSE;
	if (smlTransportGetType(env->tsp) == SML_TRANSPORT_OBEX_CLIENT) {
		/* For the obex client, we will store the context at this point since
		 * we can only answer it as soon as the device returned an answer to our san */
		env->connectCtx = ctx;
		
		/* Create the SAN */
		san = smlNotificationNew(env->version, SML_SAN_UIMODE_UNSPECIFIED, SML_SAN_INITIATOR_USER, 1, env->identifier, "/", env->useWbxml ? SML_MIMETYPE_WBXML : SML_MIMETYPE_XML, &error);
		if (!san)
			goto error;
		
		if (osync_member_objtype_enabled(env->member, "contact") && env->contactserver) {
			/* Then we add the alert to the SAN */
			if (!smlDsServerAddSan(env->contactserver, san, &error))
				goto error_free_san;
		}
		
		if ((osync_member_objtype_enabled(env->member, "event") || osync_member_objtype_enabled(env->member, "todo")) && env->calendarserver) {
			/* Then we add the alert to the SAN */
			if (!smlDsServerAddSan(env->calendarserver, san, &error))
				goto error_free_san;
		}
		
		if (osync_member_objtype_enabled(env->member, "note") && env->noteserver) {
			/* Then we add the alert to the SAN */
			if (!smlDsServerAddSan(env->noteserver, san, &error))
				goto error_free_san;
		}
		
		
		if (!smlTransportConnect(env->tsp, &error))
			goto error;
		
		if (!smlNotificationSend(san, env->tsp, &error))
			goto error_free_san;
		
		smlNotificationFree(san);
	} else if (smlTransportGetType(env->tsp) == SML_TRANSPORT_HTTP_SERVER) {
		
		/* For the http server we can report success right away since we know
		 * that we already received an alert (otherwise we could not have triggered
		 * the synchronization) */
		if (env->contactSession)
			smlDsSessionGetAlert(env->contactSession, _recv_alert, env);
			
		if (env->calendarSession)
			smlDsSessionGetAlert(env->calendarSession, _recv_alert, env);
			
		if (env->noteSession)
			smlDsSessionGetAlert(env->noteSession, _recv_alert, env);
		
		/* If we already received the final, we just report success. otherwise
		 * we let the final report success */
		if (env->gotFinal)
			osync_context_report_success(ctx);
		else
			env->connectCtx = ctx;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_san:
	smlNotificationFree(san);
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_context_report_osyncerror(ctx, &oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

static void get_changeinfo(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	env->getChangesCtx = ctx;
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	
	if (smlTransportGetType(env->tsp) == SML_TRANSPORT_OBEX_CLIENT) {
		if (env->contactSession)
			smlDsSessionGetAlert(env->contactSession, _recv_alert, env);
			
		if (env->calendarSession)
			smlDsSessionGetAlert(env->calendarSession, _recv_alert, env);
			
		if (env->noteSession)
			smlDsSessionGetAlert(env->noteSession, _recv_alert, env);
	}
	
	if (env->contactSession) {
		smlDsSessionGetSync(env->contactSession, _recv_sync, ctx);
		smlDsSessionGetChanges(env->contactSession, _recv_change, ctx);
	}
	
	if (env->calendarSession) {
		smlDsSessionGetSync(env->calendarSession, _recv_sync, ctx);
		smlDsSessionGetChanges(env->calendarSession, _recv_change, ctx);
	}
	
	if (env->noteSession) {
		smlDsSessionGetSync(env->noteSession, _recv_sync, ctx);
		smlDsSessionGetChanges(env->noteSession, _recv_change, ctx);
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

static void batch_commit(OSyncContext *ctx, OSyncContext **contexts, OSyncChange **changes)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, ctx, contexts, changes);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	int i = 0;
	int num = 0;
	
	env->commitCtx = ctx;
	
	int numContact = 0;
	int numCalendar = 0;
	int numNote = 0;
	
	for (i = 0; changes[i]; i++) {
		if (!strcmp(_format_to_contenttype(changes[i]), SML_ELEMENT_TEXT_VCARD))
			numContact++;
		else if (!strcmp(_format_to_contenttype(changes[i]), SML_ELEMENT_TEXT_VCAL))
			numCalendar++;
		else if (!strcmp(_format_to_contenttype(changes[i]), SML_ELEMENT_TEXT_PLAIN))
			numNote++;
		num++;
	}
	
	if (env->contactSession) {
		if (!smlDsSessionSendSync(env->contactSession, numContact, _recv_sync_reply, NULL, &error))
			goto error;

		for (i = 0; changes[i]; i++) {
			OSyncChange *change = changes[i];
			OSyncContext *context = contexts[i];
			
			if (!strcmp(_format_to_contenttype(changes[i]), SML_ELEMENT_TEXT_VCARD)) {
				osync_trace(TRACE_INTERNAL, "Uid: \"%s\", Format: \"%s\", Changetype: \"%i\"", osync_change_get_uid(change), osync_objtype_get_name(osync_change_get_objtype(change)), osync_change_get_changetype(change));
				
				struct commitContext *tracer = osync_try_malloc0(sizeof(struct commitContext), &oserror);
				if (!tracer)
					goto oserror;
				
				tracer->change = change;
				tracer->context = context;
	
			
				if (!smlDsSessionQueueChange(env->contactSession, _get_changetype(change), osync_change_get_uid(change), osync_change_get_data(change), osync_change_get_datasize(change), _format_to_contenttype(change), _recv_change_reply, tracer, &error))
					goto error;
				contexts[i] = NULL;
			}
		}
		
		if (!smlDsSessionCloseSync(env->contactSession, &error))
			goto error;
	}
	
	if (env->calendarSession) {
		if (!smlDsSessionSendSync(env->calendarSession, numCalendar, _recv_sync_reply, NULL, &error))
			goto error;

		for (i = 0; changes[i]; i++) {
			OSyncChange *change = changes[i];
			OSyncContext *context = contexts[i];
			
			if (!strcmp(_format_to_contenttype(changes[i]), SML_ELEMENT_TEXT_VCAL)) {
				osync_trace(TRACE_INTERNAL, "Uid: \"%s\", Format: \"%s\", Changetype: \"%i\"", osync_change_get_uid(change), osync_objtype_get_name(osync_change_get_objtype(change)), osync_change_get_changetype(change));
				
				struct commitContext *tracer = osync_try_malloc0(sizeof(struct commitContext), &oserror);
				if (!tracer)
					goto oserror;
				
				tracer->change = change;
				tracer->context = context;

				/* XXX Workaround for mobiles which only handle localtime! */
				if (env->onlyLocaltime && osync_change_get_changetype(change) != CHANGE_DELETED) {
					char *calentry = osync_time_vcal2localtime(osync_change_get_data(change));
					osync_change_free_data(change);
					osync_change_set_data(change, calentry, strlen(calentry), TRUE);
				}
	
				if (!smlDsSessionQueueChange(env->calendarSession, _get_changetype(change), osync_change_get_uid(change), osync_change_get_data(change), osync_change_get_datasize(change), _format_to_contenttype(change), _recv_change_reply, tracer, &error)) {
					goto error;
				}
				contexts[i] = NULL;

			}
		}
		
		if (!smlDsSessionCloseSync(env->calendarSession, &error))
			goto error;
	}
	
	if (env->noteSession) {
		if (!smlDsSessionSendSync(env->noteSession, numNote, _recv_sync_reply, NULL, &error))
			goto error;

		for (i = 0; changes[i]; i++) {
			OSyncChange *change = changes[i];
			OSyncContext *context = contexts[i];
			
			if (!strcmp(_format_to_contenttype(changes[i]), SML_ELEMENT_TEXT_PLAIN)) {
				osync_trace(TRACE_INTERNAL, "Uid: \"%s\", Format: \"%s\", Changetype: \"%i\"", osync_change_get_uid(change), osync_objtype_get_name(osync_change_get_objtype(change)), osync_change_get_changetype(change));
				
				struct commitContext *tracer = osync_try_malloc0(sizeof(struct commitContext), &oserror);
				if (!tracer)
					goto oserror;
				
				tracer->change = change;
				tracer->context = context;
	
			
				if (!smlDsSessionQueueChange(env->noteSession, _get_changetype(change), osync_change_get_uid(change), osync_change_get_data(change), osync_change_get_datasize(change), _format_to_contenttype(change), _recv_change_reply, tracer, &error))
					goto error;
				contexts[i] = NULL;
			}
		}
		
		if (!smlDsSessionCloseSync(env->noteSession, &error))
			goto error;
	}

	for (i = 0; i < num; i++) {
		if (contexts[i]) {
			osync_context_report_error(contexts[i], SML_ERROR_GENERIC, "content type was not configured");
		}
	}
	
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

/*static osync_bool _flush_batch(SmlPluginEnv *env, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, error);
	
	if ((!osync_member_objtype_enabled(env->member, "contact") || env->commitContactCtx) && \
		(!osync_member_objtype_enabled(env->member, "event") || env->commitCalendarCtx) && \
		(!osync_member_objtype_enabled(env->member, "todo") || env->commitTodoCtx) && \
		(!osync_member_objtype_enabled(env->member, "note") || env->commitNoteCtx)) {
			osync_trace(TRACE_EXIT, "%s: Flushing", __func__);
			return smlSessionFlush(env->session, TRUE, error);
	}
	osync_trace(TRACE_EXIT, "%s: Not flushing yet", __func__);
	return TRUE;
}

static void batch_commit_vcard(OSyncContext *ctx, OSyncContext **contexts, OSyncChange **changes)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, ctx, contexts, changes);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	
	env->commitContactCtx = ctx;
	
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

		if (!smlDsSessionQueueChange(env->contactSession, _get_changetype(change), osync_change_get_uid(change), osync_change_get_data(change), osync_change_get_datasize(change) - 1, _format_to_contenttype(change), _recv_change_reply, tracer, &error))
			goto error;
	}
	
	if (!smlDsSessionCloseSync(env->contactSession, &error))
		goto error;

	if (!_flush_batch(env, &error))
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

static void batch_commit_event(OSyncContext *ctx, OSyncContext **contexts, OSyncChange **changes)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, ctx, contexts, changes);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	int i = 0;
	
	env->commitCalendarCtx = ctx;
	
	for (i = 0; changes[i] && contexts[i]; i++) {
		OSyncChange *change = changes[i];
		OSyncContext *context = contexts[i];
		osync_trace(TRACE_INTERNAL, "Uid: \"%s\", Format: \"%s\", Changetype: \"%i\"", osync_change_get_uid(change), osync_objtype_get_name(osync_change_get_objtype(change)), osync_change_get_changetype(change));
		
		struct commitContext *tracer = osync_try_malloc0(sizeof(struct commitContext), &oserror);
		if (!tracer)
			goto oserror;
		
		tracer->change = change;
		tracer->context = context;
		
		env->eventEntries = g_list_append(env->eventEntries, tracer);
		env->numEventEntries++;
	}
	
	
	if (env->commitTodoCtx || !osync_member_objtype_enabled(env->member, "todo")) {
		if (!smlDsSessionSendSync(env->calendarSession, env->numEventEntries, _recv_sync_reply, NULL, &error))
		goto error;
	
		while (env->eventEntries) {
			struct commitContext *tracer = env->eventEntries->data;
			
			if (!smlDsSessionQueueChange(env->calendarSession, _get_changetype(tracer->change), osync_change_get_uid(tracer->change), osync_change_get_data(tracer->change), osync_change_get_datasize(tracer->change) - 1, _format_to_contenttype(tracer->change), _recv_change_reply, tracer, &error))
				goto error;
			
			env->eventEntries = g_list_delete_link(env->eventEntries, env->eventEntries);
		}
		
		if (!smlDsSessionCloseSync(env->calendarSession, &error))
			goto error;
	}
	
	if (!_flush_batch(env, &error))
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

static void batch_commit_todo(OSyncContext *ctx, OSyncContext **contexts, OSyncChange **changes)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, ctx, contexts, changes);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	int i = 0;
	
	env->commitTodoCtx = ctx;
	
	for (i = 0; changes[i] && contexts[i]; i++) {
		OSyncChange *change = changes[i];
		OSyncContext *context = contexts[i];
		osync_trace(TRACE_INTERNAL, "Uid: \"%s\", Format: \"%s\", Changetype: \"%i\"", osync_change_get_uid(change), osync_objtype_get_name(osync_change_get_objtype(change)), osync_change_get_changetype(change));
		
		struct commitContext *tracer = osync_try_malloc0(sizeof(struct commitContext), &oserror);
		if (!tracer)
			goto oserror;
		
		tracer->change = change;
		tracer->context = context;
		
		env->eventEntries = g_list_append(env->eventEntries, tracer);
		env->numEventEntries++;
	}
	
	
	if (env->commitCalendarCtx || !osync_member_objtype_enabled(env->member, "event")) {
		if (!smlDsSessionSendSync(env->calendarSession, env->numEventEntries, _recv_sync_reply, NULL, &error))
		goto error;
	
		while (env->eventEntries) {
			struct commitContext *tracer = env->eventEntries->data;
			
			if (!smlDsSessionQueueChange(env->calendarSession, _get_changetype(tracer->change), osync_change_get_uid(tracer->change), osync_change_get_data(tracer->change), osync_change_get_datasize(tracer->change) - 1, _format_to_contenttype(tracer->change), _recv_change_reply, tracer, &error))
				goto error;
			
			env->eventEntries = g_list_delete_link(env->eventEntries, env->eventEntries);
		}
		
		if (!smlDsSessionCloseSync(env->calendarSession, &error))
			goto error;
	}

	if (!_flush_batch(env, &error))
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

static void batch_commit_note(OSyncContext *ctx, OSyncContext **contexts, OSyncChange **changes)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, ctx, contexts, changes);
	SmlPluginEnv *env = (SmlPluginEnv *)osync_context_get_plugin_data(ctx);
	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	
	env->commitNoteCtx = ctx;
	
	int i = 0;
	for (i = 0; changes[i]; i++);
		
	if (!smlDsSessionSendSync(env->noteSession, i, _recv_sync_reply, NULL, &error))
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

		if (!smlDsSessionQueueChange(env->noteSession, _get_changetype(change), osync_change_get_uid(change), osync_change_get_data(change), osync_change_get_datasize(change) - 1, _format_to_contenttype(change), _recv_change_reply, tracer, &error))
			goto error;
	}
	
	if (!smlDsSessionCloseSync(env->noteSession, &error))
		goto error;

	if (!_flush_batch(env, &error))
		goto error;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
oserror:
	osync_context_report_osyncerror(ctx, &oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}*/

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
	
	env->gotFinal = FALSE;
	
	if (!smlSessionEnd(env->session, &error))
		goto error;
	
	if (env->gotDisconnect)
		osync_context_report_success(ctx);
	else
		env->disconnectCtx = ctx;
		
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
	
	info->name = "syncml-http-server";
	info->longname = "SyncML over HTTP Server";
	info->description = "Plugin to synchronize with SyncML over HTTP";
	
	info->functions.initialize = syncml_http_server_init;
	info->functions.connect = client_connect;
	info->functions.sync_done = sync_done;
	info->functions.disconnect = disconnect;
	info->functions.finalize = finalize;
	info->functions.get_changeinfo = get_changeinfo;

	info->timeouts.disconnect_timeout = 0;
	info->timeouts.connect_timeout = 0;
	info->timeouts.sync_done_timeout = 0;
	info->timeouts.get_changeinfo_timeout = 0;
	info->timeouts.get_data_timeout = 0;
	info->timeouts.commit_timeout = 0;
		
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard21", "clean");
	osync_plugin_set_batch_commit_objformat(info, "contact", "vcard21", batch_commit);
	
	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "vevent10", "clean");
	osync_plugin_set_batch_commit_objformat(info, "event", "vevent10", batch_commit);
	
	osync_plugin_accept_objtype(info, "todo");
	osync_plugin_accept_objformat(info, "todo", "vtodo10", "clean");
	osync_plugin_set_batch_commit_objformat(info, "todo", "vtodo10", batch_commit);
	
	osync_plugin_accept_objtype(info, "note");
	osync_plugin_accept_objformat(info, "note", "memo", "clean");
	osync_plugin_set_batch_commit_objformat(info, "note", "memo", batch_commit);
	
	info = osync_plugin_new_info(env);
	
	info->name = "syncml-obex-client";
	info->longname = "SyncML over OBEX Client";
	info->description = "Plugin to synchronize with SyncML over OBEX";
	
	info->functions.initialize = syncml_obex_client_init;
	info->functions.connect = client_connect;
	info->functions.sync_done = sync_done;
	info->functions.disconnect = disconnect;
	info->functions.finalize = finalize;
	info->functions.get_changeinfo = get_changeinfo;

	info->timeouts.disconnect_timeout = 0;
	info->timeouts.connect_timeout = 0;
	info->timeouts.sync_done_timeout = 0;
	info->timeouts.get_changeinfo_timeout = 0;
	info->timeouts.get_data_timeout = 0;
	info->timeouts.commit_timeout = 0;
	
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard21", "clean");
	osync_plugin_set_batch_commit_objformat(info, "contact", "vcard21", batch_commit);
	
	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "vevent10", "clean");
	osync_plugin_set_batch_commit_objformat(info, "event", "vevent10", batch_commit);
	
	osync_plugin_accept_objtype(info, "todo");
	osync_plugin_accept_objformat(info, "todo", "vtodo10", "clean");
	osync_plugin_set_batch_commit_objformat(info, "todo", "vtodo10", batch_commit);
	
	osync_plugin_accept_objtype(info, "note");
	osync_plugin_accept_objformat(info, "note", "memo", "clean");
	osync_plugin_set_batch_commit_objformat(info, "note", "memo", batch_commit);
}
