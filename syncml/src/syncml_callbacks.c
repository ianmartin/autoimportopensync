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

/* **************************************** */
/* *****     Management Callbacks     ***** */
/* **************************************** */

void _manager_event(SmlManager *manager, SmlManagerEventType type, SmlSession *session, SmlError *error, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p)", __func__, manager, type, session, error, userdata);
	SmlPluginEnv *env = userdata;
	GList *o = NULL;
	g_mutex_lock(env->mutex);

	switch (type) {
		case SML_MANAGER_SESSION_FLUSH:
			// finalChanges must be resetted because every SML_MANAGER_SESSION_FINAL
			// sets this status to true but not every message is a SYNC message
			// if we do not reset finalChanges at the beginning of a SYNC
			// then we set finalChanges at the client on receiving the SYNC alert
			// and not the sync itself from the server
			//
			// perspective: client
			//
			// client: sends sync alert
			//         --> finalChanges = FALSE on SESSION_FLUSH (gotChanges == FALSE)
			// client: receives sync alert from server
			//         --> finalChanges = TRUE on SESSION_FINAL (gotChanges == FALSE)
			// client: sends sync with changes
			//         --> finalChanges = FALSE on SESSION_FLUSH (gotChanges == FALSE)
			// client: receives sync with changes from server
			//         --> finalChanges = TRUE on SESSION_FINAL (gotChanges == TRUE)
			//         --> finalChanges + gotChanges --> getChangesCtx = NULL
			osync_trace(TRACE_INTERNAL, "resetting finalChanges if necessary ...");
			o = env->databases;
			osync_trace(TRACE_INTERNAL, "    got GList");
			for (; o; o = o->next) {
				SmlDatabase *database = o->data;
				osync_trace(TRACE_INTERNAL, "    got database");

				osync_trace(TRACE_INTERNAL, "%s: old: gotChanges: %i, finalChanges: %i, objtype: %s",
					__func__,
					database->gotChanges, database->finalChanges, database->objtype);

				// if no changes received and context present
				// then reset finalChanges
				osync_trace(TRACE_INTERNAL, "    check conditions");
				if (!database->gotChanges && database->getChangesCtx) {
					database->finalChanges = FALSE;
				}
				osync_trace(TRACE_INTERNAL, "    performed acction");

				osync_trace(TRACE_INTERNAL, "%s: new: gotChanges: %i, finalChanges: %i, objtype: %s",
					__func__,
					database->gotChanges, database->finalChanges, database->objtype);
			}
			osync_trace(TRACE_INTERNAL, "resetted finalChanges");
			break;
		case SML_MANAGER_CONNECT_DONE:
			env->gotDisconnect = FALSE;
			if (env->connectCtx) {
				osync_trace(TRACE_INTERNAL, "%s: signal successful connect", __func__);
				osync_context_report_success(env->connectCtx);
				env->connectCtx = NULL;
			} else {
				osync_trace(TRACE_INTERNAL, "%s: connect done but no context available", __func__);
			}
			break;
		case SML_MANAGER_DISCONNECT_DONE:
			osync_trace(TRACE_INTERNAL, "%s: connection with device has ended", __func__);
			SmlBool disconnectOpenSync = TRUE;
			/* If this is a OMA DS client then we need two OMA DS
			 * sessions to synchronize with OpenSync. If there is
			 * an active change context then this is only the first
			 * OMA DS session and we must initiate the second one.
			 * Additionally it is required report the success to
			 * the changes context after the second session is
			 * available.
			 */
			o = env->databases;
			for (; o; o = o->next) {
				SmlDatabase *database = o->data;
				/* check for a still active change context */
				if (smlDsServerGetServerType(database->server) == SML_DS_CLIENT &&
				    database->getChangesCtx)
				{
						disconnectOpenSync = FALSE;
				}
			}
			if (disconnectOpenSync)
			{
				/* a real disconnet happens */
				env->gotDisconnect = TRUE;
				if (env->disconnectCtx) { 
					osync_context_report_success(env->disconnectCtx);
					env->disconnectCtx = NULL;
				}
			} else {
				/* This is the disconnect of the first session
				 * of an OMA DS client. It is necessary to
				 * create a new session and after this report
				 * success for the active change contexts.
				 */
				env->connectFunction(env, NULL, NULL);
				o = env->databases;
				for (; o; o = o->next) {
					SmlDatabase *database = o->data;
					if (smlDsServerGetServerType(database->server) == SML_DS_CLIENT &&
					    database->getChangesCtx)
					{
						_try_change_ctx_cleanup(database);
					}
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
						/* Unlock the mutex, smlManagerDispatch will call this function
						   again if events are left in the smlManager queue. Avoids deadlocks! */ 
						g_mutex_unlock(env->mutex);
						smlManagerDispatch(manager);
						g_mutex_lock(env->mutex);
					}
				} else {
					env->gotDisconnect = TRUE;
					osync_trace(TRACE_EXIT, "%s: error while disconnecting: %s", __func__, smlErrorPrint(&error));
					goto error;
				}
			}
			goto error;
			break;
		case SML_MANAGER_SESSION_NEW:
			osync_trace(TRACE_INTERNAL, "%s: Just received a new session with ID %s",
				__func__, smlSessionGetSessionID(session));
			if (env->session) {
				osync_trace(TRACE_INTERNAL, "%s: WARNING: There was an old session %s in the environment.",
					__func__, smlSessionGetSessionID(env->session));
			}
			smlSessionUseStringTable(session, env->useStringtable);
			smlSessionUseOnlyReplace(session, env->onlyReplace);
			smlSessionUseNumberOfChanges(session, TRUE);
			
			if (env->recvLimit)
				smlSessionSetReceivingMaxMsgSize(session, env->recvLimit);
			if (env->maxObjSize)
				smlSessionSetReceivingMaxObjSize(session, env->maxObjSize);
			if (env->recvLimit && env->maxObjSize)
				smlSessionUseLargeObjects(session, TRUE);
			osync_trace(TRACE_INTERNAL, "%s: maxObjSize %d",
				__func__, env->maxObjSize);
			
			env->session = session;
			smlSessionRef(session);
			break;
		case SML_MANAGER_SESSION_FINAL:
			osync_trace(TRACE_INTERNAL, "Session %s reported final", smlSessionGetSessionID(session));
			env->gotFinal = TRUE;

			SmlBool flushMap = FALSE;
			o = env->databases;
			for (; o; o = o->next) {
				SmlDatabase *database = o->data;

				osync_trace(TRACE_INTERNAL, "%s: gotChanges: %i getChangesCtx: %p objtype: %s",
					__func__,
					database->gotChanges, database->getChangesCtx, database->objtype);

				if (database->getChangesCtx) {
					/* If there is a change context then a package
					 * with data was received actually. If such a
					 * package is closed by a final element then all
					 * datastores sent their complete data. So if
					 * one change context is finished then we
					 * can be sure that all others are finished too.
					 */
					database->finalChanges = TRUE;
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
						_try_change_ctx_cleanup(database);
				}

				if (database->commitCtx) {
					osync_context_report_success(database->commitCtx);
					database->commitCtx = NULL;
				}
			}

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
			if (env->prepareMapFlushing &&
			    !smlSessionFlush(env->session, TRUE, &error))
			{
				osync_trace(TRACE_INTERNAL, "%s: Flushing the map failed.", __func__);
				goto error;
			}
			env->prepareMapFlushing = FALSE;

			break;
		case SML_MANAGER_SESSION_END:
			osync_trace(TRACE_INTERNAL, "%s: Session %s has ended",
				__func__, smlSessionGetSessionID(session));
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
		default:
			printf("Unknown event received: %d.\n", type);
	}
	
	g_mutex_unlock(env->mutex);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:;
	osync_trace(TRACE_INTERNAL, "%s: Cleaning up because of an error ...", __func__);
	OSyncError *oserror = NULL;
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, smlErrorPrint(&error));

	if (env->connectCtx) {
		osync_context_report_osyncerror(env->connectCtx, oserror);
		env->connectCtx = NULL;
	}
	
	if (env->disconnectCtx) {
		osync_context_report_osyncerror(env->disconnectCtx, oserror);
		env->disconnectCtx = NULL;
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
		
	}

	g_mutex_unlock(env->mutex);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

