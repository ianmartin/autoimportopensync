#include "syncml_ds_client.h"
#include "syncml_callbacks.h"
#include "syncml_devinf.h"

SmlBool ds_client_init_databases(SmlPluginEnv *env, OSyncPluginInfo *info, OSyncError **oerror)
{

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, info, oerror);
	SmlDatabase *database = NULL;
	SmlError *error = NULL;
	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	unsigned int i, num_objtypes = osync_plugin_info_num_objtypes(info);
	osync_trace(TRACE_INTERNAL, "%s: %d objtypes", __func__, num_objtypes);


	for (i=0; i < num_objtypes; i++) {
		OSyncObjTypeSink *sink = osync_plugin_info_nth_objtype(info, i);
		osync_bool sinkEnabled = osync_objtype_sink_is_enabled(sink);
		osync_trace(TRACE_INTERNAL, "%s: enabled => %d", __func__, sinkEnabled);
		if (!sinkEnabled)
			continue;


                OSyncObjTypeSinkFunctions functions;
                memset(&functions, 0, sizeof(functions));
                functions.get_changes = ds_client_get_changeinfo;
                functions.sync_done = sync_done;
		functions.batch_commit = ds_client_batch_commit;

		const char *objtype = osync_objtype_sink_get_name(sink);
		OSyncPluginResource *res = osync_plugin_config_find_active_resource(config, objtype);
		if (!(database = syncml_config_parse_database(env, res, oerror)))
			goto oerror;
                
                database->sink = sink;

		/* TODO: Handle all available format sinks! */
		OSyncList *fs = osync_plugin_resource_get_objformat_sinks(res);
		OSyncObjFormatSink *fmtsink = osync_list_nth_data(fs, 0);
		const char *objformat = osync_objformat_sink_get_objformat(fmtsink);

		database->objformat = osync_format_env_find_objformat(formatenv, objformat);

		/* TODO: Handle error about missing objformat in a nice way. */
		g_assert(database->objformat);

		osync_objformat_ref(database->objformat);

                osync_objtype_sink_set_functions(sink, functions, database);

		env->databases = g_list_append(env->databases, database);

		if (!smlDataSyncAddDatastore(
                                        env->dsObject1,
					get_database_pref_content_type(database, oerror),
                                        NULL,
                                        database->url,
                                        &error))
                                goto error;
		if (!smlDataSyncAddDatastore(
                                        env->dsObject2,
					get_database_pref_content_type(database, oerror),
                                        NULL,
                                        database->url,
                                        &error))
                                goto error;
	}
	osync_trace(TRACE_EXIT, "%s - TRUE", __func__);
	return TRUE;
error:
	osync_error_set(oerror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
oerror:
	osync_trace(TRACE_EXIT_ERROR, "%s - %s", __func__, osync_error_print(oerror));
	return FALSE;
}

void ds_client_get_changeinfo(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);

        SmlDatabase *database = get_database_from_plugin_info(info);
	SmlPluginEnv *env = database->env;

	if (env->state1 >= SML_DATA_SYNC_EVENT_GOT_ALL_CHANGES) {
		report_success_on_context(&(ctx));
	} else {
		database->getChangesCtx = ctx;
		osync_context_ref(database->getChangesCtx);
	}

	smlDataSyncRegisterChangeCallback(env->dsObject1, _recv_change, env);
	smlDataSyncRegisterChangeCallback(env->dsObject2, _recv_unwanted_change, env);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

void ds_client_batch_commit(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncContext **contexts, OSyncChange **changes)
{
    osync_trace(TRACE_ENTRY, "%s", __func__);
    g_assert(ctx);

    SmlError  *error = NULL;
    OSyncError  *oserror = NULL;
    SmlDatabase *database = get_database_from_plugin_info(info);

    database->commitCtx = ctx;
    osync_context_ref(database->commitCtx);

    database->pendingCommits = get_num_changes(changes);
    if (database->pendingCommits == 0)
    {
        // if there are no changes then we do nothing
        // especially for OMA DS clients it makes no sense
        // to initiate a new DS session if there is nothing to do
        report_success_on_context(&(database->commitCtx));
        osync_trace(TRACE_EXIT, "%s - no changes present to send", __func__);
        return;
    }

    // a batch commit should be called after the first DsSession
    // was completely performed
    g_assert(database->pendingChanges == 0); 

    int i;
    for (i=0; i < database->pendingCommits; i++)
    {
        struct commitContext *tracer = osync_try_malloc0(sizeof(struct commitContext), &oserror);
        if (!tracer)
            goto oserror;
        tracer->change = changes[i];
        tracer->context = contexts[i];
        tracer->database = database;

	osync_change_ref(changes[i]);
	osync_context_ref(contexts[i]);

        // prepare data
        OSyncData *data = osync_change_get_data(changes[i]);
        char *buf = NULL;
        unsigned int size = 0;
        osync_data_get_data(data, &buf, &size);
	
        osync_trace(TRACE_INTERNAL, "%s: Committing entry \"%s\": \"%s\"",
                    __func__, osync_change_get_uid(changes[i]), buf);
	if (!smlDataSyncAddChange(
		database->env->dsObject2,
		database->url,
		_get_changetype(changes[i]),
		osync_change_get_uid(changes[i]),
		buf, size,
		tracer, &error))
            goto error;

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

    /* a client must create a new DsSession */

    if (!smlDataSyncInit(database->env->dsObject2, &error))
	goto error;
    if (!smlDataSyncRun(database->env->dsObject2, &error))
	goto error;

    osync_trace(TRACE_EXIT, "%s", __func__);
    return;

error:
    osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
    smlErrorDeref(&error);
oserror:
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
    report_error_on_context(&(database->commitCtx), &oserror, TRUE);
}

