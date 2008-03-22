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

#ifndef _OPENSYNC_PLUGIN_LOCALIZATION_H_
#define _OPENSYNC_PLUGIN_LOCALIZATION_H_

OSYNC_EXPORT OSyncPluginLocalization *osync_plugin_localization_new(OSyncError **error);
OSYNC_EXPORT void osync_plugin_localization_unref(OSyncPluginLocalization *local);
OSYNC_EXPORT OSyncPluginLocalization *osync_plugin_localization_ref(OSyncPluginLocalization *local);

OSYNC_EXPORT const char *osync_plugin_localization_get_encoding(OSyncPluginLocalization *local);
OSYNC_EXPORT void osync_plugin_localization_set_encoding(OSyncPluginLocalization *local, const char *encoding);

OSYNC_EXPORT const char *osync_plugin_localization_get_timezone(OSyncPluginLocalization *local);
OSYNC_EXPORT void osync_plugin_localization_set_timezone(OSyncPluginLocalization *local, const char *timezone);

OSYNC_EXPORT const char *osync_plugin_localization_get_language(OSyncPluginLocalization *local);
OSYNC_EXPORT void osync_plugin_localization_set_language(OSyncPluginLocalization *local, const char *language);

#endif /*_OPENSYNC_PLUGIN_LOCALIZATION_H_*/

