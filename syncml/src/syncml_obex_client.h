 
#ifndef _SYNCML_PLUGIN_OBEX_CLIENT_H
#define _SYNCML_PLUGIN_OBEX_CLIENT_H

extern void *syncml_obex_client_init(
		OSyncPlugin *plugin, 
		OSyncPluginInfo *info, 
		OSyncError **error);

extern osync_bool syncml_obex_client_discover(
		void *data, OSyncPluginInfo *info, 
		OSyncError **error);

#endif //_SYNCML_PLUGIN_OBEX_CLIENT_H
