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

/* Get the value of a an OSyncEnv option
 *
 * Search order:
 * - options set using osync_env_set_option()
 * - OSYNC_* environment variables
 */
static const char *osync_env_query_option(OSyncEnv *env, const char *name)
{
	const char *value;
	value = g_hash_table_lookup(env->options, name);
	if (value)
		return value;

	gchar *env_name = g_strdup_printf("OSYNC_%s", name);
	value = getenv(env_name);
	g_free(env_name);

	if (value)
		return value;

	return NULL;
}

static osync_bool osync_env_query_option_bool(OSyncEnv *env, const char *name)
{
	const char *get_value;
	if (!(get_value = osync_env_query_option(env, name)))
		return FALSE;
	if (!strcmp(get_value, "TRUE"))
		return TRUE;
	return FALSE;
}

/** Export the list of loaded plugins through the OSYNC_LOADED_PLUGINS environment variable
 *
 */
void osync_env_export_loaded_modules(OSyncEnv *env)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);

	int num_modules = g_list_length(env->modules);

	/* build an array for g_strjoinv() */
	gchar **path_array = g_malloc0(sizeof(gchar*)*num_modules + 1);
	int i;
	for (i = 0; i < num_modules; i++) {
		GModule *module = g_list_nth_data(env->modules, i);
		const gchar *path = g_module_name(module);
		osync_trace(TRACE_INTERNAL, "Path being exported: %s", path);
		/*XXX: casting to non-const, here. Ugly.
		 *
		 * We know the elements pointed by path_array won't
		 * be touched. But isn't g_strjoinv() supposed to get a
		 * 'const gchar **' instead of a 'gchar **'?
		 */
		path_array[i] = (gchar*)path;
	}

	/* Build a ':'-separated list */
	gchar *list_str = g_strjoinv(":", path_array);
	osync_trace(TRACE_INTERNAL, "MODULE_LIST: %s", list_str);
	setenv("OSYNC_MODULE_LIST", list_str, 1);
	g_free(list_str);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void export_option_to_env(gpointer key, gpointer data, gpointer user_data)
{
	const char *name = (const char*)key;
	const char *value = (const char*)data;
	gchar *env_name = g_strdup_printf("OSYNC_%s", name);
	setenv(env_name, value, 1);
	g_free(env_name);
}

/** Export all options set through osync_env_set_option() to environment variables
 *
 */
