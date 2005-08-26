/*
 * file-sync - A plugin for the opensync framework
 *
 * Copyright © 2005 Danny Backx <dannybackx@users.sourceforge.net>
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
 */
extern osync_bool commit_file_change(OSyncContext *ctx, OSyncChange *change);
extern osync_bool file_get_changeinfo(OSyncContext *ctx);
extern void file_connect(OSyncContext *ctx);
extern  bool file_callback (RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie);
extern void file_sync_done(OSyncContext *ctx);

extern void file_get_data(OSyncContext *ctx, OSyncChange *change);
extern void file_finalize(void *data);
extern void file_disconnect(OSyncContext *ctx);
