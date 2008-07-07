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

#include "opensync-format.h"
#include "opensync-plugin.h"
#include "opensync_plugin_ressource_internals.h"

OSyncPluginRessource *osync_plugin_ressource_new(OSyncError **error)
{
	OSyncPluginRessource *ressource = osync_try_malloc0(sizeof(OSyncPluginRessource), error);
	if (!ressource)
		return NULL;

	ressource->ref_count = 1;

	return ressource;
}

OSyncPluginRessource *osync_plugin_ressource_ref(OSyncPluginRessource *ressource)
{
	osync_assert(ressource);
	
	g_atomic_int_inc(&(ressource->ref_count));

	return ressource;
}

void osync_plugin_ressource_unref(OSyncPluginRessource *ressource)
{
	osync_assert(ressource);

	if (g_atomic_int_dec_and_test(&(ressource->ref_count))) {
		if (ressource->name)
			g_free(ressource->name);

		if (ressource->objtype)
			g_free(ressource->objtype);

		if (ressource->mime)
			g_free(ressource->mime);

		while (ressource->objformatsinks) {
			osync_objformat_sink_unref(ressource->objformatsinks->data);
			ressource->objformatsinks = osync_list_remove(ressource->objformatsinks, ressource->objformatsinks->data);
		}
			
		if (ressource->path)
			g_free(ressource->path);

		if (ressource->url)
			g_free(ressource->url);
			
		g_free(ressource);
	}
}

osync_bool osync_plugin_ressource_is_enabled(OSyncPluginRessource *ressource)
{
	osync_assert(ressource);
	return ressource->enabled;
}

void osync_plugin_ressource_enable(OSyncPluginRessource *ressource, osync_bool enable)
{
	osync_assert(ressource);
	ressource->enabled = enable;
}

const char *osync_plugin_ressource_get_name(OSyncPluginRessource *ressource)
{
	osync_assert(ressource);
	return ressource->name;
}

void osync_plugin_ressource_set_name(OSyncPluginRessource *ressource, const char *name)
{
	osync_assert(ressource);
	if (ressource->name)
		g_free(ressource->name);

	ressource->name = g_strdup(name);
}

const char *osync_plugin_ressource_get_mime(OSyncPluginRessource *ressource)
{
	osync_assert(ressource);
	return ressource->mime;
}

void osync_plugin_ressource_set_mime(OSyncPluginRessource *ressource, const char *mime)
{
	osync_assert(ressource);
	if (ressource->mime)
		g_free(ressource->mime);

	ressource->mime = g_strdup(mime);
}

OSyncList *osync_plugin_ressource_get_objformat_sinks(OSyncPluginRessource *ressource)
{
	osync_assert(ressource);
	return ressource->objformatsinks;
}

void osync_plugin_ressource_add_objformat_sink(OSyncPluginRessource *ressource, OSyncObjFormatSink *formatsink)
{
	osync_assert(ressource);
	osync_assert(formatsink);

	if (osync_list_find(ressource->objformatsinks, formatsink))
		return;

	osync_objformat_sink_ref(formatsink);
	ressource->objformatsinks = osync_list_prepend(ressource->objformatsinks, formatsink);
}

void osync_plugin_ressource_remove_objformat_sink(OSyncPluginRessource *ressource, OSyncObjFormatSink *formatsink)
{
	osync_assert(ressource);
	osync_assert(formatsink);

	ressource->objformatsinks = osync_list_remove(ressource->objformatsinks, formatsink);
	osync_objformat_sink_unref(formatsink);
}

const char *osync_plugin_ressource_get_objtype(OSyncPluginRessource *ressource)
{
	osync_assert(ressource);
	return ressource->objtype;
}

void osync_plugin_ressource_set_objtype(OSyncPluginRessource *ressource, const char *objtype)
{
	osync_assert(ressource);
	if (ressource->objtype)
		g_free(ressource->objtype);

	ressource->objtype = g_strdup(objtype);
}

const char *osync_plugin_ressource_get_path(OSyncPluginRessource *ressource)
{
	osync_assert(ressource);
	return ressource->path;
}

void osync_plugin_ressource_set_path(OSyncPluginRessource *ressource, const char *path)
{
	osync_assert(ressource);
	if (ressource->path)
		g_free(ressource->path);

	ressource->path = g_strdup(path);
}

const char *osync_plugin_ressource_get_url(OSyncPluginRessource *ressource)
{
	osync_assert(ressource);
	return ressource->url;
}

void osync_plugin_ressource_set_url(OSyncPluginRessource *ressource, const char *url)
{
	osync_assert(ressource);
	if (ressource->url)
		g_free(ressource->url);

	ressource->url = g_strdup(url);
}