void osync_env_export_all_options(OSyncEnv *env)
{
	g_hash_table_foreach(env->options, export_option_to_env, NULL);
}

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
long long int _osync_env_create_group_id(OSyncEnv *env)
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
OSyncEnv *osync_env_new(void)
{
	OSyncEnv *env = g_malloc0(sizeof(OSyncEnv));
	env->is_initialized = FALSE;
	env->options = g_hash_table_new(g_str_hash, g_str_equal);
	
	//Set some defaults
	osync_env_set_option(env, "LOAD_GROUPS", "TRUE");
	osync_env_set_option(env, "LOAD_FORMATS", "TRUE");
	osync_env_set_option(env, "LOAD_PLUGINS", "TRUE");
	
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
	g_assert(env);
	g_hash_table_foreach(env->options, (GHFunc)free_hash, NULL);
	g_hash_table_destroy(env->options);
	g_free(env);
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
	osync_trace(TRACE_ENTRY, "osync_env_initialize(%p, %p)", env, error);
	g_assert(env);
	
	if (env->is_initialized) {
		osync_error_set(error, OSYNC_ERROR_INITIALIZATION, "Cannot initialize the same environment twice");
		osync_trace(TRACE_EXIT_ERROR, "osync_env_initialize: %s", osync_error_print(error));
		return FALSE;
	}

	//Load the normal plugins
	if (osync_env_query_option_bool(env, "LOAD_PLUGINS")) {
		if (!osync_env_load_plugins(env, osync_env_query_option(env, "PLUGINS_DIRECTORY"), error)) {
			osync_trace(TRACE_EXIT_ERROR, "osync_env_initialize: %s", osync_error_print(error));
			return FALSE;
		}
	}

	//Load the format plugins
	if (osync_env_query_option_bool(env, "LOAD_FORMATS")) {
		if (!osync_env_load_formats(env, osync_env_query_option(env, "FORMATS_DIRECTORY"), error)) {
			osync_trace(TRACE_EXIT_ERROR, "osync_env_initialize: %s", osync_error_print(error));
			return FALSE;
		}
	}

	//Load groups
	if (osync_env_query_option_bool(env, "LOAD_GROUPS")) {
		if (!osync_env_load_groups(env, osync_env_query_option(env, "GROUPS_DIRECTORY"), error)) {
			osync_trace(TRACE_EXIT_ERROR, "osync_env_initialize: %s", osync_error_print(error));
			return FALSE;
		}
	}

	env->is_initialized = TRUE;
	osync_trace(TRACE_EXIT, "osync_env_initialize");
	return TRUE;
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
osync_bool osync_env_finalize(OSyncEnv *env, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_env_finalize(%p, %p)", env, error);
	g_assert(env);
	
	if (!env->is_initialized) {
		osync_error_set(error, OSYNC_ERROR_INITIALIZATION, "Environment has to be initialized before");
		return FALSE;
	}
	
	while (osync_env_nth_group(env, 0))
		osync_group_free(osync_env_nth_group(env, 0));
	
	GList *plugins = g_list_copy(env->plugins);
	GList *p;
	for (p = plugins; p; p = p->next) {
		OSyncPlugin *plugin = p->data;
		osync_plugin_free(plugin);
	}
	g_list_free(plugins);
	
	//Unload all loaded modules
	GList *modules = g_list_copy(env->modules);
	for (p = modules; p; p = p->next) {
		GModule *module = p->data;
		osync_module_unload(env, module);
	}
	g_list_free(modules);

	osync_trace(TRACE_EXIT, "osync_env_finalize");
	return TRUE;
}

/*! @brief Loads all format and conversion plugins
 * 
 * This command will load all plugins for the conversion system.
 * If you dont change the path before it will load the plugins
 * from the default location
 * 
 * @param env The format environment
 * @param path The path to load from or NULL if to load from default path
 * @param error The location to return a error to
 * @returns TRUE if successfull, FALSE otherwise
 * 
 */
osync_bool osync_env_load_formats(OSyncEnv *env, const char *path, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, path, error);
	osync_bool must_exist = TRUE;
	
	if (!path) {
		path = OPENSYNC_FORMATSDIR;
		must_exist = FALSE;
	}
	
	if (!osync_module_load_dir(env, path, must_exist, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
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
	GDir *dir;
	GError *gerror = NULL;
	char *filename = NULL;
	char *real_path = NULL;
	char *path = g_strdup(p);
	
	if (!path) {
		OSyncUserInfo *user = osync_user_new(error);
		if (!user)
			return FALSE;
		path = g_strdup(osync_user_get_confdir(user));
		
		if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
			if (mkdir(path, 0700) == -1) {
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to create group directory at %s: %s", path, strerror(errno));
				g_free(path);
				return FALSE;
			}
			char *enginepath = g_strdup_printf("%s/engines", path);
			if (mkdir(enginepath, 0700) == -1) {
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to create engine group directory at %s: %s", enginepath, strerror(errno));
				g_free(path);
				g_free(enginepath);
				return FALSE;
			}
			g_free(enginepath);
			osync_debug("OSGRP", 3, "Created groups configdir %s\n", path);
		}
		osync_user_free(user);
	}
	
	if (!g_path_is_absolute(path)) {
		real_path = g_strdup_printf("%s/%s", g_get_current_dir(), path);
	} else {
		real_path = g_strdup(path);
	}
	
	if (!g_file_test(real_path, G_FILE_TEST_IS_DIR)) {
		osync_debug("OSGRP", 0, "%s exists, but is no dir", real_path);
		osync_error_set(error, OSYNC_ERROR_INITIALIZATION, "%s exists, but is no dir", real_path);
		g_free(real_path);
		g_free(path);
		return FALSE;
	}
	
	dir = g_dir_open(real_path, 0, &gerror);
	if (!dir) {
		osync_debug("OSGRP", 0, "Unable to open main configdir %s: %s", real_path, gerror->message);
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to open main configdir %s: %s", real_path, gerror->message);
		g_error_free (gerror);
		g_free(real_path);
		g_free(path);
		return FALSE;
	}
  
	const gchar *de = NULL;
	while ((de = g_dir_read_name(dir))) {
		filename = g_strdup_printf ("%s/%s", real_path, de);
		
		if (!g_file_test(filename, G_FILE_TEST_IS_DIR) || g_file_test(filename, G_FILE_TEST_IS_SYMLINK) || !g_pattern_match_simple("group*", de)) {
			g_free(filename);
			continue;
		}
		
		/* Try to open the confdir*/
		OSyncError *error = NULL;
		if (!osync_group_load(env, filename, &error)) {
			osync_debug("OSGRP", 0, "Unable to load group from %s: %s", filename, error->message);
			osync_error_free(&error);
		}
		
		g_free(filename);
	}
	g_free(real_path);
	g_dir_close(dir);
	
	env->groupsdir = path;
	return TRUE;
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

/**
 * @defgroup OSyncEnvAPIMisc OpenSync Misc
 * @ingroup OSyncPublic
 * @brief Some helper functions
 * 
 */
/*@{*/

/*! @brief Opens a xml document
 * 
 * Opens a xml document
 * 
 * @param doc Pointer to a xmldoc
 * @param cur The pointer to the first node
 * @param path The path of the document
 * @param topentry the name of the top node
 * @param error Pointer to a error struct
 * @returns TRUE if successfull, FALSE otherwise
 * 
 */
osync_bool _osync_open_xml_file(xmlDocPtr *doc, xmlNodePtr *cur, const char *path, const char *topentry, OSyncError **error)
{
	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		osync_debug("OSXML", 1, "File %s does not exist", path);
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "File %s does not exist", path);
		return FALSE;
	}
	
	*doc = xmlParseFile(path);

	if (!*doc) {
		osync_debug("OSXML", 1, "Could not open: %s", path);
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Could not open: %s", path);
		return FALSE;
	}

	*cur = xmlDocGetRootElement(*doc);

	if (!*cur) {
		osync_debug("OSXML", 0, "%s seems to be empty", path);
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "%s seems to be empty", path);
		xmlFreeDoc(*doc);
		return FALSE;
	}

	if (xmlStrcmp((*cur)->name, (const xmlChar *) topentry)) {
		osync_debug("OSXML", 0, "%s seems not to be a valid configfile.\n", path);
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "%s seems not to be a valid configfile.\n", path);
		xmlFreeDoc(*doc);
		return FALSE;
	}

	*cur = (*cur)->xmlChildrenNode;
	return TRUE;
}

