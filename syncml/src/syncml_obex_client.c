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
#include "syncml_callbacks.h"
#include "syncml_devinf.h"
#include "syncml_vformat.h"
#include "syncml_ds_server.h"
#include <libsyncml/standard.h>

void *syncml_obex_client_init(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **oerror)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, oerror);
	SmlError *error = NULL;
	
	SmlPluginEnv *env = osync_try_malloc0(sizeof(SmlPluginEnv), oerror);
	if (!env)
		goto error;
	env->sessionType = SML_SESSION_TYPE_SERVER;
	env->pluginInfo = info;
	osync_plugin_info_ref(env->pluginInfo);

	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
        osync_trace(TRACE_INTERNAL, "The config: %p", config);

	/* create data sync object */
	env->dsObject1 = smlDataSyncNew(
				SML_SESSION_TYPE_SERVER,
				SML_TRANSPORT_OBEX_CLIENT,
				&error);
	if (!env->dsObject1)
		goto error;

	/* configure the instance */
	if (!parse_config(SML_TRANSPORT_OBEX_CLIENT, env->dsObject1, config, oerror))
		goto error_free_env;

	/* prepare the function list for OpenSync */
	OSyncObjTypeSinkFunctions main_functions;
	memset(&main_functions, 0, sizeof(main_functions));
	main_functions.connect = syncml_connect;
	main_functions.disconnect = disconnect;

	/* Register main sink for connect and disconnect functions */
	OSyncObjTypeSink *mainsink = osync_objtype_main_sink_new(oerror);
	if (!mainsink)
		goto error_free_env;

	osync_objtype_sink_set_functions(mainsink, main_functions, env);
	osync_plugin_info_set_main_sink(info, mainsink);
	osync_objtype_sink_unref(mainsink);

	/* prepare paths for callbacks */
	env->anchor_path = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
	env->devinf_path = g_strdup_printf("%s/devinf.db", osync_plugin_info_get_configdir(info));

	/* set callbacks */
	smlDataSyncRegisterEventCallback(env->dsObject1, _recv_event, env);
	smlDataSyncRegisterGetAlertTypeCallback(env->dsObject1, _get_alert_type, env);
	smlDataSyncRegisterGetAnchorCallback(env->dsObject1, _get_anchor, env);
	smlDataSyncRegisterSetAnchorCallback(env->dsObject1, _set_anchor, env);
	smlDataSyncRegisterWriteDevInfCallback(env->dsObject1, _write_devinf, env);
	smlDataSyncRegisterReadDevInfCallback(env->dsObject1, _read_devinf, env);
	smlDataSyncRegisterHandleRemoteDevInfCallback(env->dsObject1, _handle_remote_devinf, env);
	smlDataSyncRegisterChangeStatusCallback(env->dsObject1, _recv_change_status);

	/* configure databases */
	if (!ds_server_init_databases(env, info, oerror))
		goto error_free_env;

	/* add the datastores */
	GList *o = env->databases;
	for (; o; o = o->next) {
		SmlDatabase *database = o->data;

		char *objtype = g_ascii_strup(osync_objformat_get_objtype(database->objformat), -1);
		if (!smlDataSyncSetOption(env->dsObject1, "DATASTORE", objtype, &error))
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
