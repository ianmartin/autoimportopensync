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

#ifndef OPENSYNC_PLUGIN_INFO_H_
#define OPENSYNC_PLUGIN_INFO_H_

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
OSYNC_EXPORT OSyncPluginInfo *osync_plugin_info_new(OSyncError **error);

/*! @brief Increase the reference count on a plugin info object
 * 
 * @param info Pointer to the plugin info object
 * 
 */
OSYNC_EXPORT OSyncPluginInfo *osync_plugin_info_ref(OSyncPluginInfo *info);

/*! @brief Decrease the reference count on a plugin info object
 * 
 * @param info Pointer to the plugin info object
 * 
 */
OSYNC_EXPORT void osync_plugin_info_unref(OSyncPluginInfo *info);


/*! @brief Set reference to loop for the specific plugin 
 * 
 * @param info Pointer to the plugin info object
 * @param loop Pointer to the loop which get set for specified OSyncPluginInfo object
 * 
 */
OSYNC_EXPORT void osync_plugin_info_set_loop(OSyncPluginInfo *info, void *loop);

/*! @brief Get loop reference of OSyncPluginInfo object
 *
 * @param info Pointer to the plugin info object
 * @returns Reference to the loop of the OSyncPluginInfo object
 */
OSYNC_EXPORT void *osync_plugin_info_get_loop(OSyncPluginInfo *info);


/*! @brief Set the plugin configuration data
 *
 * @param info Pointer to the plugin info object
 * @param config Plugin configuration
 */
OSYNC_EXPORT void osync_plugin_info_set_config(OSyncPluginInfo *info, OSyncPluginConfig *config);

/*! @brief Returns the plugin configuration data
 * 
 * @param info Pointer to the plugin info object
 * @returns the plugin configuration data (null-terminated string)
 * 
 */
OSYNC_EXPORT OSyncPluginConfig *osync_plugin_info_get_config(OSyncPluginInfo *info);


/*! @brief Set plugin configuration directory 
 * 
 * @param info Pointer to the plugin info object
 * @param configdir Configuration directory to set
 * 
 */
OSYNC_EXPORT void osync_plugin_info_set_configdir(OSyncPluginInfo *info, const char *configdir);

/*! @brief Returns the plugin configuration directory
 * 
 * @param info Pointer to the plugin info object
 * @returns the full path where configuration files for the plugin are stored
 * 
 */
OSYNC_EXPORT const char *osync_plugin_info_get_configdir(OSyncPluginInfo *info);


/*! @brief Find ObjTypeSink of corresponding Object Type in OSyncPluginInfo object
 * 
 * @param info Pointer to the OSyncPluginInfo object
 * @param name Name of the Object Type
 * @returns Pointer to OSyncPluginInfo for searched objtype, NULL if not available 
 * 
 */
OSYNC_EXPORT OSyncObjTypeSink *osync_plugin_info_find_objtype(OSyncPluginInfo *info, const char *name);

/*! @brief Adds an object type (sink) to a plugin
 * 
 * @param info Pointer to the plugin info object
 * @param sink The sink to add
 * 
 */
OSYNC_EXPORT void osync_plugin_info_add_objtype(OSyncPluginInfo *info, OSyncObjTypeSink *sink);

/*! @brief Returns the number of added object types (sinks)
 * 
 * @param info Pointer to the plugin info object
 * @returns the number of object types in the plugin info
 * 
 */
OSYNC_EXPORT unsigned int osync_plugin_info_num_objtypes(OSyncPluginInfo *info);

/*! @brief Returns the nth added object type (sink)
 * 
 * @param info Pointer to the plugin info object
 * @param nth the index of the object type (sink) to return
 * @returns the object type (sink) at the specified index
 * 
 */
OSYNC_EXPORT OSyncObjTypeSink *osync_plugin_info_nth_objtype(OSyncPluginInfo *info, unsigned int nth);


/*! @brief Returns the Main Sink 
 * 
 * @param info Pointer to the plugin info object
 * @returns the Main Sink
 * 
 */
OSYNC_EXPORT OSyncObjTypeSink *osync_plugin_info_get_main_sink(OSyncPluginInfo *info);

/*! @brief Sets the Main Sink 
 * 
 * @param info Pointer to the plugin info object
 * @param sink The OSyncObjTypeSink which acts as Main Sink
 * 
 */
OSYNC_EXPORT void osync_plugin_info_set_main_sink(OSyncPluginInfo *info, OSyncObjTypeSink *sink);


/*! @brief Returns the plugin format conversion environment
 * 
 * @param info Pointer to the plugin info object
 * @returns the plugin format conversion environment
 * 
 */
OSYNC_EXPORT OSyncFormatEnv *osync_plugin_info_get_format_env(OSyncPluginInfo *info);

/*! @brief Set Format Environment for OSyncPluginInfo object
 *
 * @param info Pointer to the plugin info object
 * @param env Pointer to Format environment which gets assigned to the OSyncPluginInfo object
 *
 */
OSYNC_EXPORT void osync_plugin_info_set_format_env(OSyncPluginInfo *info, OSyncFormatEnv *env);


/*! @brief Returns the currently running sink
 * 
 * @param info Pointer to the plugin info object
 * @returns the current sink
 * 
 */
OSYNC_EXPORT OSyncObjTypeSink *osync_plugin_info_get_sink(OSyncPluginInfo *info);

/*! @brief Sets the current OSyncObjTypeSink 
 * 
 * @param info Pointer to the plugin info object
 * @param sink The OSyncObjTypeSink which should act as current OSyncObjTypeSink
 * 
 */
OSYNC_EXPORT void osync_plugin_info_set_sink(OSyncPluginInfo *info, OSyncObjTypeSink *sink);


/*! @brief Set Group Name for plugin info object 
 * 
 * @param info Pointer to the plugin info object
 * @param groupname Group name 
 * 
 */
OSYNC_EXPORT void osync_plugin_info_set_groupname(OSyncPluginInfo *info, const char *groupname);

/*! @brief Get Group Name of the OSyncPluginInfo object
 * 
 * @param info Pointer to the OSyncPluginInfo object
 * @returns Group Name of the OSyncPluginInfo object
 * 
 */
OSYNC_EXPORT const char *osync_plugin_info_get_groupname(OSyncPluginInfo *info);


/*! @brief Set OSyncVersion for OSyncPluginInfo object
 *
 * @param info Pointer to the plugin info object
 * @param version Pointer to OSyncVersion
 */
OSYNC_EXPORT void osync_plugin_info_set_version(OSyncPluginInfo *info, OSyncVersion *version);

/*! @brief Get OSyncVersion of the OSyncPluginInfo object
 *
 * @param info Pointer to the plugin info object
 * @returns Pointer of the OSyncVersion from OSyncPluginInfo object
 */
OSYNC_EXPORT OSyncVersion *osync_plugin_info_get_version(OSyncPluginInfo *info);


/*! @brief Set OSyncCapabilities of the OSyncPluginInfo object
 *
 * @param info Pointer to the plugin info object
 * @param capabilities Pointer to the capabilities
 */
OSYNC_EXPORT void osync_plugin_info_set_capabilities(OSyncPluginInfo *info, OSyncCapabilities *capabilities);

/*! @brief Get OSyncCapabilities of the OSyncPluginInfo object
 *
 * @param info Pointer to the plugin info object
 * @returns OSyncCapabilities of the OSyncPluginInfo object
 */
OSYNC_EXPORT OSyncCapabilities *osync_plugin_info_get_capabilities(OSyncPluginInfo *info);

/*@}*/

#endif /*OPENSYNC_PLUGIN_INFO_H_*/
