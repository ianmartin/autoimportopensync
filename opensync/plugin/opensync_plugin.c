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

#include "opensync-plugin.h"
#include "opensync_plugin_internals.h"

/**
 * @defgroup OSyncPluginAPI OpenSync Plugin
 * @ingroup OSyncPublic
 * @brief Functions to register and manage plugins
 * 
 */
/*@{*/


/*! @brief Registers a new plugin
 *
 * This function creates a new OSyncPlugin object, that
 * can be used to register a new plugin dynamically. This
 * can be used by a module to register multiple plugins,
 * instead of using the get_info() function which allows
 * registering only one plugin.
 *
 * @param error Pointer to an error struct
 * @returns the newly registered plugin
 */
OSyncPlugin *osync_plugin_new(OSyncError **error)
{
	OSyncPlugin *plugin = osync_try_malloc0(sizeof(OSyncPlugin), error);
	if (!plugin)
		return NULL;
	
	plugin->config_type = OSYNC_PLUGIN_NEEDS_CONFIGURATION;
	plugin->start_type = OSYNC_START_TYPE_THREAD;
	plugin->ref_count = 1;
	
	return plugin;
}

/*! @brief Increase the reference count on a plugin
 * 
 * @param plugin Pointer to the plugin
 * 
 */
OSyncPlugin *osync_plugin_ref(OSyncPlugin *plugin)
{
	osync_assert(plugin);
	
	g_atomic_int_inc(&(plugin->ref_count));

	return plugin;
}

/*! @brief Decrease the reference count on a plugin
 * 
 * @param plugin Pointer to the plugin
 * 
 */
void osync_plugin_unref(OSyncPlugin *plugin)
{
	osync_assert(plugin);
	
	if (g_atomic_int_dec_and_test(&(plugin->ref_count))) {
		if (plugin->name)
			g_free(plugin->name);
			
		if (plugin->longname)
			g_free(plugin->longname);
			
		if (plugin->description)
			g_free(plugin->description);
			
		g_free(plugin);
	}
}

/*! @brief Returns the name of a plugin
 * 
 * @param plugin Pointer to the plugin
 * @returns Name of the plugin
 * 
 */
const char *osync_plugin_get_name(OSyncPlugin *plugin)
{
	osync_assert(plugin);
	return plugin->name;
}

/*! @brief Sets the name of a plugin
 * 
 * Sets the name of a plugin. This is a short name (maybe < 15 chars).
 *
 * @param plugin Pointer to the plugin
 * @param name the name to set
 * 
 */
void osync_plugin_set_name(OSyncPlugin *plugin, const char *name)
{
	osync_assert(plugin);
	if (plugin->name)
		g_free(plugin->name);
	plugin->name = g_strdup(name);
}

/*! @brief Returns the long name of a plugin
 * 
 * @param plugin Pointer to the plugin
 * @returns Long name of the plugin
 * 
 */
const char *osync_plugin_get_longname(OSyncPlugin *plugin)
{
	osync_assert(plugin);
	return plugin->longname;
}

/*! @brief Sets the long name of a plugin
 * 
 * Sets the long name of a plugin (maybe < 50 chars).
 *
 * @param plugin Pointer to the plugin
 * @param longname the long name to set
 * 
 */
void osync_plugin_set_longname(OSyncPlugin *plugin, const char *longname)
{
	osync_assert(plugin);
	if (plugin->longname)
		g_free(plugin->longname);
	plugin->longname = g_strdup(longname);
}

/*! @brief Returns the description of a plugin
 * 
 * @param plugin Pointer to the plugin
 * @returns Description of the plugin
 * 
 */
const char *osync_plugin_get_description(OSyncPlugin *plugin)
{
	osync_assert(plugin);
	return plugin->description;
}

/*! @brief Sets the description of a plugin
 * 
 * Sets a longer description for the plugin (maybe < 200 chars).
 *
 * @param plugin Pointer to the plugin
 * @param description the description to set
 * 
 */
