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

static void syncml_free_database(SmlDatabase *database)
{
	if (database->url)
		g_free(database->url);

	if (database->objtype)
		g_free(database->objtype);

	if (database->objformat_name)
		g_free(database->objformat_name);

	if (database->sink)
		osync_objtype_sink_unref(database->sink);

	g_free(database);
}

static SmlChangeType _get_changetype(OSyncChange *change)
{
	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_ADDED:
			return SML_CHANGE_ADD;
		case OSYNC_CHANGE_TYPE_MODIFIED:
			return SML_CHANGE_REPLACE;
		case OSYNC_CHANGE_TYPE_DELETED:
			return SML_CHANGE_DELETE;
		default:
			;
	}
	return SML_CHANGE_UNKNOWN;
}

extern const char *_objtype_to_contenttype(const char *objtype)
{
	if (!strcmp(objtype, "contact")) {
		return SML_ELEMENT_TEXT_VCARD;
	}
	if (!strcmp(objtype, "event")) {
		return SML_ELEMENT_TEXT_VCAL;
	}
	if (!strcmp(objtype, "todo")) {
		return SML_ELEMENT_TEXT_VCAL;
	}
	if (!strcmp(objtype, "note")) {
		return SML_ELEMENT_TEXT_PLAIN;
	}
	if (!strcmp(objtype, "data")) {
		return SML_ELEMENT_TEXT_PLAIN;
	}
	return NULL;
}

static const char *_format_to_contenttype(OSyncChange *change)
{
	return _objtype_to_contenttype(osync_change_get_objtype(change));
}

osync_bool syncml_config_parse_database(SmlPluginEnv *env, xmlNode *cur, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, cur, error);

	SmlDatabase *database = osync_try_malloc0(sizeof(SmlDatabase), error);
	if (!database)
		goto error;

	database->env = env;

        while (cur != NULL) {
                char *str = (char*)xmlNodeGetContent(cur);
                if (str) {
                        if (!xmlStrcmp(cur->name, (const xmlChar *)"name")) {
                                database->url = g_strdup(str);
                        } else if (!xmlStrcmp(cur->name, (const xmlChar *)"objtype")) {
				database->objtype = g_strdup(str);
                        } else if (!xmlStrcmp(cur->name, (const xmlChar *)"objformat")) {
				database->objformat_name = g_strdup(str);;
			}

                        xmlFree(str);
                }
                cur = cur->next;
        }

        if (!database->url) {
                osync_error_set(error, OSYNC_ERROR_GENERIC, "Database name not set");
                goto error_free_database;
        }

        if (!database->objtype) {
                osync_error_set(error, OSYNC_ERROR_GENERIC, "\"objtype\" of a database not set");
                goto error_free_database;
        }

        if (!database->objformat_name) {
                osync_error_set(error, OSYNC_ERROR_GENERIC, "Object Fomrat \"%s\" of a database not set", database->objformat_name);
                goto error_free_database;
        }

        env->databases = g_list_append(env->databases, database);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_database:
	syncml_free_database(database);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static const char *_contenttype_to_format(const char *contenttype)
{
	if (!strcmp(contenttype, SML_ELEMENT_TEXT_VCARD)) {
		return "contact";
	}
	if (!strcmp(contenttype, SML_ELEMENT_TEXT_VCAL)) {
		return "event";
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
			return OSYNC_CHANGE_TYPE_ADDED;
		case SML_CHANGE_REPLACE:
			return OSYNC_CHANGE_TYPE_MODIFIED;
		case SML_CHANGE_DELETE:
			return OSYNC_CHANGE_TYPE_DELETED;
		default:
			;	
	}
	return OSYNC_CHANGE_TYPE_UNKNOWN;
}

static void _ds_event(SmlDsSession *dsession, SmlDsEvent event, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, dsession, event, userdata);

	SmlDatabase *database = (SmlDatabase *)userdata;

	osync_trace(TRACE_INTERNAL, "database: %s", database->objtype);
	switch (event) {
		case SML_DS_EVENT_GOTCHANGES:
			database->gotChanges = TRUE;
			if (database->gotChanges && database->finalChanges) {
				osync_trace(TRACE_INTERNAL,"getChangesCtx report success at _recv_change");
				osync_context_report_success(database->getChangesCtx);
				database->getChangesCtx = NULL;
			}
			break;
		case SML_DS_EVENT_COMMITEDCHANGES:	
			break;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
}

