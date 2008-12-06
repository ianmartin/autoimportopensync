/*
 * libopensync - A synchronization framework
 * Copyright (C) 2008  Daniel Gollub <dgollub@suse.de>
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

#ifndef _OPENSYNC_PLUGIN_CONFIG_H_
#define _OPENSYNC_PLUGIN_CONFIG_H_

#include <opensync/opensync_list.h>
/** \todo Review open OSyncPluginConfig documentation */

/**
 * @defgroup OSyncPluginConfigAPI OpenSync Plugin Config
 * @ingroup OSyncPublic
 * @brief Functions to get and set a plugins configuration
 *
 */

/*@{*/

typedef enum {
	OPENSYNC_PLUGIN_CONFIG_ADVANCEDOPTION	= (1 << 0),
	OPENSYNC_PLUGIN_CONFIG_AUTHENTICATION	= (1 << 1),
	OPENSYNC_PLUGIN_CONFIG_LOCALIZATION	= (1 << 2),
	OPENSYNC_PLUGIN_CONFIG_RESOURCES	= (1 << 3),
	OPENSYNC_PLUGIN_CONFIG_CONNECTION	= (1 << 4)
} OSyncPluginConfigSupportedFlag;

typedef unsigned int OSyncPluginConfigSupportedFlags;

/*! @brief Create a new plugin config object
 *
 * @param error Pointer to and error struct
 * @returns the newly registered plugin config object
 */
OSYNC_EXPORT OSyncPluginConfig *osync_plugin_config_new(OSyncError **error);

/*! @brief Decrease the reference count on a plugin config object
 *
 * @param Pointer to the plugin config object
 */
OSYNC_EXPORT void osync_plugin_config_unref(OSyncPluginConfig *config);

/*! @brief Increase the reference count on a plugin config object
 *
 * @param Pointer to the plugin config object
 * @returns the passed plugin config
 */
OSYNC_EXPORT OSyncPluginConfig *osync_plugin_config_ref(OSyncPluginConfig *config);


/*! @brief Load a plugin config file
 *
 * @param config Contents of config file as OsyncPluginConfig object
 * @param path Path to file to load
 * @param schemadir /todo
 * @param error Pointer to error struct
 * @returns True on success, False on failure - error will contain details
 */
OSYNC_EXPORT osync_bool osync_plugin_config_file_load(OSyncPluginConfig *config, const char *path, const char *schemadir, OSyncError **error);

/*! @brief Save a plugin config to a file
 *
 * @param config Plugin config to save
 * @param path Path to save file
 * @param error Pointer to error struct that will contain details in case of failure
 * @returns True on succes, False on failure
 */
OSYNC_EXPORT osync_bool osync_plugin_config_file_save(OSyncPluginConfig *config, const char *path, OSyncError **error);


/*! @brief
 *
 * @param config Plugin config to check
 * @param flag
 * @returns True if flag is support, False if not
 */
OSYNC_EXPORT osync_bool osync_plugin_config_is_supported(OSyncPluginConfig *config, OSyncPluginConfigSupportedFlag flag);

/*! @brief Sets the passed flags as support
 *
 * @param config An OsyncPluginConfig to modify
 * @param flags An OSyncPluginConfigSupportedFlags to set the flags to (merge or overwrite??)
 */
OSYNC_EXPORT void osync_plugin_config_set_supported(OSyncPluginConfig *config, OSyncPluginConfigSupportedFlags flags);

/* Advanced Options */
/*! @brief Get the advanced options from a config
 *
 * @param An OSyncPluginConfig to get the advanced options from
 * @returns An OSyncList of OSyncPluginAdvancedOption s
 */
OSYNC_EXPORT OSyncList *osync_plugin_config_get_advancedoptions(OSyncPluginConfig *config);

/*!@brief Get an OSyncPluginAdvancedOption from the config by name
 *
 * @param config An OSyncPluginConfig to search
 * @param name The name of the advanced option to get
 * @returns The first OSyncPluginAdvancedOption with the given name or NULL if not found
 */