void osync_plugin_set_description(OSyncPlugin *plugin, const char *description)
{
	osync_assert(plugin);
	if (plugin->description)
		g_free(plugin->description);
	plugin->description = g_strdup(description);
}

/*! @brief Returns the plugin_info data, set by the plugin
 *
 * @param plugin Pointer to the plugin
 * @returns The void pointer set on plugin->info.plugin_data
 */
void *osync_plugin_get_data(OSyncPlugin *plugin)
{
	osync_assert(plugin);
	return plugin->plugin_data;
}

void osync_plugin_set_data(OSyncPlugin *plugin, void *data)
{
	osync_assert(plugin);
	plugin->plugin_data = data;
}

/*! @brief Returns whether or not the plugin requires configuration
 *
 * @param plugin Pointer to the plugin
 * @returns The configuration requirement type of the plugin
 */
OSyncConfigurationType osync_plugin_get_config_type(OSyncPlugin *plugin)
{
	osync_assert(plugin);
	return plugin->config_type;
}

/*! @brief Sets whether or not the plugin requires configuration
 *
 * @param plugin Pointer to the plugin
 * @param config_type The configuration requirement type of the plugin
 */
void osync_plugin_set_config_type(OSyncPlugin *plugin, OSyncConfigurationType config_type)
{
	osync_assert(plugin);
	plugin->config_type = config_type;
}

OSyncStartType osync_plugin_get_start_type(OSyncPlugin *plugin)
{
	osync_assert(plugin);
	return plugin->start_type;
}

void osync_plugin_set_start_type(OSyncPlugin *plugin, OSyncStartType start_type)
{
	osync_assert(plugin);
	plugin->start_type = start_type;
}

/*! @brief Sets the initialize function for a plugin
 *
 * The initialize function of a plugin sets up sinks for the plugin as well
 * as other plugin-wide structures.
 *
 * @param plugin Pointer to the plugin
 * @param init The initialize function for the plugin
 */
void osync_plugin_set_initialize(OSyncPlugin *plugin, initialize_fn init)
{
	osync_assert(plugin);
	plugin->initialize = init;
}

/*! @brief Sets the finalize function for a plugin
 *
 * The finalize function of a plugin frees any plugin-wide structures
 * that were created in the initialize function.
 *
 * @param plugin Pointer to the plugin
 * @param fin The finalize function for the plugin
 */
void osync_plugin_set_finalize(OSyncPlugin *plugin, finalize_fn fin)
{
	osync_assert(plugin);
	
	plugin->finalize = fin;
}

/*! @brief Sets the optional discover function for a plugin
 *
 * The discover function of a plugin can be used to specify which 
 * of the sinks in the plugin are currently available, and to declare
 * the compatible device versions for the plugin. It can also
 * be used to set the plugin's capabilities.
 *
 * The discover function is optional.
 *
 * @param plugin Pointer to the plugin
 * @param discover The discover function for the plugin
 */
void osync_plugin_set_discover(OSyncPlugin *plugin, discover_fn discover)
{
	osync_assert(plugin);
	plugin->discover = discover;
}

void *osync_plugin_initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_assert(plugin);
	return plugin->initialize(plugin, info, error);
}

void osync_plugin_finalize(OSyncPlugin *plugin, void *data)
{
	osync_assert(plugin);
	plugin->finalize(data);
}

osync_bool osync_plugin_discover(OSyncPlugin *plugin, void *data, OSyncPluginInfo *info, OSyncError **error)
{
	osync_assert(plugin);
	if (!plugin->discover)
		return TRUE;
		
	return plugin->discover(data, info, error);
}

/*! @brief Checks if a plugin is available and usable
 * 
 * @param plugin The plugin to check
 * @param error If the return was FALSE, will contain information on why the plugin is not available
 * @returns TRUE if the plugin was found and is usable, FALSE otherwise
 * 
 */
osync_bool osync_plugin_is_usable(OSyncPlugin *plugin, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, plugin, error);
	
	if (plugin->useable && !plugin->useable(error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/*@}*/
