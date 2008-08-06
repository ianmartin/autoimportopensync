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

OSyncPluginResource *osync_plugin_resource_new(OSyncError **error)
{
	OSyncPluginResource *resource = osync_try_malloc0(sizeof(OSyncPluginResource), error);
	if (!resource)
		return NULL;

	resource->ref_count = 1;

	return resource;
}

OSyncPluginResource *osync_plugin_resource_ref(OSyncPluginResource *resource)
{
	osync_assert(resource);
	
	g_atomic_int_inc(&(resource->ref_count));

	return resource;
}

void osync_plugin_resource_unref(OSyncPluginResource *resource)
{
	osync_assert(resource);

	if (g_atomic_int_dec_and_test(&(resource->ref_count))) {
		if (resource->name)
			g_free(resource->name);

		if (resource->objtype)
			g_free(resource->objtype);

		if (resource->mime)
			g_free(resource->mime);

		while (resource->objformatsinks) {
			osync_objformat_sink_unref(resource->objformatsinks->data);
			resource->objformatsinks = osync_list_remove(resource->objformatsinks, resource->objformatsinks->data);
		}
			
		if (resource->path)
			g_free(resource->path);

		if (resource->url)
			g_free(resource->url);
			
		g_free(resource);
	}
}

osync_bool osync_plugin_resource_is_enabled(OSyncPluginResource *resource)
{
	osync_assert(resource);
	return resource->enabled;
}

void osync_plugin_resource_enable(OSyncPluginResource *resource, osync_bool enable)
{
	osync_assert(resource);
	resource->enabled = enable;
}

const char *osync_plugin_resource_get_name(OSyncPluginResource *resource)
{
	osync_assert(resource);
	return resource->name;
}

void osync_plugin_resource_set_name(OSyncPluginResource *resource, const char *name)
{
	osync_assert(resource);
	if (resource->name)
		g_free(resource->name);

	resource->name = g_strdup(name);
}

const char *osync_plugin_resource_get_mime(OSyncPluginResource *resource)
{
	osync_assert(resource);
	return resource->mime;
}

void osync_plugin_resource_set_mime(OSyncPluginResource *resource, const char *mime)
{
	osync_assert(resource);
	if (resource->mime)
		g_free(resource->mime);

	resource->mime = g_strdup(mime);
}

OSyncList *osync_plugin_resource_get_objformat_sinks(OSyncPluginResource *resource)
{
	osync_assert(resource);
	return resource->objformatsinks;
}

void osync_plugin_resource_add_objformat_sink(OSyncPluginResource *resource, OSyncObjFormatSink *formatsink)
{
	osync_assert(resource);
	osync_assert(formatsink);

	if (osync_list_find(resource->objformatsinks, formatsink))
		return;

	osync_objformat_sink_ref(formatsink);
	resource->objformatsinks = osync_list_prepend(resource->objformatsinks, formatsink);
}

void osync_plugin_resource_remove_objformat_sink(OSyncPluginResource *resource, OSyncObjFormatSink *formatsink)
{
	osync_assert(resource);
	osync_assert(formatsink);

	resource->objformatsinks = osync_list_remove(resource->objformatsinks, formatsink);
	osync_objformat_sink_unref(formatsink);
}

const char *osync_plugin_resource_get_objtype(OSyncPluginResource *resource)
{
	osync_assert(resource);
	return resource->objtype;
}

void osync_plugin_resource_set_objtype(OSyncPluginResource *resource, const char *objtype)
{
	osync_assert(resource);
	if (resource->objtype)
		g_free(resource->objtype);

	resource->objtype = g_strdup(objtype);
}

const char *osync_plugin_resource_get_path(OSyncPluginResource *resource)
{
	osync_assert(resource);
	return resource->path;
}

void osync_plugin_resource_set_path(OSyncPluginResource *resource, const char *path)
{
	osync_assert(resource);
	if (resource->path)
		g_free(resource->path);

	resource->path = g_strdup(path);
}

const char *osync_plugin_resource_get_url(OSyncPluginResource *resource)
{
	osync_assert(resource);
	return resource->url;
}

void osync_plugin_resource_set_url(OSyncPluginResource *resource, const char *url)
{
	osync_assert(resource);
	if (resource->url)
		g_free(resource->url);

	resource->url = g_strdup(url);
}