OSYNC_EXPORT OSyncPluginAdvancedOption *osync_plugin_config_get_advancedoption_value_by_name(OSyncPluginConfig *config, const char *name);

/*!@brief Add an OSyncPluginAdvancedOption to a config
 *
 * @param config The config to add the option to
 */
OSYNC_EXPORT void osync_plugin_config_add_advancedoption(OSyncPluginConfig *config, OSyncPluginAdvancedOption *option);

/*!@brief Remove and advanced option from a config
 *
 * @param config An OSyncPluginConfig to remove from
 * @param option The OSyncPluginAdvancedOption to be removed
 */
OSYNC_EXPORT void osync_plugin_config_remove_advancedoption(OSyncPluginConfig *config, OSyncPluginAdvancedOption *option);

/* Authentication */
/*!@brief Get the authentication settings from a config
 *
 * @param config An OSyncPluginConfig
 * @returns an OSyncPluginAuthentication with the details ?? or NULL if no authentication configured
 */
OSYNC_EXPORT OSyncPluginAuthentication *osync_plugin_config_get_authentication(OSyncPluginConfig *config);

/*!@brief Set the authentication configuration
 *
 * @param config An OSyncPluginConfig
 * @param option The new authentication settings as an OSyncPluginAuthentication
 */
OSYNC_EXPORT void osync_plugin_config_set_authentication(OSyncPluginConfig *config, OSyncPluginAuthentication *authentication);

/* Localization */
/*!@brief Get the localization settings
 *
 * @param config An OSyncPluginConfig
 * @returns the localization settings as an OSyncPluginLocalization
 */
OSYNC_EXPORT OSyncPluginLocalization *osync_plugin_config_get_localization(OSyncPluginConfig *config);

/*!@brief Set the localization settings
 *
 * @param config An OSyncPluginConfig
 * @param localization An OSyncPluginLocalization containing the new localization settings
 */
OSYNC_EXPORT void osync_plugin_config_set_localization(OSyncPluginConfig *config, OSyncPluginLocalization *localization);

/* Resources */
/*!@brief Get the configured resources
 *
 * @param config An OSyncPluginConfig
 * @returns a list of OSyncPluginResource
 */
OSYNC_EXPORT OSyncList *osync_plugin_config_get_resources(OSyncPluginConfig *config);

/*!@brief Find the active resource by object type
 *
 * @param config An OSyncPluginConfig
 * @param objtype the object type to find the active resource for
 * @returns the active resource or NULL if none was found
 */
OSYNC_EXPORT OSyncPluginResource *osync_plugin_config_find_active_resource(OSyncPluginConfig *config, const char *objtype);

/*!@brief Add a resource to a config
 *
 * @param config An OSyncPluginConfig to add to
 * @param resource An OSyncPluginResource to add to the config
 */
OSYNC_EXPORT void osync_plugin_config_add_resource(OSyncPluginConfig *config, OSyncPluginResource *resource);

/*!@brief Remove a resource from a config
 *
 * @param config An OSyncPluginConfig to remove from
 * @param resource The OSyncPluginResource to remove
 */
OSYNC_EXPORT void osync_plugin_config_remove_resource(OSyncPluginConfig *config, OSyncPluginResource *resource);

/*!@brief ?
 *
 * @param config ?
 */
OSYNC_EXPORT void osync_plugin_config_flush_resources(OSyncPluginConfig *config);

/* Connection */
/*!@brief Get the connection settings from a config
 *
 * @param config An OSyncPluginConfig
 * @returns the connection settings as an OSyncPluginConnection
 */
OSYNC_EXPORT OSyncPluginConnection *osync_plugin_config_get_connection(OSyncPluginConfig *config);

/*!@brief Set the connection settings in a config
 *
 * @param config An OSyncPluginConfig
 * @param connection the connection settings as an OSyncPluginConnection
 */
OSYNC_EXPORT void osync_plugin_config_set_connection(OSyncPluginConfig *config, OSyncPluginConnection *connection);

/*@}*/

#endif /*_OPENSYNC_PLUGIN_CONFIG_H_*/

