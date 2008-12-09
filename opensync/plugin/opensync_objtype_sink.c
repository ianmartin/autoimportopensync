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

#include "opensync_objtype_sink.h"
#include "opensync_objtype_sink_private.h"

OSyncObjTypeSink *osync_objtype_main_sink_new(OSyncError **error)
{
  return osync_objtype_sink_new(NULL, error);
}

OSyncObjTypeSink *osync_objtype_sink_new(const char *objtype, OSyncError **error)
{
  OSyncObjTypeSink *sink = osync_try_malloc0(sizeof(OSyncObjTypeSink), error);
  if (!sink)
    return FALSE;
	
  sink->objtype = g_strdup(objtype);
  sink->ref_count = 1;

  sink->preferred_format = NULL;
	
  sink->read = TRUE;
  sink->getchanges = TRUE;
  sink->write = TRUE;

  sink->enabled = TRUE;

  memset(&sink->timeout, 0, sizeof(sink->timeout));

  return sink;
}

OSyncObjTypeSink *osync_objtype_sink_ref(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
	
  g_atomic_int_inc(&(sink->ref_count));

  return sink;
}

void osync_objtype_sink_unref(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
	
  if (g_atomic_int_dec_and_test(&(sink->ref_count))) {
    while (sink->objformatsinks) {
      osync_objformat_sink_unref(sink->objformatsinks->data);
      sink->objformatsinks = osync_list_remove(sink->objformatsinks, sink->objformatsinks->data);
    }
		
    if (sink->preferred_format)
      g_free(sink->preferred_format);

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

const char *osync_objtype_sink_get_preferred_format(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->preferred_format;
}

void osync_objtype_sink_set_preferred_format(OSyncObjTypeSink *sink, const char *preferred_format)
{
  osync_assert(sink);
  if (sink->preferred_format)
    g_free(sink->preferred_format);
  sink->preferred_format = g_strdup(preferred_format);
}

unsigned int osync_objtype_sink_num_objformat_sinks(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return osync_list_length(sink->objformatsinks);
}

OSyncObjFormatSink *osync_objtype_sink_nth_objformat_sink(OSyncObjTypeSink *sink, unsigned int nth)
{
  osync_assert(sink);
  return osync_list_nth_data(sink->objformatsinks, nth);
}

OSyncObjFormatSink *osync_objtype_sink_find_objformat_sink(OSyncObjTypeSink *sink, OSyncObjFormat *objformat)
{
  OSyncList *f = NULL;
  osync_assert(sink);
  osync_assert(objformat);

  f = sink->objformatsinks;
  for (; f; f = f->next) {
    OSyncObjFormatSink *formatsink = f->data;
    const char *objformat_name = osync_objformat_get_name(objformat);
    if (!strcmp(osync_objformat_sink_get_objformat(formatsink), objformat_name))
      return formatsink;
  }
  return NULL;
}

OSyncList *osync_objtype_sink_get_objformat_sinks(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->objformatsinks;
}

void osync_objtype_sink_add_objformat_sink(OSyncObjTypeSink *sink, OSyncObjFormatSink *objformatsink)
{
  osync_assert(sink);
  osync_assert(objformatsink);

  if (!osync_list_find(sink->objformatsinks, objformatsink)) {
    sink->objformatsinks = osync_list_append(sink->objformatsinks, objformatsink);
    osync_objformat_sink_ref(objformatsink);
  }
}

void osync_objtype_sink_remove_objformat_sink(OSyncObjTypeSink *sink, OSyncObjFormatSink *objformatsink)
{
  osync_assert(sink);
  osync_assert(objformatsink);

  sink->objformatsinks = osync_list_remove(sink->objformatsinks, objformatsink);
  osync_objformat_sink_unref(objformatsink);
}

void osync_objtype_sink_set_functions(OSyncObjTypeSink *sink, OSyncObjTypeSinkFunctions functions, void *userdata)
{
  osync_assert(sink);
  sink->functions = functions;
  sink->userdata = userdata;

  if (functions.read)
    sink->func_read = TRUE;

  if (functions.get_changes)
    sink->func_getchanges = TRUE;

  if (functions.write)
    sink->func_write = TRUE;
}

osync_bool osync_objtype_sink_get_function_read(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->func_read;
}

void osync_objtype_sink_set_function_read(OSyncObjTypeSink *sink, osync_bool read)
{
  osync_assert(sink);
  sink->func_read = read;
}

osync_bool osync_objtype_sink_get_function_getchanges(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->func_getchanges;
}

void osync_objtype_sink_set_function_getchanges(OSyncObjTypeSink *sink, osync_bool getchanges)
{
  osync_assert(sink);
  sink->func_getchanges = getchanges;
}

osync_bool osync_objtype_sink_get_function_write(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->func_write;
}

void osync_objtype_sink_set_function_write(OSyncObjTypeSink *sink, osync_bool write)
{
  osync_assert(sink);
  sink->func_write = write;
}

void *osync_objtype_sink_get_userdata(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->userdata;
}

void osync_objtype_sink_get_changes(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx)
{
  OSyncObjTypeSinkFunctions functions;
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, sink, plugindata, info, ctx);
  osync_assert(sink);
  osync_assert(ctx);
	
  functions = sink->functions;
  if (sink->objtype && !functions.get_changes) {
    osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No get_changeinfo function was given");
    osync_trace(TRACE_EXIT_ERROR, "%s: No get_changes function was given", __func__);
    return;
  } else if (!functions.get_changes) {
    osync_context_report_success(ctx);
  } else {
    functions.get_changes(plugindata, info, ctx);
  }

  osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_objtype_sink_read_change(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncChange *change, OSyncContext *ctx)
{
  OSyncObjTypeSinkFunctions functions;
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, sink, plugindata, info, change, ctx);
  osync_assert(sink);
  osync_assert(ctx);
  osync_assert(change);
	
  functions = sink->functions;


  if (sink->objtype && !functions.read) {
    osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No read function was given");
    osync_trace(TRACE_EXIT_ERROR, "%s: No read function was given", __func__);
    return;
  } else if (!functions.read) {
    osync_context_report_success(ctx);
  } else {
    functions.read(plugindata, info, ctx, change);
  }

  osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_objtype_sink_connect(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx)
{
  OSyncObjTypeSinkFunctions functions;
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, sink, plugindata, info, ctx);
  osync_assert(sink);
  osync_assert(ctx);
	
  functions = sink->functions;
  if (!functions.connect) {
    osync_context_report_success(ctx);
  } else {
    functions.connect(plugindata, info, ctx);
  }

  osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_objtype_sink_disconnect(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx)
{
  OSyncObjTypeSinkFunctions functions;
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, sink, plugindata, info, ctx);
  osync_assert(sink);
  osync_assert(ctx);
	
  functions = sink->functions;
  if (!functions.disconnect) {
    osync_context_report_success(ctx);
  } else {
    functions.disconnect(plugindata, info, ctx);
  }

  osync_trace(TRACE_EXIT, "%s", __func__);
}

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

    /* Increment refcounting for batch_commit to avoid too early freeing of the context.
       Otherwise the context would get freed after this function call. But the batch_commit
       is collecting every contexts and changes and finally commits everything at once. */
    osync_context_ref(ctx);
    sink->commit_contexts = g_list_append(sink->commit_contexts, ctx);
    osync_trace(TRACE_EXIT, "%s: Waiting for batch processing", __func__);
    return;
  } else {
    // Send the change
    if (sink->objtype && !functions.commit) {
      osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No commit_change function was given");
      osync_trace(TRACE_EXIT_ERROR, "%s: No commit_change function was given", __func__);
      return;
    } else if (!functions.commit) {
      osync_context_report_success(ctx);
    } else {
      functions.commit(plugindata, info, ctx, change);
    }
  }

  osync_trace(TRACE_EXIT, "%s", __func__);
}

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

