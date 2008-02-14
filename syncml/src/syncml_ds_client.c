#include "syncml_ds_client.h"
#include "syncml_callbacks.h"

SmlBool ds_client_init_databases(SmlPluginEnv *env, OSyncPluginInfo *info, OSyncError **error)
{
	GList *o = env->databases;
	for (; o; o = o->next) {
                SmlDatabase *database = o->data;
		database->gotChanges = FALSE;
		database->finalChanges = FALSE;

                OSyncObjTypeSink *sink = osync_objtype_sink_new(database->objtype, error);
                if (!sink)
			return FALSE;
                
                database->sink = sink;

		if (!init_objformat(info, database, error))
			return FALSE;
                
                OSyncObjTypeSinkFunctions functions;
                memset(&functions, 0, sizeof(functions));
		functions.connect = ds_client_register_sync_mode;
                functions.get_changes = ds_client_get_changeinfo;
                functions.sync_done = sync_done;
		functions.batch_commit = ds_client_batch_commit;
                
                osync_objtype_sink_set_functions(sink, functions, database);
                osync_plugin_info_add_objtype(info, sink);
	}
	return TRUE;
}
	
void ds_client_register_sync_mode(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
        SmlPluginEnv *env = (SmlPluginEnv *)data;

	/* only one function can run correctly in connect context */
	g_mutex_lock(env->connectMutex);

	/* prepare the database env to handle the sync mode context */
        SmlDatabase *database = get_database_from_plugin_info(info);
        database->syncModeCtx = ctx;
        osync_context_ref(database->syncModeCtx);

	/* if there is already a connect then run the sync mode init */
	if (env->isConnected)
		ds_client_init_sync_mode(database);

	g_mutex_unlock(env->connectMutex);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void ds_client_init_sync_mode(SmlDatabase *database)
{
        osync_trace(TRACE_ENTRY, "%s(%p)", __func__, database);
        SmlPluginEnv *env = database->env;
	g_assert(env->session);

        SmlError *error = NULL;
        OSyncError *oserror = NULL;

	/* initialize the timestamps and alert type */
	SmlAlertType alertType = SML_ALERT_SLOW_SYNC;
	char *last = NULL; // perhaps NULL is better
	char *next = malloc(sizeof(char)*17);
	time_t htime = time(NULL);
	if (env->onlyLocaltime)
		strftime(next, 17, "%Y%m%dT%H%M%SZ", localtime(&htime));
	else
		strftime(next, 17, "%Y%m%dT%H%M%SZ", gmtime(&htime));
	if (!osync_objtype_sink_get_slowsync(database->sink))
        {
		/* this must be a two-way-sync */
		alertType = SML_ALERT_TWO_WAY;
		char *key = g_strdup_printf("remoteanchor%s", smlDsServerGetLocation(database->server));
		last = osync_anchor_retrieve(database->env->anchor_path, key);
		safe_cfree(&key);
	}

	/* The OMA DS client starts the synchronization so there should be no
	 * DsSession (datastore session) present.
	 */
	database->session = smlDsServerSendAlert(
				database->server,
				env->session,
				alertType,
				last, next,
				_recv_alert_reply, database,
				&error);
	safe_cfree(&next);
	if (last) safe_cfree(&last);
	if (!database->session)
		goto error;

	/* Only the alert callback is registered here because the changes should
	 * be managed after the function ds_client_get_changeinfo was called.
	 */
	smlDsSessionGetAlert(database->session, _ds_client_recv_alert, database);

	if (!flush_session_for_all_databases(env, TRUE, &error))
		goto error;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_context_report_osyncerror(database->syncModeCtx, oserror);
        osync_context_unref(database->syncModeCtx);
        database->syncModeCtx = NULL;
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

void ds_client_get_changeinfo(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
        SmlPluginEnv *env = (SmlPluginEnv *)data;

        SmlDatabase *database = get_database_from_plugin_info(info);

        database->getChangesCtx = ctx;
        osync_context_ref(database->getChangesCtx);

	/* register all functions which are needed to manage the sync command */
	smlDsSessionGetEvent(database->session, _ds_event, database);
	smlDsSessionGetSync(database->session, _recv_sync, database);
	smlDsSessionGetChanges(database->session, _recv_change, database);

	/* We received the alert from the server.
	 * The alert mode from the server was accepted.
	 * We have only to send the sync message.
	 * The sync message is flushed automatically.
	 */
        OSyncError *error = NULL;
        if (!send_sync_message(database, _recv_sync_reply, &error))
            goto error;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
error:
	osync_context_report_osyncerror(database->getChangesCtx, error);
        osync_context_unref(database->getChangesCtx);
        database->getChangesCtx = NULL;
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

void ds_client_batch_commit(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncContext **contexts, OSyncChange **changes)
{
    osync_trace(TRACE_ENTRY, "%s", __func__);
    g_assert(ctx);

    SmlError  *error = NULL;
    OSyncError  *oserror = NULL;
    SmlDatabase *database = get_database_from_plugin_info(info);
    set_session_user(database->env, __func__);

    unsigned int num = get_num_changes(changes);
    if (num == 0)
    {
        // if there are no changes then we do nothing
        // especially for OMA DS clients it makes no sense
        // to initiate a new DS session if there is nothing to do
        database->env->ignoredDatabases = g_list_add(database->env->ignoredDatabases, database);
	if (!flush_session_for_all_databases(database->env, FALSE, &error))
        	goto error;
        osync_context_report_success(ctx);
        osync_trace(TRACE_EXIT, "%s - no changes present to send", __func__);
        return;
    } else {
        database->env->ignoredDatabases = g_list_remove(database->env->ignoredDatabases, database);
        osync_trace(TRACE_INTERNAL, "%s - %i changes present to send", __func__, num);
    }

    database->commitCtx = ctx;
    osync_context_ref(database->commitCtx);

    // a batch commit should be called after the first DsSession
    // was completely performed
    g_assert(database->session);
    g_assert(database->pendingChanges == 0); 

    // cache changes
    database->syncChanges = osync_try_malloc0((num + 1)*sizeof(OSyncChange *), &oserror);
    if (!database->syncChanges) goto oserror;
    database->syncChanges[num] = NULL;
    database->syncContexts = osync_try_malloc0((num + 1)*sizeof(OSyncContext *), &oserror);
    if (!database->syncContexts) goto oserror;
    database->syncContexts[num] = NULL;
    int i;
    for (i=0; i < num; i++)
    {
        database->syncChanges[i] = changes[i];
        database->syncContexts[i] = contexts[i];
	osync_change_ref(changes[i]);
	osync_context_ref(contexts[i]);
    }

    /* I (bellmich) tested a lot with OCS and the result is
     * that it is required to start a new OMA DS session for
     * the data uploading. If I only try to send a new alert
     * then OCS fails after receiving the changes from the
     * client.
     *
     * It is important to understand that OMA DS session and
     * libsyncml DsSession are completely different things.
     * libsyncml DsSession is something like a DataStore
     * session inside of an OMA DS session. OMA DS session
     * means OMA Data Synchronization session. I think the
     * confusion happens because the interface was designed
     * before SyncML was migrated to OMA DS/DM.
     */

    /* stop the old DsSession */
    smlDsSessionUnref(database->session);
    database->session = NULL;

    /* a client must create a new DsSession */

    // start fresh ds session
    // send sync alert
    // slow sync should be initialized via _ds_client_recv_alert
    // if this happens then the DsSession is already present
    char *key = g_strdup_printf("remoteanchor%s", smlDsServerGetLocation(database->server));
    char *last = osync_anchor_retrieve(database->env->anchor_path, key);
    char *next = malloc(sizeof(char)*17);
    time_t htime = time(NULL);
    if (database->env->onlyLocaltime)
        strftime(next, 17, "%Y%m%dT%H%M%SZ", localtime(&htime));
    else
        strftime(next, 17, "%Y%m%dT%H%M%SZ", gmtime(&htime));
    database->session = smlDsServerSendAlert(
                               database->server,
                               database->env->session,
                               SML_ALERT_TWO_WAY,
                               last, next,
                               _recv_alert_reply, database,
                               &error);
    if (!database->session)
        goto error;

    /* register all callbacks again because it is a new session */
    smlDsSessionGetAlert(database->session, _ds_client_recv_alert, database);
    smlDsSessionGetEvent(database->session, _ds_event, database);
    smlDsSessionGetSync(database->session, _recv_sync, database);
    smlDsSessionGetChanges(database->session, _recv_change, database);

    if (!flush_session_for_all_databases(database->env, TRUE, &error))
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

SmlBool _ds_client_recv_alert(
			SmlDsSession *dsession,
			SmlAlertType type,
			const char *last,
			const char *next,
			void *userdata)
{
    osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %s, %p)", __func__, dsession, type, last, next, userdata);
    // we only support two types of sync alerts
    // so if the server ignores this then we should stop here
    g_assert(
        type == SML_ALERT_TWO_WAY ||
        type == SML_ALERT_SLOW_SYNC);

    SmlDatabase *database = (SmlDatabase*) userdata;
    SmlPluginEnv *env = database->env;
    OSyncError *oserror = NULL;

    char *key = g_strdup_printf("remoteanchor%s", smlDsSessionGetLocation(dsession));

    if (database->syncModeCtx)
    {
        /* actually the checks are only performed for the first session */
        if (!last && type != SML_ALERT_SLOW_SYNC)
        {
            osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "TWO-WAY-SYNC is requested but there is no LAST anchor.");
            goto oserror;
        }
        if (!osync_anchor_compare(env->anchor_path, key, last) && type != SML_ALERT_SLOW_SYNC)
        {
            char *local = osync_anchor_retrieve(env->anchor_path, key);
            osync_error_set(
                &oserror, OSYNC_ERROR_GENERIC,
                "TWO-WAY-SYNC is requested but the cached LAST anchor (%) does not match the presented LAST anchor from the remote peer (%).",
                local, last);
            safe_cfree(&local);
            goto oserror;
        }
        if (osync_objtype_sink_get_slowsync(database->sink) && type != SML_ALERT_SLOW_SYNC)
        {
            osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "SLOW-SYNC is requested but the local sink still wants a TWO-WAY-SYNC.");
            goto oserror;
        }
    }

    if (type == SML_ALERT_SLOW_SYNC)
        osync_objtype_sink_set_slowsync(database->sink, TRUE);

    // if we do the update then we should store the new  anchor
    // if the update fails then the next sync is a slow sync
    osync_anchor_update(env->anchor_path, key, next);
    safe_cfree(&key);

    /* signal the success to the context */
    if (database->syncModeCtx)
    {
        osync_context_report_success(database->syncModeCtx);
        osync_context_unref(database->syncModeCtx);
        database->syncModeCtx = NULL;
    }

    osync_trace(TRACE_EXIT, "%s: %i", __func__, TRUE);
    return TRUE;
oserror:
    if (key)
        safe_cfree(&key);
    if (database->syncModeCtx)
    {
        osync_context_report_osyncerror(database->syncModeCtx, oserror);
        osync_context_unref(database->syncModeCtx);
        database->syncModeCtx = NULL;
    }
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
    return FALSE;
}

