
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS
struct OSyncEnv {
	GList *groups;
	osync_bool is_initialized;
	GHashTable *options;
	
	char *groupsdir;
	
	GList *plugins; //The registered plugins
	GList *formatplugins; //The registered formats
	GList *modules; //The loaded modules
	
	GList *format_templates;
	GList *converter_templates;
	GList *objtype_templates;
	GList *data_detectors;
	GList *filter_functions;
	GList *extension_templates;
	
	GModule *current_module;
};
#endif

osync_bool _osync_open_xml_file(xmlDocPtr *doc, xmlNodePtr *cur, const char *path, const char *topentry, OSyncError **error);
long long int _osync_env_create_group_id(OSyncEnv *env);
