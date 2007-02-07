%{
#include <opensync/opensync-plugin.h>
%}

typedef enum {} OSyncConfigurationType;

%constant int PLUGIN_NO_CONFIGURATION = OSYNC_PLUGIN_NO_CONFIGURATION;
%constant int PLUGIN_OPTIONAL_CONFIGURATION = OSYNC_PLUGIN_OPTIONAL_CONFIGURATION;
%constant int PLUGIN_NEEDS_CONFIGURATION = OSYNC_PLUGIN_NEEDS_CONFIGURATION;


typedef struct {} OSyncPlugin;

%feature("ref")   OSyncPlugin "osync_plugin_ref($this);"
%feature("unref") OSyncPlugin "osync_plugin_unref($this);"

%extend OSyncPlugin {
	OSyncPlugin(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

	OSyncPlugin() {
		OSyncError *err = NULL;
		OSyncPlugin *plugin = osync_plugin_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return plugin;
		}

	const char *get_name() {
		return osync_plugin_get_name(self);
	}

	void set_name(const char *name) {
		osync_plugin_set_name(self, name);
	}

	const char *get_longname() {
		return osync_plugin_get_longname(self);
	}

	void set_longname(const char *longname) {
		osync_plugin_set_longname(self, longname);
	}

	const char *get_description() {
		return osync_plugin_get_description(self);
	}

	void set_description(const char *description) {
		osync_plugin_set_description(self, description);
	}

	OSyncConfigurationType get_config_type() {
		return osync_plugin_get_config_type(self);
	}

	void set_config_type(OSyncConfigurationType config_type) {
		osync_plugin_set_config_type(self, config_type);
	}

	void *initialize(OSyncPluginInfo *info) {
		OSyncError *err = NULL;
		void *ret = osync_plugin_initialize(self, info, &err);
		raise_exception_on_error(err);
		return ret;
	}

	void finalize(void *data) {
		osync_plugin_finalize(self, data);
	}

	void discover(void *data, OSyncPluginInfo *info) {
		OSyncError *err = NULL;
		osync_bool ret = osync_plugin_discover(self, data, info, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_plugin_discover failed but did not set error code");
	}

	osync_bool is_usable() {
		OSyncError *err = NULL;
		osync_bool ret = osync_plugin_is_usable(self, &err);
		raise_exception_on_error(err);
		return ret;
	}

%pythoncode %{
	name = property(get_name, set_name)
	longname = property(get_longname, set_longname)
	description = property(get_description, set_description)
	config_type = property(get_config_type, set_config_type)
	is_usable = property(is_usable)
%}
};


typedef struct {} OSyncPluginEnv;

%extend OSyncPluginEnv {
	OSyncPluginEnv(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

	OSyncPluginEnv() {
		OSyncError *err = NULL;
		OSyncPluginEnv *env = osync_plugin_env_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return env;
	}

	~OSyncPluginEnv() {
		osync_plugin_env_free(self);
	}
	
	void load(const char *path) {
		OSyncError *err = NULL;
		osync_bool ret = osync_plugin_env_load(self, path, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_plugin_env_load failed but did not set error code");
	}

	void register_plugin(OSyncPlugin *plugin) {
		osync_plugin_env_register_plugin(self, plugin);
	}
	
	void load_module(const char *filename) {
		OSyncError *err = NULL;
		osync_bool ret = osync_plugin_env_load_module(self, filename, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_plugin_env_load_module failed but did not set error code");
	}

	OSyncPlugin *find_plugin(const char *name) {
		return osync_plugin_env_find_plugin(self, name);
	}
	
	int num_plugins() {
		return osync_plugin_env_num_plugins(self);
	}

	OSyncPlugin *nth_plugin(int nth) {
		return osync_plugin_env_nth_plugin(self, nth);
	}

	osync_bool plugin_is_usable(const char *pluginname) {
		OSyncError *err = NULL;
		osync_bool ret = osync_plugin_env_plugin_is_usable(self, pluginname, &err);
		raise_exception_on_error(err);
		return ret;
	}

%pythoncode %{
	num_plugins = property(num_plugins)
%}
};


typedef struct {} OSyncPluginInfo;

%feature("ref")   OSyncPlugin "osync_plugin_info_ref($this);"
%feature("unref") OSyncPlugin "osync_plugin_info_unref($this);"

%extend OSyncPluginInfo {
	OSyncPluginInfo(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

	OSyncPluginInfo() {
		OSyncError *err = NULL;
		OSyncPluginInfo *info = osync_plugin_info_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return info;
	}

	void set_loop(void *loop) {
		osync_plugin_info_set_loop(self, loop);
	}

	void *get_loop() {
		return osync_plugin_info_get_loop(self);
	}

	void set_config(const char *config) {
		osync_plugin_info_set_config(self, config);
	}

	const char *get_config() {
		return osync_plugin_info_get_config(self);
	}

	void set_configdir(const char *configdir) {
		osync_plugin_info_set_configdir(self, configdir);
	}

	const char *get_configdir() {
		return osync_plugin_info_get_configdir(self);
	}

	OSyncObjTypeSink *find_objtype(const char *name) {
		return osync_plugin_info_find_objtype(self, name);
	}

	void add_objtype(OSyncObjTypeSink *sink) {
		osync_plugin_info_add_objtype(self, sink);
	}

	int num_objtypes() {
		return osync_plugin_info_num_objtypes(self);
	}

	OSyncObjTypeSink *nth_objtype(int nth) {
		return osync_plugin_info_nth_objtype(self, nth);
	}

	OSyncObjTypeSink *get_main_sink() {
		return osync_plugin_info_get_main_sink(self);
	}

	void set_main_sink(OSyncObjTypeSink *sink) {
		osync_plugin_info_set_main_sink(self, sink);
	}

	OSyncFormatEnv *get_format_env() {
		return osync_plugin_info_get_format_env(self);
	}

	void set_format_env(OSyncFormatEnv *env) {
		osync_plugin_info_set_format_env(self, env);
	}

	OSyncObjTypeSink *get_sink() {
		return osync_plugin_info_get_sink(self);
	}

	void set_sink(OSyncObjTypeSink *sink) {
		osync_plugin_info_set_sink(self, sink);
	}

	void set_groupname(const char *groupname) {
		osync_plugin_info_set_groupname(self, groupname);
	}

	const char *get_groupname() {
		return osync_plugin_info_get_groupname(self);
	}

	void set_version(OSyncVersion *version) {
		osync_plugin_info_set_version(self, version);
	}

	OSyncVersion *get_version() {
		return osync_plugin_info_get_version(self);
	}

	void set_capabilities(OSyncCapabilities *capabilities) {
		osync_plugin_info_set_capabilities(self, capabilities);
	}

	OSyncCapabilities *get_capabilities() {
		return osync_plugin_info_get_capabilities(self);
	}

%pythoncode %{
	loop = property(get_loop, set_loop)
	config = property(get_config, set_config)
	configdir = property(get_configdir, set_configdir)
	num_objtypes = property(num_objtypes)
	main_sink = property(get_main_sink, set_main_sink)
	format_env = property(get_format_env, set_format_env)
	sink = property(get_sink, set_sink)
	groupname = property(get_groupname, set_groupname)
	version = property(get_version, set_version)
	capabilities = property(get_capabilities, set_capabilities)
%}
}


typedef struct {} OSyncObjTypeSink;

%feature("ref")   OSyncObjTypeSink "osync_objtype_sink_ref($this);"
%feature("unref") OSyncObjTypeSink "osync_objtype_sink_unref($this);"

%extend OSyncObjTypeSink {
	OSyncObjTypeSink(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

	OSyncObjTypeSink(const char *objtype) {
		OSyncError *err = NULL;
		OSyncObjTypeSink *sink = osync_objtype_sink_new(objtype, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return sink;
	}

	const char *get_name() {
		return osync_objtype_sink_get_name(self);
	}

	void set_name(const char *name) {
		osync_objtype_sink_set_name(self, name);
	}

	int num_objformats() {
		return osync_objtype_sink_num_objformats(self);
	}

	const char *nth_objformat(int nth) {
		return osync_objtype_sink_nth_objformat(self, nth);
	}

	void add_objformat(const char *format) {
		osync_objtype_sink_add_objformat(self, format);
	}

	void remove_objformat(const char *format) {
		osync_objtype_sink_remove_objformat(self, format);
	}

	/* TODO: set_functions */

	void *get_userdata() {
		return osync_objtype_sink_get_userdata(self);
	}

	void get_changes(void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx) {
		osync_objtype_sink_get_changes(self, plugindata, info, ctx);
	}

	void read_change(void *plugindata, OSyncPluginInfo *info, OSyncChange *change, OSyncContext *ctx) {
		osync_objtype_sink_read_change(self, plugindata, info, change, ctx);
	}

	void connect(void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx) {
		osync_objtype_sink_connect(self, plugindata, info, ctx);
	}

	void disconnect(void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx) {
		osync_objtype_sink_disconnect(self, plugindata, info, ctx);
	}

	void sync_done(void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx) {
		osync_objtype_sink_sync_done(self, plugindata, info, ctx);
	}

	void commit_change(void *plugindata, OSyncPluginInfo *info, OSyncChange *change, OSyncContext *ctx) {
		osync_objtype_sink_commit_change(self, plugindata, info, change, ctx);
	}

	void committed_all(void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx) {
		osync_objtype_sink_committed_all(self, plugindata, info, ctx);
	}

	osync_bool is_enabled() {
		return osync_objtype_sink_is_enabled(self);
	}

	void set_enabled(osync_bool enabled) {
		osync_objtype_sink_set_enabled(self, enabled);
	}

	osync_bool is_available() {
		return osync_objtype_sink_is_available(self);
	}

	void set_available(osync_bool available) {
		osync_objtype_sink_set_available(self, available);
	}

	osync_bool get_write() {
		return osync_objtype_sink_get_write(self);
	}

	void set_write(osync_bool write) {
		osync_objtype_sink_set_write(self, write);
	}

	osync_bool get_read() {
		return osync_objtype_sink_get_read(self);
	}

	void set_read(osync_bool read) {
		osync_objtype_sink_set_read(self, read);
	}

	osync_bool get_slowsync() {
		return osync_objtype_sink_get_slowsync(self);
	}

	void set_slowsync(osync_bool slowsync) {
		osync_objtype_sink_set_slowsync(self, slowsync);
	}

	/* returns a list of strings */
	PyObject *get_objformats() {
		const OSyncList *list = osync_objtype_sink_get_objformats(self);
		return osynclist_to_pylist(list, SWIGTYPE_p_char);
	}

%pythoncode %{
	name = property(get_name, set_name)
	num_objformats = property(num_objformats)
	enabled = property(is_enabled, set_enabled)
	available = property(is_available, set_available)
	write = property(get_write, set_write)
	read = property(get_read, set_read)
	slowsync = property(get_slowsync, set_slowsync)
	objformats = property(get_objformats)
%}
}
