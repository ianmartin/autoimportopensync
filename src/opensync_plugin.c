#include <opensync.h>
#include "opensync_internals.h"
#include "opensync_plugin.h"


OSyncPlugin *osync_plugin_new(void)
{
        OSyncPlugin *plugin = g_malloc0(sizeof(OSyncPlugin));
        memset(&(plugin->info), 0, sizeof(plugin->info));
        memset(&(plugin->info.functions), 0, sizeof(plugin->info.functions));
        plugin->info.accepted_objtypes = g_malloc0(sizeof(OSyncPlgAcceptedTypes));
        return plugin;
}

OSyncPlugin *osync_plugin_from_name(OSyncEnv *osinfo, const char *name)
{
	OSyncPlugin *plugin;
	int i;
	for (i = 0; i < osync_env_num_plugins(osinfo); i++) {
		plugin = osync_env_get_nth_plugin(osinfo, i);
		if (g_ascii_strcasecmp(plugin->info.name, name) == 0) {
			return plugin;
		}
	}
	return NULL;
}

void *osync_plugin_get_function(OSyncPlugin *plugin, char *name)
{
	void *function;
	if (!g_module_symbol (plugin->real_plugin, name, &function)) {
		osync_debug("OSPLG", 0, "Unable to locate symbol: %s", g_module_error());
		return NULL;
	}
	return function;
}

OSyncPlgAcceptedType *osync_plugin_find_accepted_type(OSyncPluginInfo *info, const char *name)
{
	GList *element;

	g_assert(info);
	g_assert(name);
	
	for (element = info->accepted_objtypes->types; element; element = element->next) {
		OSyncPlgAcceptedType *type = element->data;
		if (!strcmp(type->name, name))
			return type;
	}
	return NULL;
}

static OSyncPlgAcceptedFormat *osync_plugin_find_accepted_format(OSyncPlgAcceptedType *type, const char *name)
{
	GList *element;

	g_assert(type);
	g_assert(name);
	
	for (element = type->formats; element; element = element->next) {
		OSyncPlgAcceptedFormat *format = element->data;
		if (!strcmp(format->name, name))
			return format;
	}
	return NULL;
}


/*FIXME: use 'const char *objformat' or 'OSyncObjFormat *objformat' ? */
/*FIXME: Should the objformat names be globally unique or unique on a objtype?
 * if unique on a objtype, we need 'objtype, objformat' as parameters. If
 * globally unique, we only need a 'objformat' parameter
 */
OSyncFormatFunctions *osync_plugin_get_objformat_functions(OSyncPlugin *plugin, const char *type, const char *format)
{
	OSyncPlgAcceptedType *plgtype = osync_plugin_find_accepted_type(&plugin->info, type);
	OSyncPlgAcceptedFormat *plgformat = osync_plugin_find_accepted_format(plgtype, format);
	return plgformat->functions;
}

osync_bool osync_plugin_register_accepted_objtype(OSyncPluginInfo *info, const char *name)
{
	OSyncPlgAcceptedType *type;
	if (!(type = osync_plugin_find_accepted_type(info, name))) {
		type = g_malloc0(sizeof(OSyncPlgAcceptedType));
		g_assert(type);
		type->name = g_strdup(name);
		info->accepted_objtypes->types = g_list_append(info->accepted_objtypes->types, type);
	}
	return TRUE;
}

osync_bool osync_plugin_register_accepted_objformat(OSyncPluginInfo *info, const char *type, const char *format,
							OSyncFormatFunctions *functions)
{
	OSyncPlgAcceptedType *plgtype;
	OSyncPlgAcceptedFormat *plgformat;

	plgtype = osync_plugin_find_accepted_type(info, type);
	if (!plgtype)
		return FALSE;

	if (!(plgformat= osync_plugin_find_accepted_format(plgtype, format))) {
		plgformat = g_malloc0(sizeof(OSyncPlgAcceptedFormat));
		g_assert(plgformat);
		plgformat->name = g_strdup(format);
		plgformat->functions = functions;
		plgtype->formats = g_list_append(plgtype->formats, plgformat);
	}
	return TRUE;
}


osync_bool osync_plugin_load_info(OSyncPlugin *plugin, char *path)
{ 
	/* Check if this platform supports dynamic
	 * loading of modules */
	if (!g_module_supported()) {
		osync_debug("OSPLG", 0, "This platform does not support loading of modules");
		return FALSE;
	}

	/* Try to open the module or fail if an error occurs */
	plugin->real_plugin = g_module_open(path, 0);
	if (!plugin->real_plugin) {
		osync_debug("OSPLG", 0, "Unable to open plugin: %s", g_module_error());
		return FALSE;
	}
	
	void (* fct_info)(OSyncPluginInfo *info);
	if (!(fct_info = osync_plugin_get_function(plugin, "get_info"))) {
		osync_debug("OSPLG", 0, "Unable to open plugin: Missing symbol get_info");
		return FALSE;
	}
	
	fct_info(&(plugin->info));
	plugin->path = path;
	
	return TRUE;
}

osync_bool osync_plugin_load_dir(OSyncEnv *os_env, char *path)
{
	GDir *dir;
	GError *error = NULL;
	osync_debug("OSPLG", 3, "Trying to open plugin directory %s", path);
	
	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		return FALSE;
	}
	
	dir = g_dir_open(path, 0, &error);
	if (error) {
		osync_debug("OSPLG", 0, "Unable to open plugin directory %s: %s", path, error->message);
		g_error_free (error);
		return FALSE;
	}
  
	if (dir) {
		const gchar *de = NULL;
		while ((de = g_dir_read_name(dir))) {
			
			char *filename = NULL;
			filename = g_strdup_printf ("%s/%s", path, de);
			
			if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR) || g_file_test(filename, G_FILE_TEST_IS_SYMLINK) || !g_pattern_match_simple("*.la", filename)) {
				continue;
			}
			
			/* Try to open the syncgroup dir*/
			OSyncPlugin *plugin = osync_plugin_new();
			if (osync_plugin_load_info(plugin, filename) && !osync_plugin_from_name(os_env, plugin->info.name)) {
				os_env->plugins = g_list_append(os_env->plugins, plugin);
			} else {
				//Free FIXME
			}
		}
	}
	return TRUE;
}

const char *osync_plugin_get_name(OSyncPlugin *plugin)
{
	g_assert(plugin);
	return plugin->info.name;
}

/*void osync_plugin_set_name(OSyncPlugin *plugin, char *name)
{
	g_assert(plugin);
	plugin->info.name = name;
}*/
