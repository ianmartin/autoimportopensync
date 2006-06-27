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

#ifndef _OPENSYNC_PLUGIN_ENV_H_
#define _OPENSYNC_PLUGIN_ENV_H_

OSYNC_EXPORT OSyncPluginEnv *osync_plugin_env_new(OSyncError **error);
OSYNC_EXPORT void osync_plugin_env_free(OSyncPluginEnv *env);
OSYNC_EXPORT osync_bool osync_plugin_env_load(OSyncPluginEnv *env, const char *path, OSyncError **error);
OSYNC_EXPORT osync_bool osync_plugin_env_load_module(OSyncPluginEnv *env, const char *filename, OSyncError **error);

OSYNC_EXPORT void osync_plugin_env_register_plugin(OSyncPluginEnv *env, OSyncPlugin *plugin);
OSYNC_EXPORT OSyncPlugin *osync_plugin_env_find_plugin(OSyncPluginEnv *env, const char *name);
OSYNC_EXPORT int osync_plugin_env_num_plugins(OSyncPluginEnv *env);
OSYNC_EXPORT OSyncPlugin *osync_plugin_env_nth_plugin(OSyncPluginEnv *env, int nth);
OSYNC_EXPORT osync_bool osync_plugin_env_plugin_is_usable(OSyncPluginEnv *env, const char *pluginname, OSyncError **error);

#endif //_OPENSYNC_PLUGIN_ENV_H_