static SmlBool _recv_change(SmlDsSession *dsession, SmlChangeType type, const char *uid, char *data, unsigned int size, const char *contenttype, void *userdata, SmlError **smlerror)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %p, %i, %s, %p, %p)", __func__, dsession, type, uid, data, size, contenttype, userdata, smlerror);
	SmlDatabase *database = (SmlDatabase *)userdata;
	OSyncError *error = NULL;
	g_assert(database->getChangesCtx);

	if (!type) {
		osync_context_report_success(database->getChangesCtx);
		osync_trace(TRACE_EXIT, "%s", __func__);
		return TRUE;
	}

	OSyncChange *change = osync_change_new(&error);
	if (!change) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "No change created: %s", osync_error_print(&error));
		goto error;
	}
	
//		osync_change_set_member(change, env->member);

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

		/*
		if (!strcmp(contenttype, SML_ELEMENT_TEXT_VCARD))
			objformat = g_strdup("plain");
		else if (!strcmp(contenttype, SML_ELEMENT_TEXT_VCAL))
			objformat = g_strdup("plain");
		else if (!strcmp(contenttype, SML_ELEMENT_TEXT_PLAIN))
			objformat = g_strdup("memo");
			*/

	}

	/* XXX Workaround for mobiles which only handle localtime! TODO: make use of UTC field in DevCap and handle it is OpenSync framework! */
	/*
	if (!strcmp(contenttype, SML_ELEMENT_TEXT_VCAL) && env->onlyLocaltime && type != SML_CHANGE_DELETE) {
		char *_data = osync_time_vcal2utc(data);
		g_free(data);
		data = _data;
		size = strlen(data);
	}
	*/

	OSyncData *odata = osync_data_new(data, size+1, database->objformat, &error);
	if (!odata) {
		osync_change_unref(change);
		goto error;
	}


//		if (_to_osync_changetype(type) == OSYNC_CHANGE_TYPE_DELETED)
	if(contenttype)
		osync_data_set_objtype(odata, _contenttype_to_format(contenttype));
	else
		osync_data_set_objtype(odata, database->objtype);


	osync_change_set_data(change, odata);
	osync_change_set_changetype(change, _to_osync_changetype(type));

	osync_data_unref(odata);

	osync_context_report_change(database->getChangesCtx, change);

	osync_change_unref(change);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
//	osync_context_report_osyncwarning(ctx, error);
	osync_error_unref(&error);

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
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
	SmlDatabase *database = (SmlDatabase *)userdata;
	
	osync_trace(TRACE_INTERNAL,"Going to receive %i changes - objtype: %s", numchanges, database->objtype);
	printf("Going to receive %i changes\n", numchanges);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _recv_alert_reply(SmlSession *session, SmlStatus *status, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, session, status, userdata);
	
	SmlDatabase *database = (SmlDatabase*) userdata;

	osync_trace(TRACE_INTERNAL, "Received an reply to our Alert - %s\n", database->objtype);
	

	osync_trace(TRACE_EXIT, "%s", __func__);
}

extern SmlBool _recv_alert(SmlDsSession *dsession, SmlAlertType type, const char *last, const char *next, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %s, %p)", __func__, dsession, type, last, next, userdata);
	SmlDatabase *database = (SmlDatabase*) userdata;
	SmlPluginEnv *env = database->env;
	SmlBool ret = TRUE;
	
	char *key = g_strdup_printf("remoteanchor%s", smlDsSessionGetLocation(dsession));

	if ((!last || !osync_anchor_compare(env->anchor_path, key, last)) && type == SML_ALERT_TWO_WAY)
		ret = FALSE;
	
	osync_bool ans = osync_objtype_sink_get_slowsync(database->sink);
	if (ans)
		ret = FALSE;
	
	if (!ret || type != SML_ALERT_TWO_WAY)
		osync_objtype_sink_set_slowsync(database->sink, TRUE);
	
	osync_anchor_update(env->anchor_path, key, next);
	g_free(key);
	
	if (!ret) {
		smlDsSessionSendAlert(dsession, SML_ALERT_SLOW_SYNC, last, next, _recv_alert_reply, database, NULL);
	} else {
		smlDsSessionSendAlert(dsession, SML_ALERT_TWO_WAY, last, next, _recv_alert_reply, database, NULL);
	}
	
	smlDevInfAgentGetDevInf(env->agent);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

extern void _ds_alert(SmlDsSession *dsession, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, dsession, userdata);

	SmlDatabase *database = (SmlDatabase *)userdata;

	database->session = dsession;
	smlDsSessionRef(dsession);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

