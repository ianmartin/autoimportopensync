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

void *syncml_obex_client_init(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **oerror)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, oerror);

	SmlError *error = NULL;
	SmlPluginEnv *env = syncml_init(
				SML_SESSION_TYPE_SERVER,
				SML_TRANSPORT_OBEX_CLIENT,
				plugin, info, oerror);
	if (!env)
		goto error;

	/* add the datastores */
	GList *o = env->databases;
	for (; o; o = o->next) {
		SmlDatabase *database = o->data;

		char *objtype = g_ascii_strup(osync_objformat_get_objtype(database->objformat), -1);
		if (!smlDataSyncSetOption(env->dsObject1, SML_TRANSPORT_CONFIG_DATASTORE, objtype, &error))
		{
			safe_cfree(&objtype);
			goto error_free_env;
		}
		safe_cfree(&objtype);
	}

	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;
error_free_env:
	finalize(env);
error:
	if (error) {
		osync_error_set(oerror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
		smlErrorDeref(&error);
	}
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(oerror));
	return NULL;
}

osync_bool syncml_obex_client_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
	return discover("syncml-obex-client", data, info, error);
}
