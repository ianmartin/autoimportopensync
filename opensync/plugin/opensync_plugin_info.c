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
 
#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-format.h"
#include "opensync-plugin.h"
#include "opensync_plugin_info_internals.h"


OSyncPluginInfo *osync_plugin_info_new(OSyncError **error)
{
	OSyncPluginInfo *info = osync_try_malloc0(sizeof(OSyncPluginInfo), error);
	if (!info)
		return NULL;
	
	info->ref_count = 1;
	
	return info;
}

void osync_plugin_info_ref(OSyncPluginInfo *info)
{
	osync_assert(info);
	
	g_atomic_int_inc(&(info->ref_count));
}

void osync_plugin_info_unref(OSyncPluginInfo *info)
{
	osync_assert(info);
	
	if (g_atomic_int_dec_and_test(&(info->ref_count))) {
		if (info->config)
			g_free(info->config);
		
		if (info->configdir)
			g_free(info->configdir);
		
		while (info->objtypes) {
			OSyncObjTypeSink *sink = info->objtypes->data;
			osync_objtype_sink_unref(sink);
			info->objtypes = g_list_remove(info->objtypes, sink);
		}
		
		if (info->sink)
			osync_objtype_sink_unref(info->sink);
		
		g_free(info);
	}
}

void osync_plugin_info_set_loop(OSyncPluginInfo *info, void *loop)
{
	osync_assert(info);
	info->loop = loop;
}

void *osync_plugin_info_get_loop(OSyncPluginInfo *info)
{
	osync_assert(info);
	return info->loop;
}

void osync_plugin_info_set_config(OSyncPluginInfo *info, const char *config)
{
	osync_assert(info);
	if (info->config)
		g_free(info->config);
	info->config = g_strdup(config);
}

const char *osync_plugin_info_get_config(OSyncPluginInfo *info)
{
	osync_assert(info);
	return info->config;
}

void osync_plugin_info_set_configdir(OSyncPluginInfo *info, const char *configdir)
{
	osync_assert(info);
	if (info->configdir)
		g_free(info->configdir);
	info->configdir = g_strdup(configdir);
}

const char *osync_plugin_info_get_configdir(OSyncPluginInfo *info)
{
	osync_assert(info);
	return info->configdir;
}

OSyncObjTypeSink *osync_plugin_info_find_objtype(OSyncPluginInfo *info, const char *name)
{
	GList *p;
	osync_assert(info);
	for (p = info->objtypes; p; p = p->next) {
		OSyncObjTypeSink *sink = p->data;
		if (g_ascii_strcasecmp(osync_objtype_sink_get_name(sink), name) == 0)
			return sink;
	}
	return NULL;
}

void osync_plugin_info_add_objtype(OSyncPluginInfo *info, OSyncObjTypeSink *sink)
{
	osync_assert(info);
	info->objtypes = g_list_append(info->objtypes, sink);
	osync_objtype_sink_ref(sink);
}

int osync_plugin_info_num_objtypes(OSyncPluginInfo *info)
{
	osync_assert(info);
	return g_list_length(info->objtypes);
}

OSyncObjTypeSink *osync_plugin_info_nth_objtype(OSyncPluginInfo *info, int nth)
{
	osync_assert(info);
	return g_list_nth_data(info->objtypes, nth);
}

OSyncObjTypeSink *osync_plugin_info_get_sink(OSyncPluginInfo *info)
{
	osync_assert(info);
	return info->sink;
}

void osync_plugin_info_set_sink(OSyncPluginInfo *info, OSyncObjTypeSink *sink)
{
	osync_assert(info);
	osync_assert(sink);
	info->sink = sink;
	osync_objtype_sink_ref(sink);
}
