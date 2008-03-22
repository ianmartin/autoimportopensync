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

OSYNC_EXPORT OSyncPluginConfig *osync_plugin_config_new(OSyncError **error);
OSYNC_EXPORT void osync_plugin_config_unref(OSyncPluginConfig *config);
OSYNC_EXPORT OSyncPluginConfig *osync_plugin_config_ref(OSyncPluginConfig *config);

/* Authentication */
OSYNC_EXPORT OSyncPluginAuthentication *osync_plugin_config_get_authentication(OSyncPluginConfig *config);
OSYNC_EXPORT void osync_plugin_config_set_authentication(OSyncPluginConfig *config, OSyncPluginAuthentication *authentication);

/* Localization */
OSYNC_EXPORT OSyncPluginLocalization *osync_plugin_config_get_localization(OSyncPluginConfig *config);
OSYNC_EXPORT void osync_plugin_config_set_localization(OSyncPluginConfig *config, OSyncPluginLocalization *localization);

/* Ressources */
/* TODO: Implement OSyncRessource
OSYNC_EXPORT OSyncRessource *osync_plugin_config_get_ressource(OSyncPluginConfig *plugin);
OSYNC_EXPORT void osync_plugin_config_set_ressource(OSyncPluginConfig *plugin, OSyncRessource *ressource);
*/

/* Connection */
OSYNC_EXPORT OSyncPluginConnection *osync_plugin_config_get_connection(OSyncPluginConfig *config);
OSYNC_EXPORT void osync_plugin_config_set_connection(OSyncPluginConfig *config, OSyncPluginConnection *connection);

#endif /*_OPENSYNC_PLUGIN_CONFIG_H_*/

