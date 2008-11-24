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

/**
 * @defgroup OSyncObjTypeSinkAPI OpenSync Object Type Sink
 * @ingroup OSyncPublic
 * @brief Functions to register and manage object type sinks
 * 
 */
/*@{*/

/*! @brief Creates a new main sink
 *
 * Main sink is objtype neutrual and should be used for object type
 * neutral actions. Actions like connecting/disconnecting which could
 * be a object type neutral - e.g. connecting to a device via bluetooth.
 * Object type specific example would be a connection to different 
 * object type specific databases/resources.
 *
 * Main sink is not limited to the connect and disconnect functions! 
 * 
 * @param error Pointer to an error struct
 * @returns the newly created main sink
 */
OSyncObjTypeSink *osync_objtype_main_sink_new(OSyncError **error)
{
	return osync_objtype_sink_new(NULL, error);
}

/*! @brief Creates a new sink for an object type
 *
 * @param objtype The name of the object type for the sink
 * @param error Pointer to an error struct
 * @returns the newly created sink
 */
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

/*! @brief Increase the reference count on a sink
 * 
 * @param sink Pointer to the sink
 * 
 */
OSyncObjTypeSink *osync_objtype_sink_ref(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	
	g_atomic_int_inc(&(sink->ref_count));

	return sink;
}

/*! @brief Decrease the reference count on a sink
 * 
 * @param sink Pointer to the sink
 * 
 */
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

/*! @brief Return the name of the object type of a sink
 * 
 * @param sink Pointer to the sink
 * @returns the name of the object type of the specified sink
 * 
 */
const char *osync_objtype_sink_get_name(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->objtype;
}

/*! @brief Set the object type of a sink
 * 
 * @param sink Pointer to the sink
 * @param name the name of the object type to set
 * 
 */
void osync_objtype_sink_set_name(OSyncObjTypeSink *sink, const char *name)
{
	osync_assert(sink);
	if (sink->objtype)
		g_free(sink->objtype);
	sink->objtype = g_strdup(name);
}

/*! @brief Return the preferred format for the conversion 
 * 
 * @param sink Pointer to the sink
 * @returns the name of the preferred format
 * 
 */
const char *osync_objtype_sink_get_preferred_format(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->preferred_format;
}

/*! @brief Set the preferred format for the conversion
 * 
 * @param sink Pointer to the sink
 * @param name the name of the preferred format to set
 * 
 */
void osync_objtype_sink_set_preferred_format(OSyncObjTypeSink *sink, const char *preferred_format)
{
	osync_assert(sink);
	if (sink->preferred_format)
		g_free(sink->preferred_format);
	sink->preferred_format = g_strdup(preferred_format);
}

/*! @brief Returns the number of object formats in the sink
 * 
 * @param sink Pointer to the sink
 * @returns the number of object formats in the sink
 * 
 */
unsigned int osync_objtype_sink_num_objformat_sinks(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return osync_list_length(sink->objformatsinks);
}

/*! @brief Returns the nth object format in the sink
 * 
 * @param sink Pointer to the sink
 * @param nth the index of the object format to return
 * @returns the name of the object format at the specified index
 * 
 */
OSyncObjFormatSink *osync_objtype_sink_nth_objformat_sink(OSyncObjTypeSink *sink, unsigned int nth)
{
	osync_assert(sink);
	return osync_list_nth_data(sink->objformatsinks, nth);
}

/*! @brief Finds the objformat sink for the corresponding objformat 
 * 
 * @param sink Pointer to the sink
 * @param objformat the objformat to look for the corresponding objformat sink
 * @returns Pointer to the corresponding objformat sink if found, NULL otherwise 
 * 
 */
OSyncObjFormatSink *osync_objtype_sink_find_objformat_sink(OSyncObjTypeSink *sink, OSyncObjFormat *objformat)
{
	osync_assert(sink);
	osync_assert(objformat);

	OSyncList *f = sink->objformatsinks;
	for (; f; f = f->next) {
		OSyncObjFormatSink *formatsink = f->data;
		const char *objformat_name = osync_objformat_get_name(objformat);
		if (!strcmp(osync_objformat_sink_get_objformat(formatsink), objformat_name))
			return formatsink;
	}
	return NULL;
}

