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
 * @defgroup OSyncPluginPrivateAPI OpenSync Plugin
 * @ingroup OSyncPrivate
 * @brief The private API of opensync
 * 
 * This gives you an insight in the private API of opensync.
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
 * @param plugin 
 * @param path Where to find this plugin
 * @param error Pointer to a error struct
 * @return Pointer to the plugin on success, NULL otherwise
 * 
 */
OSyncPlugin *osync_plugin_load(OSyncEnv *env, const char *path, OSyncError **error)
{
	/* Check if this platform supports dynamic
	 * loading of modules */
	if (!g_module_supported()) {
		osync_debug("OSPLG", 0, "This platform does not support loading of modules");
		osync_error_set(error, OSYNC_ERROR_GENERIC, "This platform does not support loading of modules");
		return NULL;
	}

	/* Try to open the module or fail if an error occurs */
	OSyncPlugin *plugin = osync_plugin_new(env);
	plugin->real_plugin = g_module_open(path, G_MODULE_BIND_LOCAL);
	if (!plugin->real_plugin) {
		osync_debug("OSPLG", 0, "Unable to open plugin %s", path);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open plugin %s: %s", path, g_module_error());
		osync_plugin_free(plugin);
		return NULL;
	}
	
	void (* fct_info)(OSyncPluginInfo *info);
	if (!(fct_info = osync_plugin_get_function(plugin, "get_info", error))) {
		osync_debug("OSPLG", 0, "Unable to open plugin: Missing symbol get_info");
		osync_plugin_unload(plugin);
		osync_plugin_free(plugin);
		return NULL;
	}
	
	fct_info(&(plugin->info));
	plugin->path = g_strdup(path);
	
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
	
	g_module_close(plugin->real_plugin);
	g_free(plugin->path);
	plugin->path = NULL;
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

void osync_plugin_set_commit_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, osync_bool (* commit_change) (OSyncContext *, OSyncChange *))
{
	OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
	osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
	OSyncObjFormatTemplate *format_template = osync_plugin_find_objformat_template(template, formatstr);
	osync_assert(format_template, "Unable to set commit function. Did you forget to add the objformat?");
	format_template->commit_change = commit_change;
}

void osync_plugin_set_access_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, osync_bool (* access) (OSyncContext *, OSyncChange *))
{
	OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
	osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
	OSyncObjFormatTemplate *format_template = osync_plugin_find_objformat_template(template, formatstr);
	osync_assert(format_template, "Unable to set commit function. Did you forget to add the objformat?");
	format_template->access = access;
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

/*@}*/

/**
 * @defgroup OSyncPluginAPI OpenSync Plugin
 * @ingroup OSyncPublic
 * @brief The public API of opensync
 * 
 * This gives you an insight in the public API of opensync.
 * 
 */
/*@{*/

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
 * 
 */
void osync_plugin_accept_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr)
{
	OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
	osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
	OSyncObjFormatTemplate *format_template = g_malloc0(sizeof(OSyncObjFormatTemplate));
	format_template->name = g_strdup(formatstr);
	template->formats = g_list_append(template->formats, format_template);
}

/*@}*/
