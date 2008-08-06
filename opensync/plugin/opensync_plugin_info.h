/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
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

#ifndef OPENSYNC_PLUGIN_INFO_H_
#define OPENSYNC_PLUGIN_INFO_H_

OSYNC_EXPORT OSyncPluginInfo *osync_plugin_info_new(OSyncError **error);
OSYNC_EXPORT OSyncPluginInfo *osync_plugin_info_ref(OSyncPluginInfo *info);
OSYNC_EXPORT void osync_plugin_info_unref(OSyncPluginInfo *info);

OSYNC_EXPORT void osync_plugin_info_set_loop(OSyncPluginInfo *info, void *loop);
OSYNC_EXPORT void *osync_plugin_info_get_loop(OSyncPluginInfo *info);

OSYNC_EXPORT void osync_plugin_info_set_config(OSyncPluginInfo *info, OSyncPluginConfig *config);
OSYNC_EXPORT OSyncPluginConfig *osync_plugin_info_get_config(OSyncPluginInfo *info);

OSYNC_EXPORT void osync_plugin_info_set_configdir(OSyncPluginInfo *info, const char *configdir);
OSYNC_EXPORT const char *osync_plugin_info_get_configdir(OSyncPluginInfo *info);

OSYNC_EXPORT OSyncObjTypeSink *osync_plugin_info_find_objtype(OSyncPluginInfo *info, const char *name);
OSYNC_EXPORT void osync_plugin_info_add_objtype(OSyncPluginInfo *info, OSyncObjTypeSink *sink);
OSYNC_EXPORT unsigned int osync_plugin_info_num_objtypes(OSyncPluginInfo *info);
OSYNC_EXPORT OSyncObjTypeSink *osync_plugin_info_nth_objtype(OSyncPluginInfo *info, unsigned int nth);

OSYNC_EXPORT OSyncObjTypeSink *osync_plugin_info_get_main_sink(OSyncPluginInfo *info);
OSYNC_EXPORT void osync_plugin_info_set_main_sink(OSyncPluginInfo *info, OSyncObjTypeSink *sink);

OSYNC_EXPORT OSyncFormatEnv *osync_plugin_info_get_format_env(OSyncPluginInfo *info);
OSYNC_EXPORT void osync_plugin_info_set_format_env(OSyncPluginInfo *info, OSyncFormatEnv *env);

OSYNC_EXPORT OSyncObjTypeSink *osync_plugin_info_get_sink(OSyncPluginInfo *info);
OSYNC_EXPORT void osync_plugin_info_set_sink(OSyncPluginInfo *info, OSyncObjTypeSink *sink);

OSYNC_EXPORT void osync_plugin_info_set_groupname(OSyncPluginInfo *info, const char *groupname);
OSYNC_EXPORT const char *osync_plugin_info_get_groupname(OSyncPluginInfo *info);

OSYNC_EXPORT void osync_plugin_info_set_version(OSyncPluginInfo *info, OSyncVersion *version);
OSYNC_EXPORT OSyncVersion *osync_plugin_info_get_version(OSyncPluginInfo *info);

OSYNC_EXPORT void osync_plugin_info_set_capabilities(OSyncPluginInfo *info, OSyncCapabilities *capabilities);
OSYNC_EXPORT OSyncCapabilities *osync_plugin_info_get_capabilities(OSyncPluginInfo *info);

#endif /*OPENSYNC_PLUGIN_INFO_H_*/
