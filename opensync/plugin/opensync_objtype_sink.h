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

#ifndef _OPENSYNC_OBJTYPE_SINK_H_
#define _OPENSYNC_OBJTYPE_SINK_H_

/**
 * @defgroup OSyncObjTypeSinkAPI OpenSync Object Type Sink
 * @ingroup OSyncPublic
 * @brief Functions to register and manage object type sinks
 * 
 */
/*@{*/

typedef void (* OSyncSinkConnectFn) (void *data, OSyncPluginInfo *info, OSyncContext *ctx);
typedef void (* OSyncSinkDisconnectFn) (void *data, OSyncPluginInfo *info, OSyncContext *ctx);
typedef void (* OSyncSinkGetChangesFn) (void *data, OSyncPluginInfo *info, OSyncContext *ctx);
typedef void (* OSyncSinkCommitFn) (void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change);
typedef osync_bool (* OSyncSinkWriteFn) (void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change);
typedef void (* OSyncSinkCommittedAllFn) (void *data, OSyncPluginInfo *info, OSyncContext *ctx);
typedef osync_bool (* OSyncSinkReadFn) (void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change);
typedef void (* OSyncSinkBatchCommitFn) (void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncContext **, OSyncChange **changes);
typedef void (* OSyncSinkSyncDoneFn) (void *data, OSyncPluginInfo *info, OSyncContext *ctx);

typedef struct OSyncObjTypeSinkFunctions {
	OSyncSinkConnectFn connect;
	OSyncSinkDisconnectFn disconnect;
	OSyncSinkGetChangesFn get_changes;
	OSyncSinkCommitFn commit;
	OSyncSinkWriteFn write;
	OSyncSinkCommittedAllFn committed_all;
	OSyncSinkReadFn read;
	OSyncSinkBatchCommitFn batch_commit;
	OSyncSinkSyncDoneFn sync_done;
} OSyncObjTypeSinkFunctions;

/*! @brief Creates a new main sink
 *
 * Main sink is objtype neutral and should be used for object type
 * neutral actions. Actions like connecting/disconnecting which could
 * be a object type neutral - e.g. connecting to a device via bluetooth.
 * Object type specific example would be a connection to different 
 * object type specific databases/resources.
 *
 * The main sink is not limited to the connect and disconnect functions however. 
 * 
 * @param error Pointer to an error struct
 * @returns the newly created main sink
 */
OSYNC_EXPORT OSyncObjTypeSink *osync_objtype_main_sink_new(OSyncError **error);

/*! @brief Creates a new sink for an object type
 *
 * @param objtype The name of the object type for the sink
 * @param error Pointer to an error struct
 * @returns the newly created objtype specific sink
 */
OSYNC_EXPORT OSyncObjTypeSink *osync_objtype_sink_new(const char *objtype, OSyncError **error);

/*! @brief Increase the reference count on a sink
 * 
 * @param sink Pointer to the sink
 * 
 */
OSYNC_EXPORT OSyncObjTypeSink *osync_objtype_sink_ref(OSyncObjTypeSink *sink);

/*! @brief Decrease the reference count on a sink
 * 
 * @param sink Pointer to the sink
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_unref(OSyncObjTypeSink *sink);

/*! @brief Return the name of the object type of a sink
 * 
 * @param sink Pointer to the sink
 * @returns the name of the object type of the specified sink
 * 
 */
OSYNC_EXPORT const char *osync_objtype_sink_get_name(OSyncObjTypeSink *sink);


/*! @brief Set the object type of a sink
 * 
 * @param sink Pointer to the sink
 * @param name the name of the object type to set
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_set_name(OSyncObjTypeSink *sink, const char *name);


/*! @brief Return the preferred format for the conversion 
 * 
 * @param sink Pointer to the sink
 * @returns the name of the preferred format
 * 
 */
OSYNC_EXPORT const char *osync_objtype_sink_get_preferred_format(OSyncObjTypeSink *sink);

