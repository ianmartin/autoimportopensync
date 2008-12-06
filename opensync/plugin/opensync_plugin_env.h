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

/**
 * @defgroup PublicAPI Public APIs
 * @brief Available public APIs
 * 
 */

/**
 * @defgroup OSyncPublic OpenSync Public API
 * @ingroup PublicAPI
 * @brief The public API of opensync
 * 
 * This gives you an insight in the public API of opensync.
 * 
 */

/**
 * @defgroup OSyncPluginEnvAPI OpenSync Environment
 * @ingroup OSyncPublic
 * @brief The public API of the opensync environment
 * 
 */
/*@{*/


/*! @brief This will create a new opensync environment
 * 
 * The environment will hold all information about plugins, groups etc
 * 
 * @returns A pointer to a newly allocated environment. NULL on error.
 * 
 */
OSYNC_EXPORT OSyncPluginEnv *osync_plugin_env_new(OSyncError **error);

/*! @brief Frees a osync environment
 * 
 * Frees a osync environment and all resources.
 * 
 * @param env Pointer to the environment to free
 * 
 */
OSYNC_EXPORT void osync_plugin_env_free(OSyncPluginEnv *env);

/*! @brief Loads the sync modules from a given directory
 * 
 * Loads all sync modules from a directory into a osync environment
 * 
 * @param env Pointer to a OSyncPluginEnv environment
 * @param path The path where to look for plugins
 * @param error Pointer to a error struct to return a error
 * @returns TRUE on success, FALSE otherwise
 * 
 */
OSYNC_EXPORT osync_bool osync_plugin_env_load(OSyncPluginEnv *env, const char *path, OSyncError **error);


/*! @brief Register plugin to plugin environment 
 * 
 * @param env Pointer to a plugin environment
 * @param plugin Pointer to plugin which should get added to environment
 * 
 */
OSYNC_EXPORT void osync_plugin_env_register_plugin(OSyncPluginEnv *env, OSyncPlugin *plugin);

/*! @brief Finds the plugin with the given name
 * 
 * Finds the plugin with the given name
 * 
 * @param env Pointer to a OSyncPluginEnv environment
 * @param name The name to search for
 * @returns The plugin or NULL if not found
 * 
 */
OSYNC_EXPORT OSyncPlugin *osync_plugin_env_find_plugin(OSyncPluginEnv *env, const char *name);

/*! @brief Returns the number of loaded plugins
 * 
 * Returns the number of loaded plugins. 0 if used before initialization
 * 
 * @param env Pointer to a OSyncPluginEnv environment
 * @returns Number of plugins
 * 
 */
OSYNC_EXPORT int osync_plugin_env_num_plugins(OSyncPluginEnv *env);

/*! @brief Returns pointer to nth plugin
 * 
 * Returns pointer to nth plugin
 * 
 * @param env Pointer to a OSyncPluginEnv environment
 * @param nth Which plugin to return
 * @returns Pointer to plugin
 * 
 */
OSYNC_EXPORT OSyncPlugin *osync_plugin_env_nth_plugin(OSyncPluginEnv *env, int nth);

/*! @brief Checks if plugin is usable 
 * 
 * @param env Pointer to a OSyncPluginEnv environment
 * @param pluginname The name of the plugin
 * @param error Pointer to error-struct
 * @returns TRUE if plugin is usable, FALSE otherwise 
 * 
 */
OSYNC_EXPORT osync_bool osync_plugin_env_plugin_is_usable(OSyncPluginEnv *env, const char *pluginname, OSyncError **error);

/*@}*/

#endif /* _OPENSYNC_PLUGIN_ENV_H_ */

