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
        
        if (env) {
        	env->plugins = g_list_append(env->plugins, plugin);
        	plugin->env = env;
        }
        
        return plugin;
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

/*! @brief dlopen()s a plugin and returns the information from it
 * 
 * The get_info() function on the plugin gets called and the information is stored
 * in the plugin struct
 * 
 * @param env The environment in which to open the plugin
 * @param path Where to find this plugin
 * @param error Pointer to a error struct
 * @return Pointer to the plugin on success, NULL otherwise
 * 
 */
OSyncPlugin *osync_plugin_load(OSyncEnv *env, const char *path, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_plugin_load(%p, %s, %p)", env, path, error);
	
	/* Check if this platform supports dynamic
	 * loading of modules */
	if (!g_module_supported()) {
		osync_debug("OSPLG", 0, "This platform does not support loading of modules");
		osync_error_set(error, OSYNC_ERROR_GENERIC, "This platform does not support loading of modules");
		osync_trace(TRACE_EXIT_ERROR, "osync_plugin_load: %s", osync_error_print(error));
		return NULL;
	}

	/* Try to open the module or fail if an error occurs */
	OSyncPlugin *plugin = osync_plugin_new(env);
	plugin->real_plugin = g_module_open(path, G_MODULE_BIND_LOCAL);
	memset(&(plugin->info.functions), 0, sizeof(OSyncPluginFunctions));
	
	if (!plugin->real_plugin) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open plugin %s: %s", path, g_module_error());
		osync_plugin_free(plugin);
		osync_trace(TRACE_EXIT_ERROR, "osync_plugin_load: %s", osync_error_print(error));
		return NULL;
	}
	
	void (* fct_info)(OSyncPluginInfo *info);
	if (!(fct_info = osync_plugin_get_function(plugin, "get_info", error))) {
		osync_debug("OSPLG", 0, "Unable to open plugin: Missing symbol get_info");
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open plugin: Missing symbol get_info");
		osync_plugin_unload(plugin);
		osync_plugin_free(plugin);
		osync_trace(TRACE_EXIT_ERROR, "osync_plugin_load: %s", osync_error_print(error));
		return NULL;
	}
	
	fct_info(&(plugin->info));
	plugin->path = g_strdup(path);
	
	osync_trace(TRACE_EXIT, "osync_plugin_load: %p", plugin);
	return plugin;
}

/*! @brief unloads a previously loaded plugin
 * 
 * This unloads the plugin (but does not free the struct itself)
 * 
 * @param plugin Pointer to the plugin
 * 
 */
void osync_plugin_unload(OSyncPlugin *plugin)
{
	g_assert(plugin);
	if (!plugin->real_plugin) {
		osync_debug("OSPLG", 0, "You need to load a plugin before unloading it");
		return;
	}
	
	//FIXME Close the module! This crashes the evo2 plugin at the moment, i have no idea why...
	//g_module_close(plugin->real_plugin);
	g_free(plugin->path);
	plugin->path = NULL;
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
osync_bool osync_format_plugin_load(OSyncEnv *env, char *path, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_format_plugin_load(%p, %s, %p)", env, path, error);
	/* Check if this platform supports dynamic
	 * loading of modules */
	 
	if (!g_module_supported()) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "This platform does not support loading of modules");
		osync_debug("OSPLG", 0, "This platform does not support loading of modules");
		osync_trace(TRACE_EXIT_ERROR, "osync_format_plugin_load: %s", osync_error_print(error));
		return FALSE;
	}

	/* Try to open the module or fail if an error occurs */
	GModule *plugin = g_module_open(path, G_MODULE_BIND_LOCAL);
	if (!plugin) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open plugin %s: %s", path, g_module_error());
		osync_debug("OSPLG", 0, "Unable to open plugin %s", path);
		osync_trace(TRACE_EXIT_ERROR, "osync_format_plugin_load: %s", osync_error_print(error));
		return FALSE;
	}
	
	void (* fct_info)(OSyncEnv *env) = NULL;
	void (** fct_infop)(OSyncEnv *env) = &fct_info;
	if (!g_module_symbol(plugin, "get_info", (void **)fct_infop)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open format plugin %s: %s", path, g_module_error());
		osync_debug("OSPLG", 0, "Unable to open format plugin %s", path);
		osync_trace(TRACE_EXIT_ERROR, "osync_format_plugin_load: %s", osync_error_print(error));
		return FALSE;
	}
	env->formatplugins = g_list_append(env->formatplugins, plugin);
	
	fct_info(env);
	osync_trace(TRACE_EXIT, "osync_format_plugin_load: %p", plugin);
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

/*! @brief Sets the commit function of a format
 * 
 * @param info Pointer to a plugin info struct to fill
 * @param objtypestr The name of the object type
 * @param formatstr The name of the format
 * @param commit_change The pointer to your commit_change function
 * 
 */
void osync_plugin_set_commit_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, osync_bool (* commit_change) (OSyncContext *, OSyncChange *))
{
	OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
	osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
	OSyncObjFormatTemplate *format_template = osync_plugin_find_objformat_template(template, formatstr);
	osync_assert(format_template, "Unable to set commit function. Did you forget to add the objformat?");
	format_template->commit_change = commit_change;
}

/*! @brief Sets the access function of a format
 * 
 * @param info Pointer to a plugin info struct to fill
 * @param objtypestr The name of the object type
 * @param formatstr The name of the format
 * @param access The pointer to your access function
 * 
 */
void osync_plugin_set_access_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, osync_bool (* access) (OSyncContext *, OSyncChange *))
{
	OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
	osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
	OSyncObjFormatTemplate *format_template = osync_plugin_find_objformat_template(template, formatstr);
	osync_assert(format_template, "Unable to set commit function. Did you forget to add the objformat?");
	format_template->access = access;
}

/*! @brief Sets the read function of a format
 * 
 * @param info Pointer to a plugin info struct to fill
 * @param objtypestr The name of the object type
 * @param formatstr The name of the format
 * @param read The pointer to your read function
 * 
 */
void osync_plugin_set_read_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, void (* read) (OSyncContext *, OSyncChange *))
{
	OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
	osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
	OSyncObjFormatTemplate *format_template = osync_plugin_find_objformat_template(template, formatstr);
	osync_assert(format_template, "Unable to set commit function. Did you forget to add the objformat?");
	format_template->read = read;
}

/*! @brief Sets the batch_commit function of a format
 * 
 * @param info Pointer to a plugin info struct to fill
 * @param objtypestr The name of the object type
 * @param formatstr The name of the format
 * @param batch The pointer to your batch_commit function
 * 
 */
void osync_plugin_set_batch_commit_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, void (* batch) (void *, OSyncContext **, OSyncChange **))
{
	OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
	osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
	OSyncObjFormatTemplate *format_template = osync_plugin_find_objformat_template(template, formatstr);
	osync_assert(format_template, "Unable to set batch commit function. Did you forget to add the objformat?");
	format_template->batch_commit = batch;
}

/*! @brief Sets the committed_all function of a format
 * 
 * @param info Pointer to a plugin info struct to fill
 * @param objtypestr The name of the object type
 * @param formatstr The name of the format
 * @param committed_all The pointer to your committed_all function
 * 
 */
void osync_plugin_set_committed_all_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, void (* committed_all) (void *))
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