/*! @brief Set the preferred format for the conversion
 * 
 * @param sink Pointer to the sink
 * @param name the name of the preferred format to set
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_set_preferred_format(OSyncObjTypeSink *sink, const char *preferred_format);


/*! @brief Returns the number of object formats in the sink
 * 
 * @param sink Pointer to the sink
 * @returns the number of object formats in the sink
 * 
 */
OSYNC_EXPORT unsigned int osync_objtype_sink_num_objformat_sinks(OSyncObjTypeSink *sink);

/*! @brief Returns the nth object format in the sink
 * 
 * @param sink Pointer to the sink
 * @param nth the index of the object format to return
 * @returns the name of the object format at the specified index
 * 
 */
OSYNC_EXPORT OSyncObjFormatSink *osync_objtype_sink_nth_objformat_sink(OSyncObjTypeSink *sink, unsigned int nth);

/*! @brief Finds the objformat sink for the corresponding objformat 
 * 
 * @param sink Pointer to the sink
 * @param objformat the objformat to look for the corresponding objformat sink
 * @returns Pointer to the corresponding objformat sink if found, NULL otherwise 
 * 
 */
OSYNC_EXPORT OSyncObjFormatSink *osync_objtype_sink_find_objformat_sink(OSyncObjTypeSink *sink, OSyncObjFormat *objformat);

/*! @brief Get list of object format sinks 
 * 
 * @param sink Pointer to the sink
 * @returns List of object format sinks 
 * 
 */
OSYNC_EXPORT OSyncList *osync_objtype_sink_get_objformat_sinks(OSyncObjTypeSink *sink);

/** @brief Adds an object format sink to the sink
 * 
 * @param sink Pointer to the sink
 * @param objformat The object format sink to add 
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_add_objformat_sink(OSyncObjTypeSink *sink, OSyncObjFormatSink *objformatsink);

/** @brief Removes an object format from the sink
 * 
 * @param sink Pointer to the sink
 * @param format name of the object format to remove
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_remove_objformat_sink(OSyncObjTypeSink *sink, OSyncObjFormatSink *objformatsink);


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
OSYNC_EXPORT void osync_objtype_sink_set_functions(OSyncObjTypeSink *sink, OSyncObjTypeSinkFunctions functions, void *userdata);

/** @brief Gets the user data from a sink
 * 
 * Gets the user data from a sink, as previously set by osync_objtype_sink_set_functions()
 *
 * @param sink Pointer to the sink
 * @returns the sink-specific user data
 * 
 */
OSYNC_EXPORT void *osync_objtype_sink_get_userdata(OSyncObjTypeSink *sink);


/*! @brief Checks if a sink is enabled
 * 
 * @param sink Pointer to the sink
 * @returns TRUE if the sink is enabled, FALSE otherwise
 * 
 */
OSYNC_EXPORT osync_bool osync_objtype_sink_is_enabled(OSyncObjTypeSink *sink);

/*! @brief Sets the enabled/disabled state of a sink
 * 
 * @param sink Pointer to the sink
 * @param enabled TRUE if the sink is enabled, FALSE otherwise
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_set_enabled(OSyncObjTypeSink *sink, osync_bool enabled);


/*! @brief Checks if a sink is available
 * 
 * @param sink Pointer to the sink
 * @returns TRUE if the sink is available, FALSE otherwise
 * 
 */
OSYNC_EXPORT osync_bool osync_objtype_sink_is_available(OSyncObjTypeSink *sink);

/*! @brief Sets the available state of a sink
 * 
 * @param sink Pointer to the sink
 * @param available TRUE if the sink is available, FALSE otherwise
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_set_available(OSyncObjTypeSink *sink, osync_bool available);


/*! @brief Checks if sink is allowed to write (commit)
 *
 * If the sink is not allowed to write, then no changes will be commited to
 * the sink.
 *
 * @param sink Pointer to the sink
 * @returns TRUE if the sink is allowed to write (commit), FALSE otherwise
 */
OSYNC_EXPORT osync_bool osync_objtype_sink_get_write(OSyncObjTypeSink *sink);

