#include "syncml_common.h"

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

	SmlPluginEnv *env = syncml_init(
				SML_SESSION_TYPE_CLIENT,
				SML_TRANSPORT_HTTP_CLIENT,
				plugin, info, oerror);
	if (!env)
		goto error;

	osync_trace(TRACE_EXIT, "%s - %p", __func__, env);
	return env;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s - %s", __func__, osync_error_print(oerror));
	return NULL;
}

osync_bool syncml_http_client_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
	return discover("syncml-http-client", data, info, error);
}
