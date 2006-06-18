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

typedef void (* OSyncSinkConnectFn) (OSyncContext *);
typedef void (* OSyncSinkDisconnectFn) (OSyncContext *);
typedef void (* OSyncSinkGetChangesFn) (OSyncContext *);
typedef osync_bool (* OSyncSinkCommitFn) (OSyncContext *, OSyncChange *);
typedef osync_bool (* OSyncSinkAccessFn) (OSyncContext *, OSyncChange *);
typedef void (* OSyncSinkCommittedAllFn) (OSyncContext *);
typedef void (* OSyncSinkReadFn) (OSyncContext *, OSyncChange *);
typedef void (* OSyncSinkBatchCommitFn) (OSyncContext *, OSyncContext **, OSyncChange **);
typedef void (* OSyncSinkSyncDoneFn) (OSyncContext *ctx);

typedef struct OSyncObjTypeSinkFunctions {
	OSyncSinkConnectFn connect;
	OSyncSinkDisconnectFn disconnect;
	OSyncSinkGetChangesFn get_changes;
	OSyncSinkCommitFn commit;
	OSyncSinkAccessFn access;
	OSyncSinkCommittedAllFn committed_all;
	OSyncSinkReadFn read;
	OSyncSinkBatchCommitFn batch_commit;
	OSyncSinkSyncDoneFn sync_done;
} OSyncObjTypeSinkFunctions;

OSYNC_EXPORT OSyncObjTypeSink *osync_objtype_sink_new(const char *objtype, OSyncError **error);
OSYNC_EXPORT void osync_objtype_sink_ref(OSyncObjTypeSink *sink);
OSYNC_EXPORT void osync_objtype_sink_unref(OSyncObjTypeSink *sink);

OSYNC_EXPORT const char *osync_objtype_sink_get_name(OSyncObjTypeSink *sink);

OSYNC_EXPORT int osync_objtype_sink_num_objformats(OSyncObjTypeSink *sink);
OSYNC_EXPORT const char *osync_objtype_sink_nth_objformat(OSyncObjTypeSink *sink, int nth);
OSYNC_EXPORT void osync_objtype_sink_add_objformat(OSyncObjTypeSink *sink, const char *format);
OSYNC_EXPORT void osync_objtype_sink_remove_objformat(OSyncObjTypeSink *sink, const char *format);

OSYNC_EXPORT void osync_objtype_sink_get_changes(OSyncObjTypeSink *sink, OSyncContext *ctx);
OSYNC_EXPORT void osync_objtype_sink_read_change(OSyncObjTypeSink *sink, OSyncChange *change, OSyncContext *ctx);
OSYNC_EXPORT void osync_objtype_sink_connect(OSyncObjTypeSink *sink, OSyncContext *ctx);
OSYNC_EXPORT void osync_objtype_sink_disconnect(OSyncObjTypeSink *sink, OSyncContext *ctx);
OSYNC_EXPORT void osync_objtype_sink_sync_done(OSyncObjTypeSink *sink, OSyncContext *ctx);
OSYNC_EXPORT void osync_objtype_sink_commit_change(OSyncObjTypeSink *sink, OSyncChange *change, OSyncContext *ctx);
OSYNC_EXPORT void osync_objtype_sink_committed_all(OSyncObjTypeSink *sink, OSyncContext *ctx);

osync_bool osync_objtype_sink_is_enabled(OSyncObjTypeSink *sink);
void osync_objtype_sink_set_enabled(OSyncObjTypeSink *sink, osync_bool enabled);

#endif //_OPENSYNC_SINK_H_
