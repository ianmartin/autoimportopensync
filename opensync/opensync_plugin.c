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
 * @defgroup OSyncPluginPrivateAPI OpenSync Plugin Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the plugins API
 * 
 */
/*@{*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS
OSyncObjTypeSink *osync_objtype_sink_from_template(OSyncGroup *group, OSyncObjTypeTemplate *template)
{
	g_assert(group);
	g_assert(template);
	OSyncObjTypeSink *sink = g_malloc0(sizeof(OSyncObjTypeSink));
	OSyncObjType *type = osync_conv_find_objtype(group->conv_env, template->name);
	if (!type) {
		osync_debug("OSYNC", 0, "Unable to find objtype named %s to create objtype sink", template->name);
		return NULL;
	}
	sink->objtype = type;
	sink->enabled = TRUE;
	sink->write = TRUE;
	sink->read = TRUE;
	return sink;
}

OSyncObjFormatSink *osync_objformat_sink_from_template(OSyncGroup *group, OSyncObjFormatTemplate *template)
{
	OSyncObjFormatSink *sink = g_malloc0(sizeof(OSyncObjFormatSink));
	OSyncObjFormat *format = osync_conv_find_objformat(group->conv_env, template->name);
	if (!format)
		return NULL;
	sink->format = format;
	sink->functions.commit_change = template->commit_change;
	sink->functions.access = template->access;
	sink->functions.read = template->read;
	sink->functions.committed_all = template->committed_all;
	sink->functions.batch_commit = template->batch_commit;
	sink->extension_name = g_strdup(template->extension_name);
	return sink;
}

OSyncObjTypeTemplate *osync_plugin_find_objtype_template(OSyncPlugin *plugin, const char *objtypestr)
{
	GList *o;
	for (o = plugin->accepted_objtypes; o; o = o->next) {
		OSyncObjTypeTemplate *template = o->data;
		if (!strcmp(template->name, objtypestr))
			return template;
	}
	return NULL;
}

OSyncObjFormatTemplate *osync_plugin_find_objformat_template(OSyncObjTypeTemplate *type_template, const char *objformatstr)
{
	GList *f;
	for (f = type_template->formats; f; f = f->next) {
		OSyncObjFormatTemplate *template = f->data;
		if (!strcmp(template->name, objformatstr))
			return template;
	}
	return NULL;
}

OSyncObjFormatSink *osync_objtype_find_format_sink(OSyncObjTypeSink *sink, const char *formatstr)
{
	GList *f;
	for (f = sink->formatsinks; f; f = f->next) {
		OSyncObjFormatSink *sink = f->data;
		if (!strcmp(sink->format->name, formatstr))
			return sink;
	}
	return NULL;
}
#endif

/*@}*/

/**
 * @defgroup OSyncPluginAPI OpenSync Plugin
 * @ingroup OSyncPublic
 * @brief Functions to register and manage plugins
 * 
 */
/*@{*/

/*! @brief This will create a new plugin struct
 * 
 * The plugin struct represents a sync plugin
 * 
 * @param env For which environment to register this plugin. May be NULL
 * @returns A pointer to a newly allocated plugin.
 * 
 */
OSyncPlugin *osync_plugin_new(OSyncEnv *env)
{
	OSyncPlugin *plugin = g_malloc0(sizeof(OSyncPlugin));
	g_assert(plugin);
	memset(&(plugin->info), 0, sizeof(plugin->info));
	memset(&(plugin->info.functions), 0, sizeof(plugin->info.functions));
	memset(&(plugin->info.timeouts), 0, sizeof(plugin->info.timeouts));
	
	//Set the default timeouts;
	plugin->info.timeouts.connect_timeout = 60;
	plugin->info.timeouts.sync_done_timeout = 60;
	plugin->info.timeouts.disconnect_timeout = 60;
	plugin->info.timeouts.get_changeinfo_timeout = 60;
	plugin->info.timeouts.get_data_timeout = 60;
	plugin->info.timeouts.commit_timeout = 60;
	plugin->info.timeouts.read_change_timeout = 60;
	
	plugin->info.plugin = plugin;
	plugin->info.config_type = NEEDS_CONFIGURATION;
	
	if (env) {
		env->plugins = g_list_append(env->plugins, plugin);
		plugin->env = env;
		plugin->real_plugin = env->current_module;
	}
	
	return plugin;
}


