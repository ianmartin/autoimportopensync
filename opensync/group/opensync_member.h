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

#ifndef _OPENSYNC_MEMBER_H_
#define _OPENSYNC_MEMBER_H_

OSYNC_EXPORT OSyncMember *osync_member_new(OSyncError **error);
OSYNC_EXPORT void osync_member_ref(OSyncMember *member);
OSYNC_EXPORT void osync_member_unref(OSyncMember *member);

OSYNC_EXPORT const char *osync_member_get_pluginname(OSyncMember *member);
OSYNC_EXPORT void osync_member_set_pluginname(OSyncMember *member, const char *pluginname);

OSYNC_EXPORT const char *osync_member_get_configdir(OSyncMember *member);
OSYNC_EXPORT void osync_member_set_configdir(OSyncMember *member, const char *configdir);

OSYNC_EXPORT osync_bool osync_member_get_config_or_default(OSyncMember *member, char **data, int *size, OSyncError **error);
OSYNC_EXPORT osync_bool osync_member_get_config(OSyncMember *member, char **data, int *size, OSyncError **error);
OSYNC_EXPORT void osync_member_set_config(OSyncMember *member, const char *data, int size);

OSYNC_EXPORT osync_bool osync_member_load(OSyncMember *member, const char *path, OSyncError **error);
OSYNC_EXPORT osync_bool osync_member_save(OSyncMember *member, OSyncError **error);
OSYNC_EXPORT osync_bool osync_member_delete(OSyncMember *member, OSyncError **error);

OSYNC_EXPORT long long int osync_member_get_id(OSyncMember *member);

OSYNC_EXPORT void osync_member_add_objtype(OSyncMember *member, const char *objtype);
OSYNC_EXPORT int osync_member_num_objtypes(OSyncMember *member);
OSYNC_EXPORT const char *osync_member_nth_objtype(OSyncMember *member, int nth);

OSYNC_EXPORT osync_bool osync_member_objtype_enabled(OSyncMember *member, const char *objtype);
OSYNC_EXPORT void osync_member_set_objtype_enabled(OSyncMember *member, const char *objtype, osync_bool enabled);

OSYNC_EXPORT void osync_member_set_start_type(OSyncMember *member, OSyncStartType type);
OSYNC_EXPORT OSyncStartType osync_member_get_start_type(OSyncMember *member);

#endif /* _OPENSYNC_MEMBER_H_ */