/*! @brief Get list of object format sinks 
 * 
 * @param sink Pointer to the sink
 * @returns List of object format sinks 
 * 
 */
OSyncList *osync_objtype_sink_get_objformat_sinks(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->objformatsinks;
}


/** @brief Adds an object format sink to the sink
 * 
 * @param sink Pointer to the sink
 * @param objformat The object format sink to add 
 * 
 */
void osync_objtype_sink_add_objformat_sink(OSyncObjTypeSink *sink, OSyncObjFormatSink *objformatsink)
{
	osync_assert(sink);
	osync_assert(objformatsink);

	if (!osync_list_find(sink->objformatsinks, objformatsink)) {
		sink->objformatsinks = osync_list_append(sink->objformatsinks, objformatsink);
		osync_objformat_sink_ref(objformatsink);
	}
}

/** @brief Removes an object format from the sink
 * 
 * @param sink Pointer to the sink
 * @param format name of the object format to remove
 * 
 */
void osync_objtype_sink_remove_objformat_sink(OSyncObjTypeSink *sink, OSyncObjFormatSink *objformatsink)
{
	osync_assert(sink);
	osync_assert(objformatsink);

	sink->objformatsinks = osync_list_remove(sink->objformatsinks, objformatsink);
	osync_objformat_sink_unref(objformatsink);
}

/** @brief Sets the sink functions and user data
 * 
 * Sets the functions used by the sink, as well as a user data pointer that
 * you can retrieve within the functions. In most cases you will be able to
 * share the same functions between multiple sinks and just have different
 * user data for each one.
 *
 * @param sink Pointer to the sink
 * @param functions struct containing pointers to the sink functions
 * @param userdata user data pointer that can be retrieved within the functions using osync_objtype_sink_get_userdata()
 * 
 */
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

/*! @brief Checks if sink has a read single entries function (read)
 *
 * @param sink Pointer to the sink
 * @returns TRUE if the sink has a read single entries function (read), FALSE otherwise
 */
osync_bool osync_objtype_sink_get_function_read(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->func_read;
}

/*! @brief Sets the status of the read sink function
 *
 * @param sink Pointer to sink
 * @param read TRUE if the sink has a read function, FALSE otherwise
 */
void osync_objtype_sink_set_function_read(OSyncObjTypeSink *sink, osync_bool read)
{
	osync_assert(sink);
	sink->func_read = read;
}

/*! @brief Checks if sink has a get latest changes function (get_changes)
 *
 * @param sink Pointer to the sink
 * @returns TRUE if the sink has a get latest changes function (get_changes), FALSE otherwise
 */
osync_bool osync_objtype_sink_get_function_getchanges(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->func_getchanges;
}

/*! @brief Sets the status of the get_changes sink function
 *
 * @param sink Pointer to sink
 * @param getchanges TRUE if the sink has a get_changes function, FALSE otherwise
 */
void osync_objtype_sink_set_function_getchanges(OSyncObjTypeSink *sink, osync_bool getchanges)
{
	osync_assert(sink);
	sink->func_getchanges = getchanges;
}

/*! @brief Checks if sink has a write function (commit)
 *
 * @param sink Pointer to the sink
 * @returns TRUE if the sink has a write function (commit), FALSE otherwise
 */
osync_bool osync_objtype_sink_get_function_write(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->func_write;
}

/*! @brief Sets the status of the write sink function
 *
 * @param sink Pointer to sink
 * @param write TRUE if the sink has a write function, FALSE otherwise
 */
void osync_objtype_sink_set_function_write(OSyncObjTypeSink *sink, osync_bool write)
{
	osync_assert(sink);
	sink->func_write = write;
}

