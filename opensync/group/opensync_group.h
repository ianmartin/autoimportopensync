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

#ifndef _OPENSYNC_GROUP_H_
#define _OPENSYNC_GROUP_H_

/*! @ingroup OSyncGroupAPI
 * @brief The lock state of a group
 */
typedef enum {
	OSYNC_LOCK_OK,
	OSYNC_LOCKED,
	OSYNC_LOCK_STALE
} OSyncLockState;

typedef enum {
	OSYNC_CONFLICT_RESOLUTION_UNKNOWN,
	OSYNC_CONFLICT_RESOLUTION_DUPLICATE,
	OSYNC_CONFLICT_RESOLUTION_IGNORE,
	OSYNC_CONFLICT_RESOLUTION_NEWER,
	OSYNC_CONFLICT_RESOLUTION_SELECT
} OSyncConflictResolution;

OSYNC_EXPORT OSyncGroup *osync_group_new(OSyncError **error);
OSYNC_EXPORT void osync_group_ref(OSyncGroup *group);
OSYNC_EXPORT void osync_group_unref(OSyncGroup *group);

OSYNC_EXPORT OSyncLockState osync_group_lock(OSyncGroup *group);
OSYNC_EXPORT void osync_group_unlock(OSyncGroup *group);

OSYNC_EXPORT void osync_group_set_name(OSyncGroup *group, const char *name);
OSYNC_EXPORT const char *osync_group_get_name(OSyncGroup *group);

OSYNC_EXPORT osync_bool osync_group_save(OSyncGroup *group, OSyncError **error);
OSYNC_EXPORT osync_bool osync_group_delete(OSyncGroup *group, OSyncError **error);
OSYNC_EXPORT osync_bool osync_group_reset(OSyncGroup *group, OSyncError **error);
OSYNC_EXPORT osync_bool osync_group_load(OSyncGroup *group, const char *path, OSyncError **error);

OSYNC_EXPORT void osync_group_add_member(OSyncGroup *group, OSyncMember *member);
OSYNC_EXPORT void osync_group_remove_member(OSyncGroup *group, OSyncMember *member);
OSYNC_EXPORT OSyncMember *osync_group_find_member(OSyncGroup *group, int id);
OSYNC_EXPORT OSyncMember *osync_group_nth_member(OSyncGroup *group, int nth);
OSYNC_EXPORT int osync_group_num_members(OSyncGroup *group);

OSYNC_EXPORT const char *osync_group_get_configdir(OSyncGroup *group);
OSYNC_EXPORT void osync_group_set_configdir(OSyncGroup *group, const char *directory);

OSYNC_EXPORT int osync_group_num_objtypes(OSyncGroup *group);
OSYNC_EXPORT const char *osync_group_nth_objtype(OSyncGroup *group, int nth);
OSYNC_EXPORT void osync_group_set_objtype_enabled(OSyncGroup *group, const char *objtype, osync_bool enabled);
OSYNC_EXPORT int osync_group_objtype_enabled(OSyncGroup *group, const char *objtype);

OSYNC_EXPORT void osync_group_add_filter(OSyncGroup *group, OSyncFilter *filter);
OSYNC_EXPORT void osync_group_remove_filter(OSyncGroup *group, OSyncFilter *filter);
OSYNC_EXPORT int osync_group_num_filters(OSyncGroup *group);
OSYNC_EXPORT OSyncFilter *osync_group_nth_filter(OSyncGroup *group, int nth);

OSYNC_EXPORT void osync_group_set_last_synchronization(OSyncGroup *group, time_t last_sync);
OSYNC_EXPORT time_t osync_group_get_last_synchronization(OSyncGroup *group);

OSYNC_EXPORT void osync_group_set_conflict_resolution(OSyncGroup *group, OSyncConflictResolution res, int num);
OSYNC_EXPORT void osync_group_get_conflict_resolution(OSyncGroup *group, OSyncConflictResolution *res, int *num);

OSYNC_EXPORT osync_bool osync_group_get_merger_enabled(OSyncGroup *group);
OSYNC_EXPORT void osync_group_set_merger_enabled(OSyncGroup *group, osync_bool enable_merger);

OSYNC_EXPORT osync_bool osync_group_get_converter_enabled(OSyncGroup *group);
OSYNC_EXPORT void osync_group_set_converter_enabled(OSyncGroup *group, osync_bool enable_converter);

#endif /* _OPENSYNC_GROUP_H_ */
