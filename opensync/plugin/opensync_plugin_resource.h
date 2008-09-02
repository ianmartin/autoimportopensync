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

#ifndef _OPENSYNC_PLUGIN_RESOURCE_H_
#define _OPENSYNC_PLUGIN_RESOURCE_H_

typedef enum {
	OSYNC_PLUGIN_RESOURCE_NAME	= (1 << 0),
	OSYNC_PLUGIN_RESOURCE_PATH	= (1 << 1),
	OSYNC_PLUGIN_RESOURCE_URL	= (1 << 2),
} OSyncPluginResourceOptionSupportedFlag;

typedef unsigned int OSyncPluginResourceOptionSupportedFlags;

OSYNC_EXPORT OSyncPluginResource *osync_plugin_resource_new(OSyncError **error);
OSYNC_EXPORT void osync_plugin_resource_unref(OSyncPluginResource *resource);
OSYNC_EXPORT OSyncPluginResource *osync_plugin_resource_ref(OSyncPluginResource *resource);

OSYNC_EXPORT osync_bool osync_plugin_resource_option_is_supported(OSyncPluginResource *resource, OSyncPluginResourceOptionSupportedFlag flag);
OSYNC_EXPORT void osync_plugin_resource_option_set_supported(OSyncPluginResource *resource, OSyncPluginResourceOptionSupportedFlags flags);

OSYNC_EXPORT osync_bool osync_plugin_resource_is_enabled(OSyncPluginResource *resource);
OSYNC_EXPORT void osync_plugin_resource_enable(OSyncPluginResource *resource, osync_bool enable);

OSYNC_EXPORT const char *osync_plugin_resource_get_name(OSyncPluginResource *resource);
OSYNC_EXPORT void osync_plugin_resource_set_name(OSyncPluginResource *resource, const char *name);

OSYNC_EXPORT const char *osync_plugin_resource_get_mime(OSyncPluginResource *resource);
OSYNC_EXPORT void osync_plugin_resource_set_mime(OSyncPluginResource *resource, const char *mime);

/*
OSYNC_EXPORT OSyncObjFormat *osync_plugin_resource_get_objformat(OSyncPluginResource *resource);
OSYNC_EXPORT void osync_plugin_resource_set_objformat(OSyncPluginResource *resource, OSyncObjFormat *objformat);
*/

OSYNC_EXPORT OSyncList *osync_plugin_resource_get_objformat_sinks(OSyncPluginResource *resource);
OSYNC_EXPORT void osync_plugin_resource_add_objformat_sink(OSyncPluginResource *resource, OSyncObjFormatSink *formatsink);
OSYNC_EXPORT void osync_plugin_resource_remove_objformat_sink(OSyncPluginResource *resource, OSyncObjFormatSink *formatsink);

OSYNC_EXPORT const char *osync_plugin_resource_get_objtype(OSyncPluginResource *resource);
OSYNC_EXPORT void osync_plugin_resource_set_objtype(OSyncPluginResource *resource, const char *objtype);

OSYNC_EXPORT const char *osync_plugin_resource_get_path(OSyncPluginResource *resource);
OSYNC_EXPORT void osync_plugin_resource_set_path(OSyncPluginResource *resource, const char *path);

OSYNC_EXPORT const char *osync_plugin_resource_get_url(OSyncPluginResource *resource);
OSYNC_EXPORT void osync_plugin_resource_set_url(OSyncPluginResource *resource, const char *url);


#endif /*_OPENSYNC_PLUGIN_RESOURCE_H_*/

