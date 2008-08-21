/*
 * syncml plugin - A syncml plugin for OpenSync
 * Copyright (C) 2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2008  Michael Bell <michael.bell@opensync.org>
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
#include "syncml_vformat.h"
#include "syncml_devinf.h"
#include "syncml_ds_client.h"
#include "syncml_ds_server.h"

/* **************************************** */
/* *****     Management Callbacks     ***** */
/* **************************************** */

void _recv_event(
		SmlDataSyncObject *dsObject,
		SmlDataSyncEventType type,
		void *userdata,
		SmlError *error)
{
	smlTrace(TRACE_ENTRY, "%s(%p, %i, %p, %p)", __func__, dsObject, type, userdata, error);
	SmlError *locerror = NULL;
	SmlPluginEnv *env = userdata;
	GList *o = NULL;

	/* cache connection state */
	if (dsObject == env->dsObject1)
		env->state1 = type;
	else
		env->state2 = type;
	
	switch (type) {
		case SML_DATA_SYNC_EVENT_ERROR:
			smlErrorDuplicate(&error, &locerror);
			smlErrorDeref(&error);
			goto error;
			break;
		case SML_DATA_SYNC_EVENT_CONNECT:
			break;
		case SML_DATA_SYNC_EVENT_DISCONNECT:
			break;
		case SML_DATA_SYNC_EVENT_FINISHED:
			if (dsObject == env->dsObject1 && env->dsObject2) {
				if (!smlDataSyncInit(env->dsObject2, &locerror))
					goto error;
				if (!smlDataSyncRun(env->dsObject2, &locerror))
					goto error;
			} else {
				/* This is the real end of the sync process and
				 * so it is a good idea to commit the changes here
				 * and not earlier because now the remote peer
				 * commits too.
				 */
				o = env->databases;
				for (; o; o = o->next) {
					SmlDatabase *database = o->data;
					g_assert(database->commitCtx);
					report_success_on_context(&(database->commitCtx));
				}

				/* a real disconnet happens */
				if (env->disconnectCtx)
					report_success_on_context(&(env->disconnectCtx));
			}
			break;
		case SML_DATA_SYNC_EVENT_GOT_ALL_ALERTS:
			if (dsObject == env->dsObject1)
			{
				osync_trace(TRACE_INTERNAL, "session established");
				if (env->connectCtx)
					report_success_on_context(&(env->connectCtx));
			} else {
				if (!smlDataSyncSendChanges(env->dsObject2, &locerror))
					goto error;
			}
			break;
		case SML_DATA_SYNC_EVENT_GOT_ALL_CHANGES:
			if (dsObject == env->dsObject1)
			{
				o = env->databases;
				for (; o; o = o->next) {
					SmlDatabase *database = o->data;
					g_assert(database->getChangesCtx);
					report_success_on_context(&(database->getChangesCtx));
				}
			}
			break;
		case SML_DATA_SYNC_EVENT_GOT_ALL_MAPPINGS:
			break;
		default:
			g_error("Unknown event(%d).\n", type);
			break;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
error:
	osync_trace(TRACE_INTERNAL, "%s: Cleaning up because of an error ...", __func__);
	OSyncError *oserror = NULL;
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, smlErrorPrint(&error));

	if (env->connectCtx)
		report_error_on_context(&(env->connectCtx), &oserror, FALSE);
	
	if (env->disconnectCtx)
		report_error_on_context(&(env->disconnectCtx), &oserror, FALSE);

	o = env->databases;
	for (; o; o = o->next) {
		SmlDatabase *database = o->data;
		
		if (database->syncModeCtx)
			report_error_on_context(&(database->syncModeCtx), &oserror, FALSE);

		if (database->getChangesCtx)
			report_error_on_context(&(database->getChangesCtx), &oserror, FALSE);

		if (database->commitCtx)
			report_error_on_context(&(database->commitCtx), &oserror, FALSE);
	}

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
	osync_error_unref(&oserror);
}

/* *********************************** */
/* *****     Alert Callbacks     ***** */
/* *********************************** */

SmlAlertType _get_alert_type(
			SmlDataSyncObject *dsObject,
			const char *source,
			SmlAlertType type,
			void *userdata,
			SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s - %s: %d", __func__, source, type);

	SmlPluginEnv *env = userdata;
	SmlDatabase *database = get_database_from_source(env, source, error);
	if (!database)
		goto error;

	/* locate alert type of sink */
	if (osync_objtype_sink_get_slowsync(database->sink))
		type = SML_ALERT_SLOW_SYNC;
	else
		type = SML_ALERT_TWO_WAY;

	osync_trace(TRACE_EXIT, "%s - %d", __func__, type);
	return type;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s - %s", __func__, smlErrorPrint(error));
	return SML_ALERT_UNKNOWN;
}

