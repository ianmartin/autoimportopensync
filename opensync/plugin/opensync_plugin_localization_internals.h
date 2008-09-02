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

#ifndef _OPENSYNC_PLUGIN_LOCALIZATION_INTERNALS_H_
#define _OPENSYNC_PLUGIN_LOCALIZATION_INTERNALS_H_

/*! @brief Gives information about current localization settings 
 * 
 * @ingroup OSyncPluginLocalizationPrivateAPI 
 **/
struct OSyncPluginLocalization {
	/** Encoding of device/service/application (e.g. cp1925, ...) */
	char *encoding;
	/** Timezone of device/service/application (e.g. Europe/Berlin, ...) */
	char *timezone;
	/** Language of device/service/application (e.g. en_US, de_DE, ...) */
	char *language;

	/** Supported localization options */
	OSyncPluginLocalizationOptionSupportedFlags supported_options;

	/** Object reference counting */
	int ref_count;
};

#endif /*_OPENSYNC_PLUGIN_LOCALIZATION_INTERNALS_H_*/

