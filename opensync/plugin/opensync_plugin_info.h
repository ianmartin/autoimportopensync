#ifndef OPENSYNC_PLUGIN_INFO_H_
#define OPENSYNC_PLUGIN_INFO_H_

OSYNC_EXPORT OSyncPluginInfo *osync_plugin_info_new(OSyncError **error);
OSYNC_EXPORT void osync_plugin_info_ref(OSyncPluginInfo *info);
OSYNC_EXPORT void osync_plugin_info_unref(OSyncPluginInfo *info);

OSYNC_EXPORT void osync_plugin_info_set_loop(OSyncPluginInfo *info, void *loop);
OSYNC_EXPORT void *osync_plugin_info_get_loop(OSyncPluginInfo *info);

OSYNC_EXPORT void osync_plugin_info_set_config(OSyncPluginInfo *info, const char *config);
OSYNC_EXPORT const char *osync_plugin_info_get_config(OSyncPluginInfo *info);

OSYNC_EXPORT void osync_plugin_info_set_configdir(OSyncPluginInfo *info, const char *configdir);
OSYNC_EXPORT const char *osync_plugin_info_get_configdir(OSyncPluginInfo *info);

OSYNC_EXPORT OSyncObjTypeSink *osync_plugin_info_find_objtype(OSyncPluginInfo *info, const char *name);
OSYNC_EXPORT void osync_plugin_info_add_objtype(OSyncPluginInfo *info, OSyncObjTypeSink *sink);
OSYNC_EXPORT int osync_plugin_info_num_objtypes(OSyncPluginInfo *info);
OSYNC_EXPORT OSyncObjTypeSink *osync_plugin_info_nth_objtype(OSyncPluginInfo *info, int nth);

OSYNC_EXPORT OSyncObjTypeSink *osync_plugin_info_get_main_sink(OSyncPluginInfo *info);
OSYNC_EXPORT void osync_plugin_info_set_main_sink(OSyncPluginInfo *info, OSyncObjTypeSink *sink);

OSYNC_EXPORT OSyncFormatEnv *osync_plugin_info_get_format_env(OSyncPluginInfo *info);
OSYNC_EXPORT void osync_plugin_info_set_format_env(OSyncPluginInfo *info, OSyncFormatEnv *env);

OSYNC_EXPORT OSyncObjTypeSink *osync_plugin_info_get_sink(OSyncPluginInfo *info);
OSYNC_EXPORT void osync_plugin_info_set_sink(OSyncPluginInfo *info, OSyncObjTypeSink *sink);

OSYNC_EXPORT void osync_plugin_info_set_groupname(OSyncPluginInfo *info, const char *groupname);
OSYNC_EXPORT const char *osync_plugin_info_get_groupname(OSyncPluginInfo *info);

#endif /*OPENSYNC_PLUGIN_INFO_H_*/
