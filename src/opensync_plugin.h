OSyncPlugin *osync_plugin_new(void);
void osync_plugin_free(OSyncPlugin *plugin);

osync_bool osync_plugin_load_info(OSyncPlugin *plugin, const char *path);
void osync_plugin_unload(OSyncPlugin *plugin);

OSyncPlugin *osync_plugin_from_name(OSyncEnv *osinfo, const char *name);
//OSyncPluginFunctions *osync_plugin_load_functions(OSyncPlugin *plugin);
const char *osync_plugin_get_name(OSyncPlugin *plugin);
void *osync_plugin_get_function(OSyncPlugin *plugin, char *name);
void osync_plugin_accept_objtype(OSyncPluginInfo *info, const char *objtypestr);
void osync_plugin_accept_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr);
void osync_plugin_set_commit_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, osync_bool (* commit_change) (OSyncContext *, OSyncChange *));
void osync_plugin_set_access_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, osync_bool (* access) (OSyncContext *, OSyncChange *));
