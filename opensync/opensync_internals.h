#include <glib.h>
#include <gmodule.h>
#include <string.h>
#include <glib/gprintf.h>

#include "config.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <errno.h>
extern int errno;

#define osync_assert(x, msg) if (!(x)) { fprintf(stderr, "%s:%i:E:%s: %s\n", __FILE__, __LINE__, __FUNCTION__, msg); abort();}
#define segfault_me char **blablabla = NULL; *blablabla = "test";

#define osync_return_if_fail(condition) do {                                            \
  if (!(condition)) {                                                                   \
    osync_debug ("ASSERT", 0, "%i: Assertion failed: \"%s\" in %s:%i:%s", getpid (), #condition, __FILE__, __LINE__, __FUNCTION__);  \
    return;                                                                             \
  } } while (0)

#define osync_return_val_if_fail(condition, val) do {                                   \
  if (!(condition)) {                                                                   \
    return (val);                                                                       \
  } } while (0)

/**
 * @defgroup PublicAPI Public APIs
 * @brief Available public APIs
 * 
 */

/**
 * @defgroup OSyncPublic OpenSync Public API
 * @ingroup PublicAPI
 * @brief The public API of opensync
 * 
 * This gives you an insight in the public API of opensync.
 * 
 */

/**
 * @defgroup PrivateAPI Private APIs
 * @brief Available private APIs
 * 
 */

/**
 * @defgroup OSyncPrivate OpenSync Private API
 * @ingroup PrivateAPI
 * @brief The private API of opensync
 * 
 * This gives you an insight in the private API of opensync.
 * 
 */

typedef struct OSyncDB OSyncDB;

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

struct OSyncHashTable {
	OSyncDB *dbhandle;
	GHashTable *used_entries;
};

struct OSyncMember {
	long long int id;
	char *configdir;
	char *configdata;
	int configsize;
	OSyncPlugin *plugin;
	OSyncMemberFunctions *memberfunctions;
	OSyncGroup *group;
	
	void *enginedata;
	void *plugindata;
	
	GList *format_sinks;
	GList *objtype_sinks;
	char *pluginname;
	
	//For the filters
	GList *accepted_objtypes;
	GList *filters;

};

struct OSyncContext {
	OSyncEngCallback callback_function;
	void *calldata;
	OSyncMember *member;
	osync_bool success;
};

struct OSyncGroup {
	char *name;
	GList *members;
	char *configdir;
	OSyncEnv *env;
	OSyncFormatEnv *conv_env;
	void *data;
	long long int id;
	int lock_fd;
	GList *filters;
	char *changes_path;
	OSyncDB *changes_db;
};

struct OSyncPlugin {
	GModule *real_plugin;
	char *path;
	OSyncPluginInfo info;
	GList *accepted_objtypes;
	OSyncEnv *env;
};

struct OSyncChange {
	char *uid; //unique resource locater
	char *hash; //Hash value to identify changes
	char *data; //The data of the object
	int size;
	osync_bool has_data;
	/*FIXME: do we need this field, as OSyncObjFormat has
	 * a objtype field set?
	 */
	OSyncObjType *objtype;
	char *objtype_name;
	OSyncObjFormat *format;
	char *format_name;
	
	OSyncMember *member;
	OSyncChangeType changetype;
	void *engine_data;
	long long int id;
	int refcount;
	long long int mappingid;
	OSyncDB *changes_db;
	
	//For the filters
	char *destobjtype;
	char *sourceobjtype;
	OSyncMember *sourcemember;
	osync_bool is_detected;
};

#include "opensync_env_internals.h"
#include "opensync_error_internals.h"
#include "opensync_db_internals.h"
#include "opensync_format_internals.h"
#include "opensync_member_internals.h"
#include "opensync_plugin_internals.h"
#include "opensync_filter_internals.h"
#include <sys/stat.h>
