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

#ifndef _OPENSYNC_ENV_H_
#define _OPENSYNC_ENV_H_

void osync_env_free(OSyncEnv *env);
OSyncEnv *osync_env_new(OSyncError **error);
osync_bool osync_env_initialize(OSyncEnv *env, OSyncError **error);
osync_bool osync_env_finalize(OSyncEnv *env, OSyncError **error);
void osync_env_set_option(OSyncEnv *env, const char *name, const char *value);

OSyncPlugin *osync_plugin_from_name(OSyncEnv *osinfo, const char *name);
int osync_env_num_plugins (OSyncEnv *osstruct);
OSyncPlugin *osync_env_nth_plugin(OSyncEnv *osstruct, int nth);
OSyncPlugin *osync_env_find_plugin(OSyncEnv *env, const char *name);
osync_bool osync_env_plugin_is_usable(OSyncEnv *env, const char *pluginname, OSyncError **error);

void osync_env_remove_group(OSyncEnv *osstruct, OSyncGroup *group);
OSyncGroup *osync_env_find_group(OSyncEnv *env, const char *name);
int osync_env_num_groups(OSyncEnv *env);
void osync_env_append_group(OSyncEnv *os_env, OSyncGroup *group);
OSyncGroup *osync_env_nth_group(OSyncEnv *osinfo, int nth);

osync_bool osync_env_load_groups(OSyncEnv *osyncinfo, const char *path, OSyncError **error);
osync_bool osync_env_load_formats(OSyncEnv *env, const char *path, OSyncError **oserror);
osync_bool osync_env_load_plugins(OSyncEnv *env, const char *path, OSyncError **oserror);

osync_bool osync_module_load_dir(OSyncEnv *env, const char *path, osync_bool must_exist, OSyncError **oserror);

#endif //_OPENSYNC_ENV_H_
