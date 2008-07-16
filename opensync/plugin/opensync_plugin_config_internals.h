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

#ifndef _OPENSYNC_PLUGIN_CONFIG_INTERNALS_H_
#define _OPENSYNC_PLUGIN_CONFIG_INTERNALS_H_

#define OSYNC_PLUGIN_CONFING_SCHEMA "plugin_config.xsd"

/*! @brief Gives information about the plugin configuration 
 * 
 * @ingroup OSyncPluginConfigPrivateAPI 
 **/
struct OSyncPluginConfig {
	/** Advanced Options */
	OSyncList *advancedoptions;
	/** Connection configuration */
	OSyncPluginConnection *connection;
	/** Authentication configuration */
	OSyncPluginAuthentication *authentication;
	/** Localization configuration */
	OSyncPluginLocalization *localization;
	/** List of ressource configurations */
	OSyncList *ressources;

	/** Object reference counting */
	int ref_count;
};

#endif /*_OPENSYNC_PLUGIN_CONFIG_INTERNALS_H_*/

