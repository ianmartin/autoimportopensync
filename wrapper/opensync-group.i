%{
#include <opensync/opensync-group.h>
%}

typedef struct {} OSyncGroupEnv;

%extend OSyncGroupEnv {
	OSyncGroupEnv(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

	OSyncGroupEnv() {
		OSyncError *err = NULL;
		OSyncGroupEnv *env = osync_group_env_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return env;
	}

	~OSyncGroupEnv() {
		osync_group_env_free(self);
	}

	void load_groups(const char *path) {
		OSyncError *err = NULL;
		osync_bool ret = osync_group_env_load_groups(self, path, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_group_env_load_groups failed but did not set error code");
	}

	OSyncGroup *find_group(const char *name) {
		return osync_group_env_find_group(self, name);
	}

	void add_group(OSyncGroup *group) {
		osync_group_env_add_group(self, group);
	}

	void remove_group(OSyncGroup *group) {
		osync_group_env_remove_group(self, group);
	}

	int num_groups() {
		return osync_group_env_num_groups(self);
	}

	OSyncGroup *nth_group(int nth) {
		return osync_group_env_nth_group(self, nth);
	}

%pythoncode %{
	num_groups = property(num_groups)
%}
}

enum OSyncLockState {};
%constant int LOCK_OK = OSYNC_LOCK_OK;
%constant int LOCKED = OSYNC_LOCKED;
%constant int LOCK_STALE = OSYNC_LOCK_STALE;

typedef struct {} OSyncGroup;

%feature("ref")   OSyncGroup "osync_group_ref($this);"
%feature("unref") OSyncGroup "osync_group_unref($this);"

%extend OSyncGroup {
	OSyncGroup(PyObject *obj) {
		OSyncGroup *group = PyCObject_AsVoidPtr(obj);
		return group;
	}

	OSyncGroup() {
		OSyncError *err = NULL;
		OSyncGroup *group = osync_group_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return group;
	}

	OSyncLockState lock() {
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
		OSyncError *err = NULL;
		osync_bool ret = osync_group_save(self, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_group_save failed but did not set error code");
	}

	void delete() {
		OSyncError *err = NULL;
		osync_bool ret = osync_group_delete(self, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_group_delete failed but did not set error code");
	}

	void reset() {
		OSyncError *err = NULL;
		osync_bool ret = osync_group_reset(self, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_group_reset failed but did not set error code");
	}

	void load(const char *path) {
		OSyncError *err = NULL;
		osync_bool ret = osync_group_load(self, path, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_group_load failed but did not set error code");
	}

	void add_member(OSyncMember *member) {
		osync_group_add_member(self, member);
	}

	void remove_member(OSyncMember *member) {
		osync_group_remove_member(self, member);
	}

	OSyncMember *find_member(int id) {
		return osync_group_find_member(self, id);
	}

	OSyncMember *nth_member(int nth) {
		return osync_group_nth_member(self, nth);
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

	void set_objtype_enabled(const char *objtype, osync_bool enabled) {
		osync_group_set_objtype_enabled(self, objtype, enabled);
	}

	int objtype_enabled(const char *objtype) {
		return osync_group_objtype_enabled(self, objtype);
	}

	void add_filter(OSyncFilter *filter) {
		osync_group_add_filter(self, filter);
	}

	void remove_filter(OSyncFilter *filter) {
		osync_group_remove_filter(self, filter);
	}

	int num_filters() {
		return osync_group_num_filters(self);
	}

	OSyncFilter *nth_filter(int nth) {
		return osync_group_nth_filter(self, nth);
	}

	void set_last_synchronization(time_t last_sync) {
		osync_group_set_last_synchronization(self, last_sync);
	}

	time_t get_last_synchronization() {
		return osync_group_get_last_synchronization(self);
	}

	void set_conflict_resolution(OSyncConflictResolution res, int num) {
		osync_group_set_conflict_resolution(self, res, num);
	}

	%apply OSyncConflictResolution *OUTPUT { OSyncConflictResolution *res, int *num };
	%apply int *OUTPUT { OSyncConflictResolution *res, int *num };
	void get_conflict_resolution(OSyncConflictResolution *res, int *num) {
		osync_group_get_conflict_resolution(self, res, num);
	}

%pythoncode %{
	name = property(get_name, set_name);
	configdir = property(get_configdir, set_configdir);
	num_members = property(num_members)
	num_objtypes = property(num_objtypes)
	num_filters = property(num_filters)
	last_synchronization = property(get_last_synchronization, set_last_synchronization)
%}
}

typedef struct {} OSyncMember;

%feature("ref")   OSyncMember "osync_member_ref($this);"
%feature("unref") OSyncMember "osync_member_unref($this);"

%extend OSyncMember {
	OSyncMember(PyObject *obj) {
		OSyncMember *member = PyCObject_AsVoidPtr(obj);
		return member;
	}

	OSyncMember() {
		OSyncError *err = NULL;
		OSyncMember *member = osync_member_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return member;
	}

	const char *get_pluginname() {
		return osync_member_get_pluginname(self);
	}

	void set_pluginname(const char *pluginname) {
		osync_member_set_pluginname(self, pluginname);
	}

	const char *get_configdir() {
		return osync_member_get_configdir(self);
	}

	void set_configdir(const char *configdir) {
		osync_member_set_configdir(self, configdir);
	}

	const char *get_config_or_default() {
		OSyncError *err = NULL;
		const char *ret = osync_member_get_config_or_default(self, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return ret;
	}

	osync_bool has_config() {
		return osync_member_has_config(self);
	}

	const char *get_config() {
		OSyncError *err = NULL;
		const char *ret = osync_member_get_config(self, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return ret;
	}

	void set_config(const char *data) {
		osync_member_set_config(self, data);
	}

	void load(const char *path) {
		OSyncError *err = NULL;
		osync_bool ret = osync_member_load(self, path, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_member_load failed but did not set error code");
	}

	void save() {
		OSyncError *err = NULL;
		osync_bool ret = osync_member_save(self, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_member_save failed but did not set error code");
	}

	void delete() {
		OSyncError *err = NULL;
		osync_bool ret = osync_member_delete(self, &err);
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
		OSyncError *err = NULL;
		const OSyncList *list = osync_member_get_objformats(self, objtype, &err);
		if (raise_exception_on_error(err) || list == NULL)
			return NULL;

		return osynclist_to_pylist(list, SWIGTYPE_p_char);
	}

	void add_objtype(const char *objtype) {
		osync_member_add_objtype(self, objtype);
	}

	int num_objtypes() {
		return osync_member_num_objtypes(self);
	}

	const char *nth_objtype(int nth) {
		return osync_member_nth_objtype(self, nth);
	}

	osync_bool objtype_enabled(const char *objtype) {
		return osync_member_objtype_enabled(self, objtype);
	}

	void set_objtype_enabled(const char *objtype, osync_bool enabled) {
		osync_member_set_objtype_enabled(self, objtype, enabled);
	}

	void set_start_type(OSyncStartType type) {
		osync_member_set_start_type(self, type);
	}

	OSyncStartType get_start_type() {
		return osync_member_get_start_type(self);
	}

	OSyncCapabilities *get_capabilities() {
		return osync_member_get_capabilities(self);
	}

	void set_capabilities(OSyncCapabilities *capabilities) {
		OSyncError *err = NULL;
		osync_bool ret = osync_member_set_capabilities(self, capabilities, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_member_set_capabilities failed but did not set error code");
	}

	OSyncMerger *get_merger() {
		return osync_member_get_merger(self);
	}

	%pythoncode %{
		pluginname = property(get_pluginname, set_pluginname);
		configdir = property(get_configdir, set_configdir);
		config = property(get_config, set_config)
		id = property(get_id)
		num_objtypes = property(num_objtypes)
		start_type = property(get_start_type, set_start_type);
		capabilities = property(get_capabilities, set_capabilities);
		merger = property(get_merger);
	%}
}
