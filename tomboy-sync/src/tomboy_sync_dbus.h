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

#ifndef TOMBOY_SYNC_DBUS_H_
#define TOMBOY_SYNC_DBUS_H_

#ifdef ENABLE_DBUS

#define TOMBOY_DBUS_NAME "org.gnome.Tomboy"
#define TOMBOY_DBUS_PATH "/org/gnome/Tomboy/RemoteControl"
#define TOMBOY_DBUS_INTERFACE "org.gnome.Tomboy.RemoteControl"

void osync_tomboysync_dbus_get_changes(void *data, OSyncPluginInfo *info, OSyncContext *ctx);
void osync_tomboysync_dbus_commit_change(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change);
void osync_tomboysync_dbus_sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx);
osync_bool osync_tomboysync_dbus_write(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change);
osync_bool osync_tomboysync_dbus_read(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change);

osync_bool osync_tomboysync_dbus_initalize(OSyncTomboyEnv *tomboyenv, OSyncError **error);

#endif /* ENABLE_DBUS */

#endif /* TOMBOY_SYNC_DBUS_H_ */
