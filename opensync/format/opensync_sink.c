/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
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
 
#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-context.h"
#include "opensync-format.h"
#include "opensync_sink_internals.h"

OSyncObjFormatSink *osync_sink_new(OSyncObjFormatSinkFunctions functions, const char *format, const char *objtype, OSyncError **error)
{
	OSyncObjFormatSink *sink = osync_try_malloc0(sizeof(OSyncObjFormatSink), error);
	if (!sink)
		return FALSE;
	
	sink->format = g_strdup(format);
	sink->objtype = g_strdup(objtype);
	sink->functions = functions;
	sink->ref_count = 1;
	
	return sink;
}

void osync_sink_ref(OSyncObjFormatSink *sink)
{
	osync_assert(sink);
	
	g_atomic_int_inc(&(sink->ref_count));
}

void osync_sink_unref(OSyncObjFormatSink *sink)
{
	osync_assert(sink);
	
	if (g_atomic_int_dec_and_test(&(sink->ref_count))) {
		if (sink->format)
			g_free(sink->format);
			
		if (sink->objtype)
			g_free(sink->objtype);
		
		g_free(sink);
	}
}

const char *osync_sink_get_objformat(OSyncObjFormatSink *sink)
{
	osync_assert(sink);
	return sink->format;
}

const char *osync_sink_get_objtype(OSyncObjFormatSink *sink)
{
	osync_assert(sink);
	return sink->objtype;
}

/** @brief Queries a plugin for the changed objects since the last sync
 * 
 * Calls the get_changeinfo function on a plugin
 * 
 * @param member The member
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_sink_get_changes(OSyncObjFormatSink *sink, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, sink, ctx);
	osync_assert(sink);
	osync_assert(ctx);
	
	OSyncObjFormatSinkFunctions functions = sink->functions;
	if (!functions.get_changes) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No get_changeinfo function was given");
		osync_trace(TRACE_EXIT_ERROR, "%s: No get_changes function was given", __func__);
		return;
	}
	functions.get_changes(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Reads a single object by its uid
 * 
 * Calls the read_change function on the plugin
 * 
 * @param member The member
 * @param change The change to read. The change must have the uid set
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_sink_read_change(OSyncObjFormatSink *sink, OSyncChange *change, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, sink, change, ctx);
	osync_assert(sink);
	osync_assert(ctx);
	osync_assert(change);
	
	OSyncObjFormatSinkFunctions functions = sink->functions;
	if (!functions.read) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No read function was given");
		osync_trace(TRACE_EXIT_ERROR, "%s: No read function was given", __func__);
		return;
	}
	functions.read(ctx, change);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Connects a member to its device
 * 
 * Calls the connect function on a plugin
 * 
 * @param member The member
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_sink_connect(OSyncObjFormatSink *sink, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, sink, ctx);
	osync_assert(sink);
	osync_assert(ctx);
	
	OSyncObjFormatSinkFunctions functions = sink->functions;
	if (!functions.connect) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No connect function was given");
		osync_trace(TRACE_EXIT_ERROR, "%s: No connect function was given", __func__);
		return;
	}
	functions.connect(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Disconnects a member from its device
 * 
 * Calls the disconnect function on a plugin
 * 
 * @param member The member
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_sink_disconnect(OSyncObjFormatSink *sink, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, sink, ctx);
	osync_assert(sink);
	osync_assert(ctx);
	
	OSyncObjFormatSinkFunctions functions = sink->functions;
	if (!functions.disconnect) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No disconnect function was given");
		osync_trace(TRACE_EXIT_ERROR, "%s: No disconnect function was given", __func__);
		return;
	}
	functions.disconnect(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Tells the plugin that the sync was successfull
 * 
 * Calls the sync_done function on a plugin
 * 
 * @param member The member
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_sink_sync_done(OSyncObjFormatSink *sink, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, sink, ctx);
	osync_assert(sink);
	osync_assert(ctx);
	
	OSyncObjFormatSinkFunctions functions = sink->functions;
	if (!functions.sync_done) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No sync_done function was given");
		osync_trace(TRACE_EXIT_ERROR, "%s: No sync_done function was given", __func__);
		return;
	}
	functions.sync_done(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Commits a change to the device
 * 
 * Calls the commit_change function on a plugin
 * 
 * @param member The member
 * @param change The change to write
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_sink_commit_change(OSyncObjFormatSink *sink, OSyncChange *change, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, sink, change, ctx);
	g_assert(sink);
	g_assert(change);
	g_assert(ctx);

	OSyncObjFormatSinkFunctions functions = sink->functions;

	if (functions.batch_commit) {
		//Append to the stored changes
		sink->commit_changes = g_list_append(sink->commit_changes, change);
		sink->commit_contexts = g_list_append(sink->commit_contexts, ctx);
		osync_trace(TRACE_EXIT, "%s: Waiting for batch processing", __func__);
		return;
	} else {
		// Send the change
		if (!functions.commit) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No commit_change function was given");
			osync_trace(TRACE_EXIT_ERROR, "%s: No commit_change function was given", __func__);
			return;
		}
		functions.commit(ctx, change);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Tells the plugin that all changes have been committed
 * 
 * Calls the committed_all function on a plugin or the batch_commit function
 * depending on which function the plugin wants to use.
 * 
 * @param member The member
 * @param function The callback that will receive the answer
 * @param user_data The userdata to pass to the callback
 * 
 */
void osync_sink_committed_all(OSyncObjFormatSink *sink, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, sink, ctx);
	osync_assert(sink);
	osync_assert(ctx);
	
	OSyncObjFormatSinkFunctions functions = sink->functions;
	if (functions.batch_commit) {
		int i = 0;
		OSyncChange **changes = g_malloc0(sizeof(OSyncChange *) * (g_list_length(sink->commit_changes) + 1));
		OSyncContext **contexts = g_malloc0(sizeof(OSyncContext *) * (g_list_length(sink->commit_contexts) + 1));
		
		GList *o = sink->commit_contexts;
		GList *c = NULL;
		for (c = sink->commit_changes; c && o; c = c->next) {
			OSyncChange *change = c->data;
			OSyncContext *context = o->data;
			
			changes[i] = change;
			contexts[i] = context;
			
			i++;
			o = o->next;
		}
		
		g_list_free(sink->commit_changes);
		g_list_free(sink->commit_contexts);
		
		functions.batch_commit(ctx, contexts, changes);
		
		g_free(changes);
		g_free(contexts);
	} else if (functions.committed_all) {
		functions.committed_all(ctx);
	} else {
		osync_context_report_success(ctx);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}
