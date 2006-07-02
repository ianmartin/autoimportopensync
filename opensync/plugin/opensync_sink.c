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
#include "opensync_sink.h"
#include "opensync_sink_internals.h"

OSyncObjTypeSink *osync_objtype_sink_new(const char *objtype, OSyncError **error)
{
	OSyncObjTypeSink *sink = osync_try_malloc0(sizeof(OSyncObjTypeSink), error);
	if (!sink)
		return FALSE;
	
	sink->objtype = g_strdup(objtype);
	sink->ref_count = 1;
	
	return sink;
}

void osync_objtype_sink_ref(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	
	g_atomic_int_inc(&(sink->ref_count));
}

void osync_objtype_sink_unref(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	
	if (g_atomic_int_dec_and_test(&(sink->ref_count))) {
		while (sink->objformats) {
			char *format = sink->objformats->data;
			g_free(format);
			sink->objformats = g_list_remove(sink->objformats, format);
		}
		
		if (sink->objtype)
			g_free(sink->objtype);
		
		g_free(sink);
	}
}

const char *osync_objtype_sink_get_name(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->objtype;
}

void osync_objtype_sink_set_name(OSyncObjTypeSink *sink, const char *name)
{
	osync_assert(sink);
	if (sink->objtype)
		g_free(sink->objtype);
	sink->objtype = g_strdup(name);
}

int osync_objtype_sink_num_objformats(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return g_list_length(sink->objformats);
}

const char *osync_objtype_sink_nth_objformat(OSyncObjTypeSink *sink, int nth)
{
	osync_assert(sink);
	return g_list_nth_data(sink->objformats, nth);
}

void osync_objtype_sink_add_objformat(OSyncObjTypeSink *sink, const char *format)
{
	osync_assert(sink);
	osync_assert(format);
	sink->objformats = g_list_append(sink->objformats, g_strdup(format));
}

void osync_objtype_sink_remove_objformat(OSyncObjTypeSink *sink, const char *format)
{
	GList *f = NULL;
	osync_assert(sink);
	osync_assert(format);
	for (f = sink->objformats; f; f = f->next) {
		if (!strcmp((char *)f->data, format)) {
			sink->objformats = g_list_remove(sink->objformats, f->data);
			break;
		}
	}
}

void osync_objtype_sink_set_functions(OSyncObjTypeSink *sink, OSyncObjTypeSinkFunctions functions)
{
	osync_assert(sink);
	sink->functions = functions;
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
void osync_objtype_sink_get_changes(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	OSyncObjTypeSinkFunctions functions;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, sink, plugindata, info, ctx);
	osync_assert(sink);
	osync_assert(ctx);
	
	functions = sink->functions;
	if (!functions.get_changes) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No get_changeinfo function was given");
		osync_trace(TRACE_EXIT_ERROR, "%s: No get_changes function was given", __func__);
		return;
	}
	functions.get_changes(plugindata, info, ctx);
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
void osync_objtype_sink_read_change(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncChange *change, OSyncContext *ctx)
{
	OSyncObjTypeSinkFunctions functions;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, sink, plugindata, info, change, ctx);
	osync_assert(sink);
	osync_assert(ctx);
	osync_assert(change);
	
	functions = sink->functions;
	if (!functions.read) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No read function was given");
		osync_trace(TRACE_EXIT_ERROR, "%s: No read function was given", __func__);
		return;
	}
	functions.read(plugindata, info, ctx, change);
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
void osync_objtype_sink_connect(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	OSyncObjTypeSinkFunctions functions;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, sink, plugindata, info, ctx);
	osync_assert(sink);
	osync_assert(ctx);
	
	functions = sink->functions;
	if (!functions.connect) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No connect function was given");
		osync_trace(TRACE_EXIT_ERROR, "%s: No connect function was given", __func__);
		return;
	}
	functions.connect(plugindata, info, ctx);
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
void osync_objtype_sink_disconnect(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	OSyncObjTypeSinkFunctions functions;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, sink, plugindata, info, ctx);
	osync_assert(sink);
	osync_assert(ctx);
	
	functions = sink->functions;
	if (!functions.disconnect) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No disconnect function was given");
		osync_trace(TRACE_EXIT_ERROR, "%s: No disconnect function was given", __func__);
		return;
	}
	functions.disconnect(plugindata, info, ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Tells the plugin that the sync was successful
 * 
 * Calls the sync_done function on a plugin
 * 
 * @param member The member
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_objtype_sink_sync_done(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	OSyncObjTypeSinkFunctions functions;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, sink, plugindata, info, ctx);
	osync_assert(sink);
	osync_assert(ctx);
	
	functions = sink->functions;
	if (!functions.sync_done)
		osync_context_report_success(ctx);
	else
		functions.sync_done(plugindata, info, ctx);
	
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
void osync_objtype_sink_commit_change(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncChange *change, OSyncContext *ctx)
{
	OSyncObjTypeSinkFunctions functions;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, sink, plugindata, info, change, ctx);
	g_assert(sink);
	g_assert(change);
	g_assert(ctx);

	functions = sink->functions;

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
		functions.commit(plugindata, info, ctx, change);
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
void osync_objtype_sink_committed_all(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	OSyncObjTypeSinkFunctions functions;
	int i = 0;
	GList *o = NULL;
	GList *c = NULL;
	OSyncChange *change = NULL;
	OSyncContext *context = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, sink, plugindata, info, ctx);
	osync_assert(sink);
	osync_assert(ctx);
	
	functions = sink->functions;
	if (functions.batch_commit) {
		OSyncChange **changes = g_malloc0(sizeof(OSyncChange *) * (g_list_length(sink->commit_changes) + 1));
		OSyncContext **contexts = g_malloc0(sizeof(OSyncContext *) * (g_list_length(sink->commit_contexts) + 1));
		
		o = sink->commit_contexts;
		c = NULL;
		for (c = sink->commit_changes; c && o; c = c->next) {
			change = c->data;
			context = o->data;
			
			changes[i] = change;
			contexts[i] = context;
			
			i++;
			o = o->next;
		}
		
		g_list_free(sink->commit_changes);
		g_list_free(sink->commit_contexts);
		
		functions.batch_commit(plugindata, info, ctx, contexts, changes);
		
		g_free(changes);
		g_free(contexts);
	} else if (functions.committed_all) {
		functions.committed_all(plugindata, info, ctx);
	} else {
		osync_context_report_success(ctx);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool osync_objtype_sink_is_enabled(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->enabled;
}

void osync_objtype_sink_set_enabled(OSyncObjTypeSink *sink, osync_bool enabled)
{
	osync_assert(sink);
	sink->enabled = enabled;
}

osync_bool osync_objtype_sink_is_available(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->available;
}

void osync_objtype_sink_set_available(OSyncObjTypeSink *sink, osync_bool available)
{
	osync_assert(sink);
	sink->available = available;
}
