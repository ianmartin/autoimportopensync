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

typedef enum {
	OPENSYNC_PLUGIN_CONFIG_ADVANCEDOPTION	= (1 << 0),
	OPENSYNC_PLUGIN_CONFIG_AUTHENTICATION	= (1 << 1),
	OPENSYNC_PLUGIN_CONFIG_LOCALIZATION	= (1 << 2),
	OPENSYNC_PLUGIN_CONFIG_RESOURCES	= (1 << 3),
	OPENSYNC_PLUGIN_CONFIG_CONNECTION	= (1 << 4)
} OSyncPluginConfigSupportedFlag;

typedef unsigned int OSyncPluginConfigSupportedFlags;

OSYNC_EXPORT OSyncPluginConfig *osync_plugin_config_new(OSyncError **error);
OSYNC_EXPORT void osync_plugin_config_unref(OSyncPluginConfig *config);
OSYNC_EXPORT OSyncPluginConfig *osync_plugin_config_ref(OSyncPluginConfig *config);

OSYNC_EXPORT osync_bool osync_plugin_config_file_load(OSyncPluginConfig *config, const char *path, const char *schemadir, OSyncError **error);
OSYNC_EXPORT osync_bool osync_plugin_config_file_save(OSyncPluginConfig *config, const char *path, OSyncError **error);

OSYNC_EXPORT osync_bool osync_plugin_config_is_supported(OSyncPluginConfig *config, OSyncPluginConfigSupportedFlag flag);
OSYNC_EXPORT void osync_plugin_config_set_supported(OSyncPluginConfig *config, OSyncPluginConfigSupportedFlags flags);

/* Advanced Options */
OSYNC_EXPORT OSyncList *osync_plugin_config_get_advancedoptions(OSyncPluginConfig *config);
OSYNC_EXPORT OSyncPluginAdvancedOption *osync_plugin_config_get_advancedoption_value_by_name(OSyncPluginConfig *config, const char *name);
OSYNC_EXPORT void osync_plugin_config_add_advancedoption(OSyncPluginConfig *config, OSyncPluginAdvancedOption *option);
OSYNC_EXPORT void osync_plugin_config_remove_advancedoption(OSyncPluginConfig *config, OSyncPluginAdvancedOption *option);

/* Authentication */
OSYNC_EXPORT OSyncPluginAuthentication *osync_plugin_config_get_authentication(OSyncPluginConfig *config);
OSYNC_EXPORT void osync_plugin_config_set_authentication(OSyncPluginConfig *config, OSyncPluginAuthentication *authentication);

/* Localization */
OSYNC_EXPORT OSyncPluginLocalization *osync_plugin_config_get_localization(OSyncPluginConfig *config);
OSYNC_EXPORT void osync_plugin_config_set_localization(OSyncPluginConfig *config, OSyncPluginLocalization *localization);

/* Resources */
OSYNC_EXPORT OSyncList *osync_plugin_config_get_resources(OSyncPluginConfig *plugin);
OSYNC_EXPORT OSyncPluginResource *osync_plugin_config_find_active_resource(OSyncPluginConfig *config, const char *objtype);
OSYNC_EXPORT void osync_plugin_config_add_resource(OSyncPluginConfig *plugin, OSyncPluginResource *resource);
OSYNC_EXPORT void osync_plugin_config_remove_resource(OSyncPluginConfig *plugin, OSyncPluginResource *resource);

/* Connection */
OSYNC_EXPORT OSyncPluginConnection *osync_plugin_config_get_connection(OSyncPluginConfig *config);
OSYNC_EXPORT void osync_plugin_config_set_connection(OSyncPluginConfig *config, OSyncPluginConnection *connection);

#endif /*_OPENSYNC_PLUGIN_CONFIG_H_*/