/** @brief Gets the user data from a sink
 * 
 * Gets the user data from a sink, as previously set by osync_objtype_sink_set_functions()
 *
 * @param sink Pointer to the sink
 * @returns the sink-specific user data
 * 
 */
void *osync_objtype_sink_get_userdata(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->userdata;
}

/** @brief Queries a sink for the changed objects since the last sync
 * 
 * Calls the get_changes function on a sink
 * 
 * @param sink Pointer to the sink
 * @param plugindata User data that will be passed on to the callback function
 * @param info Pointer to the plugin info object
 * @param ctx The sync context
 * 
 */
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

/** @brief Reads a single object by its uid
 * 
 * Calls the read_change function on the sink
 * 
 * @param sink Pointer to the sink
 * @param plugindata User data that will be passed on to the callback function
 * @param info Pointer to the plugin info object
 * @param change The change to read. The change must have the uid set
 * @param ctx The sync context
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

/** @brief Connects a sink to its device
 * 
 * Calls the connect function on a sink
 * 
 * @param sink Pointer to the sink
 * @param plugindata User data that will be passed on to the callback function
 * @param info Pointer to the plugin info object
 * @param ctx The sync context
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
		osync_context_report_success(ctx);
	} else {
		functions.connect(plugindata, info, ctx);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Disconnects a sink from its device
 * 
 * Calls the disconnect function on a sink
 * 
 * @param sink Pointer to the sink
 * @param plugindata User data that will be passed on to the callback function
 * @param info Pointer to the plugin info object
 * @param ctx The sync context
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
		osync_context_report_success(ctx);
	} else {
		functions.disconnect(plugindata, info, ctx);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Tells the sink that the sync was successfully completed
 * 
 * Calls the sync_done function on a sink
 * 
 * @param sink Pointer to the sink
 * @param plugindata User data that will be passed on to the callback function
 * @param info Pointer to the plugin info object
 * @param ctx The sync context
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
 * Calls the commit_change function on a sink
 * 
 * @param sink Pointer to the sink
 * @param plugindata User data that will be passed on to the callback function
 * @param info Pointer to the plugin info object
 * @param change The change to write
 * @param ctx The sync context
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

/** @brief Tells the sink that all changes have been committed
 * 
 * Calls the committed_all function on a sink or the batch_commit function
 * depending on which function the sink wants to use.
 * 
 * @param sink Pointer to the sink
 * @param plugindata User data that will be passed on to the callback function
 * @param info Pointer to the plugin info object
 * @param ctx The sync context
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

/*! @brief Checks if a sink is enabled
 * 
 * @param sink Pointer to the sink
 * @returns TRUE if the sink is enabled, FALSE otherwise
 * 
 */
osync_bool osync_objtype_sink_is_enabled(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->enabled;
}

/*! @brief Sets the enabled/disabled state of a sink
 * 
 * @param sink Pointer to the sink
 * @param enabled TRUE if the sink is enabled, FALSE otherwise
 * 
 */
void osync_objtype_sink_set_enabled(OSyncObjTypeSink *sink, osync_bool enabled)
{
	osync_assert(sink);
	sink->enabled = enabled;
}

/*! @brief Checks if a sink is available
 * 
 * @param sink Pointer to the sink
 * @returns TRUE if the sink is available, FALSE otherwise
 * 
 */
osync_bool osync_objtype_sink_is_available(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->available;
}

/*! @brief Sets the available state of a sink
 * 
 * @param sink Pointer to the sink
 * @param available TRUE if the sink is available, FALSE otherwise
 * 
 */
void osync_objtype_sink_set_available(OSyncObjTypeSink *sink, osync_bool available)
{
	osync_assert(sink);
	sink->available = available;
}

/*! @brief Checks if sink is allowed to write (commit)
 *
 * If the sink is not allowed to write, then no changes will be commited to
 * the sink.
 *
 * @param sink Pointer to the sink
 * @returns TRUE if the sink is allowed to write (commit), FALSE otherwise
 */
