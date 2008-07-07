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

#ifndef _OPENSYNC_PLUGIN_RESSOURCE_H_
#define _OPENSYNC_PLUGIN_RESSOURCE_H_

OSYNC_EXPORT OSyncPluginRessource *osync_plugin_ressource_new(OSyncError **error);
OSYNC_EXPORT void osync_plugin_ressource_unref(OSyncPluginRessource *ressource);
OSYNC_EXPORT OSyncPluginRessource *osync_plugin_ressource_ref(OSyncPluginRessource *ressource);

OSYNC_EXPORT osync_bool osync_plugin_ressource_is_enabled(OSyncPluginRessource *ressource);
OSYNC_EXPORT void osync_plugin_ressource_enable(OSyncPluginRessource *ressource, osync_bool enable);

OSYNC_EXPORT const char *osync_plugin_ressource_get_name(OSyncPluginRessource *ressource);
OSYNC_EXPORT void osync_plugin_ressource_set_name(OSyncPluginRessource *ressource, const char *name);

OSYNC_EXPORT const char *osync_plugin_ressource_get_mime(OSyncPluginRessource *ressource);
OSYNC_EXPORT void osync_plugin_ressource_set_mime(OSyncPluginRessource *ressource, const char *mime);

/*
OSYNC_EXPORT OSyncObjFormat *osync_plugin_ressource_get_objformat(OSyncPluginRessource *ressource);
OSYNC_EXPORT void osync_plugin_ressource_set_objformat(OSyncPluginRessource *ressource, OSyncObjFormat *objformat);
*/

OSYNC_EXPORT OSyncList *osync_plugin_ressource_get_objformat_sinks(OSyncPluginRessource *ressource);
OSYNC_EXPORT void osync_plugin_ressource_add_objformat_sink(OSyncPluginRessource *ressource, OSyncObjFormatSink *formatsink);
OSYNC_EXPORT void osync_plugin_ressource_remove_objformat_sink(OSyncPluginRessource *ressource, OSyncObjFormatSink *formatsink);

OSYNC_EXPORT const char *osync_plugin_ressource_get_objtype(OSyncPluginRessource *ressource);
OSYNC_EXPORT void osync_plugin_ressource_set_objtype(OSyncPluginRessource *ressource, const char *objtype);

OSYNC_EXPORT const char *osync_plugin_ressource_get_path(OSyncPluginRessource *ressource);
OSYNC_EXPORT void osync_plugin_ressource_set_path(OSyncPluginRessource *ressource, const char *path);

OSYNC_EXPORT const char *osync_plugin_ressource_get_url(OSyncPluginRessource *ressource);
OSYNC_EXPORT void osync_plugin_ressource_set_url(OSyncPluginRessource *ressource, const char *url);


#endif /*_OPENSYNC_PLUGIN_RESSOURCE_H_*/

