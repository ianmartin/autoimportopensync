
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

/*! @brief The functions for accessing formats on a plugin
 * @ingroup OSyncPluginAPI 
 **/
typedef struct OSyncFormatFunctions {
	/** The commit function of this format */
	osync_bool (* commit_change) (OSyncContext *, OSyncChange *);
	/** The function that will be called once the plugin has received all commits */
	void (* committed_all) (void *);
	/** This function will be called by opensync with an array of changes to commit */
	void (* batch_commit) (void *, OSyncContext **, OSyncChange **);
	/** The function to write a change WITHOUT updating hashtables or similar stuff */
	osync_bool (* access) (OSyncContext *, OSyncChange *);
	/** The function to read a change by its uid */
	void (* read) (OSyncContext *, OSyncChange *);
} OSyncFormatFunctions;

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
	osync_bool has_configuration;
	/** The pointer to the plugin (for internal use) */
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
void osync_plugin_set_read_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, void (* read_fn) (OSyncContext *, OSyncChange *));
void osync_plugin_set_batch_commit_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, void (* batch) (void *, OSyncContext **, OSyncChange **));
void osync_plugin_set_committed_all_objformat(OSyncPluginInfo *info, const char *objtypestr, const char *formatstr, void (* committed_all) (void *));
