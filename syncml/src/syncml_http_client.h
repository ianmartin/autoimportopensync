 
#ifndef _SYNCML_PLUGIN_HTTP_CLIENT_H
#define _SYNCML_PLUGIN_HTTP_CLIENT_H

void *syncml_http_client_init(
		OSyncPlugin *plugin, 
		OSyncPluginInfo *info, 
		OSyncError **error);

osync_bool syncml_http_client_discover(
		void *data, OSyncPluginInfo *info, 
		OSyncError **error);

#endif //_SYNCML_PLUGIN_HTTP_CLIENT_H