/*! @brief Registers a new plugin
 *
 * This function creates a new OSyncPluginInfo object, that
 * can be used to register a new plugin dynamically. This
 * can be used by a module to register multiple plugins,
 * instead of using get_info() function, that allows
 * registering of only one plugin.
 */
OSyncPluginInfo *osync_plugin_new_info(OSyncEnv *env)
{
	OSyncPlugin *plg = osync_plugin_new(env);
	osync_trace(TRACE_INTERNAL, "%s(%p): %p", __func__, env, plg);
	if (!plg)
		return NULL;

	return &plg->info;
}

/*! @brief Used to free a plugin
 * 
 * Frees a plugin
 * 
 * @param plugin Pointer to the plugin
 * 
 */
void osync_plugin_free(OSyncPlugin *plugin)
{
	osync_trace(TRACE_INTERNAL, "osync_plugin_free(%p)", plugin);
	g_assert(plugin);
	if (plugin->env)
		plugin->env->plugins = g_list_remove(plugin->env->plugins, plugin);

	//FIXME Free more stuff?
	g_free(plugin);
}

/*! @brief Used to look up a symbol on the plugin
 * 
 * Looks up and returns a function
 * 
 * @param plugin Pointer to the plugin
 * @param name The name of the function to look up
 * @param error Pointer to a error struct
 * @return Pointer to the function
 * 
 */
void *osync_plugin_get_function(OSyncPlugin *plugin, const char *name, OSyncError **error)
{
	void *function;
	if (!plugin->real_plugin) {
		osync_debug("OSPLG", 1, "You need to load a plugin before getting a function");
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "You need to load a plugin before getting a function");
		return NULL;
	}
	
	if (!g_module_symbol (plugin->real_plugin, name, &function)) {
		osync_debug("OSPLG", 0, "Unable to locate symbol %s on plugin", name);
		osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to locate symbol %s: %s", name, g_module_error());
		return NULL;
	}
	return function;
}

/*! @brief dlopen()s a format plugin
 * 
 * The get_info() function on the format plugin gets called
 * 
 * @param env The environment in which to open the plugin
 * @param path Where to find this plugin
 * @param error Pointer to a error struct
 * @return Pointer to the plugin on success, NULL otherwise
 * 
 */
osync_bool osync_module_load(OSyncEnv *env, const char *path, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, path, error);
	/* Check if this platform supports dynamic
	 * loading of modules */
	 
	if (!g_module_supported()) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "This platform does not support loading of modules");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	/* Try to open the module or fail if an error occurs */
	GModule *module = g_module_open(path, 0);
	if (!module) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open module %s: %s", path, g_module_error());
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	/* Load the get_info symbol */
	void (* fct_info)(OSyncEnv *env) = NULL;
	void (** fct_infop)(OSyncEnv *env) = &fct_info;
	if (!g_module_symbol(module, "get_info", (void **)fct_infop)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get symbol from module %s: %s", path, g_module_error());
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	env->modules = g_list_append(env->modules, module);
	
	/* Call the get_info function */
	env->current_module = module;
	fct_info(env);
	env->current_module = NULL;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, module);
	return TRUE;
}

/*! @brief Closes a module
 * 
 * @param module The module to unload
 * 
 */
void osync_module_unload(OSyncEnv *env, GModule *module)
{
	osync_trace(TRACE_INTERNAL, "%s(%p, %p)", __func__, env, module);
	//FIXME Close the module! This crashes the evo2 plugin at the moment, i have no idea why...
	//g_module_close(plugin->real_plugin);
	env->modules = g_list_remove(env->modules, module);
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
osync_bool osync_module_load_dir(OSyncEnv *env, const char *path, osync_bool must_exist, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, path, error);
	GDir *dir;
	GError *gerror = NULL;
	char *filename = NULL;

	if (!path) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Not path given to load the modules from");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	//Load all available shared libraries (plugins)
	if (!g_file_test(path, G_FILE_TEST_IS_DIR)) {
		if (must_exist) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Path is not loadable");
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
			return FALSE;
		} else {
			osync_trace(TRACE_EXIT, "%s: Directory does not exist (non-fatal)", __func__);
			return TRUE;
		}
	}
	
	dir = g_dir_open(path, 0, &gerror);
	if (!dir) {
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to open directory %s: %s", path, gerror->message);
		g_error_free(gerror);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
  
	const gchar *de = NULL;
	while ((de = g_dir_read_name(dir))) {
		filename = g_strdup_printf ("%s/%s", path, de);
		
		if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR) || g_file_test(filename, G_FILE_TEST_IS_SYMLINK) || !g_pattern_match_simple("*.so", filename)) {
			g_free(filename);
			continue;
		}
		
		OSyncError *error = NULL;
		if (!osync_module_load(env, filename, &error)) {
			osync_debug("OSPLG", 0, "Unable to load plugin %s: %s", filename, error->message);
			osync_error_free(&error);
		}
		g_free(filename);
	}
	g_dir_close(dir);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/*! @brief Returns the name of the loaded plugin
 * 
 * @param plugin Pointer to the plugin
 * @returns Name of the plugin
 * 
 */
