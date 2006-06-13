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

typedef struct OSyncObjFormatSinkFunctions {
	OSyncSinkConnectFn connect;
	OSyncSinkDisconnectFn disconnect;
	OSyncSinkGetChangesFn get_changes;
	OSyncSinkCommitFn commit;
	OSyncSinkAccessFn access;
	OSyncSinkCommittedAllFn committed_all;
	OSyncSinkReadFn read;
	OSyncSinkBatchCommitFn batch_commit;
	OSyncSinkSyncDoneFn sync_done;
} OSyncObjFormatSinkFunctions;

OSyncObjFormatSink *osync_sink_new(OSyncObjFormatSinkFunctions functions, const char *format, const char *objtype, OSyncError **error);
void osync_sink_ref(OSyncObjFormatSink *sink);
void osync_sink_unref(OSyncObjFormatSink *sink);

const char *osync_sink_get_objformat(OSyncObjFormatSink *sink);
const char *osync_sink_get_objtype(OSyncObjFormatSink *sink);

void osync_sink_get_changes(OSyncObjFormatSink *sink, OSyncContext *ctx);
void osync_sink_read_change(OSyncObjFormatSink *sink, OSyncChange *change, OSyncContext *ctx);
void osync_sink_connect(OSyncObjFormatSink *sink, OSyncContext *ctx);
void osync_sink_disconnect(OSyncObjFormatSink *sink, OSyncContext *ctx);
void osync_sink_sync_done(OSyncObjFormatSink *sink, OSyncContext *ctx);
void osync_sink_commit_change(OSyncObjFormatSink *sink, OSyncChange *change, OSyncContext *ctx);
void osync_sink_committed_all(OSyncObjFormatSink *sink, OSyncContext *ctx);

#endif //_OPENSYNC_SINK_H_
