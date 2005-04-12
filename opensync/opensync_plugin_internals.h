
/*! @brief Represent a synchronzation plugin
 * @ingroup OSyncPluginPrivateAPI
 **/
struct OSyncPlugin {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	GModule *real_plugin;
	char *path;
	OSyncPluginInfo info;
	GList *accepted_objtypes;
	OSyncEnv *env;
#endif
};

OSyncObjTypeSink *osync_objtype_sink_from_template(OSyncGroup *group, OSyncObjTypeTemplate *template);
OSyncObjFormatSink *osync_objformat_sink_from_template(OSyncGroup *group, OSyncObjFormatTemplate *template);
OSyncObjTypeTemplate *osync_plugin_find_objtype_template(OSyncPlugin *plugin, const char *objtypestr);
OSyncObjFormatTemplate *osync_plugin_find_objformat_template(OSyncObjTypeTemplate *type_template, const char *objformatstr);
OSyncObjFormatSink *osync_objtype_find_format_sink(OSyncObjTypeSink *sink, const char *formatstr);

void osync_module_unload(OSyncEnv *env, GModule *module);