/* *************************************** */
/* *****     DsSession Callbacks     ***** */
/* *************************************** */

void _ds_event(SmlDsSession *dsession, SmlDsEvent event, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, dsession, event, userdata);

	SmlDatabase *database = (SmlDatabase *)userdata;

	osync_trace(TRACE_INTERNAL, "database: %s", database->objtype);
	switch (event) {
		case SML_DS_EVENT_GOTCHANGES:
			database->gotChanges = TRUE;
			if (smlDsServerGetServerType(database->server) == SML_DS_SERVER)
			{
				/* The OMA DS client will be managed on base of
				 * the FINAL element. This is necessary because
				 * the client needs a much more sophisticated
				 * handling.
				 */
				_try_change_ctx_cleanup(database);
			}
			break;
		case SML_DS_EVENT_COMMITEDCHANGES:	
			break;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
}

void _ds_alert(SmlDsSession *dsession, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, dsession, userdata);
	g_assert(dsession);
	g_assert(userdata);

	SmlDatabase *database = (SmlDatabase *)userdata;
	osync_trace(TRACE_INTERNAL, "%s: %s", __func__, database->objtype);

	/* store device info */
	if (database->env->devinf_path)
	{
		OSyncError *error = NULL;
		store_devinf(database->env->devinf, database->env->devinf_path, &error);
		load_remote_devinf(database->env, &error);
	}

	/* set callbacks if the DsSession was not ready before */
	database->session = dsession;
	smlDsSessionRef(dsession);
	register_ds_session_callbacks(database, NULL);

	osync_trace(TRACE_EXIT, "%s", __func__);
}


