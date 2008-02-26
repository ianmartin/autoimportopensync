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
#include "syncml_callbacks.h"
#include "syncml_devinf.h"

void set_session_user(SmlPluginEnv *env, const char* user)
{
    g_assert(user);

    if (env->sessionUser == NULL ||
        strcmp(env->sessionUser, user))
    {
        // the session user (function called by OpenSync) changed
        // so we have to cleanup the environment
        g_list_free(env->ignoredDatabases);
        env->ignoredDatabases = NULL;
    }

    if (env->sessionUser)
        safe_cfree(&(env->sessionUser));
    env->sessionUser = g_strdup(user);
}

GList *g_list_add(GList *databases, void *database)
{
    osync_trace(TRACE_ENTRY, "%s", __func__);

    // if we find the item in the list
    // then we only return and do nothing

    if (g_list_find(databases, database) != NULL)
    {
        osync_trace(TRACE_EXIT, "%s - the item is an element of the list", __func__);
        return databases;
    }

    // add the item to the list

    GList *result = g_list_append(databases, database);
    osync_trace(TRACE_EXIT, "%s - add a new list item %p", __func__, database);
    return result;
}

/* General notice about libsyncml usage:
 *
 * Clients are the active part in a SyncML session. They initiate all
 * actions. They send/request DevInf, send alerts for synchronization,
 * starts sync and send maps. So it is a good idea to flush actively
 * during these actions.
 *
 * Servers have a more passive way of communication. They usually react
 * on actions of the client. So it is not necessary to flush actively.
 * libsyncml flushes automatically after it manages the complete
 * received body of a SyncML message.
 *
 * NumberOfChanges is supported by some mobiles but we cannot rely on it
 * because some mobiles like SE M600i say that they support it but they
 * do not send it :(
 *
 * If this is a client then flush should be executed after:
 *     - DevInf preparation
 *     - alerting sync for all databases
 *     - called sync for all databases
 *     - sent modifications for all databases
 *     - sent map for all databases
 *
 * If this is a server then flush should be executed after:
 *     - alerting sync for all databases
 *     - sent modifications for all databases
 *
 * The logic behind this is simple. If we do something as a response to
 * a request then the response is automatically flushed from libsyncml.
 * If we do something on our own which is not a direct response then a
 * flush is required.
 *
 * Please note that sometimes we have to wait for the final event to
 * start the next phase. The final event will be send on a per DsSession
 * base (Data Synchronization Session not SyncML session).
 *
 */
SmlBool flush_session_for_all_databases(
			SmlPluginEnv *env,
			SmlBool activeDatabase,
			SmlError **error)
{
    // avoid flushing to early

    osync_trace(TRACE_ENTRY, "%s", __func__);
    g_assert(env);

    if (activeDatabase) env->num++;
    osync_trace(TRACE_INTERNAL, "%s - flush: %i, ignore: %i",
                __func__, env->num, g_list_length(env->ignoredDatabases));
    if ( (
          env->num != 0 ||
          smlDsServerGetServerType(((SmlDatabase *) env->databases->data)->server) == SML_DS_SERVER
         )
        &&
        env->num + g_list_length(env->ignoredDatabases) >= g_list_length(env->databases))
    {
	/* reset the flush database counter */
        env->num = 0;

	/* reset the map flushing of OMA DS clients */
	if (env->prepareMapFlushing)
		env->prepareMapFlushing = FALSE;

	/* perform the flush */
        if (!smlSessionFlush(env->session, TRUE, error))
        {
            osync_trace(TRACE_EXIT_ERROR, "%s - session flush failed", __func__);
            return FALSE;
        }
        else
        {
            osync_trace(TRACE_EXIT, "%s - session flush succeeded", __func__);
            return TRUE;
        }
    }
    osync_trace(TRACE_EXIT, "%s - session flush delayed", __func__);
    return TRUE;
}