extern void _manager_event(SmlManager *manager, SmlManagerEventType type, SmlSession *session, SmlError *error, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p)", __func__, manager, type, session, error, userdata);
	SmlPluginEnv *env = userdata;
	GList *o = NULL;

	switch (type) {
		case SML_MANAGER_SESSION_FLUSH:
		case SML_MANAGER_CONNECT_DONE:
			env->gotDisconnect = FALSE;
			break;
		case SML_MANAGER_DISCONNECT_DONE:
			osync_trace(TRACE_INTERNAL, "connection with device has ended");
			env->gotDisconnect = TRUE;
			
			o = env->databases;
			for (; o; o = o->next) {
				SmlDatabase *database = o->data;

				if (database->disconnectCtx) {
					osync_context_report_success(database->disconnectCtx);
					database->disconnectCtx = NULL;
				}
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

			o = env->databases;
			for (; o; o = o->next) {
				SmlDatabase *database = o->data;

				osync_trace(TRACE_INTERNAL, "gotChanges: %i getChangesCtx: %p objtype: %s",
					database->gotChanges, database->getChangesCtx, database->objtype);

				if (database->getChangesCtx) {
					database->finalChanges = TRUE;
					if (database->gotChanges && database->finalChanges) {
						osync_trace(TRACE_INTERNAL,"getChangesCtx report success at final");
						osync_context_report_success(database->getChangesCtx);
						database->getChangesCtx = NULL;
					}

				}


				if (database->commitCtx) {
					osync_context_report_success(database->commitCtx);
					database->commitCtx = NULL;
				}
			

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
		osync_context_report_osyncerror(env->connectCtx, oserror);
		env->connectCtx = NULL;
	}

	
	o = env->databases;
	for (; o; o = o->next) {
		SmlDatabase *database = o->data;

		
		if (database->getChangesCtx) {
			osync_context_report_osyncerror(database->getChangesCtx, oserror);
			database->getChangesCtx = NULL;
		}

		if (database->commitCtx) {
			osync_context_report_osyncerror(database->commitCtx, oserror);
			database->commitCtx = NULL;
		}

		if (database->disconnectCtx) {
			osync_context_report_osyncerror(database->disconnectCtx, oserror);
			database->disconnectCtx = NULL;
		}

		
	}

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

extern gboolean _sessions_prepare(GSource *source, gint *timeout_)
{
	*timeout_ = 50;
	return FALSE;
}

extern gboolean _sessions_check(GSource *source)
{
	SmlPluginEnv *env = *((SmlPluginEnv **)(source + 1));

	GList *o = env->databases;
	for (; o; o = o->next) {
		SmlDatabase *database = o->data;
		if (database->session && smlDsSessionCheck(database->session))
			return TRUE;
	}
		
	if (smlManagerCheck(env->manager))
		return TRUE;
		
	return FALSE;
}

extern gboolean _sessions_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
	SmlPluginEnv *env = user_data;
	
	GList *o = env->databases;
	for (; o; o = o->next) {
		SmlDatabase *database = o->data;
		if (database->session)
			smlDsSessionDispatch(database->session);
	}

	smlManagerDispatch(env->manager);
	return TRUE;
}

extern void _verify_user(SmlAuthenticator *auth, const char *username, const char *password, void *userdata, SmlErrorType *reply)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s, %p, %p)", __func__, auth, username, password, userdata, reply);
	SmlPluginEnv *env = userdata;
	
	osync_trace(TRACE_SENSITIVE, "configured is %s, %s", env->username, env->password);
	if (env->username && (!env->password || !username || !password || strcmp(env->username, username) || strcmp(env->password, password))) {
		*reply = SML_ERROR_AUTH_REJECTED;
	} else {
		*reply = SML_AUTH_ACCEPTED;
	}
	osync_trace(TRACE_EXIT, "%s: %i", __func__, *reply);
}

