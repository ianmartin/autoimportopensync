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

#ifndef _OPENSYNC_CHANGE_H_
#define _OPENSYNC_CHANGE_H_

OSYNC_EXPORT OSyncChange *osync_change_new(OSyncError **error);
OSYNC_EXPORT OSyncChange *osync_change_ref(OSyncChange *change);
OSYNC_EXPORT void osync_change_unref(OSyncChange *change);

OSYNC_EXPORT void osync_change_set_hash(OSyncChange *change, const char *hash);
OSYNC_EXPORT const char *osync_change_get_hash(OSyncChange *change);

OSYNC_EXPORT void osync_change_set_uid(OSyncChange *change, const char *uid);
OSYNC_EXPORT const char *osync_change_get_uid(OSyncChange *change);

OSYNC_EXPORT OSyncChangeType osync_change_get_changetype(OSyncChange *change);
OSYNC_EXPORT void osync_change_set_changetype(OSyncChange *change, OSyncChangeType type);

OSYNC_EXPORT void osync_change_set_data(OSyncChange *change, OSyncData *data);
OSYNC_EXPORT OSyncData *osync_change_get_data(OSyncChange *change);

OSYNC_EXPORT OSyncObjFormat *osync_change_get_objformat(OSyncChange *change);
OSYNC_EXPORT const char *osync_change_get_objtype(OSyncChange *change);
OSYNC_EXPORT void osync_change_set_objtype(OSyncChange *change, const char *objtype);

#endif /*_OPENSYNC_CHANGE_H_*/