void syncml_free_database(SmlDatabase *database)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, database);

	// cleanup libsyncml stuff

	if (database->session) {
		smlDsSessionUnref(database->session);
		database->session = NULL;
	}
	if (database->server) {
		smlDsServerFree(database->server);
		database->server = NULL;
	}
	osync_trace(TRACE_INTERNAL, "%s - libsyncml cleaned", __func__);

	// cleanup configuration stuff

	if (database->url)
		safe_cfree(&(database->url));
	if (database->objtype)
		safe_cfree(&(database->objtype));
	if (database->objformat_name)
		safe_cfree(&(database->objformat_name));
	if (database->objformat) {
		osync_objformat_unref(database->objformat);
		database->objformat = NULL;
	}
	if (database->sink) {
		osync_objtype_sink_unref(database->sink);
		database->sink = NULL;
	}
	osync_trace(TRACE_INTERNAL, "%s - configuration cleaned", __func__);

	// cleanup contexts
	// if something is present here then it must be failed
	// because this is a software bug

	while (database->syncChanges != NULL && database->syncChanges[0] != NULL) {
		osync_trace(TRACE_ERROR, "%s: detected old change", __func__);
		osync_change_unref(database->syncChanges[0]);
		database->syncChanges[0] = NULL;
		database->syncChanges = &(database->syncChanges[1]);
	}
	if (database->syncChanges != NULL) {
		osync_trace(TRACE_ERROR, "%s: detected old change array", __func__);
		safe_free((void **) &(database->syncChanges));
	}
	while (database->syncContexts != NULL && database->syncContexts[0] != NULL) {
		OSyncError **error = NULL;
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s - context discovered on finalize", __func__);
		report_error_on_context(&(database->syncContexts[0]), error, TRUE);
		database->syncContexts = &(database->syncContexts[1]);
	}
	if (database->syncContexts != NULL) {
		osync_trace(TRACE_ERROR, "%s: detected old change context array", __func__);
		safe_free((void **) &(database->syncContexts));
	}

	if (database->syncModeCtx) {
		OSyncError **error = NULL;
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s - syncModeCtx context discovered on finalize", __func__);
		report_error_on_context(&(database->syncModeCtx), error, TRUE);
	}
	if (database->getChangesCtx) {
		OSyncError **error = NULL;
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s - getChangesCtx context discovered on finalize", __func__);
		report_error_on_context(&(database->getChangesCtx), error, TRUE);
	}
	if (database->commitCtx) {
		OSyncError **error = NULL;
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s - commitCtx context discovered on finalize", __func__);
		report_error_on_context(&(database->commitCtx), error, TRUE);
	}
	osync_trace(TRACE_INTERNAL, "%s - contexts cleaned", __func__);

	// free the database struct itself

	safe_free((gpointer *)&database);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

SmlBool _init_change_ctx_cleanup(SmlDatabase *database, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(gotChanges: %i, gotFinal %i)", __func__,
		 database->gotChanges, database->env->gotFinal);

	g_assert(database->getChangesCtx);
	
	// we should try to cleanup getChangesCtx if
	//     SML_MANAGER_SESSION_FINAL
	//     SML_DS_EVENT_GOTCHANGES
	// both events are required for a complete cleanup
	// gotFinal must be resetted at every new message
	if (database->gotChanges && database->env->gotFinal) {
		/* Cleanup of contexts / Reporting to OpenSync:
		 * 
		 * SERVER: The next step after receiving the
		 *         package with the changes from the
		 *         client is to send the package with
		 *         the calculated changes. This is
		 *         done by batch_commit. Therefore it
		 *         is required to report the success
		 *         on the change context.
		 *
		 * CLIENT: If the client received the changes
		 *         from the server then the client must
		 *         send the map and have to wait for
		 *         the acknowledgement from the server
		 *         for the map. After this the client
		 *         can disconnect and re-connect to
		 *         send the calculated changes of
		 *         OpenSync. If the client wants to
		 *         avoid to early calls of OpenSync to
		 *         the function batch_commit then the
		 *         client should report success only
		 *         after disconnect.
		 */
		if (smlDsServerGetServerType(database->server) == SML_DS_SERVER)
		{
			osync_trace(TRACE_INTERNAL, "%s: reported success on server change context.", __func__);
			report_success_on_context(&(database->getChangesCtx));
		} else {
			osync_trace(TRACE_INTERNAL, "%s: flushing the map of a client.", __func__);

			/* Flush handling:
			 *
			 * SERVER: An explicit flush is not necessary
			 *         because the server replies
			 *         automatically with its own changes if
			 *         the success is reported via the
			 *         change context.
			 *
			 * CLIENT: An explicit flush is necessary
			 *         because the client prepares the map
			 *         which must be send to the server.
			 *         This is required to complete the DS
			 *         session correctly.
			 *
			 * The maps are only flushed if this is a client and all
			 * changes were received (because FINAL is set).
			 */
			if (database->env->prepareMapFlushing &&
			    !flush_session_for_all_databases(database->env, TRUE, error))
			{
				osync_trace(TRACE_EXIT_ERROR, "%s: Flushing the map failed.", __func__);
				return FALSE;
			}
		}
	}

	osync_trace(TRACE_EXIT, "%s - success", __func__);
	return TRUE;
}

