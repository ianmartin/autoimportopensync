OSyncPlugin *osync_plugin_new(OSyncEnv *env);
void osync_plugin_free(OSyncPlugin *plugin);

OSyncPlugin *osync_plugin_load(OSyncEnv *env, const char *path, OSyncError **error);
void osync_plugin_unload(OSyncPlugin *plugin);

OSyncPlugin *osync_plugin_from_name(OSyncEnv *osinfo, const char *name);
const char *osync_plugin_get_name(OSyncPlugin *plugin);
const char *osync_plugin_get_longname(OSyncPlugin *plugin);
const char *osync_plugin_get_description(OSyncPlugin *plugin);
OSyncPluginTimeouts osync_plugin_get_timeouts(OSyncPlugin *plugin);

void *osync_plugin_get_function(OSyncPlugin *plugin, const char *name, OSyncError **error);
void osync_plugin_accept_objtype(OSyncPluginInfo *info, const char *objtypestr);
void osync_plugin_accept_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr);
void osync_plugin_set_commit_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, osync_bool (* commit_change) (OSyncContext *, OSyncChange *));
void osync_plugin_set_access_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, osync_bool (* access) (OSyncContext *, OSyncChange *));