const char *osync_plugin_get_name(OSyncPlugin *plugin)
{
	g_assert(plugin);
	return plugin->info.name;
}

/*! @brief Returns the long name of the loaded plugin
 * 
 * @param plugin Pointer to the plugin
 * @returns Long name of the plugin
 * 
 */
const char *osync_plugin_get_longname(OSyncPlugin *plugin)
{
	g_assert(plugin);
	return plugin->info.longname;
}

/*! @brief Returns the description of the plugin
 * 
 * @param plugin Pointer to the plugin
 * @returns Description of the plugin
 * 
 */
const char *osync_plugin_get_description(OSyncPlugin *plugin)
{
	g_assert(plugin);
	return plugin->info.description;
}

/*! @brief Returns the timeouts of the plugin
 * 
 * @param plugin Pointer to the plugin
 * @returns Timeouts of the plugin
 * 
 */
OSyncPluginTimeouts osync_plugin_get_timeouts(OSyncPlugin *plugin)
{
	g_assert(plugin);
	return plugin->info.timeouts;
}

/*! @brief Returns the plugin_info data, set by the plugin
 *
 * @param plugin Pointer to the plugin
 * @returns The void pointer set on plugin->info.plugin_data
 */
void *osync_plugin_get_plugin_data(OSyncPlugin *plugin)
{
	g_assert(plugin);
	return plugin->info.plugin_data;
}

void _osync_format_set_commit(OSyncObjTypeTemplate *template, const char *formatstr, OSyncFormatCommitFn commit_change)
{
	OSyncObjFormatTemplate *format_template = NULL;
	if (formatstr) {
		OSyncObjFormatTemplate *format_template = osync_plugin_find_objformat_template(template, formatstr);
		osync_assert(format_template, "Unable to set commit function. Did you forget to add the objformat?");
		format_template->commit_change = commit_change;
	} else {
		GList *f = NULL;
		for (f = template->formats; f; f = f->next) {
			format_template = f->data;
			format_template->commit_change = commit_change;
		}
	}
}

/*! @brief Sets the commit function of a format
 * 
 * @param info Pointer to a plugin info struct to fill
 * @param objtypestr The name of the object type
 * @param formatstr The name of the format
 * @param commit_change The pointer to your commit_change function
 * 
 */
void osync_plugin_set_commit_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, OSyncFormatCommitFn commit_change)
{
	OSyncObjTypeTemplate *template = NULL;
	
	if (objtypestr) {
		OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
		osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
		_osync_format_set_commit(template, formatstr, commit_change);
	} else {
		GList *o = NULL;
		for (o = info->plugin->accepted_objtypes; o; o = o->next) {
			template = o->data;
			_osync_format_set_commit(template, formatstr, commit_change);
		}
	}
}

void _osync_format_set_access(OSyncObjTypeTemplate *template, const char *formatstr, OSyncFormatAccessFn access)
{
	OSyncObjFormatTemplate *format_template = NULL;
	if (formatstr) {
		format_template = osync_plugin_find_objformat_template(template, formatstr);
		osync_assert(format_template, "Unable to set commit function. Did you forget to add the objformat?");
		format_template->access = access;
	} else {
		GList *f = NULL;
		for (f = template->formats; f; f = f->next) {
			format_template = f->data;
			format_template->access = access;
		}
	}
}

/*! @brief Sets the access function of a format
 * 
 * @param info Pointer to a plugin info struct to fill
 * @param objtypestr The name of the object type
 * @param formatstr The name of the format
 * @param access The pointer to your access function
 * 
 */
