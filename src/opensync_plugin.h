OSyncPluginFunctions *osync_pluginfunctions_new(void);
OSyncPlugin *osync_plugin_new(void);
OSyncPlugin *osync_plugin_from_name(OSyncEnv *osinfo, const char *name);
osync_bool osync_plugin_load_info(OSyncPlugin *plugin, char *path);
OSyncPluginFunctions *osync_plugin_load_functions(OSyncPlugin *plugin);
const char *osync_plugin_get_name(OSyncPlugin *plugin);
void *osync_plugin_get_function(OSyncPlugin *plugin, char *name);
