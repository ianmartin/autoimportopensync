typedef struct {} GroupEnv;
%extend GroupEnv {
	GroupEnv() {
		Error *err = NULL;
		GroupEnv *env = osync_group_env_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return env;
	}

	~GroupEnv() {
		osync_group_env_free(self);
	}

	void load_groups(const char *path = NULL) {
		Error *err = NULL;
		bool ret = osync_group_env_load_groups(self, path, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_group_env_load_groups failed but did not set error code");
	}

	Group *find_group(const char *name) {
		Group *group = osync_group_env_find_group(self, name);
		if (group)
			osync_group_ref(group);
		return group;
	}

	void add_group(Group *group) {
		Error *err = NULL;
		bool ret = osync_group_env_add_group(self, group, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_group_env_add_group failed but did not set error code");
	}

	void remove_group(Group *group) {
		osync_group_env_remove_group(self, group);
	}

	int num_groups() {
		return osync_group_env_num_groups(self);
	}

	Group *nth_group(int nth) {
		Group *group = osync_group_env_nth_group(self, nth);
		if (group)
			osync_group_ref(group);
		return group;
	}

%pythoncode %{
	# extend the SWIG-generated constructor, so that we can setup our list-wrapper classes
	__oldinit = __init__
	def __init__(self, *args):
		self.__oldinit(*args)
		self.groups = _ListWrapper(self.num_groups, self.nth_group)
%}
}

enum LockState {};
%constant int LOCK_OK = OSYNC_LOCK_OK;
%constant int LOCKED = OSYNC_LOCKED;
%constant int LOCK_STALE = OSYNC_LOCK_STALE;

enum ConflictResolution ();
%constant int CONFLICT_RESOLUTION_UNKNOWN = OSYNC_CONFLICT_RESOLUTION_UNKNOWN;
%constant int CONFLICT_RESOLUTION_DUPLICATE = OSYNC_CONFLICT_RESOLUTION_DUPLICATE;
%constant int CONFLICT_RESOLUTION_IGNORE = OSYNC_CONFLICT_RESOLUTION_IGNORE;
%constant int CONFLICT_RESOLUTION_NEWER = OSYNC_CONFLICT_RESOLUTION_NEWER;
%constant int CONFLICT_RESOLUTION_SELECT = OSYNC_CONFLICT_RESOLUTION_SELECT;

typedef struct {} Group;
%extend Group {
	Group() {
		Error *err = NULL;
		Group *group = osync_group_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return group;
	}

	~Group() {
		osync_group_unref(self);
	}

	LockState lock() {
		return osync_group_lock(self);
	}

	void unlock() {
		osync_group_unlock(self);
	}

	void set_name(const char *name) {
		osync_group_set_name(self, name);
	}

	const char *get_name() {
		return osync_group_get_name(self);
	}

	void save() {
		Error *err = NULL;
		bool ret = osync_group_save(self, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_group_save failed but did not set error code");
	}

	void delete() {
		Error *err = NULL;
		bool ret = osync_group_delete(self, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_group_delete failed but did not set error code");
	}

	void reset() {
		Error *err = NULL;
		bool ret = osync_group_reset(self, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_group_reset failed but did not set error code");
	}

	void load(const char *path) {
		Error *err = NULL;
		bool ret = osync_group_load(self, path, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_group_load failed but did not set error code");
	}

	void add_member(Member *member) {
		osync_group_add_member(self, member);
	}

	void remove_member(Member *member) {
		osync_group_remove_member(self, member);
	}

	Member *find_member(int id) {
		Member *member = osync_group_find_member(self, id);
		if (member)
			osync_member_ref(member);
        return member;
	}

	Member *nth_member(int nth) {
		Member *member = osync_group_nth_member(self, nth);
		if (member)
			osync_member_ref(member);
        return member;
	}

	int num_members() {
		return osync_group_num_members(self);
	}

	const char *get_configdir() {
		return osync_group_get_configdir(self);
	}

	void set_configdir(const char *directory) {
		osync_group_set_configdir(self, directory);
	}

	int num_objtypes() {
		return osync_group_num_objtypes(self);
	}

	const char *nth_objtype(int nth) {
		return osync_group_nth_objtype(self, nth);
	}

	void set_objtype_enabled(const char *objtype, bool enabled) {
		osync_group_set_objtype_enabled(self, objtype, enabled);
	}

	int objtype_enabled(const char *objtype) {
		return osync_group_objtype_enabled(self, objtype);
	}

	void add_filter(Filter *filter) {
		osync_group_add_filter(self, filter);
	}

	void remove_filter(Filter *filter) {
		osync_group_remove_filter(self, filter);
	}

	int num_filters() {
		return osync_group_num_filters(self);
	}

	Filter *nth_filter(int nth) {
		Filter *filter = osync_group_nth_filter(self, nth);
		if (filter)
			osync_filter_ref(filter);
		return filter;
	}

	void set_last_synchronization(time_t last_sync) {
		osync_group_set_last_synchronization(self, last_sync);
	}

	time_t get_last_synchronization() {
		return osync_group_get_last_synchronization(self);
	}

	void set_conflict_resolution(ConflictResolution res, int num) {
		osync_group_set_conflict_resolution(self, res, num);
	}

	%apply ConflictResolution *OUTPUT { ConflictResolution *res, int *num };
	%apply int *OUTPUT { ConflictResolution *res, int *num };
	void get_conflict_resolution(ConflictResolution *res, int *num) {
		osync_group_get_conflict_resolution(self, res, num);
	}

	bool get_merger_enabled() {
		return osync_group_get_merger_enabled(self);
	}

	void set_merger_enabled(bool enable_merger) {
		osync_group_set_merger_enabled(self, enable_merger);
	}

	bool get_converter_enabled() {
		return osync_group_get_converter_enabled(self);
	}

	void set_converter_enabled(bool enable_converter) {
		osync_group_set_converter_enabled(self, enable_converter);
	}

%pythoncode %{
	name = property(get_name, set_name)
	configdir = property(get_configdir, set_configdir)
	last_synchronization = property(get_last_synchronization, set_last_synchronization)
	merger_enabled = property(get_merger_enabled, set_merger_enabled)
	converter_enabled = property(get_converter_enabled, set_converter_enabled)
	
	# extend the SWIG-generated constructor, so that we can setup our list-wrapper classes
	__oldinit = __init__
	def __init__(self, *args):
		self.__oldinit(*args)
		self.members = _ListWrapper(self.num_members, self.nth_member)
		self.objtypes = _ListWrapper(self.num_objtypes, self.nth_objtype)
		self.filters = _ListWrapper(self.num_filters, self.nth_filter)
%}
}


typedef enum {} StartType;

%constant int START_TYPE_UNKNOWN = OSYNC_START_TYPE_UNKNOWN;
%constant int START_TYPE_PROCESS = OSYNC_START_TYPE_PROCESS;
%constant int START_TYPE_THREAD = OSYNC_START_TYPE_THREAD;
%constant int START_TYPE_EXTERNAL = OSYNC_START_TYPE_EXTERNAL;


typedef struct {} Member;
%extend Member {
	Member() {
		Error *err = NULL;
		Member *member = osync_member_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return member;
	}

	~Member() {
		osync_member_unref(self);
	}

	const char *get_pluginname() {
		return osync_member_get_pluginname(self);
	}

	void set_pluginname(const char *pluginname) {
		osync_member_set_pluginname(self, pluginname);
	}

	const char *get_name() {
		return osync_member_get_name(self);
	}

	void set_name(const char *name) {
		osync_member_set_name(self, name);
	}

	const char *get_configdir() {
		return osync_member_get_configdir(self);
	}

	void set_configdir(const char *configdir) {
		osync_member_set_configdir(self, configdir);
	}

	PluginConfig *get_config_or_default() {
		Error *err = NULL;
		PluginConfig *ret = osync_member_get_config_or_default(self, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return ret;
	}

	bool has_config() {
		return osync_member_has_config(self);
	}

	PluginConfig *get_config() {
		Error *err = NULL;
		PluginConfig *ret = osync_member_get_config(self, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return ret;
	}

	void set_config(PluginConfig *data) {
		osync_member_set_config(self, data);
	}

	void load(const char *path) {
		Error *err = NULL;
		bool ret = osync_member_load(self, path, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_member_load failed but did not set error code");
	}

	void save() {
		Error *err = NULL;
		bool ret = osync_member_save(self, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_member_save failed but did not set error code");
	}

	void delete() {
		Error *err = NULL;
		bool ret = osync_member_delete(self, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_member_delete failed but did not set error code");
	}

	long long int get_id() {
		return osync_member_get_id(self);
	}

	void add_objformat(const char *objtype, const char *format) {
		osync_member_add_objformat(self, objtype, format);
	}

	/* returns a list of strings */
	PyObject *get_objformats(const char *objtype) {
		Error *err = NULL;
		const OSyncList *list = osync_member_get_objformats(self, objtype, &err);
		if (raise_exception_on_error(err) || list == NULL)
			return NULL;

		return osynclist_to_pylist(list, SWIGTYPE_p_char);
	}

	void add_objtype_sink(ObjTypeSink *sink) {
		osync_member_add_objtype_sink(self, sink);
	}

	int num_objtypes() {
		return osync_member_num_objtypes(self);
	}

	const char *nth_objtype(int nth) {
		return osync_member_nth_objtype(self, nth);
	}

	bool objtype_enabled(const char *objtype) {
		return osync_member_objtype_enabled(self, objtype);
	}

	void set_objtype_enabled(const char *objtype, bool enabled) {
		osync_member_set_objtype_enabled(self, objtype, enabled);
	}

	Capabilities *get_capabilities() {
		Capabilities *caps = osync_member_get_capabilities(self);
		if (caps)
			osync_capabilities_ref(caps);
		return caps;
	}

	void set_capabilities(Capabilities *capabilities) {
		Error *err = NULL;
		bool ret = osync_member_set_capabilities(self, capabilities, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_member_set_capabilities failed but did not set error code");
	}

	Merger *get_merger() {
		Merger *merger = osync_member_get_merger(self);
		if (merger)
			osync_merger_ref(merger);
		return merger;
	}

	void flush_objtypes() {
		osync_member_flush_objtypes(self);
	}

%pythoncode %{
	pluginname = property(get_pluginname, set_pluginname)
	name = property(get_name, set_name)
	configdir = property(get_configdir, set_configdir)
	config = property(get_config, set_config)
	id = property(get_id)
	capabilities = property(get_capabilities, set_capabilities)
	merger = property(get_merger)
	
	# extend the SWIG-generated constructor, so that we can setup our list-wrapper classes
	__oldinit = __init__
	def __init__(self, *args):
		self.__oldinit(*args)
		self.objtypes = _ListWrapper(self.num_objtypes, self.nth_objtype)
%}
}
