#include "syncml_ds_server.h"
#include "syncml_callbacks.h"

SmlBool ds_server_init_databases(SmlPluginEnv *env, OSyncPluginInfo *info, OSyncError **error)
{
	GList *o = env->databases;
	for (; o; o = o->next) {
                SmlDatabase *database = o->data;
		database->gotChanges = FALSE;

                OSyncObjTypeSink *sink = osync_objtype_sink_new(database->objtype, error);
                if (!sink)
			return FALSE;
                
                database->sink = sink;
                
		if (!init_objformat(info, database, error))
			return FALSE;
                
                OSyncObjTypeSinkFunctions functions;
                memset(&functions, 0, sizeof(functions));
		functions.connect = ds_server_register_sync_mode;
                functions.get_changes = ds_server_get_changeinfo;
                functions.sync_done = sync_done;
		functions.batch_commit = ds_server_batch_commit;
                
                osync_objtype_sink_set_functions(sink, functions, database);
                osync_plugin_info_add_objtype(info, sink);
	}
	return TRUE;
}

void ds_server_register_sync_mode(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
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
	if (env->isConnected && database->session)
		ds_server_init_sync_mode(database);

	g_mutex_unlock(env->connectMutex);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

void ds_server_init_sync_mode(SmlDatabase *database)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, database);
	SmlPluginEnv *env = database->env;
	set_session_user(env, __func__);
	g_assert(database->session);

	/* This function should only be called if the DsSession is available.
	/* Only the alert callback is registered here because the changes should
	 * be managed after the function ds_server_get_changeinfo was called.
	 */
	smlDsSessionGetAlert(database->session, _ds_server_recv_alert, database);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

void ds_server_get_changeinfo(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
        SmlPluginEnv *env = (SmlPluginEnv *)data;

        SmlDatabase *database = get_database_from_plugin_info(info);
	g_assert(database->session);

        database->getChangesCtx = ctx;
        osync_context_ref(database->getChangesCtx);

	/* register all functions which are needed to manage the sync command */
	smlDsSessionGetEvent(database->session, _ds_event, database);
	smlDsSessionGetSync(database->session, _recv_sync, database);
	smlDsSessionGetChanges(database->session, _recv_change, database);


	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

void ds_server_batch_commit(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncContext **contexts, OSyncChange **changes)
{
    osync_trace(TRACE_ENTRY, "%s", __func__);
    g_assert(ctx);

    SmlError  *error = NULL;
    OSyncError  *oserror = NULL;
    SmlDatabase *database = get_database_from_plugin_info(info);
    set_session_user(database->env, __func__);

    /* Servers only answer on client alerts.
     * If a client already started a sync for a database
     * then this sync must be completed.
     */
    database->pendingCommits = get_num_changes(changes);
    database->env->ignoredDatabases = g_list_remove(database->env->ignoredDatabases, database);
    osync_trace(TRACE_INTERNAL, "%s - %i changes present to send", __func__, database->pendingCommits);

    database->commitCtx = ctx;
    osync_context_ref(database->commitCtx);

    // a batch commit should be called after the first DsSession
    // was completely performed
    g_assert(database->session);
    g_assert(database->pendingChanges == 0); 

    // cache changes
    database->syncChanges = osync_try_malloc0((database->pendingCommits + 1)*sizeof(OSyncChange *), &oserror);
    if (!database->syncChanges) goto oserror;
    database->syncChanges[database->pendingCommits] = NULL;
    database->syncContexts = osync_try_malloc0((database->pendingCommits + 1)*sizeof(OSyncContext *), &oserror);
    if (!database->syncContexts) goto oserror;
    database->syncContexts[database->pendingCommits] = NULL;
    int i;
    for (i=0; i < database->pendingCommits; i++)
    {
        database->syncChanges[i] = changes[i];
        database->syncContexts[i] = contexts[i];
	osync_change_ref(changes[i]);
	osync_context_ref(contexts[i]);
    }

    /* a server can send the sync message directly */
    if (!send_sync_message(database, _recv_sync_reply, &oserror))
        goto oserror;

    osync_trace(TRACE_EXIT, "%s", __func__);
    return;

error:
    osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
    smlErrorDeref(&error);
oserror:
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
    report_error_on_context(&(database->commitCtx), &oserror, TRUE);
}

SmlBool _ds_server_recv_alert(SmlDsSession *dsession, SmlAlertType type, const char *last, const char *next, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %s, %p)", __func__, dsession, type, last, next, userdata);
	SmlDatabase *database = (SmlDatabase*) userdata;
	SmlPluginEnv *env = database->env;
	SmlBool ret = TRUE;
	OSyncError *oserror = NULL;
	SmlError *error = NULL;
	
	char *key = g_strdup_printf("remoteanchor%s", smlDsSessionGetLocation(dsession));

	/* We return FALSE if we need a special return code as answer:
	 * SML_ERROR_REQUIRE_REFRESH 508
	 * This return code enforces a SLOW-SYNC.
	 */
	if (type == SML_ALERT_TWO_WAY)
	{
		if (!last)
		{
			osync_trace(TRACE_INTERNAL, "%s: TWO-WAY-SYNC but last is missing", __func__);
			ret = FALSE;
		}
		if (!osync_anchor_compare(env->anchor_path, key, last))
		{
			char *local = osync_anchor_retrieve(env->anchor_path, key);
			osync_trace(TRACE_INTERNAL,
				"%s: TWO-WAY-SYNC but received LAST(%s) and cached LAST (%s) mismatch",
				__func__, local, last);
			if (local)
				safe_cfree(&local);
			ret = FALSE;
		}
		if (osync_objtype_sink_get_slowsync(database->sink))
		{
			/* OpenSync wants to perform a SLOW-SYNC */
			osync_trace(TRACE_INTERNAL, "%s: TWO-WAY-SYNC but OpenSync needs SLOW-SYNC", __func__);
			ret = FALSE;
		}
	}
	
	if (!ret || type != SML_ALERT_TWO_WAY)
		osync_objtype_sink_set_slowsync(database->sink, TRUE);
	
	osync_trace(TRACE_INTERNAL, "%s: updating sync anchor %s to %s", __func__, key, next);
	osync_anchor_update(env->anchor_path, key, next);
	safe_cfree(&key);
	
	if (osync_objtype_sink_get_slowsync(database->sink)) {
		if (!smlDsSessionSendAlert(dsession, SML_ALERT_SLOW_SYNC, last, next, _recv_alert_reply, database, &error))
			goto error;
	} else {
		if (!smlDsSessionSendAlert(dsession, SML_ALERT_TWO_WAY, last, next, _recv_alert_reply, database, &error))
			goto error;
	}

	// This is a server function only - so we are in server mode.
	// If a server replies on an alert and sends back an alert
	// then we must flush after all alerts are ready to send.
	if (!flush_session_for_all_databases(database->env, TRUE, &error))
		goto error;

	/* signal the success to the sync mode context */
	if (database->syncModeCtx)
		report_success_on_context(&(database->syncModeCtx));

	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
oserror:
	osync_trace(TRACE_EXIT_ERROR, "%s - %s", __func__, osync_error_print(&oserror));
	if (database->syncModeCtx)
		report_error_on_context(&(database->syncModeCtx), &oserror, TRUE);
	return FALSE;
}