char *_get_anchor(
		SmlDataSyncObject *dsObject,
		const char *name,
		void *userdata,
		SmlError **error)
{
	SmlPluginEnv *env = userdata;
	return osync_anchor_retrieve(env->anchor_path, name);
}

SmlBool _set_anchor(
		SmlDataSyncObject *dsObject,
		const char *name,
		const char *value,
		void *userdata,
		SmlError **error)
{
	SmlPluginEnv *env = userdata;
	osync_anchor_update(env->anchor_path, name, value);
	return TRUE;
}

/* ************************************ */
/* *****     Change Callbacks     ***** */
/* ************************************ */

SmlBool _recv_change(
		SmlDataSyncObject *dsObject,
		const char *source,
		SmlChangeType type,
		const char *uid,
		char *data,
		unsigned int size,
		void *userdata,
		SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %p, %i, %s, %p, %p)", __func__, dsObject, type, uid, data, size, source, userdata, error);
	SmlPluginEnv *env = userdata;
	SmlDatabase *database = get_database_from_source(env, source, error);
	if (!database)
		goto error;
	OSyncError *oerror = NULL;

	g_assert(database->getChangesCtx);
	g_assert(type);

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
		safe_cfree(&data);
		data = _data;
		size = strlen(data);
	}
	*/

	osync_trace(TRACE_INTERNAL,
		"%s: objformat: %s", __func__,
		osync_objformat_get_name(database->objformat));
	OSyncData *odata = osync_data_new(data, size, database->objformat, &oerror);
	if (!odata) {
		goto oerror;
	}

	osync_data_set_objtype(odata, database->objtype);

	osync_change_set_data(change, odata);

	/* If a SLOW-SYNC happens then OpenSync only expects ADD commands.
	 * If a REPLACE is detected instead of an ADD then this is fixed.
	 */
	if (osync_objtype_sink_get_slowsync(database->sink) &&
	    type == SML_CHANGE_REPLACE)
	{
		osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_ADDED);
	} else {
		osync_change_set_changetype(change, _to_osync_changetype(type));
	}

	osync_data_unref(odata);
	odata = NULL;

	osync_context_report_change(database->getChangesCtx, change);

	// if this is a client then we should send a map item here
	if (env->sessionType == SML_SESSION_TYPE_CLIENT)
	{
		// ok let's prepare the map
		if (!smlDataSyncAddMapping(dsObject, source, uid, osync_change_get_uid(change), error))
			goto error;
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

SmlBool _recv_change_status(
		SmlDataSyncObject *dsObject,
		unsigned int code,
		const char *newuid,
		void *userdata,
		SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %d, %s, %p)", __func__, dsObject, code, newuid, userdata);
	struct commitContext *ctx = userdata;
	
	if (code < 200 || 299 < code) {
		OSyncError *error = NULL;
		osync_error_set(
			&error, OSYNC_ERROR_GENERIC,
			"Unable to commit change. Error %i",
			code);
		report_error_on_context(&(ctx->context), &error, TRUE);
	} else {
		if (newuid)
			osync_change_set_uid(ctx->change, newuid);
		report_success_on_context(&(ctx->context));
	}

	// cleanup
	osync_change_unref(ctx->change);
	ctx->database = NULL;
	safe_free((gpointer *)&ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

SmlBool _recv_unwanted_change(
		SmlDataSyncObject *dsObject,
		const char *source,
		SmlChangeType type,
		const char *uid,
		char *data,
		unsigned int size,
		void *userdata,
		SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %p, %i, %s, %p, %p)", __func__, dsObject, type, uid, data, size, source, userdata, error);

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

	if (type == SML_CHANGE_DELETE)
	{
		/* Now we are in real trouble. The server wants to
		 * delete an entry and it is impossible to propagate
		 * this to OpenSync's plugin mechanisms. So the
		 * important question when does this happen?
		 *
		 * Answer: on SLOW-SYNC
		 *
		 * 1. No data is send from the OMA DS client to the OMA
		 *    DS server during the first session.
		 * 2. The OMA DS server sends all relevant(!!!) data
		 *    to the OMA DS client during the first session.
		 * 3. OpenSync runs its merger.
		 * 4. The OMA DS client sends all data to the OMA DS
		 *    server including some old events.
		 * 5. The OMA DS server tries to delete these events
		 *    on the client because the client should only use
		 *    the actual events (common option on calendar
		 *    servers).
		 *
		 * It is not necessary to do something because both
		 * parties now it and both can ignore it now.
		 */
		osync_trace(TRACE_EXIT, "%s - ignore Delete command", __func__);
		return TRUE;
	} else {
		/* This problem should be fixed with the next SLOW-SYNC. */
		osync_trace(TRACE_EXIT_ERROR, "%s - unexpected Add or Replace command", __func__);
		smlErrorSet(error, SML_ERROR_TEMPORARY, "Unwanted Add or Replace command on second OMA DS session.");
		return FALSE;
	}
}

