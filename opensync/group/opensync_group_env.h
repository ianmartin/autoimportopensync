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

#ifndef _OPENSYNC_GROUP_ENV_H_
#define _OPENSYNC_GROUP_ENV_H_

OSYNC_EXPORT OSyncGroupEnv *osync_group_env_new(OSyncError **error);
OSYNC_EXPORT void osync_group_env_free(OSyncGroupEnv *env);

OSYNC_EXPORT osync_bool osync_group_env_load_groups(OSyncGroupEnv *env, const char *p, OSyncError **error);
OSYNC_EXPORT OSyncGroup *osync_group_env_find_group(OSyncGroupEnv *env, const char *name);
OSYNC_EXPORT void osync_group_env_add_group(OSyncGroupEnv *env, OSyncGroup *group);
OSYNC_EXPORT void osync_group_env_remove_group(OSyncGroupEnv *env, OSyncGroup *group);
OSYNC_EXPORT int osync_group_env_num_groups(OSyncGroupEnv *env);
OSYNC_EXPORT OSyncGroup *osync_group_env_nth_group(OSyncGroupEnv *env, int nth);

#endif /*_OPENSYNC_GROUP_ENV_H_*/
