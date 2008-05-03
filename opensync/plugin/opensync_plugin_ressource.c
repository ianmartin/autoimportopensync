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

		if (ressource->mime)
			g_free(ressource->mime);

		if (ressource->objformat)
			g_free(ressource->objformat);
		/* osync_objformat_unref(ressource->objformat); */
			
		if (ressource->path)
			g_free(ressource->path);

		if (ressource->url)
			g_free(ressource->url);
			
		g_free(ressource);
	}
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

const char *osync_plugin_ressource_get_objformat(OSyncPluginRessource *ressource)
{
	osync_assert(ressource);
	return ressource->objformat;
}

void osync_plugin_ressource_set_objformat(OSyncPluginRessource *ressource, const char *objformat)
{
	osync_assert(ressource);
	if (ressource->objformat)
		g_free(ressource->objformat);

	ressource->objformat = g_strdup(objformat);
}

/*
OSyncObjFormat *osync_plugin_ressource_get_objformat(OSyncPluginRessource *ressource)
{
	osync_assert(ressource);
	return ressource->objformat;
}

void osync_plugin_ressource_set_objformat(OSyncPluginRessource *ressource, OSyncObjFormat *objformat)
{
	osync_assert(ressource);
	if (ressource->objformat)
		osync_objformat_unref(ressource->objformat);

	ressource->objformat = osync_objformat_unref(objformat);
}
*/

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

