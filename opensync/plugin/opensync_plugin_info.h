#ifndef OPENSYNC_PLUGIN_INFO_H_
#define OPENSYNC_PLUGIN_INFO_H_

OSyncPluginInfo *osync_plugin_info_new(OSyncError **error);
void osync_plugin_info_ref(OSyncPluginInfo *info);
void osync_plugin_info_unref(OSyncPluginInfo *info);

void osync_plugin_info_set_loop(OSyncPluginInfo *info, void *loop);
void *osync_plugin_info_get_loop(OSyncPluginInfo *info);

void osync_plugin_info_set_config(OSyncPluginInfo *info, const char *config);
const char *osync_plugin_info_get_config(OSyncPluginInfo *info);

void osync_plugin_info_set_configdir(OSyncPluginInfo *info, const char *configdir);
const char *osync_plugin_info_get_configdir(OSyncPluginInfo *info);

OSyncObjTypeSink *osync_plugin_info_find_objtype(OSyncPluginInfo *info, const char *name);
void osync_plugin_info_add_objtype(OSyncPluginInfo *info, OSyncObjTypeSink *sink);
int osync_plugin_info_num_objtypes(OSyncPluginInfo *info);
OSyncObjTypeSink *osync_plugin_info_nth_objtype(OSyncPluginInfo *info, int nth);

OSyncObjTypeSink *osync_plugin_info_get_sink(OSyncPluginInfo *info);
void osync_plugin_info_set_sink(OSyncPluginInfo *info, OSyncObjTypeSink *sink);

#endif /*OPENSYNC_PLUGIN_INFO_H_*/