/* *********************************** */
/* *****     Alert Callbacks     ***** */
/* *********************************** */

SmlBool _recv_alert_from_server(
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
    if (
        ((!last || !osync_anchor_compare(env->anchor_path, key, last)) && type == SML_ALERT_TWO_WAY)
        ||
        (osync_objtype_sink_get_slowsync(database->sink) && type != SML_ALERT_SLOW_SYNC)
       )
    {
        // send slow sync alert
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
    }

    osync_anchor_update(env->anchor_path, key, next);
    g_free(key);

    // start the sync message
    if (!send_sync_message(database, _recv_sync_reply, &oserror))
        goto oserror;

    osync_trace(TRACE_EXIT, "%s: %i", __func__, TRUE);
    return TRUE;
error:
    osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
    smlErrorDeref(&error);
oserror:
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
    return FALSE;
}

SmlBool _recv_alert(SmlDsSession *dsession, SmlAlertType type, const char *last, const char *next, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %s, %p)", __func__, dsession, type, last, next, userdata);
	SmlDatabase *database = (SmlDatabase*) userdata;
	SmlPluginEnv *env = database->env;
	SmlBool ret = TRUE;
	
	char *key = g_strdup_printf("remoteanchor%s", smlDsSessionGetLocation(dsession));

	if ((!last || !osync_anchor_compare(env->anchor_path, key, last)) && type == SML_ALERT_TWO_WAY)
		ret = FALSE;
	
	if (!ret || type != SML_ALERT_TWO_WAY)
		osync_objtype_sink_set_slowsync(database->sink, TRUE);
	
	osync_trace(TRACE_INTERNAL, "%s: updating sync anchor %s to %s", __func__, key, next);
	osync_anchor_update(env->anchor_path, key, next);
	g_free(key);
	
	if (osync_objtype_sink_get_slowsync(database->sink)) {
		smlDsSessionSendAlert(dsession, SML_ALERT_SLOW_SYNC, last, next, _recv_alert_reply, database, NULL);
	} else {
		smlDsSessionSendAlert(dsession, SML_ALERT_TWO_WAY, last, next, _recv_alert_reply, database, NULL);
	}

	// This is a server function only - so we are in server mode.
	// If a server replies on an alert and sends back an alert
	// then we must flush after all alerts are ready to send.
	if (!flush_session_for_all_databases(database->env, TRUE, NULL))
	{
		osync_trace(TRACE_EXIT_ERROR, "%s - flush failed", __func__);
		return FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

void _recv_alert_reply(SmlSession *session, SmlStatus *status, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, session, status, userdata);
	
	SmlDatabase *database = (SmlDatabase*) userdata;

	osync_trace(TRACE_INTERNAL, "Received an reply to our Alert - %s\n", database->objtype);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

/* ********************************** */
/* *****     Sync Callbacks     ***** */
/* ********************************** */

void _recv_sync(SmlDsSession *dsession, unsigned int numchanges, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, dsession, numchanges, userdata);
	SmlDatabase *database = (SmlDatabase *)userdata;
	
	osync_trace(TRACE_INTERNAL,"Going to receive %i changes - objtype: %s", numchanges, database->objtype);
	// printf("Going to receive %i changes\n", numchanges);
	database->pendingChanges = numchanges;

	/* If this is an OMA DS client then it is necessary to send a map after
	 * all changes were received. It is a good idea to do this if a FINAL
	 * was received together with a SYNC.
	 */
	if (smlDsServerGetServerType(database->server) == SML_DS_CLIENT)
	{
		osync_trace(TRACE_INTERNAL,"%s: enable map flushing for OMA DS client", __func__);
		database->env->prepareMapFlushing = TRUE;
	}

    	//if (smlDsServerGetServerType(database->server) == SML_DS_CLIENT &&
	//    numchanges == 0)
	//{
	//	// The session must be flushed
	//	// otherwise the DsSession would not complete correctly
	//	// FIXME: an error handling is not possible here
	//	osync_trace(TRACE_INTERNAL, "%s: no changes present - flushing directly", __func__);
	//	database->env->ignoredDatabases = g_list_add(database->env->ignoredDatabases, database);
	//	flush_session_for_all_databases(database->env, FALSE, NULL);
	//}

	/* If the client does not send the DevInf and the server does not
	 * cache the DevInf of the client then it can happen that we
	 * receive here the DevInf from the client (third message).
	 */
	if (database->env->devinf_path && !database->env->remote_devinf)
	{
		OSyncError *error = NULL;
		load_remote_devinf(database->env, &error);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
}

void _recv_sync_reply(SmlSession *session, SmlStatus *status, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, session, status, userdata);
	SmlDatabase *database = userdata;
	
	printf("Received an reply to our sync\n");
	if (smlStatusGetClass(status) != SML_ERRORCLASS_SUCCESS)
	{
		// inform user
		osync_trace(TRACE_INTERNAL, "%s: The synchronisation request failed.", __func__);
		osync_trace(TRACE_INTERNAL, "%s: Location => %s", __func__, database->env->url);
		osync_trace(TRACE_INTERNAL, "%s: Database => %s", __func__, database->url);
		osync_trace(TRACE_INTERNAL, "%s: Error => %d", __func__, smlStatusGetCode(status));
		if (smlStatusGetCode(status) == SML_ERROR_TIMEOUT &&
		    (strstr(database->env->url, "ocst") || strstr(database->env->url, "ocas")))
		{
			/* this is a potential Oracle Collaboration Suite */
			/* typical errorcode from OCS if there is something wrong */
			osync_trace(TRACE_INTERNAL,
				"%s: Oracle Collaboration Suite detected.",
				__func__);
			osync_trace(TRACE_INTERNAL,
				"%s: Typical undefined error from OCS (503 - SyncML timeout error).",
				__func__);
			osync_trace(TRACE_INTERNAL,
				"%s: Please wait 5 minutes before retry - default session timeout.",
				__func__);
		}
		// stop session
		// FIXME: this is not available in a clean way today
		// FIXME: we need a session state
		// FIXME: osync must be signalled
		// FIXME: we need a mutex lock on database->env
		// smlSessionEnd(database->env->session, NULL);
		// printf("    Session finished.\n");
		// smlManagerSessionRemove(database->env->manager, database->env->session);
		// smlManagerStop(database->env->manager);
		// smlManagerQuit(database->env->manager);
		// printf("    Manager finished.\n");
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}


/* ************************************ */
/* *****     Change Callbacks     ***** */
/* ************************************ */

SmlBool _recv_change(SmlDsSession *dsession, SmlChangeType type, const char *uid, char *data, unsigned int size, const char *contenttype, void *userdata, SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %p, %i, %s, %p, %p)", __func__, dsession, type, uid, data, size, contenttype, userdata, error);
	SmlDatabase *database = (SmlDatabase *)userdata;
	OSyncError *oerror = NULL;

	if (smlDsServerGetServerType(database->server) == SML_DS_CLIENT &&
	    !database->getChangesCtx)
	{
		/* This should be the second OMA DS session of an OMA DS
		 * client. If this is the case then we simply ignore what the
		 * server sends as answer for the second synchronization.
		 * This is not clean but we have here two servers and both
		 * wants to have the last word :(
		 *
		 * OMA DS protocol allows that a client does not send a map.
		 * This is no problem here because the first OMA DS session
		 * should guarantee a clean server database at the remote peer.
		 * Nevertheless this is a hack.
		 */
		osync_trace(TRACE_EXIT, "%s: second OMA DS client connection detected", __func__);
		return TRUE;
	}

	g_assert(database->getChangesCtx);

	if (!type) {
		osync_context_report_success(database->getChangesCtx);
		osync_trace(TRACE_EXIT, "%s", __func__);
		return TRUE;
	}

	OSyncChange *change = osync_change_new(&oerror);
	if (!change) {
		smlErrorSet(error, SML_ERROR_GENERIC, "No change created: %s", osync_error_print(&oerror));
		osync_error_unref(&oerror);
		oerror = NULL;
		goto error;
	}
	
	osync_change_set_uid(change, uid);

	/* XXX Workaround for mobiles which only handle localtime! TODO: make use of UTC field in DevCap and handle it is OpenSync framework! */
	/*
	if (!strcmp(contenttype, SML_ELEMENT_TEXT_VCAL) && env->onlyLocaltime && type != SML_CHANGE_DELETE) {
		char *_data = osync_time_vcal2utc(data);
		g_free(data);
		data = _data;
		size = strlen(data);
	}
	*/

	osync_trace(TRACE_INTERNAL,
		"%s: objformat: %s", __func__,
		osync_objformat_get_name(database->objformat));
	OSyncData *odata = osync_data_new(data, size+1, database->objformat, &oerror);
	if (!odata) {
		goto oerror;
	}

	// who has commented this out => who can remove it?
	// if (_to_osync_changetype(type) == OSYNC_CHANGE_TYPE_DELETED)
	// the contenttype is fully useless here
	// because the contenttype cannot identify the objtype
	// because several objformat share the same contenttype
	osync_data_set_objtype(odata, database->objtype);

	osync_change_set_data(change, odata);
	osync_change_set_changetype(change, _to_osync_changetype(type));

	osync_data_unref(odata);
	odata = NULL;

	osync_context_report_change(database->getChangesCtx, change);

	// if this is a client then we should send a map item here
	if (smlDsServerGetServerType(database->server) == SML_DS_CLIENT)
	{
		// ok let's prepare the map
		if (!smlDsSessionQueueMap(database->session, uid, osync_change_get_uid(change), error))
			goto error;
		database->pendingChanges--;
		if (database->pendingChanges == 0)
		{
			if (!smlDsSessionCloseMap(database->session, _recv_map_reply, database, error))
				goto error;
		}
		osync_trace(TRACE_INTERNAL, "%s: still %d pendingChanges",
			__func__, database->pendingChanges);
	} else {
		// First pendingChanges should only be managed if NumberOfChanges
		// are supported by the client.
		// Second we cannot simply check DevInf for the announced
		// support of the NumberOfChanges feature because some devices
		// (like Sony Ericsson M600i) announce this support but does not
		// send the NumberOfChanges.
		// Third we only need the pending changes at the server side for
		// an assertion which checks that all changes were handled before
		// the batch_commit function is called by OpenSync.
		// So we must only guarantee that pendingChanges is never a
		// negative number and anything is fine.
		if (database->pendingChanges != 0)
			database->pendingChanges--;
	}

	osync_change_unref(change);
	change = NULL;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

oerror:
	smlErrorSet(error, SML_ERROR_GENERIC, "%s", osync_error_print(&oerror));
	osync_error_unref(&oerror);
	oerror = NULL;
error:
	if (change)
	{
		osync_change_unref(change);
		change = NULL;
	}
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, smlErrorPrint(error));
	return FALSE;
}

void _recv_change_reply(SmlDsSession *dsession, SmlStatus *status, const char *newuid, void *userdata)
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

/* ********************************* */
/* *****     Map Callbacks     ***** */
/* ********************************* */

void _recv_map_reply(SmlSession *session, SmlStatus *status, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, session, status, userdata);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/* ******************************************* */
/* *****     Authentication Callback     ***** */
/* ******************************************* */

void _verify_user(SmlAuthenticator *auth, const char *username, const char *password, void *userdata, SmlErrorType *reply)
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

