#include <glib.h>
#include <gmodule.h>
#include <string.h>
#include <glib/gprintf.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

osync_bool osync_plugin_load_dir(OSyncEnv *os_env, char *path);
OSyncUserInfo *_osync_get_user(void);
osync_bool _osync_open_xml_file(xmlDocPtr *doc, xmlNodePtr *cur, char *path, char *topentry);
osync_bool osync_conv_find_shortest_path(GList *vertices, OSyncObjFormat *start, OSyncObjFormat *end, GList **retlist);
void osync_error_set_vargs(OSyncError *error, OSyncErrorType type, const char *format, va_list args);

#define osync_assert(x, msg) if (!(x)) { printf("** ERROR **: file %s: line %i (%s):\n%s\n", __FILE__, __LINE__, __FUNCTION__, msg); abort();}
#define segfault_me char **blablabla = NULL; *blablabla = "test";

struct OSyncEnv {
	GList *plugins;
	GList *groups;
	char *configdir;
	char *plugindir;
	OSyncConvEnv *conv_env;
};

struct OSyncHashTable {
	DB *dbhandle;
	GHashTable *used_entries;
};

struct OSyncMember {
	//char *name;
	char *configdir;
	OSyncPlugin *plugin;
	void *enginedata;
	void *plugindata;
	OSyncMemberFunctions *memberfunctions;
	OSyncGroup *group;
	GList *entries;
	unsigned int id;
};

struct OSyncContext {
	OSyncEngCallback callback_function;
	void *calldata;
	OSyncMember *member;
	OSyncError error;
	osync_bool success;
};

struct OSyncGroup {
	gchar *name;
	GList *members;
	gchar *configdir;
	OSyncEnv *env;
	DB_ENV *dbenv;
	void *data;
};

struct OSyncPlugin {
	    GModule *real_plugin;
        gchar *path;
        OSyncPluginInfo info;
};

struct OSyncChange {
	char *uid; //unique resource locater
	char *hash; //Hash value to identify changes
	char *data; //The data of the object
	int size;
	osync_bool has_data;
	OSyncObjType *objtype;
	GList *objformats;
	OSyncMember *member;
	OSyncChangeType changetype;
	//OSyncMappingEntry *entry;
	void *engine_data;
	unsigned long id;
	int refcount;
	OSyncMapping *mapping;
};

struct OSyncMapping {
	GList *entries;
	OSyncChange *master;
	void *engine_data;
	unsigned long id;
	OSyncMappingTable *table;
};

struct OSyncMappingTable {
	GList *mappings;
	DB *maptable;
	DB *entrytable;
	char *db_path;
	OSyncGroup *group;
	GList *unmapped;
};

struct OSyncConvEnv {
	GList *objtypes;
	GList *objformats;
	GList *converters;
	char *pluginpath;
	OSyncObjFormat *common_format;
};

struct OSyncObjType {
	char *name;
	GList *formats;
	GList *converters;
	OSyncConvEnv *env;
};

struct OSyncFormatProperty {
	char *name;
	void *add_func;
	void *remove_func;
	OSyncFormatDetectFunc detect_func;
};

struct OSyncObjFormat {
	OSyncObjType *objtype;
	char *name;
	OSyncFormatCompareFunc cmp_func;
	OSyncFormatMergeFunc merge_func;
	OSyncFormatDetectFunc detect_func;
	OSyncFormatDuplicateFunc duplicate_func;
	OSyncFormatCreateFunc create_func;
	GList *properties;
};

struct OSyncFormatConverter {
	OSyncObjFormat *source_format;
	OSyncObjFormat *target_format;
	OSyncFormatConvertFunc convert_func;
	ConverterType type;
};

#include <sys/stat.h>
