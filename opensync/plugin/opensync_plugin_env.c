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
 
#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-module.h"
#include "opensync-plugin.h"
#include "opensync_plugin_env_internals.h"

/*! @brief Loads the modules from a given directory
 * 
 * Loads all modules from a directory into a osync environment
 * 
 * @param env Pointer to a OSyncEnv environment
 * @param path The path where to look for plugins, NULL for the default sync module directory
 * @param must_exist If set to TRUE, this function will return an error if the directory does not exist
 * @param error Pointer to a error struct to return a error
 * @returns TRUE on success, FALSE otherwise
 * 
 */
static osync_bool _osync_plugin_env_load_modules(OSyncPluginEnv *env, const char *path, osync_bool must_exist, OSyncError **error)
{
	GDir *dir = NULL;
	GError *gerror = NULL;
	char *filename = NULL;
	OSyncModule *module = NULL;
	const gchar *de = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %i, %p)", __func__, env, path, must_exist, error);
	osync_assert(env);
	osync_assert(path);
	
	//Load all available shared libraries (plugins)
	if (!g_file_test(path, G_FILE_TEST_IS_DIR)) {
		if (must_exist) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Path is not loadable");
			goto error;
		} else {
			osync_trace(TRACE_EXIT, "%s: Directory does not exist (non-fatal)", __func__);
			return TRUE;
		}
	}
	
	dir = g_dir_open(path, 0, &gerror);
	if (!dir) {
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to open directory %s: %s", path, gerror->message);
		g_error_free(gerror);
		goto error;
	}
	
	while ((de = g_dir_read_name(dir))) {
		filename = g_strdup_printf ("%s/%s", path, de);
		
		if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR) || g_file_test(filename, G_FILE_TEST_IS_SYMLINK) || !g_pattern_match_simple("*.so", filename)) {
			g_free(filename);
			continue;
		}
		
		module = osync_module_new(error);
		if (!module)
			goto error_free_filename;
		
		if (!osync_module_load(module, filename, error)) {
			osync_trace(TRACE_INTERNAL, "Unable to load module %s: %s", filename, osync_error_print(error));
			osync_error_unref(error);
			osync_module_free(module);
		} else {
			if (!osync_module_get_sync_info(module, env, error))
				goto error_free_module;
		}
		g_free(filename);
	}
	
	g_dir_close(dir);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_module:
	osync_module_free(module);
error_free_filename:
	g_free(filename);
	g_dir_close(dir);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

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
 * @defgroup OSyncEnvAPI OpenSync Environment
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
OSyncPluginEnv *osync_plugin_env_new(OSyncError **error)
{
	OSyncPluginEnv *env = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	env = osync_try_malloc0(sizeof(OSyncPluginEnv), error);
	if (!env) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return env;
}

/*! @brief Frees a osync environment
 * 
 * Frees a osync environment and all resources.
 * 
 * @param env Pointer to the environment to free
 * 
 */
void osync_plugin_env_free(OSyncPluginEnv *env)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);
	g_assert(env);
	
	/* Free the plugins */
	while (env->plugins) {
		osync_plugin_unref(env->plugins->data);
		env->plugins = g_list_remove(env->plugins, env->plugins->data);
	}
	
	/* Unload all modules */
	while (env->modules) {
		osync_module_unload(env->modules->data);
		osync_module_free(env->modules->data);
		env->modules = g_list_remove(env->modules, env->modules->data);
	}
	
	g_free(env);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Loads the sync modules from a given directory
 * 
 * Loads all sync modules from a directory into a osync environment
 * 
 * @param env Pointer to a OSyncEnv environment
 * @param path The path where to look for plugins
 * @param error Pointer to a error struct to return a error
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_plugin_env_load(OSyncPluginEnv *env, const char *path, OSyncError **error)
{
	osync_bool must_exist = TRUE;
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, path, error);
	
	if (!path) {
		path = OPENSYNC_PLUGINDIR;
		must_exist = FALSE;
	}
	
	if (!_osync_plugin_env_load_modules(env, path, must_exist, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/*! @brief Finds the plugin with the given name
 * 
 * Finds the plugin with the given name
 * 
 * @param env Pointer to a OSyncEnv environment
 * @param name The name to search for
 * @returns The plugin or NULL if not found
 * 
 */
OSyncPlugin *osync_plugin_env_find_plugin(OSyncPluginEnv *env, const char *name)
{
	GList *p;
	osync_assert(env);
	for (p = env->plugins; p; p = p->next) {
		OSyncPlugin *plugin = p->data;
		if (g_ascii_strcasecmp(osync_plugin_get_name(plugin), name) == 0)
			return plugin;
	}
	return NULL;
}

/*! @brief Returns the number of loaded plugins
 * 
 * Returns the number of loaded plugins. 0 if used before initialization
 * 
 * @param env Pointer to a OSyncEnv environment
 * @returns Number of plugins
 * 
 */
int osync_plugin_env_num_plugins(OSyncPluginEnv *env)
{
	return g_list_length(env->plugins);
}

/*! @brief Returns pointer to nth plugin
 * 
 * Returns pointer to nth plugin
 * 
 * @param env Pointer to a OSyncEnv environment
 * @param nth Which plugin to return
 * @returns Pointer to plugin
 * 
 */
OSyncPlugin *osync_plugin_env_nth_plugin(OSyncPluginEnv *env, int nth)
{
	return (OSyncPlugin *)g_list_nth_data(env->plugins, nth);
}

osync_bool osync_plugin_env_plugin_is_usable(OSyncPluginEnv *env, const char *pluginname, OSyncError **error)
{
	return TRUE;
}

/*@}*/
