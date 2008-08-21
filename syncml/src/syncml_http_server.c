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

#include "syncml_common.h"

void *syncml_http_server_init(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **oerror)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, oerror);

	SmlPluginEnv *env = syncml_init(
				SML_SESSION_TYPE_SERVER,
				SML_TRANSPORT_HTTP_SERVER,
				plugin, info, oerror);
	if (!env)
		goto error;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(oerror));
	return NULL;
}

osync_bool syncml_http_server_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
	return discover("syncml-http-server", data, info, error);
}

