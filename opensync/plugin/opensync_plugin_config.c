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

#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-plugin.h"
#include "opensync_plugin_config_internals.h"

OSyncPluginConfig *osync_plugin_config_new(OSyncError **error)
{
	OSyncPluginConfig *config = osync_try_malloc0(sizeof(OSyncPluginConfig), error);
	if (!config)
		return NULL;

	config->ref_count = 1;

	return config;
}

void osync_plugin_config_unref(OSyncPluginConfig *config)
{
	osync_assert(config);

	if (g_atomic_int_dec_and_test(&(config->ref_count))) {
		if (config->connection)
			osync_plugin_connection_unref(config->connection);

		if (config->localization)
			osync_plugin_localization_unref(config->localization);
			
		if (config->authentication)
			osync_plugin_authentication_unref(config->authentication);
			
		g_free(config);
	}
}

OSyncPluginConfig *osync_plugin_config_ref(OSyncPluginConfig *config)
{
	osync_assert(config);
	
	g_atomic_int_inc(&(config->ref_count));

	return config;
}

OSyncPluginAuthentication *osync_plugin_config_get_authentication(OSyncPluginConfig *config)
{
	osync_assert(config);
	return config->authentication;
}

void osync_plugin_config_set_authentication(OSyncPluginConfig *config, OSyncPluginAuthentication *authentication)
{
	osync_assert(config);
	if (config->authentication)
		osync_plugin_authentication_unref(config->authentication);

	config->authentication = osync_plugin_authentication_ref(authentication);
}

OSyncPluginLocalization *osync_plugin_config_get_localization(OSyncPluginConfig *config)
{
	osync_assert(config);
	return config->localization;
}

void osync_plugin_config_set_localization(OSyncPluginConfig *config, OSyncPluginLocalization *localization)
{
	osync_assert(config);
	if (config->localization)
		osync_plugin_localization_unref(config->localization);

	config->localization = osync_plugin_localization_ref(localization);
}

/* Ressources */
/* TODO: Implement OSyncRessource
OSyncRessource *osync_plugin_config_get_ressource(OSyncPluginConfig *plugin)
{
	osync_assert(config);

}

void osync_plugin_config_set_ressource(OSyncPluginConfig *plugin, OSyncRessource *ressource)
{
	osync_assert(config);

}
*/

OSyncPluginConnection *osync_plugin_config_get_connection(OSyncPluginConfig *config)
{
	osync_assert(config);
	return config->connection;
}

void osync_plugin_config_set_connection(OSyncPluginConfig *config, OSyncPluginConnection *connection)
{
	osync_assert(config);
	if (config->connection)
		osync_plugin_connection_unref(config->connection);

	config->connection = osync_plugin_connection_ref(connection);
}