void _init_commit_ctx_cleanup(SmlDatabase *database)
{
	osync_trace(TRACE_ENTRY, "%s(commitWrite: %i, gotFinal %i)", __func__,
		 database->commitWrite, database->env->gotFinal);

	g_assert(database->commitCtx);

	// we should try to cleanup commitCtx if
	//     SML_MANAGER_SESSION_FINAL
	//     SML_DS_EVENT_COMMITEDCHANGES
	// both events are required for a complete cleanup
	// gotFinal must be resetted at every new message
	// until we received a sync command (not alert)
	if (database->env->gotFinal && database->commitWrite && !database->pendingCommits) {
		osync_trace(TRACE_INTERNAL, "%s: reported success on server commit context.", __func__);
		report_success_on_context(&(database->commitCtx));
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
}

SmlChangeType _get_changetype(OSyncChange *change)
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

osync_bool syncml_config_parse_database(SmlPluginEnv *env, xmlNode *cur, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, cur, error);
	g_assert(env);
	g_assert(cur);

	SmlDatabase *database = osync_try_malloc0(sizeof(SmlDatabase), error);
	if (!database)
		goto error;

	database->env = env;
	database->syncChanges = NULL;
	database->syncContexts = NULL;

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

OSyncChangeType _to_osync_changetype(SmlChangeType type)
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

gboolean _sessions_prepare(GSource *source, gint *timeout_)
{
	*timeout_ = 50;
	return FALSE;
}

gboolean _sessions_check(GSource *source)
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

gboolean _sessions_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
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

void sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	report_success_on_context(&ctx);
}

void disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)data;
	g_assert(env);

	OSyncError *oserror = NULL;
	SmlError *error = NULL;
	
	env->gotFinal = FALSE;

	if (env->gotDisconnect)
	{
		/* The disconnect already happened. */
		report_success_on_context(&ctx);
	} else {
		/* It is necessary to place the context at the right position before
		 * calling a function of libsyncml because the library can send signals
		 * very fast if there is nothing to do (e.g. no status/command is
		 * waiting for a flush).
		 */
		env->disconnectCtx = ctx;
		osync_context_ref(env->disconnectCtx);

		if (!smlSessionEnd(env->session, &error))
			goto error;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	if (env->disconnectCtx)
		report_error_on_context(&(env->disconnectCtx), &oserror, TRUE);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

void finalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	SmlPluginEnv *env = (SmlPluginEnv *)data;

	/* glib stuff */
	/* It is necessary to stop the glib dispatching or
	 * otherwise it can happen that the manager is called
	 * when it is already cleaned up.
	 */

	if (env->source) {
		g_source_destroy(env->source);
		g_source_unref(env->source);
	}
	if (env->source_functions)
		safe_free((gpointer *)&(env->source_functions));
	if (env->context) {
		g_main_context_unref(env->context);
		env->context = NULL;
	}
	if (env->managerMutex) {
		g_mutex_free(env->managerMutex);
		env->managerMutex = NULL;
	}
	osync_trace(TRACE_INTERNAL, "%s - glib cleaned", __func__);

	/* Stop the manager */

	if (env->manager)
		smlManagerStop(env->manager);
	osync_trace(TRACE_INTERNAL, "%s - manager stopped", __func__);

	/* Cleanup the libsyncml library */

	if (env->tsp) {
		smlTransportFinalize(env->tsp, NULL);
	}
	if (env->manager) {
		/* The manager needs a transport.
		 * So never free the transport before the manager.
		 */
		smlManagerFree(env->manager);
		env->manager = NULL;
	}
	if (env->tsp) {
		smlTransportFree(env->tsp);
		env->tsp = NULL;
	}
	if (env->san) {
		smlNotificationFree(env->san);
		env->san = NULL;
	}
	if (env->devinf) {
		smlDevInfUnref(env->devinf);
		env->devinf = NULL;
	}
	if (env->remote_devinf) {
		smlDevInfUnref(env->remote_devinf);
		env->remote_devinf = NULL;
	}
	if (env->session) {
		smlSessionUnref(env->session);
		env->session = NULL;
	}
	if (env->agent) {
		smlDevInfAgentFree(env->agent);
		env->agent = NULL;
	}
	if (env->auth) {
		smlAuthFree(env->auth);
		env->auth = NULL;
	}
	while (env->databases) {
		SmlDatabase *db = env->databases->data;
		syncml_free_database(db);
		env->databases = g_list_remove(env->databases, db);
	}
	osync_trace(TRACE_INTERNAL, "%s - libsyncml cleaned", __func__);

	/* cleanup the configuration */

	if (env->identifier)
		safe_cfree(&(env->identifier));
	if (env->username)
		safe_cfree(&(env->username));
	if (env->password)
		safe_cfree(&(env->password));
	if (env->bluetoothAddress)
		safe_cfree(&(env->bluetoothAddress));
	if (env->bluetoothChannel)
		safe_cfree(&(env->bluetoothChannel));
	if (env->url)
		safe_cfree(&(env->url));
	if (env->port)
		safe_cfree(&(env->port));
	if (env->proxy)
		safe_cfree(&(env->proxy));
	if (env->cafile)
		safe_cfree(&(env->cafile));
	if (env->fakeManufacturer)
		safe_cfree(&(env->fakeManufacturer));
	if (env->fakeModel)
		safe_cfree(&(env->fakeModel));
	if (env->fakeSoftwareVersion)
		safe_cfree(&(env->fakeSoftwareVersion));
	osync_trace(TRACE_INTERNAL, "%s - libsyncml configuration cleaned", __func__);

	/* plugin config */

	if (env->anchor_path)
		safe_cfree(&(env->anchor_path));
	if (env->devinf_path)
		safe_cfree(&(env->devinf_path));
	if (env->sessionUser)
		safe_cfree(&(env->sessionUser));
	if (env->ignoredDatabases) {
		g_list_free(env->ignoredDatabases);
		env->ignoredDatabases = NULL;
	}
	osync_trace(TRACE_INTERNAL, "%s - plugin configuration cleaned", __func__);

	/* Signal forgotten contexts */

	if (env->connectCtx) {
		OSyncError **error = NULL;
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s - detected forgotten connect context", __func__);
		report_error_on_context(&(env->connectCtx), error, TRUE);
	}
	if (env->disconnectCtx) {
		OSyncError **error = NULL;
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s - detected forgotten connect context", __func__);
		report_error_on_context(&(env->disconnectCtx), error, TRUE);
	}
	osync_trace(TRACE_INTERNAL, "%s - contexts cleaned", __func__);

	/* cleanup OpenSync stuff */

	if (env->pluginInfo) {
		osync_plugin_info_unref(env->pluginInfo);
		env->pluginInfo = NULL;
	}
	osync_trace(TRACE_INTERNAL, "%s - plugin info cleaned", __func__);

	/* final cleanup */

	safe_free((gpointer *) &env);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

unsigned int get_num_changes(OSyncChange **changes)
{
    osync_trace(TRACE_ENTRY, "%s", __func__);

    if (changes == NULL || changes[0] == NULL)
    {
        osync_trace(TRACE_EXIT, "%s - no changes present", __func__);
        return 0;
    }

    unsigned int num = 0;
    unsigned int i;
    for (i = 0; changes[i]; i++) {
        num++;
    }

    osync_trace(TRACE_EXIT, "%s (%d)", __func__, num);
    return num;
}

SmlBool send_sync_message(
                SmlDatabase *database,
                void *func_ptr, OSyncError **oserror)
{
    osync_trace(TRACE_ENTRY, "%s(%p)", __func__, database);
    g_assert(database);
    g_assert(database->session);

    SmlError *error = NULL;
    int num = get_num_changes(database->syncChanges);

    if (!smlDsSessionSendSync(database->session, num, func_ptr, database, &error))
        goto error;

    int i = 0;
    for (i = 0; i < num; i++) {
        osync_trace(TRACE_INTERNAL, "%s: handling change %i", __func__, i);
        OSyncChange *change = database->syncChanges[i];
        OSyncContext *context = database->syncContexts[i];
        g_assert(change);
        g_assert(context);
        osync_trace(TRACE_INTERNAL, "%s: params checked (%p, %p)", __func__, change, context);
		
        osync_trace(TRACE_INTERNAL,
                    "%s: Uid: \"%s\", Format: \"%s\", Changetype: \"%i\"",
                    __func__,
                    osync_change_get_uid(change),
                    osync_change_get_objtype(change),
                    osync_change_get_changetype(change));

        // prepare change/commit context
        struct commitContext *tracer = osync_try_malloc0(sizeof(struct commitContext), oserror);
        if (!tracer)
            goto oserror;
        tracer->change = change;
        tracer->context = context;
        tracer->database = database;

        // prepare data
        OSyncData *data = osync_change_get_data(change);
        char *buf = NULL;
        unsigned int size = 0;
        osync_data_get_data(data, &buf, &size);
	
        osync_trace(TRACE_INTERNAL, "%s: Committing entry \"%s\": \"%s\"",
                    __func__, osync_change_get_uid(change), buf);
        if (!smlDsSessionQueueChange(
                 database->session,
                 _get_changetype(change),
                 osync_change_get_uid(change),
                 buf, size,
                 get_database_pref_content_type(database, oserror),
                 _recv_change_reply, tracer, &error))
            goto error;

	// DO NOT unref change and context here because they are used by the tracer
	// osync_change_unref(change);
	// osync_context_unref(context);
        database->syncChanges[i] = NULL;
        database->syncContexts[i] = NULL;
    }
    if (num || smlDsServerGetServerType(database->server) == SML_DS_SERVER)
    {
        /* If this is an OMA DS server then the arrays are
         * always configured and must be freed.
         */
        safe_free((gpointer *) &(database->syncChanges));
        safe_free((gpointer *) &(database->syncContexts));
    }
	
    if (!smlDsSessionCloseSync(database->session, &error))
        goto error;

    if (!flush_session_for_all_databases(database->env, TRUE, &error))
        goto error;

    osync_trace(TRACE_EXIT, "%s", __func__);
    return TRUE;

error:
    osync_error_set(oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
    smlErrorDeref(&error);
oserror:
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(oserror));
    report_error_on_context(&(database->commitCtx), oserror, FALSE);
    return FALSE;
}

osync_bool init_objformat(OSyncPluginInfo *info, SmlDatabase *database, OSyncError **error)
{
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);

	database->objformat = osync_format_env_find_objformat(formatenv, database->objformat_name);
	if (!database->objformat) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find \"%s\" object format. Are format plugins correctly installed?", database->objformat_name);
		return FALSE;
	}

	osync_objformat_ref(database->objformat);
	osync_trace(TRACE_INTERNAL, "%s: objformat is %s for %s", __func__,
		osync_objformat_get_objtype(database->objformat),
		osync_objformat_get_name(database->objformat));

	// TODO:... in case of maemo ("plain text") we have to set "memo"...
	osync_objtype_sink_add_objformat(database->sink, database->objformat_name);

	return TRUE;
}

