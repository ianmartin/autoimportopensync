/*
 * tomboy-sync - A plugin for the opensync framework
 * Copyright (C) 2008  Bjoern Ricks <bjoern.ricks@googlemail.com>
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

#ifndef TOMBOY_SYNC_FILE_H_
#define TOMBOY_SYNC_FILE_H_

#include <opensync/opensync.h>

void osync_tomboysync_file_commit_change(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change);
void osync_tomboysync_file_get_changes(void *data, OSyncPluginInfo *info, OSyncContext *ctx);

osync_bool osync_tomboysync_file_read(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change);
osync_bool osync_tomboysync_file_write(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change);

void osync_tomboysync_file_sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx);

void osync_tomboysync_file_report_dir(OSyncTomboyDir *directory, OSyncPluginInfo *info, const char *subdir, OSyncContext *ctx);

osync_bool osync_tomboysync_file_initalize(OSyncTomboyEnv *tomboyenv, OSyncError **error);

#endif /* TOMBOY_SYNC_FILE_H_ */
