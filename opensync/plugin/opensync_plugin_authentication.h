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

OSYNC_EXPORT OSyncPluginAuthentication *osync_plugin_authentication_new(OSyncError **error);
OSYNC_EXPORT void osync_plugin_authentication_unref(OSyncPluginAuthentication *auth);
OSYNC_EXPORT OSyncPluginAuthentication *osync_plugin_authentication_ref(OSyncPluginAuthentication *auth);

OSYNC_EXPORT const char *osync_plugin_authentication_get_username(OSyncPluginAuthentication *auth);
OSYNC_EXPORT void osync_plugin_authentication_set_username(OSyncPluginAuthentication *auth, const char *username);

OSYNC_EXPORT const char *osync_plugin_authentication_get_password(OSyncPluginAuthentication *auth);
OSYNC_EXPORT void osync_plugin_authentication_set_password(OSyncPluginAuthentication *auth, const char *password);

OSYNC_EXPORT const char *osync_plugin_authentication_get_reference(OSyncPluginAuthentication *auth);
OSYNC_EXPORT void osync_plugin_authentication_set_reference(OSyncPluginAuthentication *auth, const char *reference);

#endif /*_OPENSYNC_PLUGIN_AUTHENTICATON_H_*/

