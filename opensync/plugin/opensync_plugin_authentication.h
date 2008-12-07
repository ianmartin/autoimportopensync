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

#ifndef _OPENSYNC_PLUGIN_AUTHENTICATON_H_
#define _OPENSYNC_PLUGIN_AUTHENTICATON_H_

/**
 * @defgroup OSyncPluginAuthAPI OpenSync Plugin Authentication
 * @ingroup OSyncPublic
 * @brief Functions for configuring plugin authentication
 * 
 */
/*@{*/

/*! @brief Types of authentication options supported by a plugin
 * 
 **/
typedef enum {
	/** Authentication includes username */
	OSYNC_PLUGIN_AUTHENTICATION_USERNAME	= (1 << 0),
	/** Authentication includes password */
	OSYNC_PLUGIN_AUTHENTICATION_PASSWORD	= (1 << 1),
	/** Authentication includes a reference to a password db entry */
	OSYNC_PLUGIN_AUTHENTICATION_REFERENCE	= (1 << 2),
} OSyncPluginAuthenticationOptionSupportedFlag;

/*! @brief Set of OSyncPluginAuthenticationOptionSupportedFlags
 * 
 **/
typedef unsigned int OSyncPluginAuthenticationOptionSupportedFlags;

/*! @brief Create a new OSyncPluginAuthentication object
 *
 * @param error Pointer to an error struct
 * @returns the newly created object, or NULL in case of an error.
 *
 */
OSYNC_EXPORT OSyncPluginAuthentication *osync_plugin_authentication_new(OSyncError **error);

/*! @brief Decrease the reference count on an OSyncPluginAuthentication object
 * 
 * @param auth Pointer to the OSyncPluginAuthentication object
 * 
 */
OSYNC_EXPORT void osync_plugin_authentication_unref(OSyncPluginAuthentication *auth);

/*! @brief Increase the reference count on an OSyncPluginAuthentication object
 * 
 * @param auth Pointer to the OSyncPluginAuthentication object
 * @returns The OSyncPluginAuthentication object passed in
 * 
 */
OSYNC_EXPORT OSyncPluginAuthentication *osync_plugin_authentication_ref(OSyncPluginAuthentication *auth);


/*! @brief Determine if an authentication option is supported by a plugin
 *
 * @param auth Pointer to the OSyncPluginAuthentication object
 * @param flag Authentication option to check
 * @returns TRUE if the specified option is supported, FALSE otherwise.
 *
 */
OSYNC_EXPORT osync_bool osync_plugin_authentication_option_is_supported(OSyncPluginAuthentication *auth, OSyncPluginAuthenticationOptionSupportedFlag flag);

/*! @brief Set the authentication options supported by a plugin
 *
 * @param auth Pointer to the OSyncPluginAuthentication object
 * @param flags Authentication option to check
 *
 */
OSYNC_EXPORT void osync_plugin_authentication_option_set_supported(OSyncPluginAuthentication *auth, OSyncPluginAuthenticationOptionSupportedFlags flags);


/*! @brief Get the authentication username
 *
 * @param auth Pointer to the OSyncPluginAuthentication object
 * @returns the username or NULL if not set
 *
 */
OSYNC_EXPORT const char *osync_plugin_authentication_get_username(OSyncPluginAuthentication *auth);

/*! @brief Set the authentication username
 *
 * @param auth Pointer to the OSyncPluginAuthentication object
 * @param username the username to set
 *
 */
OSYNC_EXPORT void osync_plugin_authentication_set_username(OSyncPluginAuthentication *auth, const char *username);


/*! @brief Get the authentication password
 *
 * @param auth Pointer to the OSyncPluginAuthentication object
 * @returns the username or NULL if not set
 *
 */
OSYNC_EXPORT const char *osync_plugin_authentication_get_password(OSyncPluginAuthentication *auth);

/*! @brief Set the authentication password
 *
 * WARNING: This password will be stored in plain text in the plugin configuration file. In
 * future versions of OpenSync, interfaces should be available to password databases
 * (KWallet, Gnome-keyring etc.), and once they are available you should use them instead.
 * Until then, you should warn users of your plugin that the password is stored unencrypted.
 *
 * @param auth Pointer to the OSyncPluginAuthentication object
 * @param password the password to set
 *
 */
OSYNC_EXPORT void osync_plugin_authentication_set_password(OSyncPluginAuthentication *auth, const char *password);


/*! @brief Get the authentication reference.
 *
 * This reference is intended to be a key used to look up the password using some kind of
 * password storage system, such as KDE's KWallet or Gnome-keyring. As this is not yet
 * fully implemented this interface may be subject to change in future versions of OpenSync.
 *
 * @param auth Pointer to the OSyncPluginAuthentication object
 * @returns the reference or NULL if not set
 *
 */
OSYNC_EXPORT const char *osync_plugin_authentication_get_reference(OSyncPluginAuthentication *auth);

/*! @brief Set the authentication reference
 *
 * This reference is intended to be a key used to look up the password using some kind of
 * password storage system, such as KDE's KWallet or Gnome-keyring. As this is not yet
 * fully implemented this interface may be subject to change in future versions of OpenSync.
 *
 * @param auth Pointer to the OSyncPluginAuthentication object
 * @param reference the reference to set
 *
 */
OSYNC_EXPORT void osync_plugin_authentication_set_reference(OSyncPluginAuthentication *auth, const char *reference);

/*@}*/

#endif /*_OPENSYNC_PLUGIN_AUTHENTICATON_H_*/