SmlDatabase *get_database_from_plugin_info(OSyncPluginInfo *info)
{
    osync_trace(TRACE_ENTRY, "%s", __func__);
    OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
    SmlDatabase *database = osync_objtype_sink_get_userdata(sink);
    osync_trace(TRACE_EXIT, "%s - %s", __func__, database->url);
    return database;
} 

void safe_cfree(char **address)
{
    safe_free((gpointer *)address);
}

void safe_free(gpointer *address)
{
    g_assert(address);
    g_assert(*address);
    g_free(*address);
    *address = NULL;
}

void report_success_on_context(OSyncContext **ctx)
{
    osync_trace(TRACE_INTERNAL, "%s: report success on osync_context %p.", __func__, *ctx);
    osync_context_report_success(*ctx);
    osync_context_unref(*ctx);
    *ctx = NULL;
}

void report_error_on_context(OSyncContext **ctx, OSyncError **error, osync_bool cleanupError)
{
    osync_trace(
        TRACE_INTERNAL, "%s: report error on osync_context %p (%s).", __func__,
        *ctx, osync_error_print(error));
    osync_context_report_osyncerror(*ctx, *error);
    osync_context_unref(*ctx);
    *ctx = NULL;
    if (cleanupError)
    {
        osync_error_unref(error);
        *error = NULL;
    }
}