/*! @brief Writes data to a file
 * 
 * Writes data to a file
 * 
 * @param filename Where to save the data
 * @param data Pointer to the data
 * @param size Size of the data
 * @param mode The mode to set on the file
 * @param oserror Pointer to a error struct
 * @returns TRUE if successfull, FALSE otherwise
 * 
 */
osync_bool osync_file_write(const char *filename, const char *data, int size, int mode, OSyncError **oserror)
{
	osync_bool ret = FALSE;
	GError *error = NULL;
	GIOChannel *chan = g_io_channel_new_file(filename, "w", &error);
	if (!chan) {
		osync_debug("OSYNC", 3, "Unable to open file %s for writing: %s", filename, error->message);
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to open file %s for writing: %s", filename, error->message);
		return FALSE;
	}
	if (mode) {
		int fd = g_io_channel_unix_get_fd(chan);
		if (fchmod(fd, mode)) {
			osync_debug("OSYNC", 3, "Unable to set file permissions %i for file %s", mode, filename);
			osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to set file permissions %i for file %s", mode, filename);
			return FALSE;
		}
	}
	gsize writen;
	g_io_channel_set_encoding(chan, NULL, NULL);
	if (g_io_channel_write_chars(chan, data, size, &writen, &error) != G_IO_STATUS_NORMAL) {
		osync_debug("OSYNC", 3, "Unable to write contents of file %s: %s", filename, error->message);
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to write contents of file %s: %s", filename, error->message);
	} else {
		g_io_channel_flush(chan, NULL);
		ret = TRUE;
	}
	g_io_channel_shutdown(chan, FALSE, NULL);
	g_io_channel_unref(chan);
	return ret;
}

/*! @brief Reads a file
 * 
 * Reads a file
 * 
 * @param filename Where to read the data from
 * @param data Pointer to the data
 * @param size Size of the data
 * @param oserror Pointer to a error struct
 * @returns TRUE if successfull, FALSE otherwise
 * 
 */
