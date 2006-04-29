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
extern void file_connect(OSyncContext *ctx);
extern  bool file_callback (RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie);

extern osync_bool synce_file_get_changeinfo(OSyncContext *ctx);
extern void       synce_file_getdata(OSyncContext *ctx, OSyncChange *change);
extern osync_bool synce_file_commit(OSyncContext *ctx, OSyncChange *change);
extern void file_read(OSyncContext *ctx, OSyncChange *change);
extern osync_bool file_access(OSyncContext *ctx, OSyncChange *change);
