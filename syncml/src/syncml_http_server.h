 
#ifndef _SYNCML_PLUGIN_HTTP_SERVER_H
#define _SYNCML_PLUGIN_HTTP_SERVER_H

extern void *syncml_http_server_init(
		OSyncPlugin *plugin, 
		OSyncPluginInfo *info, 
		OSyncError **error);

extern osync_bool syncml_http_server_discover(
		void *data, OSyncPluginInfo *info, 
		OSyncError **error);

#endif //_SYNCML_PLUGIN_HTTP_SERVER_H