osync_bool osync_objtype_sink_get_write(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->write;
}

/*! @brief Sets the write status of the sink (commit)
 *
 * See osync_objtype_sink_get_write()
 *
 * @param sink Pointer to sink
 * @param write TRUE if the sink is allowed to write changes (commit), FALSE otherwise
 *
 */
void osync_objtype_sink_set_write(OSyncObjTypeSink *sink, osync_bool write)
{
	osync_assert(sink);
	sink->write = write;
}

/*! @brief Sets the get latest changes status of the sink (get_change)
 *
 * See osync_objtype_sink_get_getchanges()
 *
 * @param sink Pointer to sink
 * @param getchanges Set TRUE if the sink is allowed to get latest changes, FALSE otherwise
 *
 */
void osync_objtype_sink_set_getchanges(OSyncObjTypeSink *sink, osync_bool getchanges)
{
	osync_assert(sink);
	sink->getchanges = getchanges;
}

/*! @brief Checks if sink is allowed to get latest changes 
 *
 * @param sink Pointer to the sink
 * @returns TRUE if the sink is allowed to get latest changed entries, FALSE otherwise
 *
 */
osync_bool osync_objtype_sink_get_getchanges(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->getchanges;
}

/*! @brief Checks if sink is allowed to read single entries
 *
 * "Read" means to request a single entry and does not mean to get the
 * latest changes since last sink. See osync_objtype_sink_get_getchanges().
 * The read function explicitly means to read a single entry without triggering
 * a full sync. This is used for example to check if a conflict between entries
 * could be ignored. Ignoring conflicts is only possible if the sink is allowed to
 * read this conflicting entries on the next sync without triggering a SlowSync.
 *
 * @param sink Pointer to the sink
 * @returns TRUE if the sink is allowed to read single entries, FALSE otherwise
 *
 */
osync_bool osync_objtype_sink_get_read(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->read;
}

/*! @brief Sets the (single) read status of a sink 
 *
 * See osync_objtype_sink_get_read()
 *
 * @param sink Pointer to the sink
 * @param read TRUE if the sink is able to read (single entries), FALSE otherwise
 *
 */
void osync_objtype_sink_set_read(OSyncObjTypeSink *sink, osync_bool read)
{
	osync_assert(sink);
	sink->read = read;
}

/*! @brief Checks if slow-sync has been requested
 * 
 * When slow-sync is requested, OpenSync synchronizes all entries rather than
 * just the changes.
 *
 * If you are using hashtables, you should call this function in your sink's
 * get_changes() function and if slow-sync has been requested, call
 * osync_hashtable_slowsync() on your hashtable. If you don't do this, OpenSync
 * will assume that all entries should be deleted, which is usually not what 
 * the user wants.
 *
 * @param sink Pointer to the sink
 * @returns TRUE if slow-sync has been requested, FALSE for normal sync
 * 
 */
osync_bool osync_objtype_sink_get_slowsync(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->slowsync;
}

/*! @brief Sets the slow-sync state of a sink
 * 
 * When slow-sync is requested, OpenSync synchronizes all entries rather than
 * just the changes.
 *
 * Slow-sync should be requested if you know that your device's memory has
 * been erased. If it is appropriate for your device, you can use OpenSync's 
 * anchor system to determine if you should request slow-sync.
 *
 * @param sink Pointer to the sink
 * @param slowsync TRUE to request slow-sync, FALSE for normal sync
 * 
 */
void osync_objtype_sink_set_slowsync(OSyncObjTypeSink *sink, osync_bool slowsync)
{
	osync_assert(sink);
	osync_trace(TRACE_INTERNAL, "%s: Setting slow-sync of object type \"%s\" to %i", __func__, sink->objtype, slowsync);
	sink->slowsync = slowsync;
}