extern void get_changeinfo(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)data;

	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	SmlDatabase *database = osync_objtype_sink_get_userdata(sink);

	database->getChangesCtx = ctx;
	osync_context_ref(database->getChangesCtx);

	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	
	if (smlTransportGetType(env->tsp) == SML_TRANSPORT_OBEX_CLIENT) {
		smlDsSessionGetAlert(database->session, _recv_alert, database);
	}
	
	smlDsSessionGetEvent(database->session, _ds_event, database);
	smlDsSessionGetSync(database->session, _recv_sync, database);
	smlDsSessionGetChanges(database->session, _recv_change, database);

	// avoid flushing to early
	env->num++;

	if (env->num >= g_list_length(env->databases)) {
		env->num = 0;
		if (!smlSessionFlush(env->session, TRUE, &error))
			goto error;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_context_report_osyncerror(ctx, oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

extern void sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_context_report_success(ctx);
}

extern void disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)data;

	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	SmlDatabase *database = osync_objtype_sink_get_userdata(sink);

	OSyncError *oserror = NULL;
	SmlError *error = NULL;
	
	env->gotFinal = FALSE;
	
	if (!smlSessionEnd(env->session, &error))
		goto error;
	
	if (env->gotDisconnect) {
		osync_context_report_success(ctx);
	} else {
		database->disconnectCtx = ctx;
		osync_context_ref(database->disconnectCtx);
	}


	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_context_report_osyncerror(ctx, oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

extern void finalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	SmlPluginEnv *env = (SmlPluginEnv *)data;
	
	/* Stop the manager */
	if (env->manager)
		smlManagerStop(env->manager);
	
	if (env->tsp)
		smlTransportFinalize(env->tsp, NULL);
	
	if (env->tsp)
		smlTransportFree(env->tsp);

	if (env->san)
		smlNotificationFree(env->san);

	if (env->identifier)
		g_free(env->identifier);

	if (env->username)
		g_free(env->username);

	if (env->password)
		g_free(env->password);

	if (env->bluetoothAddress)
		g_free(env->bluetoothAddress);

	if (env->url)
		g_free(env->url);

	if (env->anchor_path)
		g_free(env->anchor_path);

	if (env->source) {
		g_source_destroy(env->source);
		g_source_unref(env->source);
		g_free(env->source_functions);
	}

	while (env->databases) {
		SmlDatabase *db = env->databases->data;
		syncml_free_database(db);

		env->databases = g_list_remove(env->databases, db);
	}
	
	g_free(env);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

extern void batch_commit(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncContext **contexts, OSyncChange **changes)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, ctx, contexts, changes);
	SmlPluginEnv *env = (SmlPluginEnv *)data;

	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	SmlDatabase *database = osync_objtype_sink_get_userdata(sink);

	SmlError *error = NULL;
	OSyncError *oserror = NULL;
	int i = 0;
	int num = 0;
	
	database->commitCtx = ctx;
	osync_context_ref(database->commitCtx);
	
	for (i = 0; changes[i]; i++) {
		/*
		if (!strcmp(_format_to_contenttype(changes[i]), SML_ELEMENT_TEXT_VCARD))
			numContact++;
		else if (!strcmp(_format_to_contenttype(changes[i]), SML_ELEMENT_TEXT_VCAL))
			numCalendar++;
		else if (!strcmp(_format_to_contenttype(changes[i]), SML_ELEMENT_TEXT_PLAIN))
			numNote++;
		*/	
		num++;
	}
	
	if (!smlDsSessionSendSync(database->session, num, _recv_sync_reply, NULL, &error))
		goto error;

	for (i = 0; changes[i]; i++) {
		OSyncChange *change = changes[i];
		OSyncContext *context = contexts[i];
		
		osync_trace(TRACE_INTERNAL, "Uid: \"%s\", Format: \"%s\", Changetype: \"%i\"", osync_change_get_uid(change), osync_change_get_objtype(change), osync_change_get_changetype(change));
		
		struct commitContext *tracer = osync_try_malloc0(sizeof(struct commitContext), &oserror);
		if (!tracer)
			goto oserror;
		
		tracer->change = change;
		tracer->context = context;

		OSyncData *data = osync_change_get_data(change);
		char *buf = NULL;
		unsigned int size = 0;
		osync_data_get_data(data, &buf, &size);
	
		osync_trace(TRACE_INTERNAL, "Committing entry \"%s\": \"%s\"", osync_change_get_uid(change), buf);
		if (!smlDsSessionQueueChange(database->session, _get_changetype(change), osync_change_get_uid(change), buf, size, _format_to_contenttype(change), _recv_change_reply, tracer, &error))
			goto error;
		//contexts[i] = NULL;

		//g_free(buf);
	}
	
	if (!smlDsSessionCloseSync(database->session, &error))
		goto error;

/*
	for (i = 0; i < num; i++) {
		if (contexts[i]) {
			osync_context_report_error(contexts[i], SML_ERROR_GENERIC, "content type was not configured");
		}
	}
*/
	env->num++;

	// avoid flushing to early...
	if(env->num >= g_list_length(env->databases))
		if (!smlSessionFlush(env->session, TRUE, &error))
			goto error;


	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
oserror:
	osync_context_report_osyncerror(ctx, oserror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}
