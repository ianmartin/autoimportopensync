#include <opensync.h>
#include "opensync_internals.h"
#include "opensync_plugin.h"


OSyncPlugin *osync_plugin_new(void)
{
        OSyncPlugin *plugin = g_malloc0(sizeof(OSyncPlugin));
        memset(&(plugin->info), 0, sizeof(plugin->info));
        memset(&(plugin->info.functions), 0, sizeof(plugin->info.functions));
        plugin->info.plugin = plugin;
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

OSyncObjTypeSink *osync_objtype_sink_from_template(OSyncGroup *group, OSyncObjTypeTemplate *template)
{
	OSyncObjTypeSink *sink = g_malloc0(sizeof(OSyncObjTypeSink));
	OSyncObjType *type = osync_conv_find_objtype(group->conv_env, template->name);
	if (!type) {
		osync_debug("OSYNC", 0, "Unable to find objtype named %s to create objtype sink", template->name);
		return NULL;
	}
	sink->objtype = type;
	sink->enabled = TRUE;
	sink->write = TRUE;
	sink->read = TRUE;
	return sink;
}

OSyncObjFormatSink *osync_objformat_sink_from_template(OSyncGroup *group, OSyncObjFormatTemplate *template)
{
	OSyncObjFormatSink *sink = g_malloc0(sizeof(OSyncObjFormatSink));
	OSyncObjFormat *format = osync_conv_find_objformat(group->conv_env, template->name);
	if (!format)
		return NULL;
	sink->format = format;
	sink->functions.commit_change = template->commit_change;
	sink->functions.access = template->access;
	return sink;
}

OSyncObjTypeTemplate *osync_plugin_find_objtype_template(OSyncPlugin *plugin, const char *objtypestr)
{
	GList *o;
	for (o = plugin->accepted_objtypes; o; o = o->next) {
		OSyncObjTypeTemplate *template = o->data;
		if (!strcmp(template->name, objtypestr))
			return template;
	}
	return NULL;
}

OSyncObjFormatTemplate *osync_plugin_find_objformat_template(OSyncObjTypeTemplate *type_template, const char *objformatstr)
{
	GList *f;
	for (f = type_template->formats; f; f = f->next) {
		OSyncObjFormatTemplate *template = f->data;
		if (!strcmp(template->name, objformatstr))
			return template;
	}
	return NULL;
}

void osync_plugin_accept_objtype(OSyncPluginInfo *info, const char *objtypestr)
{
	OSyncObjTypeTemplate *template = g_malloc0(sizeof(OSyncObjTypeTemplate));
	template->name = g_strdup(objtypestr);
	info->plugin->accepted_objtypes = g_list_append(info->plugin->accepted_objtypes, template);
}

void osync_plugin_accept_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr)
{
	OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
	osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
	OSyncObjFormatTemplate *format_template = g_malloc0(sizeof(OSyncObjFormatTemplate));
	format_template->name = g_strdup(formatstr);
	template->formats = g_list_append(template->formats, format_template);
}

void osync_plugin_set_commit_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, osync_bool (* commit_change) (OSyncContext *, OSyncChange *))
{
	OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
	osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
	OSyncObjFormatTemplate *format_template = osync_plugin_find_objformat_template(template, formatstr);
	osync_assert(format_template, "Unable to set commit function. Did you forget to add the objformat?");
	format_template->commit_change = commit_change;
}

void osync_plugin_set_access_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, osync_bool (* access) (OSyncContext *, OSyncChange *))
{
	OSyncObjTypeTemplate *template = osync_plugin_find_objtype_template(info->plugin, objtypestr);
	osync_assert(template, "Unable to accept objformat. Did you forget to add the objtype?");
	OSyncObjFormatTemplate *format_template = osync_plugin_find_objformat_template(template, formatstr);
	osync_assert(format_template, "Unable to set commit function. Did you forget to add the objformat?");
	format_template->access = access;
}

OSyncObjFormatSink *osync_objtype_find_format_sink(OSyncObjTypeSink *sink, const char *formatstr)
{
	GList *f;
	for (f = sink->formatsinks; f; f = f->next) {
		OSyncObjFormatSink *sink = f->data;
		if (!strcmp(sink->format->name, formatstr))
			return sink;
	}
	return NULL;
}
