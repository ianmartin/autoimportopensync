#include "syncml_ds_client.h"
#include "syncml_callbacks.h"

void ds_client_get_changeinfo(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
        SmlPluginEnv *env = (SmlPluginEnv *)data;

        SmlDatabase *database = get_database_from_plugin_info(info);

        database->getChangesCtx = ctx;
        osync_context_ref(database->getChangesCtx);

        SmlError *error = NULL;
        OSyncError *oserror = NULL;

	/* let's wait for the device info of the server */
	while (!smlDevInfAgentGetDevInf(env->agent) && !smlSessionCheck(env->session))
	{
		unsigned int sleeping = 5;
		osync_trace(TRACE_INTERNAL,
			"%s: SyncML HTTP client is waiting for server's device info (%d seconds).",
			__func__, sleeping);
		sleep(sleeping);
	}
	SmlDevInf *devinf = smlDevInfAgentGetDevInf(env->agent);
	unsigned int stores = smlDevInfNumDataStores(devinf);
	unsigned int i;
	SmlBool supportedDatabase = FALSE;
	for (i=0; i < stores; i++)
	{
		const SmlDevInfDataStore *datastore = smlDevInfGetNthDataStore(devinf, i);
		// if (!strcmp(smlDevInfDataStoreGetSourceRef(datastore), database->objtype))
		if (!strcmp(smlDevInfDataStoreGetSourceRef(datastore),
			    database->url))
			supportedDatabase = TRUE;
	}
	if (!supportedDatabase)
	{
		osync_trace(TRACE_INTERNAL,
			"%s: SyncML HTTP client uses unsupported objtype (%s) ...",
			__func__, database->objtype);
		for (i=0; i < stores; i++)
		{
			const SmlDevInfDataStore *datastore = smlDevInfGetNthDataStore(devinf, i);
			osync_trace(TRACE_INTERNAL, "%s: %s (supported)",
				__func__, smlDevInfDataStoreGetSourceRef(datastore));
		}
	} else {
		osync_trace(TRACE_INTERNAL,
			"%s: SyncML HTTP client uses supported objtype (%s: %s).\n",
			__func__, database->objtype, database->url);
	}

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

	register_ds_session_callbacks(database, _ds_client_recv_alert);

	if (!flush_session_for_all_databases(env, TRUE, &error))
		goto error;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_context_report_osyncerror(ctx, oserror);
        osync_context_unref(database->getChangesCtx);
        database->getChangesCtx = NULL;
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
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

    register_ds_session_callbacks(database, _ds_client_recv_alert);

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
    SmlError *error = NULL;
    OSyncError *oserror = NULL;

    char *key = g_strdup_printf("remoteanchor%s", smlDsSessionGetLocation(dsession));

    // we get a normal sync but we have trouble with the sync anchors
    // this requires a slow sync alert before doing anything else
    //
    // if we expect a slow sync but get a normal sync
    // then we should sends a new slow sync alert
    //
    // if this is the second session the we can ignore the checks
    // if this is the second session then we only want to send
    // the changes to the server
    if (
        (
         ( ( !last ||                                           // missing anchor
             !osync_anchor_compare(env->anchor_path, key, last) // last mismatches database
           )
          && type == SML_ALERT_TWO_WAY
         )
         ||
         (osync_objtype_sink_get_slowsync(database->sink) && type != SML_ALERT_SLOW_SYNC)
        )
        && database->getChangesCtx                              // this is the first session
       )
    {
        // send slow sync alert
        // FIXME: this is clearly an error
        // FIXME: doe sit be better to crash on this error?
        osync_objtype_sink_set_slowsync(database->sink, TRUE);
        database->session = smlDsServerSendAlert(
                                database->server,
                                env->session,
                                SML_ALERT_SLOW_SYNC,
                                NULL, next,
                               _recv_alert_reply, database,
                               &error);
        if (!database->session)
            goto error;
        if (!flush_session_for_all_databases(database->env, TRUE, &error))
            goto error;
        // re-register this callback
	register_ds_session_callbacks(database, _ds_client_recv_alert);
    } else {

        // if we do the update then we should store the new  anchor
        // if the update fails then the next sync is a slow sync
        osync_anchor_update(env->anchor_path, key, next);

        // start the sync message
        if (!send_sync_message(database, _recv_sync_reply, &oserror))
            goto oserror;
    }

    safe_cfree(&key);
    osync_trace(TRACE_EXIT, "%s: %i", __func__, TRUE);
    return TRUE;
error:
    osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
    smlErrorDeref(&error);
oserror:
    if (key) safe_cfree(&key);
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
    osync_error_unref(&oserror);
    return FALSE;
}