/*! @brief Sets the write status of the sink (commit)
 *
 * See osync_objtype_sink_get_write()
 *
 * @param sink Pointer to sink
 * @param write TRUE if the sink is allowed to write changes (commit), FALSE otherwise
 *
 */
OSYNC_EXPORT void osync_objtype_sink_set_write(OSyncObjTypeSink *sink, osync_bool write);


/*! @brief Checks if sink is allowed to get latest changes 
 *
 * @param sink Pointer to the sink
 * @returns TRUE if the sink is allowed to get latest changed entries, FALSE otherwise
 *
 */
OSYNC_EXPORT osync_bool osync_objtype_sink_get_getchanges(OSyncObjTypeSink *sink);

/*! @brief Sets the get latest changes status of the sink (get_change)
 *
 * See osync_objtype_sink_get_getchanges()
 *
 * @param sink Pointer to sink
 * @param getchanges Set TRUE if the sink is allowed to get latest changes, FALSE otherwise
 *
 */
OSYNC_EXPORT void osync_objtype_sink_set_getchanges(OSyncObjTypeSink *sink, osync_bool write);


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
OSYNC_EXPORT osync_bool osync_objtype_sink_get_read(OSyncObjTypeSink *sink);

/*! @brief Sets the (single) read status of a sink 
 *
 * See osync_objtype_sink_get_read()
 *
 * @param sink Pointer to the sink
 * @param read TRUE if the sink is able to read (single entries), FALSE otherwise
 *
 */
OSYNC_EXPORT void osync_objtype_sink_set_read(OSyncObjTypeSink *sink, osync_bool read);


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
OSYNC_EXPORT osync_bool osync_objtype_sink_get_slowsync(OSyncObjTypeSink *sink);

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
OSYNC_EXPORT void osync_objtype_sink_set_slowsync(OSyncObjTypeSink *sink, osync_bool slowsync);


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
OSYNC_EXPORT void osync_objtype_sink_get_changes(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx);

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
OSYNC_EXPORT void osync_objtype_sink_read_change(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncChange *change, OSyncContext *ctx);

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
OSYNC_EXPORT void osync_objtype_sink_connect(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx);

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
OSYNC_EXPORT void osync_objtype_sink_disconnect(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx);

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
OSYNC_EXPORT void osync_objtype_sink_sync_done(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx);

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
OSYNC_EXPORT void osync_objtype_sink_commit_change(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncChange *change, OSyncContext *ctx);

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
OSYNC_EXPORT void osync_objtype_sink_committed_all(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx);


/*! @brief Sets the connect timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_set_connect_timeout(OSyncObjTypeSink *sink, unsigned int timeout);

/*! @brief Sets the disconnect timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_set_disconnect_timeout(OSyncObjTypeSink *sink, unsigned int timeout);

/*! @brief Sets the get_changes timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_set_getchanges_timeout(OSyncObjTypeSink *sink, unsigned int timeout);

/*! @brief Sets the commit timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_set_commit_timeout(OSyncObjTypeSink *sink, unsigned int timeout);

/*! @brief Sets the batchcommit timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_set_batchcommit_timeout(OSyncObjTypeSink *sink, unsigned int timeout);

/*! @brief Sets the committedall timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_set_committedall_timeout(OSyncObjTypeSink *sink, unsigned int timeout);

/*! @brief Sets the syncdone timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_set_syncdone_timeout(OSyncObjTypeSink *sink, unsigned int timeout);

/*! @brief Sets the write timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_set_write_timeout(OSyncObjTypeSink *sink, unsigned int timeout);

/*! @brief Sets the read timeout in seconds for the OSyncObjTypeSink 
 * 
 * @param sink Pointer to the sink
 * @param timeout The timeout in seconds 
 * 
 */
OSYNC_EXPORT void osync_objtype_sink_set_read_timeout(OSyncObjTypeSink *sink, unsigned int timeout);

/*@}*/

#endif /* _OPENSYNC_OBJTYPE_SINK_H_ */

