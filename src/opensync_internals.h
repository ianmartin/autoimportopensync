#include <glib.h>
#include <gmodule.h>
#include <string.h>
#include <glib/gprintf.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#define osync_assert(x, msg) if (!(x)) { fprintf(stderr, "%s:%i:E:%s: %s\n", __FILE__, __LINE__, __FUNCTION__, msg); abort();}
#define segfault_me char **blablabla = NULL; *blablabla = "test";

/** Hook types and macros
 *
 *@{
 */
typedef void (*OSyncHookFnChange)(OSyncChange *);
typedef GList/* OSyncHookFnChange * */ *OSyncChangeHook;

/** Call the hook functions of a hook */
#define osync_run_change_hook(hook, change) \
	do { \
		GList *i; \
		for (i = hook; i; i = i->next) { \
			OSyncHookFnChange f = i->data; \
			f(change); \
		} \
	} while (0)

/** @} */

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
	GList *plugins;
	GList *groups;
	char *configdir;
	char *plugindir;
	osync_bool is_initialized;
};

struct OSyncHashTable {
	OSyncDB *dbhandle;
	GHashTable *used_entries;
};

struct OSyncMember {
	char *configdir;
	char *configdata;
	int configsize;
	OSyncPlugin *plugin;
	void *enginedata;
	void *plugindata;
	OSyncMemberFunctions *memberfunctions;
	OSyncGroup *group;
	GList *entries;
	long long int id;
	GList *format_sinks;
	GList *objtype_sinks;

	OSyncChangeHook before_convert_hook;
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
	void *data;
	OSyncFormatEnv *conv_env;
	long long int id;
	OSyncChangeHook before_convert_hook;
};

struct OSyncPlugin {
	GModule *real_plugin;
	gchar *path;
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
	OSyncObjFormat *format;
	OSyncMember *member;
	OSyncChangeType changetype;
	void *engine_data;
	long long int id;
	int refcount;
	OSyncMapping *mapping;
};

struct OSyncMapping {
	GList *entries;
	OSyncChange *master;
	void *engine_data;
	long long int id;
	OSyncMappingTable *table;
};

struct OSyncMappingTable {
	GList *mappings;
	OSyncDB *entrytable;
	char *db_path;
	OSyncGroup *group;
	GList *unmapped;
};

#include "opensync_env_internals.h"
#include "opensync_error_internals.h"
#include "opensync_db_internals.h"
#include "opensync_format_internals.h"
#include "opensync_member_internals.h"
#include "opensync_plugin_internals.h"
#include <sys/stat.h>
