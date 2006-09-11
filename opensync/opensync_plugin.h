
/*! @brief The functions that can be called on a plugin
 * @ingroup OSyncPluginAPI 
 **/
typedef struct OSyncPluginFunctions {
	/** The function that gets a configuration file for this plugin (optional) */
	osync_bool (* get_config) (char *, char **, int *);
	/** The function that stores the configuration file for this plugin (optional) */
	osync_bool (* store_config) (char *, const char *, int);
	/** A function to check if the backend is available. (optional) */
	osync_bool (* is_available) (OSyncError **);
	/** The function to initialize the plugin. */
	void * (* initialize) (OSyncMember *, OSyncError **);
	/** The function to finalize the plugin. The input will be the output of the initialize function */
	void (* finalize) (void *);
	/** Make a connection to your device here */
	void (* connect) (OSyncContext *);
	/** This function gets called if the sync was successfull (Optional) */
	void (* sync_done) (OSyncContext *ctx);
	/** Disconnect from the device */
	void (* disconnect) (OSyncContext *);
	/** Get all available changes here. */
	void (* get_changeinfo) (OSyncContext *);
	/** Get the data for a change here (Optional) */
	void (* get_data) (OSyncContext *, OSyncChange *);
	/** */
} OSyncPluginFunctions;

/*! @brief The timeouts for the asynchronous functions of a plugin
 * 
 * The default timeout it 60 seconds. Set this to 0 if you dont want
 * to have a timeout
 * 
 * @ingroup OSyncPluginAPI 
 **/
typedef struct OSyncPluginTimeouts {
	/** The timeout of the connect function */
	unsigned int connect_timeout;
	/** The timeout of the syncdone function */
	unsigned int sync_done_timeout;
	/** The timeout of the disconnect function */
	unsigned int disconnect_timeout;
	/** The timeout of the function that gets the changes */
	unsigned int get_changeinfo_timeout;
	/** The timeout of the function that gets the data of change */
	unsigned int get_data_timeout;
	/** The timeout of the commit function */
	unsigned int commit_timeout;
	/** The timeout of the function that reads a change */
	unsigned int read_change_timeout;
} OSyncPluginTimeouts;

typedef osync_bool (* OSyncFormatCommitFn) (OSyncContext *, OSyncChange *);
typedef osync_bool (* OSyncFormatAccessFn) (OSyncContext *, OSyncChange *);
typedef void (* OSyncFormatCommittedAllFn) (OSyncContext *);
typedef void (* OSyncFormatReadFn) (OSyncContext *, OSyncChange *);
typedef void (* OSyncFormatBatchCommitFn) (OSyncContext *, OSyncContext **, OSyncChange **);

/*! @brief The functions for accessing formats on a plugin
 * @ingroup OSyncPluginAPI 
 **/
typedef struct OSyncFormatFunctions {
	/** The commit function of this format */
	OSyncFormatCommitFn commit_change;
	/** The function that will be called once the plugin has received all commits */
	OSyncFormatCommittedAllFn committed_all;
	/** This function will be called by opensync with an array of changes to commit */
	OSyncFormatBatchCommitFn batch_commit;
	/** The function to write a change WITHOUT updating hashtables or similar stuff */
	OSyncFormatAccessFn access;
	/** The function to read a change by its uid */
	OSyncFormatReadFn read;
} OSyncFormatFunctions;

/*! @brief Gives information about wether the plugin
 * has to be configured or not
 * 
 * @ingroup OSyncPluginAPI 
 **/
typedef enum {
	/** Plugin has no configuration options */
	NO_CONFIGURATION = 0,
	/** Plugin can be configured, but will accept the default config in the initialize function */
	OPTIONAL_CONFIGURATION = 1,
	/** Plugin must be configured to run correctly */
	NEEDS_CONFIGURATION = 2
} OSyncConfigurationTypes;

/*! @brief Gives information about a plugin
 * @ingroup OSyncPluginAPI 
 **/
typedef struct OSyncPluginInfo {
	/** The version of Opensync API this plugin uses*/
	int version;
	/** The name of this plugin */
	const char *name;
	/** The longer, more descriptive name of the plugin */
	const char *longname;
	/** A short description what the plugin does */
	const char *description;
	/** Is this plugin considered thread-safe? (unused atm) */
	osync_bool is_threadsafe;
	/** The functions of your plugin */
	OSyncPluginFunctions functions;
	/** The timeouts of your plugin */
	OSyncPluginTimeouts timeouts;
	/** Does the plugin have configuration options? */
	OSyncConfigurationTypes config_type;
	/** The pointer to the plugin (for internal use) */
	OSyncPlugin *plugin;
	/** Plugin-specific data
	 *
	 * Can be used when a single module registers many plugins,
	 * such as the python-module plugin
	 */
	void *plugin_data;
} OSyncPluginInfo;

OSyncPlugin *osync_plugin_new(OSyncEnv *env);
OSyncPluginInfo *osync_plugin_new_info(OSyncEnv *env);
void osync_plugin_free(OSyncPlugin *plugin);

osync_bool osync_module_load(OSyncEnv *env, const char *path, OSyncError **error);
osync_bool osync_module_load_dir(OSyncEnv *env, const char *path, osync_bool must_exist, OSyncError **oserror);

OSyncPlugin *osync_plugin_from_name(OSyncEnv *osinfo, const char *name);
const char *osync_plugin_get_name(OSyncPlugin *plugin);
const char *osync_plugin_get_longname(OSyncPlugin *plugin);
const char *osync_plugin_get_description(OSyncPlugin *plugin);
OSyncPluginTimeouts osync_plugin_get_timeouts(OSyncPlugin *plugin);
void *osync_plugin_get_plugin_data(OSyncPlugin *plugin);
const char *osync_plugin_get_path(OSyncPlugin *plugin);

void *osync_plugin_get_function(OSyncPlugin *plugin, const char *name, OSyncError **error);
void osync_plugin_accept_objtype(OSyncPluginInfo *info, const char *objtypestr);
void osync_plugin_accept_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, const char *extension);
void osync_plugin_set_commit_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, OSyncFormatCommitFn commit_change);
void osync_plugin_set_access_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, OSyncFormatAccessFn access_fn);
void osync_plugin_set_read_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, OSyncFormatReadFn read_fn);
void osync_plugin_set_batch_commit_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, OSyncFormatBatchCommitFn batch);
void osync_plugin_set_committed_all_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, OSyncFormatCommittedAllFn committed_all);