osync_bool osync_objtype_sink_get_write(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->write;
}

void osync_objtype_sink_set_write(OSyncObjTypeSink *sink, osync_bool write)
{
  osync_assert(sink);
  sink->write = write;
}

void osync_objtype_sink_set_getchanges(OSyncObjTypeSink *sink, osync_bool getchanges)
{
  osync_assert(sink);
  sink->getchanges = getchanges;
}

osync_bool osync_objtype_sink_get_getchanges(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->getchanges;
}

osync_bool osync_objtype_sink_get_read(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->read;
}

void osync_objtype_sink_set_read(OSyncObjTypeSink *sink, osync_bool read)
{
  osync_assert(sink);
  sink->read = read;
}

osync_bool osync_objtype_sink_get_slowsync(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->slowsync;
}

void osync_objtype_sink_set_slowsync(OSyncObjTypeSink *sink, osync_bool slowsync)
{
  osync_assert(sink);
  osync_trace(TRACE_INTERNAL, "%s: Setting slow-sync of object type \"%s\" to %i", __func__, sink->objtype, slowsync);
  sink->slowsync = slowsync;
}

void osync_objtype_sink_set_connect_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
  osync_assert(sink);
  sink->timeout.connect = timeout;
}

unsigned int osync_objtype_sink_get_connect_timeout_or_default(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.connect ? sink->timeout.connect : OSYNC_SINK_TIMEOUT_CONNECT;
}

