
/**
 * @defgroup OSyncPluginAPI OpenSync Plugin
 * @ingroup OSyncPublic
 * @brief Functions to register and manage plugins
 * 
 */
/*@{*/

/*! @brief The functions that can be called on a plugin */
typedef struct OSyncPluginFunctions {
	osync_bool (* get_config) (char *, char **, int *);
	osync_bool (* store_config) (char *, const char *, int);
	void * (* initialize) (OSyncMember *, OSyncError **);
	void (* finalize) (void *);
	void (* connect) (OSyncContext *);
	void (* sync_done) (OSyncContext *ctx);
	void (* disconnect) (OSyncContext *);
	void (* get_changeinfo) (OSyncContext *);
	void (* get_data) (OSyncContext *, OSyncChange *);
} OSyncPluginFunctions;

/*! @brief The timeouts for the asynchronous functions of a plugin*/
typedef struct OSyncPluginTimeouts {
	unsigned int connect_timeout;
	unsigned int sync_done_timeout;
	unsigned int disconnect_timeout;
	unsigned int get_changeinfo_timeout;
	unsigned int get_data_timeout;
	unsigned int commit_timeout;
} OSyncPluginTimeouts;

/*! @brief The functions for accessing formats on a plugin */
typedef struct OSyncFormatFunctions {
	osync_bool (* commit_change) (OSyncContext *, OSyncChange *);
	osync_bool (* access) (OSyncContext *, OSyncChange *);
} OSyncFormatFunctions;

/*! @brief Gives information about a plugin */
typedef struct OSyncPluginInfo {
	int version;
	const char *name;
	const char *longname;
	const char *description;
	osync_bool is_threadsafe;
	OSyncPluginFunctions functions;
	OSyncPluginTimeouts timeouts;
	OSyncPlugin *plugin;
} OSyncPluginInfo;

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
void osync_plugin_accept_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, const char *extension);
void osync_plugin_set_commit_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, osync_bool (* commit_change) (OSyncContext *, OSyncChange *));
void osync_plugin_set_access_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, osync_bool (* access_fn) (OSyncContext *, OSyncChange *));

/*@}*/