osync_bool osync_file_read(const char *filename, char **data, int *size, OSyncError **oserror)
{
	osync_bool ret = FALSE;
	GError *error = NULL;
	gsize sz = 0;
	
	if (!filename) {
		osync_debug("OSYNC", 3, "No file open specified");
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "No file to open specified");
		return FALSE;
	}
	GIOChannel *chan = g_io_channel_new_file(filename, "r", &error);
	if (!chan) {
		osync_debug("OSYNC", 3, "Unable to read file %s: %s", filename, error->message);
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to open file %s for reading: %s", filename, error->message);
		return FALSE;
	}
	g_io_channel_set_encoding(chan, NULL, NULL);
	if (g_io_channel_read_to_end(chan, data, &sz, &error) != G_IO_STATUS_NORMAL) {
		osync_debug("OSYNC", 3, "Unable to read contents of file %s: %s", filename, error->message);
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to read contents of file %s: %s", filename, error->message);
	} else {
		ret = TRUE;
		*size = (int)sz;
	}
	g_io_channel_shutdown(chan, FALSE, NULL);
	g_io_channel_unref(chan);
	return ret;
}

/*! @brief Returns the version of opensync
 * 
 * Returns a string identifying the major and minor version
 * of opensync (something like "0.11")
 * 
 * @returns String with version
 * 
 */
const char *osync_get_version(void)
{
	return VERSION;
}

/*! @brief Safely tries to malloc memory
 * 
 * Tries to malloc memory but returns an error in an OOM situation instead
 * of aborting
 * 
 * @param size The size in bytes to malloc
 * @param error The error which will hold the info in case of an error
 * @returns A pointer to the new memory or NULL in case of error
 * 
 */
void *osync_try_malloc0(unsigned int size, OSyncError **error)
{
	void *result = g_try_malloc(size);
	if (!result) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No memory left");
		return NULL;
	}
	memset(result, 0, size);
	return result;
}

/*@}*/

OSyncThread *osync_thread_new(GMainContext *context, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, context, error);
	
	OSyncThread *thread = osync_try_malloc0(sizeof(OSyncThread), error);
	if (!thread)
		goto error;

	if (!g_thread_supported ()) g_thread_init (NULL);
	
	thread->started_mutex = g_mutex_new();
	thread->started = g_cond_new();
	thread->context = context;
	if (thread->context)
		g_main_context_ref(thread->context);
	thread->loop = g_main_loop_new(thread->context, FALSE);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, thread);
	return thread;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void osync_thread_free(OSyncThread *thread)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, thread);
	osync_assert(thread);
	
	if (thread->started_mutex)
		g_mutex_free(thread->started_mutex);

	if (thread->started)
		g_cond_free(thread->started);
	
	if (thread->loop)
		g_main_loop_unref(thread->loop);
	
	if (thread->context)
		g_main_context_unref(thread->context);
		
	g_free(thread);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*static gpointer osyncThreadStartCallback(gpointer data)
{
	OSyncThread *thread = data;
	
	g_mutex_lock(thread->started_mutex);
	g_cond_signal(thread->started);
	g_mutex_unlock(thread->started_mutex);
	
	g_main_loop_run(thread->loop);
	
	return NULL;
}*/

static gboolean osyncThreadStopCallback(gpointer data)
{
	OSyncThread *thread = data;
	
	g_main_loop_quit(thread->loop);
	
	return FALSE;
}

/*void osync_thread_start(OSyncThread *thread)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, thread);
	osync_assert(thread);
	
	//Start the thread
	g_mutex_lock(thread->started_mutex);
	thread->thread = g_thread_create (osyncThreadStartCallback, thread, TRUE, NULL);
	g_cond_wait(thread->started, thread->started_mutex);
	g_mutex_unlock(thread->started_mutex);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}*/

static gboolean osyncThreadStartCallback(gpointer data)
{
	OSyncThread *thread = data;
	
	g_mutex_lock(thread->started_mutex);
	g_cond_signal(thread->started);
	g_mutex_unlock(thread->started_mutex);
	return FALSE;
}

void osync_thread_start(OSyncThread *thread)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, thread);
	
	g_mutex_lock(thread->started_mutex);
	GSource *idle = g_idle_source_new();
	g_source_set_callback(idle, osyncThreadStartCallback, thread, NULL);
	g_source_attach(idle, thread->context);
	thread->thread = g_thread_create ((GThreadFunc)g_main_loop_run, thread->loop, TRUE, NULL);
	g_cond_wait(thread->started, thread->started_mutex);
	g_mutex_unlock(thread->started_mutex);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}	

void osync_thread_stop(OSyncThread *thread)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, thread);
	osync_assert(thread);
	
	GSource *source = g_idle_source_new();
	g_source_set_callback(source, osyncThreadStopCallback, thread, NULL);
	g_source_attach(source, thread->context);

	g_thread_join(thread->thread);
	thread->thread = NULL;
	
	g_source_unref(source);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}
