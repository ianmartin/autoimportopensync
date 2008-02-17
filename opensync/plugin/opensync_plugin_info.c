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

#include "opensync-format.h"
#include "opensync-plugin.h"
#include "opensync_plugin_info_internals.h"

#include "opensync-merger.h"
#include "opensync-version.h"

/**
 * @defgroup OSyncPluginInfoAPI OpenSync Plugin Info
 * @ingroup OSyncPublic
 * @brief Functions to get and set information about a plugin
 * 
 */
/*@{*/


/*! @brief Create a new plugin info object
 *
 * @param error Pointer to an error struct
 * @returns the newly registered plugin info object
 */
OSyncPluginInfo *osync_plugin_info_new(OSyncError **error)
{
	OSyncPluginInfo *info = osync_try_malloc0(sizeof(OSyncPluginInfo), error);
	if (!info)
		return NULL;
	
	info->ref_count = 1;
	
	return info;
}

/*! @brief Increase the reference count on a plugin info object
 * 
 * @param info Pointer to the plugin info object
 * 
 */
OSyncPluginInfo *osync_plugin_info_ref(OSyncPluginInfo *info)
{
	osync_assert(info);
	
	g_atomic_int_inc(&(info->ref_count));

	return info;
}

/*! @brief Decrease the reference count on a plugin info object
 * 
 * @param info Pointer to the plugin info object
 * 
 */
void osync_plugin_info_unref(OSyncPluginInfo *info)
{
	osync_assert(info);
	
	if (g_atomic_int_dec_and_test(&(info->ref_count))) {
		if (info->config)
			g_free(info->config);
		
		if (info->configdir)
			g_free(info->configdir);
		
		if (info->groupname)
			g_free(info->groupname);
		
		while (info->objtypes) {
			OSyncObjTypeSink *sink = info->objtypes->data;
			osync_objtype_sink_unref(sink);
			info->objtypes = g_list_remove(info->objtypes, sink);
		}
		
		if (info->main_sink)
			osync_objtype_sink_unref(info->main_sink);
			
		if (info->version)
			osync_version_unref(info->version);
			
		if (info->capabilities)
			osync_capabilities_unref(info->capabilities);
		
		g_free(info);
	}
}

/*! @brief Set reference to loop for the specific plugin 
 * 
 * @param info Pointer to the plugin info object
 * @param loop Pointer to the loop which get set for specified OSyncPluginInfo object
 * 
 */
void osync_plugin_info_set_loop(OSyncPluginInfo *info, void *loop)
{
	osync_assert(info);
	info->loop = loop;
}

/*! @brief Get loop reference of OSyncPluginInfo object
 *
 * @param info Pointer to the plugin info object
 * @returns Reference to the loop of the OSyncPluginInfo object
 */
void *osync_plugin_info_get_loop(OSyncPluginInfo *info)
{
	osync_assert(info);
	return info->loop;
}

/*! @brief Set  the plugin configuration data
 *
 * @param info Pointer to the plugin info object
 * @param config Plugin configuration data
 */
void osync_plugin_info_set_config(OSyncPluginInfo *info, const char *config)
{
	osync_assert(info);
	if (info->config)
		g_free(info->config);
	info->config = g_strdup(config);
}

/*! @brief Returns the plugin configuration data
 * 
 * @param info Pointer to the plugin info object
 * @returns the plugin configuration data (null-terminated string)
 * 
 */
const char *osync_plugin_info_get_config(OSyncPluginInfo *info)
{
	osync_assert(info);
	return info->config;
}

/*! @brief Set plugin configuration directory 
 * 
 * @param info Pointer to the plugin info object
 * @param configdir Configuration directory to set
 * 
 */
void osync_plugin_info_set_configdir(OSyncPluginInfo *info, const char *configdir)
{
	osync_assert(info);
	if (info->configdir)
		g_free(info->configdir);
	info->configdir = g_strdup(configdir);
}

/*! @brief Returns the plugin configuration directory
 * 
 * @param info Pointer to the plugin info object
 * @returns the full path where configuration files for the plugin are stored
 * 
 */
const char *osync_plugin_info_get_configdir(OSyncPluginInfo *info)
{
	osync_assert(info);
	return info->configdir;
}

/*! @brief Set Group Name for plugin info object 
 * 
 * @param info Pointer to the plugin info object
 * @param groupname Group name 
 * 
 */
void osync_plugin_info_set_groupname(OSyncPluginInfo *info, const char *groupname)
{
	osync_assert(info);
	if (info->groupname)
		g_free(info->groupname);
	info->groupname = g_strdup(groupname);
}

/*! @brief Get Group Name of the OSyncPluginInfo object
 * 
 * @param info Pointer to the OSyncPluginInfo object
 * @returns Group Name of the OSyncPluginInfo object
 * 
 */
const char *osync_plugin_info_get_groupname(OSyncPluginInfo *info)
{
	osync_assert(info);
	return info->groupname;
}

/*! @brief Find ObjTypeSink of corresponding Object Type in OSyncPluginInfo object
 * 
 * @param info Pointer to the OSyncPluginInfo object
 * @param name Name of the Object Type
 * @returns Pointer to OSyncPluginInfo for searched objtype, NULL if not available 
 * 
 */
