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

/**
 * @defgroup PrivateAPI Private APIs
 * @brief Available private APIs
 * 
 */

/**
 * @defgroup OSyncPrivate OpenSync Private API
 * @ingroup PrivateAPI
 * @brief The private API of opensync
 * 
 * This gives you an insight in the private API of opensync.
 * 
 */

/**
 * @defgroup OSyncEnvPrivate OpenSync Environment Internals
 * @ingroup OSyncPrivate
 * @brief The internals of the opensync environment
 * 
 */
/*@{*/



static void free_hash(char *key, char *value, void *data)
{
	g_free(key);
	g_free(value);
}

/*! @brief Returns the next free number for a group in the environments configdir
 * 
 * Returns the next free number for a group in the environments configdir
 * 
 * @param env The osync environment
 * @returns The next free number
 * 
 */
long long int osync_env_create_group_id(OSyncEnv *env)
{
	char *filename = NULL;
	long long int i = 0;
	do {
		i++;
		if (filename)
			g_free(filename);
		filename = g_strdup_printf("%s/group%lli", env->groupsdir, i);
	} while (g_file_test(filename, G_FILE_TEST_EXISTS));
	g_free(filename);
	return i;
}

/*@}*/

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
OSyncEnv *osync_env_new(OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	OSyncEnv *env = osync_try_malloc0(sizeof(OSyncEnv), error);
	if (!env) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	env->is_initialized = FALSE;
	env->options = g_hash_table_new(g_str_hash, g_str_equal);
	
	//Set some defaults
	osync_env_set_option(env, "LOAD_GROUPS", "TRUE");
	osync_env_set_option(env, "LOAD_FORMATS", "TRUE");
	osync_env_set_option(env, "LOAD_PLUGINS", "TRUE");
	
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
void osync_env_free(OSyncEnv *env)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);
	g_assert(env);
	
	osync_env_finalize(env);
	
	g_hash_table_foreach(env->options, (GHFunc)free_hash, NULL);
	g_hash_table_destroy(env->options);
	
	g_free(env);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Sets a options on the environment
 * 
 * @param env Pointer to the environment
 * @param name Name of the option to set
 * @param value Value to set
 * 
 */
void osync_env_set_option(OSyncEnv *env, const char *name, const char *value)
{
	if (value)
		g_hash_table_insert(env->options, g_strdup(name), g_strdup(value));
	else
		g_hash_table_remove(env->options, name);
}


/* Get the value of a an OSyncEnv option
 *
 */
const char *osync_env_get_option(OSyncEnv *env, const char *name)
{
	const char *value;
	value = g_hash_table_lookup(env->options, name);
	if (value)
		return value;

	return NULL;
}

osync_bool osync_env_get_option_bool(OSyncEnv *env, const char *name)
{
	const char *get_value = NULL;
	if (!(get_value = osync_env_get_option(env, name)))
		return FALSE;
	
	if (!strcmp(get_value, "TRUE"))
		return TRUE;
	
	return FALSE;
}

/*! @brief Initializes the environment (loads plugins)
 * 
 * This will load all available plugins from disk. You can configure the location to look
 * for plugins before calling this function
 * 
 * @param env Pointer to a OSyncEnv environment
 * @param error Pointer to a error struct to return a error
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_env_initialize(OSyncEnv *env, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, error);
	g_assert(env);

	//Load the normal plugins
	if (osync_env_get_option_bool(env, "LOAD_PLUGINS")) {
		if (!osync_env_load_plugins(env, osync_env_query_option(env, "PLUGINS_DIRECTORY"), error))
			goto error;
	}

	//Load the format plugins
	if (osync_env_get_option_bool(env, "LOAD_FORMATS")) {
		if (!osync_env_load_formats(env, osync_env_query_option(env, "FORMATS_DIRECTORY"), error))
			goto error_finalize;
	}

	//Load groups
	if (osync_env_get_option_bool(env, "LOAD_GROUPS")) {
		if (!osync_env_load_groups(env, osync_env_query_option(env, "GROUPS_DIRECTORY"), error))
			goto error_finalize;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_finalize:
	osync_env_finalize(env);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/*! @brief Finalizes the environment
 * 
 * This will finalize the environment and unload and free all loaded plugins
 * 
 * @param env Pointer to a OSyncEnv environment
 * @param error Pointer to a error struct to return a error
 * @returns TRUE on success, FALSE otherwise
 * 
 */
