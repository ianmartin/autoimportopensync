#include "syncml_common.h"
#include "syncml_callbacks.h"
#include "syncml_devinf.h"
#include "syncml_ds_client.h"
#include "syncml_vformat.h"

#include <opensync/plugin/opensync_sink.h>

/* Some informations about the SyncML protocol
 * 
 * A synchronization with SyncML which is initiated by the client
 * requires the following actions (two way sync):
 * 
 * 1. client sends sync alert
 * 2. server sends sync alert
 * 3. client sends changes
 * 4. server sends status and changes
 * 5. client sends status and map
 * 6. server sends status
 *
 * OpenSync uses the following communication pattern:
 *
 * 1. OpenSync requests all changes
 * 2. The other side sends all changes
 * 3. OpenSync performs all necessary operations
 * 4. OpenSync sends all necessary changes
 *
 * OpenSync'c behaviour is always the behaviour of an SyncML server
 * even if we use OpenSync as a SyncML client. This requires some special
 * use cases of SyncML (OpenSync as SyncML client):
 *
 * 1. OpenSync sends sync alert
 * 2. SyncML server sends sync alert
 * 3. OpenSync sends numberOfChanges 0 (no changes at client)
 * 4. SyncML server sends all of its changes
 * 5.1 OpenSync sends ok and the mapping entries of the changes
 * 5.2 OpenSync perform the necessary comparisons internally
 * 6.1 SyncML server sends ok (will be ignored/tolerated by OpenSync)
 * 6.2 OpenSync sends sync alert
 * 7. SyncML server sends sync alert
 * 8. OpenSync sends changes
 * 9. SyncML server sends changes
 * 10. OpenSync sends ok and mapping
 * 11. SyncML server sends ok
 *
 * The problem is that we do not like a complete second implementation
 * only for the client. So the solution is that we know states and events
 * in the SyncML communication. Clients and servers simply start in
 * different modes.
 *
 */

void *syncml_http_client_init(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **oerror)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, oerror);
	SmlError *error = NULL;
	
	SmlPluginEnv *env = osync_try_malloc0(sizeof(SmlPluginEnv), oerror);
	if (!env)
		goto error;
	env->sessionType = SML_SESSION_TYPE_CLIENT;
	env->pluginInfo = info;
	osync_plugin_info_ref(env->pluginInfo);

	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
        osync_trace(TRACE_INTERNAL, "The config: %p", config);

	/* create data sync object */
	env->dsObject1 = smlDataSyncNew(
				SML_SESSION_TYPE_CLIENT,
				SML_TRANSPORT_HTTP_CLIENT,
				&error);
	if (!env->dsObject1)
		goto error;
	env->dsObject2 = smlDataSyncNew(
				SML_SESSION_TYPE_CLIENT,
				SML_TRANSPORT_HTTP_CLIENT,
				&error);
	if (!env->dsObject2)
		goto error;

	/* configure the instance */
	if (!parse_config(SML_TRANSPORT_HTTP_CLIENT, env->dsObject1, config, oerror))
		goto error_free_env;
	if (!parse_config(SML_TRANSPORT_HTTP_CLIENT, env->dsObject2, config, oerror))
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

	osync_objtype_sink_set_functions(mainsink, main_functions, NULL);
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
	smlDataSyncRegisterEventCallback(env->dsObject2, _recv_event, env);
	smlDataSyncRegisterGetAlertTypeCallback(env->dsObject2, _get_alert_type, env);
	smlDataSyncRegisterGetAnchorCallback(env->dsObject2, _get_anchor, env);
	smlDataSyncRegisterSetAnchorCallback(env->dsObject2, _set_anchor, env);
	smlDataSyncRegisterWriteDevInfCallback(env->dsObject2, _write_devinf, env);
	smlDataSyncRegisterReadDevInfCallback(env->dsObject2, _read_devinf, env);
	smlDataSyncRegisterHandleRemoteDevInfCallback(env->dsObject2, _handle_remote_devinf, env);
	smlDataSyncRegisterChangeStatusCallback(env->dsObject2, _recv_change_status);

	/* configure databases */
	if (!ds_client_init_databases(env, info, oerror))
		goto error_free_env;

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

osync_bool syncml_http_client_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
	return discover("syncml-http-client", data, info, error);
}