void osync_plugin_set_access_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, OSyncFormatAccessFn access)
{
	OSyncObjTypeTemplate *template = NULL;
	
	if (objtypestr) {
		template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
		osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
		_osync_format_set_access(template, formatstr, access);
	} else {
		GList *o = NULL;
		for (o = info->plugin->accepted_objtypes; o; o = o->next) {
			template = o->data;
			_osync_format_set_access(template, formatstr, access);
		}
	}
}

/*! @brief Sets the read function of a format
 * 
 * @param info Pointer to a plugin info struct to fill
 * @param objtypestr The name of the object type
 * @param formatstr The name of the format
 * @param read The pointer to your read function
 * 
 */
void osync_plugin_set_read_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, OSyncFormatReadFn read)
{
	OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
	osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
	OSyncObjFormatTemplate *format_template = osync_plugin_find_objformat_template(template, formatstr);
	osync_assert(format_template, "Unable to set commit function. Did you forget to add the objformat?");
	format_template->read = read;
}

void _osync_format_set_batch(OSyncObjTypeTemplate *template, const char *formatstr, OSyncFormatBatchCommitFn batch)
{
	OSyncObjFormatTemplate *format_template = NULL;
	if (formatstr) {
		format_template = osync_plugin_find_objformat_template(template, formatstr);
		osync_assert(format_template, "Unable to set batch commit function. Did you forget to add the objformat?");
		format_template->batch_commit = batch;
	} else {
		GList *f = NULL;
		for (f = template->formats; f; f = f->next) {
			format_template = f->data;
			format_template->batch_commit = batch;
		}
	}
}

/*! @brief Sets the batch_commit function of a format
 * 
 * @param info Pointer to a plugin info struct to fill
 * @param objtypestr The name of the object type
 * @param formatstr The name of the format
 * @param batch The pointer to your batch_commit function
 * 
 */
void osync_plugin_set_batch_commit_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, OSyncFormatBatchCommitFn batch)
{
	OSyncObjTypeTemplate *template = NULL;
	
	if (objtypestr) {
		template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
		osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
		_osync_format_set_batch(template, formatstr, batch);
	} else {
		GList *o = NULL;
		for (o = info->plugin->accepted_objtypes; o; o = o->next) {
			template = o->data;
			_osync_format_set_batch(template, formatstr, batch);
		}
	}
}

/*! @brief Sets the committed_all function of a format
 * 
 * @param info Pointer to a plugin info struct to fill
 * @param objtypestr The name of the object type
 * @param formatstr The name of the format
 * @param committed_all The pointer to your committed_all function
 * 
 */
void osync_plugin_set_committed_all_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, OSyncFormatCommittedAllFn committed_all)
{
	OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
	osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
	OSyncObjFormatTemplate *format_template = osync_plugin_find_objformat_template(template, formatstr);
	osync_assert(format_template, "Unable to set committed_all function. Did you forget to add the objformat?");
	format_template->committed_all = committed_all;
}

/*! @brief Tells opensync that the plugin can accepts this object
 * 
 * Tells opensync that the plugin can accepts this object. Used by the plugin
 * in the get_info() function
 * 
 * @param info The plugin info on which to operate
 * @param objtypestr The name of the object which to accept
 * 
 */
void osync_plugin_accept_objtype(OSyncPluginInfo *info, const char *objtypestr)
{
	OSyncObjTypeTemplate *template = g_malloc0(sizeof(OSyncObjTypeTemplate));
	template->name = g_strdup(objtypestr);
	info->plugin->accepted_objtypes = g_list_append(info->plugin->accepted_objtypes, template);
}

/*! @brief Tells opensync that the plugin can accepts this format for the given object
 * 
 * Tells opensync that the plugin can accepts this format. Used by the plugin
 * in the get_info() function
 * 
 * @param info The plugin info on which to operate
 * @param objtypestr The name of the objecttype
 * @param formatstr The name of the format to accept
 * @param extension The name of the extension that the plugin wants. NULL if none
 * 
 */
void osync_plugin_accept_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, const char *extension)
{
	OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
	osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
	OSyncObjFormatTemplate *format_template = g_malloc0(sizeof(OSyncObjFormatTemplate));
	format_template->name = g_strdup(formatstr);
	if (extension)
		format_template->extension_name = g_strdup(extension);
	template->formats = g_list_append(template->formats, format_template);
}

/*@}*/