void osync_env_finalize(OSyncEnv *env)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env);
	g_assert(env);
	
	/* Free the groups */
	while (env->groups) {
		osync_group_free(env->groups->data);
		env->groups = g_list_remove(env->groups, env->groups->data);
	}
	
	/* Free the plugins */
	while (env->plugins) {
		osync_plugin_free(env->plugins->data);
		env->plugins = g_list_remove(env->plugins, env->plugins->data);
	}
	
	/* Unload all modules */
	while (env->modules) {
		osync_module_unload(env, module);
		osync_module_free(env->modules->data);
		env->modules = g_list_remove(env->modules, env->modules->data);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

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
osync_bool osync_env_load_modules(OSyncEnv *env, const char *path, osync_bool must_exist, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %i, %p)", __func__, env, path, must_exist, error);
	osync_assert(env);
	osync_assert(path);
	
	GDir *dir = NULL;
	GError *gerror = NULL;
	char *filename = NULL;
	
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
  
	const gchar *de = NULL;
	while ((de = g_dir_read_name(dir))) {
		filename = g_strdup_printf ("%s/%s", path, de);
		
		if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR) || g_file_test(filename, G_FILE_TEST_IS_SYMLINK) || !g_pattern_match_simple("*.so", filename)) {
			g_free(filename);
			continue;
		}
		
		OSyncModule *module = osync_module_new(error);
		if (!module)
			goto error_free_filename;
		
		if (!osync_module_load(module, filename, error)) {
			osync_trace(TRACE_INTERNAL, "Unable to load module %s: %s", filename, osync_error_print(error));
			osync_error_unref(&error);
			osync_module_free(module);
		} else {
			if (!osync_module_get_info(module, env, error))
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
error_close_dir:
	g_dir_close(dir);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
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
osync_bool osync_env_load_plugins(OSyncEnv *env, const char *path, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, path, error);
	osync_bool must_exist = TRUE;
	
	if (!path) {
		path = OPENSYNC_PLUGINDIR;
		must_exist = FALSE;
	}
	
	if (!osync_module_load_dir(env, path, must_exist, error)) {
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
OSyncPlugin *osync_env_find_plugin(OSyncEnv *env, const char *name)
{
	g_assert(env);
	OSyncPlugin *plugin;
	int i;
	for (i = 0; i < osync_env_num_plugins(env); i++) {
		plugin = osync_env_nth_plugin(env, i);
		if (g_ascii_strcasecmp(plugin->info.name, name) == 0) {
			return plugin;
		}
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
int osync_env_num_plugins(OSyncEnv *env)
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
OSyncPlugin *osync_env_nth_plugin(OSyncEnv *env, int nth)
{
	return (OSyncPlugin *)g_list_nth_data(env->plugins, nth);
}

/*! @brief Checks if a plugin is available and usable
 * 
 * @param env The environment in which the plugin should be loaded
 * @param pluginname The name of the plugin to check for
 * @param error If the return was FALSE, will contain the information why the plugin is not available
 * @returns TRUE if plugin was found and is usable, FALSE otherwise
 * 
 */
osync_bool osync_env_plugin_is_usable(OSyncEnv *env, const char *pluginname, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, pluginname, error);
	
	OSyncPlugin *plugin = osync_env_find_plugin(env, pluginname);
	if (!plugin) {
		osync_error_set(error, OSYNC_ERROR_PLUGIN_NOT_FOUND, "Unable to find plugin \"%s\". This can be caused by unresolved symbols", pluginname);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	if (plugin->info.functions.is_available) {
		osync_bool ret = plugin->info.functions.is_available(error);
		osync_trace(ret ? TRACE_EXIT : TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return ret;
	}
	
	osync_trace(TRACE_EXIT, "%s: TRUE: No is_available function", __func__);
	return TRUE;
}

/*! @brief Loads the plugins from a given directory
 * 
 * Loads all plugins from a directory into a osync environment.
 * The directory must exist prior to opening.
 * 
 * @param env Pointer to a OSyncEnv environment
 * @param path The path where to look for groups
 * @param error Pointer to a error struct to return a error
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_env_load_groups(OSyncEnv *env, const char *p, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, p, error);
	
	GDir *dir = NULL;
	GError *gerror = NULL;
	char *filename = NULL;
	char *real_path = NULL;
	char *path = NULL;
	
	if (!p) {
		OSyncUserInfo *user = osync_user_new(error);
		if (!user)
			goto error;
		path = g_strdup(osync_user_get_confdir(user));
		osync_user_free(user);
		
		if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
			if (mkdir(path, 0700) < 0) {
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to create group directory at %s: %s", path, strerror(errno));
				goto error_free_path;
			}
			osync_trace(TRACE_INTERNAL, "Created groups configdir %s\n", path);
		}
	} else
		path = g_strdup(p);
	
	if (!g_path_is_absolute(path)) {
		real_path = g_strdup_printf("%s/%s", g_get_current_dir(), path);
	} else {
		real_path = g_strdup(path);
	}
	
	if (!g_file_test(real_path, G_FILE_TEST_IS_DIR)) {
		osync_error_set(error, OSYNC_ERROR_INITIALIZATION, "%s exists, but is no dir", real_path);
		goto error_free_real_path;
	}
	
	dir = g_dir_open(real_path, 0, &gerror);
	if (!dir) {
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to open main configdir %s: %s", real_path, gerror->message);
		g_error_free (gerror);
		goto error_free_real_path;
	}
  
	const gchar *de = NULL;
	while ((de = g_dir_read_name(dir))) {
		filename = g_strdup_printf ("%s/%s", real_path, de);
		
		if (!g_file_test(filename, G_FILE_TEST_IS_DIR) || g_file_test(filename, G_FILE_TEST_IS_SYMLINK) || !g_pattern_match_simple("group*", de)) {
			g_free(filename);
			continue;
		}
		
		/* Try to open the confdir*/
		OSyncGroup *group = osync_group_new(env, error);
		if (!group) {
			g_free(filename);
			goto error_free_real_path;
		}
		
		if (!osync_group_load(group, filename, error)) {
			g_free(filename);
			osync_group_free(group);
			goto error_free_real_path;
		}
		
		g_free(filename);
	}
	g_free(real_path);
	g_dir_close(dir);
	
	env->groupsdir = path;
	return TRUE;
	
	
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_real_path:
	g_free(real_path);
error_free_path:
	g_free(path);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/*! @brief Finds the group with the given name
 * 
 * Finds the group with the given name
 * 
 * @param env Pointer to a OSyncEnv environment
 * @param name Name of the group to search
 * @returns Pointer to group. NULL if not found
 * 
 */
OSyncGroup *osync_env_find_group(OSyncEnv *env, const char *name)
{
	OSyncGroup *group;
	int i;
	for (i = 0; i < osync_env_num_groups(env); i++) {
		group = osync_env_nth_group(env, i);
		if (g_ascii_strcasecmp(group->name, name) == 0) {
			return group;
		}
	}
	osync_debug("OSPLG", 0, "Couldnt find the group with the name %s", name);
	return NULL;
}

/*! @brief Adds the given group to the environment
 * 
 * Adds the given group to the environment
 * 
 * @param env Pointer to a OSyncEnv environment
 * @param group The group to add
 * 
 */
void osync_env_append_group(OSyncEnv *env, OSyncGroup *group)
{
	if (!group->configdir) {
		char *configdir = g_strdup_printf("%s/group%lli", env->groupsdir, osync_env_create_group_id(env));
		osync_group_set_configdir(group, configdir);
		g_free(configdir);
	}
	
	env->groups = g_list_append(env->groups, group);
}

/*! @brief Removes the given group from the enviroment
 * 
 * Removes the given group from the environment
 * 
 * @param env Pointer to a OSyncEnv environment
 * @param group The group to add
 * 
 */
void osync_env_remove_group(OSyncEnv *env, OSyncGroup *group)
{
	env->groups = g_list_remove(env->groups, group);
}

/*! @brief Counts the groups in the environment
 * 
 * Returns the number of groups
 * 
 * @param env Pointer to a OSyncEnv environment
 * @returns Number of groups
 * 
 */
int osync_env_num_groups(OSyncEnv *env)
{
	return g_list_length(env->groups);
}

/*! @brief Returns the nth group
 * 
 * Returns the nth groups from the environment
 * 
 * @param env Pointer to a OSyncEnv environment
 * @param nth Which group to return
 * @returns Pointer to the group
 * 
 */
OSyncGroup *osync_env_nth_group(OSyncEnv *env, int nth)
{
	return (OSyncGroup *)g_list_nth_data(env->groups, nth);;
}

/*@}*/
