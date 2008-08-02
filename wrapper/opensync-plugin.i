typedef enum {} ConfigurationType;

%constant int PLUGIN_NO_CONFIGURATION = OSYNC_PLUGIN_NO_CONFIGURATION;
%constant int PLUGIN_OPTIONAL_CONFIGURATION = OSYNC_PLUGIN_OPTIONAL_CONFIGURATION;
%constant int PLUGIN_NEEDS_CONFIGURATION = OSYNC_PLUGIN_NEEDS_CONFIGURATION;


typedef struct {} Plugin;
%extend Plugin {
	/* called by python-module plugin */
	Plugin(PyObject *obj) {
		Plugin *plugin = PyCObject_AsVoidPtr(obj);
		osync_plugin_ref(plugin);
		return plugin;
	}

	Plugin() {
		Error *err = NULL;
		Plugin *plugin = osync_plugin_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return plugin;
		}

	~Plugin() {
		osync_plugin_unref(self);
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

	ConfigurationType get_config_type() {
		return osync_plugin_get_config_type(self);
	}

	void set_config_type(ConfigurationType config_type) {
		osync_plugin_set_config_type(self, config_type);
	}

	void *initialize(PluginInfo *info) {
		Error *err = NULL;
		void *ret = osync_plugin_initialize(self, info, &err);
		raise_exception_on_error(err);
		return ret;
	}

	void finalize(void *data) {
		osync_plugin_finalize(self, data);
	}

	void discover(void *data, PluginInfo *info) {
		Error *err = NULL;
		bool ret = osync_plugin_discover(self, data, info, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_plugin_discover failed but did not set error code");
	}

	bool is_usable() {
		Error *err = NULL;
		bool ret = osync_plugin_is_usable(self, &err);
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


typedef struct {} PluginEnv;
%extend PluginEnv {
	PluginEnv() {
		Error *err = NULL;
		PluginEnv *env = osync_plugin_env_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return env;
	}

	~PluginEnv() {
		osync_plugin_env_free(self);
	}

	void load(const char *path = NULL) {
		Error *err = NULL;
		bool ret = osync_plugin_env_load(self, path, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_plugin_env_load failed but did not set error code");
	}

	void register_plugin(Plugin *plugin) {
		osync_plugin_env_register_plugin(self, plugin);
	}

	void load_module(const char *filename) {
		Error *err = NULL;
		bool ret = osync_plugin_env_load_module(self, filename, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_plugin_env_load_module failed but did not set error code");
	}

	Plugin *find_plugin(const char *name) {
		Plugin *plugin = osync_plugin_env_find_plugin(self, name);
		if (plugin)
			osync_plugin_ref(plugin);
		return plugin;
	}

	int num_plugins() {
		return osync_plugin_env_num_plugins(self);
	}

	Plugin *nth_plugin(int nth) {
		Plugin *plugin = osync_plugin_env_nth_plugin(self, nth);
		if (plugin)
			osync_plugin_ref(plugin);
		return plugin;
	}

	bool plugin_is_usable(const char *pluginname) {
		Error *err = NULL;
		bool ret = osync_plugin_env_plugin_is_usable(self, pluginname, &err);
		raise_exception_on_error(err);
		return ret;
	}

%pythoncode %{
	# extend the SWIG-generated constructor, so that we can setup our list-wrapper classes
	__oldinit = __init__
	def __init__(self, *args):
		self.__oldinit(*args)
		self.plugins = _ListWrapper(self.num_plugins, self.nth_plugin)
%}
};


typedef struct {} PluginInfo;
%extend PluginInfo {
	/* called by python-module plugin */
	PluginInfo(PyObject *obj) {
		PluginInfo *info = PyCObject_AsVoidPtr(obj);
		osync_plugin_info_ref(info);
		return info;
	}

	PluginInfo() {
		Error *err = NULL;
		PluginInfo *info = osync_plugin_info_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return info;
	}

	~PluginInfo() {
		osync_plugin_info_unref(self);
	}

	void set_loop(void *loop) {
		osync_plugin_info_set_loop(self, loop);
	}

	void *get_loop() {
		return osync_plugin_info_get_loop(self);
	}

	void set_config(PluginConfig *config) {
		osync_plugin_info_set_config(self, config);
	}

	PluginConfig *get_config() {
		return osync_plugin_info_get_config(self);
	}

	void set_configdir(const char *configdir) {
		osync_plugin_info_set_configdir(self, configdir);
	}

	const char *get_configdir() {
		return osync_plugin_info_get_configdir(self);
	}

	ObjTypeSink *find_objtype(const char *name) {
		return osync_plugin_info_find_objtype(self, name);
	}

	void add_objtype(ObjTypeSink *sink) {
		osync_plugin_info_add_objtype(self, sink);
	}

	int num_objtypes() {
		return osync_plugin_info_num_objtypes(self);
	}

	ObjTypeSink *nth_objtype(int nth) {
		ObjTypeSink *ret = osync_plugin_info_nth_objtype(self, nth);
		if (ret)
			osync_objtype_sink_ref(ret);
		return ret;
	}

	ObjTypeSink *get_main_sink() {
		ObjTypeSink *ret = osync_plugin_info_get_main_sink(self);
		if (ret)
			osync_objtype_sink_ref(ret);
		return ret;
	}

	void set_main_sink(ObjTypeSink *sink) {
		osync_plugin_info_set_main_sink(self, sink);
	}

	FormatEnv *get_format_env() {
		return osync_plugin_info_get_format_env(self);
	}

	void set_format_env(FormatEnv *env) {
		osync_plugin_info_set_format_env(self, env);
	}

	ObjTypeSink *get_sink() {
		ObjTypeSink *ret = osync_plugin_info_get_sink(self);
		if (ret)
			osync_objtype_sink_ref(ret);
		return ret;
	}

	void set_sink(ObjTypeSink *sink) {
		osync_plugin_info_set_sink(self, sink);
	}

	void set_groupname(const char *groupname) {
		osync_plugin_info_set_groupname(self, groupname);
	}

	const char *get_groupname() {
		return osync_plugin_info_get_groupname(self);
	}

	void set_version(Version *version) {
		osync_plugin_info_set_version(self, version);
	}

	Version *get_version() {
		Version *ret = osync_plugin_info_get_version(self);
		if (ret)
			osync_version_ref(ret);
		return ret;
	}

	void set_capabilities(Capabilities *capabilities) {
		osync_plugin_info_set_capabilities(self, capabilities);
	}

	Capabilities *get_capabilities() {
		Capabilities *ret = osync_plugin_info_get_capabilities(self);
		if (ret)
			osync_capabilities_ref(ret);
		return ret;
	}

%pythoncode %{
	loop = property(get_loop, set_loop)
	config = property(get_config, set_config)
	configdir = property(get_configdir, set_configdir)
	main_sink = property(get_main_sink, set_main_sink)
	format_env = property(get_format_env, set_format_env)
	sink = property(get_sink, set_sink)
	groupname = property(get_groupname, set_groupname)
	version = property(get_version, set_version)
	capabilities = property(get_capabilities, set_capabilities)

	# extend the SWIG-generated constructor, so that we can setup our list-wrapper classes
	__oldinit = __init__
	def __init__(self, *args):
		self.__oldinit(*args)
		self.objtypes = _ListWrapper(self.num_objtypes, self.nth_objtype)
%}
}

typedef struct {} PluginConfig;
%extend PluginConfig {
	/* called by python-module plugin */
	PluginConfig(PyObject *obj) {
		PluginConfig *config = PyCObject_AsVoidPtr(obj);
		osync_plugin_config_ref(config);
		return config;
	}

	PluginConfig() {
		Error *err = NULL;
		PluginConfig *config = osync_plugin_config_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return config;
	}

	~PluginConfig() {
		osync_plugin_config_unref(self);
	}
}


typedef struct {} ObjTypeSink;
%extend ObjTypeSink {
	/* create new sink object
	 * when using the python-module plugin, the second argument is
	 * the python object that will get callbacks for this sink */
	ObjTypeSink(const char *objtype, PyObject *callback_obj = NULL) {
		Error *err = NULL;
		ObjTypeSink *sink = osync_objtype_sink_new(objtype, &err);
		if (raise_exception_on_error(err))
			return NULL;

		/* set userdata pointer to supplied python wrapper object */
		if (callback_obj) {
			OSyncObjTypeSinkFunctions functions;
			Py_INCREF(callback_obj);
			memset(&functions, 0, sizeof(functions));
			osync_objtype_sink_set_functions(sink, functions, callback_obj);
		}

		return sink;
	}

	~ObjTypeSink() {
		PyObject *callback_obj = osync_objtype_sink_get_userdata(self);
		if (callback_obj) {
			Py_DECREF(callback_obj);
		}
		osync_objtype_sink_unref(self);
	}

	const char *get_name() {
		return osync_objtype_sink_get_name(self);
	}

	void set_name(const char *name) {
		osync_objtype_sink_set_name(self, name);
	}

	unsigned int num_objformats() {
		return osync_objtype_sink_num_objformat_sinks(self);
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

	/* as above, but return it as a PyObject * for python code */
	PyObject *get_callback_obj() {
		return osync_objtype_sink_get_userdata(self);
	}

	void get_changes(void *plugindata, PluginInfo *info, Context *ctx) {
		osync_objtype_sink_get_changes(self, plugindata, info, ctx);
	}

	void read_change(void *plugindata, PluginInfo *info, Change *change, Context *ctx) {
		osync_objtype_sink_read_change(self, plugindata, info, change, ctx);
	}

	void connect(void *plugindata, PluginInfo *info, Context *ctx) {
		osync_objtype_sink_connect(self, plugindata, info, ctx);
	}

	void disconnect(void *plugindata, PluginInfo *info, Context *ctx) {
		osync_objtype_sink_disconnect(self, plugindata, info, ctx);
	}

	void sync_done(void *plugindata, PluginInfo *info, Context *ctx) {
		osync_objtype_sink_sync_done(self, plugindata, info, ctx);
	}

	void commit_change(void *plugindata, PluginInfo *info, Change *change, Context *ctx) {
		osync_objtype_sink_commit_change(self, plugindata, info, change, ctx);
	}

	void committed_all(void *plugindata, PluginInfo *info, Context *ctx) {
		osync_objtype_sink_committed_all(self, plugindata, info, ctx);
	}

	bool is_enabled() {
		return osync_objtype_sink_is_enabled(self);
	}

	void set_enabled(bool enabled) {
		osync_objtype_sink_set_enabled(self, enabled);
	}

	bool is_available() {
		return osync_objtype_sink_is_available(self);
	}

	void set_available(bool available) {
		osync_objtype_sink_set_available(self, available);
	}

	bool get_write() {
		return osync_objtype_sink_get_write(self);
	}

	void set_write(bool write) {
		osync_objtype_sink_set_write(self, write);
	}

	bool get_read() {
		return osync_objtype_sink_get_read(self);
	}

	void set_read(bool read) {
		osync_objtype_sink_set_read(self, read);
	}

	bool get_slowsync() {
		return osync_objtype_sink_get_slowsync(self);
	}

	void set_slowsync(bool slowsync) {
		osync_objtype_sink_set_slowsync(self, slowsync);
	}

	/* returns a list of strings */
/*
	PyObject *get_objformats() {
		const OSyncList *list = osync_objtype_sink_get_objformats(self);
		return osynclist_to_pylist(list, SWIGTYPE_p_char);
	}
*/

%pythoncode %{
	name = property(get_name, set_name)
	enabled = property(is_enabled, set_enabled)
	available = property(is_available, set_available)
	write = property(get_write, set_write)
	read = property(get_read, set_read)
	slowsync = property(get_slowsync, set_slowsync)
	callback_obj = property(get_callback_obj)
	
	# extend the SWIG-generated constructor, so that we can setup our list-wrapper classes
	__oldinit = __init__
	def __init__(self, *args):
		self.__oldinit(*args)
		self.objformats = _ListWrapper(self.num_objformats, self.nth_objformat)
%}
}

%pythoncode %{
class ObjTypeSinkCallbacks:
	"""A purely-Python class that should be subclassed by plugins implementing their own sinks."""
	def __init__(self, objtype):
		# construct ObjTypeSink object and pass it our reference
		self.sink = ObjTypeSink(objtype, self)

	def connect(self, info, ctx):
		pass

	def get_changes(self, info, ctx):
		pass

	def commit(self, info, ctx, chg):
		pass

	def committed_all(self, info, ctx):
		pass

	def read(self, info, ctx, chg):
		pass

	def write(self, info, ctx, chg):
		pass

	def disconnect(self, info, ctx):
		pass

	def sync_done(self, info, ctx):
		pass
%}