OSyncObjTypeSink *osync_plugin_info_find_objtype(OSyncPluginInfo *info, const char *name)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, info, name);
	GList *p;
	osync_assert(info);

	OSyncObjTypeSink *sink = NULL;
	for (p = info->objtypes; p; p = p->next) {
		sink = p->data;
		if (g_ascii_strcasecmp(osync_objtype_sink_get_name(sink), name) == 0)
			goto done;
	}
	
	/* If we couldnt find the requested objtype, look if we find a sink
	 * which accepts any objtype ("data") */
	for (p = info->objtypes; p; p = p->next) {
		sink = p->data;
		if (g_ascii_strcasecmp(osync_objtype_sink_get_name(sink), "data") == 0)
			goto done;
	}
	
	osync_trace(TRACE_EXIT, "%s: NULL", __func__);
	return NULL;

done:
	osync_trace(TRACE_EXIT, "%s: %p", __func__, sink);
	return sink;

}

/*! @brief Adds an object type (sink) to a plugin
 * 
 * @param info Pointer to the plugin info object
 * @param sink The sink to add
 * 
 */
void osync_plugin_info_add_objtype(OSyncPluginInfo *info, OSyncObjTypeSink *sink)
{
	osync_assert(info);
	info->objtypes = g_list_append(info->objtypes, sink);
	osync_objtype_sink_ref(sink);
}

/*! @brief Returns the number of added object types (sinks)
 * 
 * @param info Pointer to the plugin info object
 * @returns the number of object types in the plugin info
 * 
 */
int osync_plugin_info_num_objtypes(OSyncPluginInfo *info)
{
	osync_assert(info);
	return g_list_length(info->objtypes);
}

/*! @brief Returns the nth added object type (sink)
 * 
 * @param info Pointer to the plugin info object
 * @param nth the index of the object type (sink) to return
 * @returns the object type (sink) at the specified index
 * 
 */
OSyncObjTypeSink *osync_plugin_info_nth_objtype(OSyncPluginInfo *info, int nth)
{
	osync_assert(info);
	return g_list_nth_data(info->objtypes, nth);
}

/*! @brief Returns the Main Sink 
 * 
 * @param info Pointer to the plugin info object
 * @returns the Main Sink
 * 
 */
OSyncObjTypeSink *osync_plugin_info_get_main_sink(OSyncPluginInfo *info)
{
	osync_assert(info);
	return info->main_sink;
}


/*! @brief Sets the Main Sink 
 * 
 * @param info Pointer to the plugin info object
 * @param sink The OSyncObjTypeSink which acts as Main Sink
 * 
 */
void osync_plugin_info_set_main_sink(OSyncPluginInfo *info, OSyncObjTypeSink *sink)
{
	osync_assert(info);
	osync_assert(sink);
	info->main_sink = sink;
	osync_objtype_sink_ref(sink);
}

/*! @brief Returns the currently running sink
 * 
 * @param info Pointer to the plugin info object
 * @returns the current sink
 * 
 */
OSyncObjTypeSink *osync_plugin_info_get_sink(OSyncPluginInfo *info)
{
	osync_assert(info);
	return info->current_sink;
}

/*! @brief Sets the current OSyncObjTypeSink 
 * 
 * @param info Pointer to the plugin info object
 * @param sink The OSyncObjTypeSink which should act as current OSyncObjTypeSink
 * 
 */
void osync_plugin_info_set_sink(OSyncPluginInfo *info, OSyncObjTypeSink *sink)
{
	osync_assert(info);
	osync_assert(sink);
	info->current_sink = sink;
}

/*! @brief Returns the plugin format conversion environment
 * 
 * @param info Pointer to the plugin info object
 * @returns the plugin format conversion environment
 * 
 */
OSyncFormatEnv *osync_plugin_info_get_format_env(OSyncPluginInfo *info)
{
	osync_assert(info);
	return info->formatenv;
}

/*! @brief Set Format Environment for OSyncPluginInfo object
 *
 * @param info Pointer to the plugin info object
 * @param env Pointer to Format environment which gets assigned to the OSyncPluginInfo object
 *
 */
void osync_plugin_info_set_format_env(OSyncPluginInfo *info, OSyncFormatEnv *env)
{
	osync_assert(info);
	osync_assert(env);
	info->formatenv = env;
}


/*! @brief Set OSyncVersion for OSyncPluginInfo object
 *
 * @param info Pointer to the plugin info object
 * @param version Pointer to OSyncVersion
 */
void osync_plugin_info_set_version(OSyncPluginInfo *info, OSyncVersion *version)
{
	osync_assert(info);
	osync_assert(version);
	
	if(info->version)
		osync_version_unref(info->version);
	
	osync_version_ref(version);
	info->version = version;
}

/*! @brief Get OSyncVersion of the OSyncPluginInfo object
 *
 * @param info Pointer to the plugin info object
 * @returns Pointer of the OSyncVersion from OSyncPluginInfo object
 */
OSyncVersion *osync_plugin_info_get_version(OSyncPluginInfo *info)
{
	osync_assert(info);
	return info->version;
}

/*! @brief Set OSyncCapabilities of the OSyncPluginInfo object
 *
 * @param info Pointer to the plugin info object
 * @param capabilities Pointer to the capabilities
 */
void osync_plugin_info_set_capabilities(OSyncPluginInfo *info, OSyncCapabilities *capabilities)
{
	osync_assert(info);
	osync_assert(capabilities);
	
	if(info->capabilities)
		osync_capabilities_unref(info->capabilities);
	
	osync_capabilities_ref(capabilities);
	info->capabilities = capabilities;
}

/*! @brief Get OSyncCapabilities of the OSyncPluginInfo object
 *
 * @param info Pointer to the plugin info object
 * @returns OSyncCapabilities of the OSyncPluginInfo object
 */
OSyncCapabilities *osync_plugin_info_get_capabilities(OSyncPluginInfo *info)
{
	osync_assert(info);
	return info->capabilities;
}

/*@}*/