unsigned int osync_objtype_sink_get_connect_timeout(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.connect;
}

void osync_objtype_sink_set_disconnect_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
  osync_assert(sink);
  sink->timeout.disconnect = timeout;
}

unsigned int osync_objtype_sink_get_disconnect_timeout_or_default(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.disconnect ? sink->timeout.disconnect : OSYNC_SINK_TIMEOUT_DISCONNECT;
}

unsigned int osync_objtype_sink_get_disconnect_timeout(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.disconnect;
}

void osync_objtype_sink_set_getchanges_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
  osync_assert(sink);
  sink->timeout.get_changes = timeout;
}

unsigned int osync_objtype_sink_get_getchanges_timeout_or_default(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.commit ? sink->timeout.get_changes : OSYNC_SINK_TIMEOUT_GETCHANGES;
}

unsigned int osync_objtype_sink_get_getchanges_timeout(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.get_changes;
}

void osync_objtype_sink_set_commit_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
  osync_assert(sink);
  sink->timeout.commit = timeout;
}

unsigned int osync_objtype_sink_get_commit_timeout_or_default(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.commit ? sink->timeout.commit : OSYNC_SINK_TIMEOUT_COMMIT;
}

unsigned int osync_objtype_sink_get_commit_timeout(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.commit;
}

void osync_objtype_sink_set_batchcommit_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
  osync_assert(sink);
  sink->timeout.batch_commit = timeout;
}

unsigned int osync_objtype_sink_get_batchcommit_timeout_or_default(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.batch_commit ? sink->timeout.batch_commit : OSYNC_SINK_TIMEOUT_BATCHCOMMIT;
}

unsigned int osync_objtype_sink_get_batchcommit_timeout(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.batch_commit;
}

void osync_objtype_sink_set_committedall_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
  osync_assert(sink);
  sink->timeout.committed_all = timeout;
}

unsigned int osync_objtype_sink_get_committedall_timeout_or_default(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.committed_all ? sink->timeout.committed_all : OSYNC_SINK_TIMEOUT_COMMITTEDALL;
}

unsigned int osync_objtype_sink_get_committedall_timeout(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.committed_all;
}

void osync_objtype_sink_set_syncdone_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
  osync_assert(sink);
  sink->timeout.sync_done = timeout;
}

unsigned int osync_objtype_sink_get_syncdone_timeout_or_default(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.sync_done ? sink->timeout.sync_done : OSYNC_SINK_TIMEOUT_SYNCDONE;
}

unsigned int osync_objtype_sink_get_syncdone_timeout(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.sync_done;
}

void osync_objtype_sink_set_write_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
  osync_assert(sink);
  sink->timeout.write = timeout;
}

unsigned int osync_objtype_sink_get_write_timeout_or_default(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.write ? sink->timeout.write : OSYNC_SINK_TIMEOUT_WRITE;
}

unsigned int osync_objtype_sink_get_write_timeout(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.write;
}

void osync_objtype_sink_set_read_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
  osync_assert(sink);
  sink->timeout.read = timeout;
}

unsigned int osync_objtype_sink_get_read_timeout_or_default(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.read ? sink->timeout.read : OSYNC_SINK_TIMEOUT_READ;
}

unsigned int osync_objtype_sink_get_read_timeout(OSyncObjTypeSink *sink)
{
  osync_assert(sink);
  return sink->timeout.read;
}

