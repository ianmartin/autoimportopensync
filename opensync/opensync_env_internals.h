
#ifndef DOXYGEN_SHOULD_SKIP_THIS
struct OSyncEnv {
	GList *groups;
	osync_bool is_initialized;
	GHashTable *options;
	
	char *groupsdir;
	
	GList *plugins;
	GList *formatplugins;
	
	GList *format_templates;
	GList *converter_templates;
	GList *objtype_templates;
	GList *data_detectors;
	GList *filter_functions;
	GList *extension_templates;
};
#endif

osync_bool _osync_open_xml_file(xmlDocPtr *doc, xmlNodePtr *cur, const char *path, const char *topentry, OSyncError **error);
long long int _osync_env_create_group_id(OSyncEnv *env);

