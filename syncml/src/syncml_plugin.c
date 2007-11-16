/*
 * syncml plugin - A syncml plugin for OpenSync
 * Copyright (C) 2005  Armin Bauer <armin.bauer@opensync.org>
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

#include "config.h"

#include "syncml_plugin.h"
#include "syncml_common.h"

#ifdef ENABLE_HTTP
#  include "syncml_http_client.h"
#  include "syncml_http_server.h"
#endif

#ifdef ENABLE_OBEX
#  include "syncml_obex_client.h"
#endif

osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
    OSyncPlugin *plugin;

#ifdef ENABLE_HTTP
    plugin = osync_plugin_new(error);

    if (!plugin)
        goto error;

    osync_plugin_set_name(plugin, "syncml-http-server");
    osync_plugin_set_longname(plugin, "SyncML over HTTP Server");
    osync_plugin_set_description(plugin, "Plugin to synchronize with SyncML over HTTP");

    osync_plugin_set_initialize(plugin, syncml_http_server_init);
    osync_plugin_set_finalize(plugin, finalize);
    osync_plugin_set_discover(plugin, syncml_http_server_discover);

    osync_plugin_env_register_plugin(env, plugin);
    osync_plugin_unref(plugin);

    plugin = osync_plugin_new(error);

    if (!plugin)
        goto error;

    osync_plugin_set_name(plugin, "syncml-http-client");
    osync_plugin_set_longname(plugin, "SyncML over HTTP Client");
    osync_plugin_set_description(plugin, "Plugin to synchronize with SyncML over HTTP");

    osync_plugin_set_initialize(plugin, syncml_http_client_init);
    osync_plugin_set_finalize(plugin, finalize);
    osync_plugin_set_discover(plugin, syncml_http_client_discover);

    osync_plugin_env_register_plugin(env, plugin);
    osync_plugin_unref(plugin);
#endif

#ifdef ENABLE_OBEX
    plugin = osync_plugin_new(error);
    if (!plugin)
        goto error;

    osync_plugin_set_name(plugin, "syncml-obex-client");
    osync_plugin_set_longname(plugin, "SyncML over OBEX Client");
    osync_plugin_set_description(plugin, "Plugin to synchronize with SyncML over OBEX");

    osync_plugin_set_initialize(plugin, syncml_obex_client_init);
    osync_plugin_set_finalize(plugin, finalize);
    osync_plugin_set_discover(plugin, syncml_obex_client_discover);

    osync_plugin_env_register_plugin(env, plugin);
    osync_plugin_unref(plugin);

    return TRUE;
#endif

error:
    osync_trace(TRACE_ERROR, "Unable to register: %s", osync_error_print(error));
    osync_error_unref(error);
    return FALSE;
}

int get_version(void)
{
    return 1;
}