/*! @brief Sets the connect timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
void osync_objtype_sink_set_connect_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
	osync_assert(sink);
	sink->timeout.connect = timeout;
}

/*! @brief Get the current or default connect timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_connect_timeout_or_default(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.connect ? sink->timeout.connect : OSYNC_SINK_TIMEOUT_CONNECT;
}

/*! @brief Get the current connect timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_connect_timeout(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.connect;
}

/*! @brief Sets the disconnect timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
void osync_objtype_sink_set_disconnect_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
	osync_assert(sink);
	sink->timeout.disconnect = timeout;
}

/*! @brief Get the current or default disconnect timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_disconnect_timeout_or_default(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.disconnect ? sink->timeout.disconnect : OSYNC_SINK_TIMEOUT_DISCONNECT;
}

/*! @brief Get the current disconnect timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_disconnect_timeout(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.disconnect;
}

/*! @brief Sets the get_changes timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
void osync_objtype_sink_set_getchanges_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
	osync_assert(sink);
	sink->timeout.get_changes = timeout;
}

/*! @brief Get the current or default getchanges timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_getchanges_timeout_or_default(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.commit ? sink->timeout.get_changes : OSYNC_SINK_TIMEOUT_GETCHANGES;
}

/*! @brief Get the current getchanges timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_getchanges_timeout(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.get_changes;
}

/*! @brief Sets the commit timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
void osync_objtype_sink_set_commit_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
	osync_assert(sink);
	sink->timeout.commit = timeout;
}

/*! @brief Get the current or default commit timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_commit_timeout_or_default(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.commit ? sink->timeout.commit : OSYNC_SINK_TIMEOUT_COMMIT;
}

/*! @brief Get the current commit timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_commit_timeout(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.commit;
}

/*! @brief Sets the batchcommit timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
void osync_objtype_sink_set_batchcommit_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
	osync_assert(sink);
	sink->timeout.batch_commit = timeout;
}

/*! @brief Get the current or default batchcommit timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_batchcommit_timeout_or_default(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.batch_commit ? sink->timeout.batch_commit : OSYNC_SINK_TIMEOUT_BATCHCOMMIT;
}

/*! @brief Get the current batchcommit timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_batchcommit_timeout(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.batch_commit;
}

/*! @brief Sets the committedall timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
void osync_objtype_sink_set_committedall_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
	osync_assert(sink);
	sink->timeout.committed_all = timeout;
}

/*! @brief Get the current or default committedall timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_committedall_timeout_or_default(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.committed_all ? sink->timeout.committed_all : OSYNC_SINK_TIMEOUT_COMMITTEDALL;
}

/*! @brief Get the current committedall timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_committedall_timeout(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.committed_all;
}

/*! @brief Sets the syncdone timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
void osync_objtype_sink_set_syncdone_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
	osync_assert(sink);
	sink->timeout.sync_done = timeout;
}

/*! @brief Get the current or default syncdone timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_syncdone_timeout_or_default(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.sync_done ? sink->timeout.sync_done : OSYNC_SINK_TIMEOUT_SYNCDONE;
}

/*! @brief Get the current syncdone timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_syncdone_timeout(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.sync_done;
}

/*! @brief Sets the write timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
void osync_objtype_sink_set_write_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
	osync_assert(sink);
	sink->timeout.write = timeout;
}

/*! @brief Get the current or default write timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_write_timeout_or_default(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.write ? sink->timeout.write : OSYNC_SINK_TIMEOUT_WRITE;
}

/*! @brief Get the current write timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_write_timeout(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.write;
}

/*! @brief Sets the read timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
void osync_objtype_sink_set_read_timeout(OSyncObjTypeSink *sink, unsigned int timeout)
{
	osync_assert(sink);
	sink->timeout.read = timeout;
}

/*! @brief Get the current or default read timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_read_timeout_or_default(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.read ? sink->timeout.read : OSYNC_SINK_TIMEOUT_READ;
}

/*! @brief Get the current read timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_read_timeout(OSyncObjTypeSink *sink)
{
	osync_assert(sink);
	return sink->timeout.read;
}

/*@}*/