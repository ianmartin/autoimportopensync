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

#ifndef _OPENSYNC_SINK_H_
#define _OPENSYNC_SINK_H_

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

OSYNC_EXPORT OSyncObjTypeSink *osync_objtype_main_sink_new(OSyncError **error);
OSYNC_EXPORT OSyncObjTypeSink *osync_objtype_sink_new(const char *objtype, OSyncError **error);
OSYNC_EXPORT OSyncObjTypeSink *osync_objtype_sink_ref(OSyncObjTypeSink *sink);
OSYNC_EXPORT void osync_objtype_sink_unref(OSyncObjTypeSink *sink);

OSYNC_EXPORT const char *osync_objtype_sink_get_name(OSyncObjTypeSink *sink);
OSYNC_EXPORT void osync_objtype_sink_set_name(OSyncObjTypeSink *sink, const char *name);

OSYNC_EXPORT int osync_objtype_sink_num_objformats(OSyncObjTypeSink *sink);
OSYNC_EXPORT const char *osync_objtype_sink_nth_objformat(OSyncObjTypeSink *sink, int nth);
OSYNC_EXPORT const char *osync_objtype_sink_nth_objformat_config(OSyncObjTypeSink *sink, int nth);
OSYNC_EXPORT void osync_objtype_sink_add_objformat(OSyncObjTypeSink *sink, const char *format);
OSYNC_EXPORT void osync_objtype_sink_add_objformat_with_config(OSyncObjTypeSink *sink, const char *format, const char *format_config);
OSYNC_EXPORT void osync_objtype_sink_remove_objformat(OSyncObjTypeSink *sink, const char *format);

OSYNC_EXPORT void osync_objtype_sink_set_functions(OSyncObjTypeSink *sink, OSyncObjTypeSinkFunctions functions, void *userdata);
OSYNC_EXPORT void *osync_objtype_sink_get_userdata(OSyncObjTypeSink *sink);

OSYNC_EXPORT osync_bool osync_objtype_sink_get_function_read(OSyncObjTypeSink *sink);
OSYNC_EXPORT void osync_objtype_sink_set_function_read(OSyncObjTypeSink *sink, osync_bool read);

OSYNC_EXPORT osync_bool osync_objtype_sink_get_function_getchanges(OSyncObjTypeSink *sink);
OSYNC_EXPORT void osync_objtype_sink_set_function_getchanges(OSyncObjTypeSink *sink, osync_bool getchanges);

OSYNC_EXPORT osync_bool osync_objtype_sink_get_function_write(OSyncObjTypeSink *sink);
OSYNC_EXPORT void osync_objtype_sink_set_function_write(OSyncObjTypeSink *sink, osync_bool write);

OSYNC_EXPORT void osync_objtype_sink_get_changes(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx);
OSYNC_EXPORT void osync_objtype_sink_read_change(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncChange *change, OSyncContext *ctx);
// OSYNC_EXPORT void osync_objtype_sink_write(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncChange *change, OSyncContext *ctx);
OSYNC_EXPORT void osync_objtype_sink_connect(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx);
OSYNC_EXPORT void osync_objtype_sink_disconnect(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx);
OSYNC_EXPORT void osync_objtype_sink_sync_done(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx);
OSYNC_EXPORT void osync_objtype_sink_commit_change(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncChange *change, OSyncContext *ctx);
OSYNC_EXPORT void osync_objtype_sink_committed_all(OSyncObjTypeSink *sink, void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx);

OSYNC_EXPORT osync_bool osync_objtype_sink_is_enabled(OSyncObjTypeSink *sink);
OSYNC_EXPORT void osync_objtype_sink_set_enabled(OSyncObjTypeSink *sink, osync_bool enabled);

OSYNC_EXPORT osync_bool osync_objtype_sink_is_available(OSyncObjTypeSink *sink);
OSYNC_EXPORT void osync_objtype_sink_set_available(OSyncObjTypeSink *sink, osync_bool available);

OSYNC_EXPORT osync_bool osync_objtype_sink_get_write(OSyncObjTypeSink *sink);
OSYNC_EXPORT void osync_objtype_sink_set_write(OSyncObjTypeSink *sink, osync_bool write);

OSYNC_EXPORT osync_bool osync_objtype_sink_get_getchanges(OSyncObjTypeSink *sink);
OSYNC_EXPORT void osync_objtype_sink_set_getchanges(OSyncObjTypeSink *sink, osync_bool write);

OSYNC_EXPORT osync_bool osync_objtype_sink_get_read(OSyncObjTypeSink *sink);
OSYNC_EXPORT void osync_objtype_sink_set_read(OSyncObjTypeSink *sink, osync_bool read);

OSYNC_EXPORT osync_bool osync_objtype_sink_get_slowsync(OSyncObjTypeSink *sink);
OSYNC_EXPORT void osync_objtype_sink_set_slowsync(OSyncObjTypeSink *sink, osync_bool slowsync);

const OSyncList *osync_objtype_sink_get_objformats(OSyncObjTypeSink *sink);

#endif //_OPENSYNC_SINK_H_
