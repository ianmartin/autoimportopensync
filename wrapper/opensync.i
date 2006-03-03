%module opensync
%include "cdata.i"

%{
#include "opensync.h"
#include <glib.h>
%}

%include "opensync.h"
typedef struct {} OSyncEnv;
typedef struct {} OSyncPlugin;
typedef struct {} OSyncPluginInfo;
typedef struct {} OSyncContext;
typedef struct {} OSyncChange;
typedef struct {} OSyncMember;
typedef struct {} OSyncHashTable;

%extend OSyncMember {
	OSyncMember(PyObject *obj) {
		OSyncMember *member = NULL;
		member = (OSyncMember *)PyCObject_AsVoidPtr(obj);
		return member;
	}
	
	osync_bool get_slow_sync(const char *objtype) {
		return osync_member_get_slow_sync(self, objtype);
	}
	
	void set_slow_sync(const char *objtype, osync_bool set) {
		osync_member_set_slow_sync(self, objtype, set);
	}
}

%extend OSyncChange {
	OSyncChange(PyObject *obj=NULL) {
		OSyncChange *change = NULL;
		if (obj)
			change = (OSyncChange *)PyCObject_AsVoidPtr(obj);
		else
			change = osync_change_new();
		return change;
	}
	
	~OSyncChange() {
		osync_trace(TRACE_INTERNAL, "Deleting change %p", self);
	}
	
	void data_set(char *data, int datasize, osync_bool has_data) {
		/* take a copy of the data, so python does not try to reclaim it
		 * this memory should be freed by opensync after the change is written
		 * FIXME: if the change is not handed over to opensync, this will leak
		 */
		char *copy = memcpy(malloc(datasize), data, datasize);
		osync_change_set_data(self, copy, datasize, has_data);
	}
	
	void *data_get() {
		return osync_change_get_data(self);
	}
	
	int datasize_get() {
		return osync_change_get_datasize(self);
	}
	
	%pythoncode %{
	def get_data(self):
		data = cdata(self.data_get(), self.datasize_get())
		# FIXME: despite passing the size around, sometimes the data
		# seems also to be null-terminated; remove this.
		if data[-1] == '\0':
			data = data[:-1]
		return data
	def set_data(self, data, has_data=True):
		return self.data_set(data, len(data), has_data)
	data = property(get_data,set_data)
	%}
	
	void report(OSyncContext *ctx)
	{
		osync_context_report_change(ctx, self);
	}
	
	void uid_set(const char *uid) {
		osync_change_set_uid(self, uid);
	}
	
	const char *uid_get() {
		return osync_change_get_uid(self);
	}
	
	%pythoncode %{
	def get_uid(self):
		return self.uid_get()
	def set_uid(self, uid):
		self.uid_set(uid)
	uid = property(get_uid, set_uid)
	%}
	
	void format_set(const char *format) {
		osync_change_set_objformat_string(self, format);
	}
	
	const char *format_get() {
		OSyncObjFormat *format = osync_change_get_objformat(self);
		if (!format)
			return NULL;
		return osync_objformat_get_name(format);
	}
	
	%pythoncode %{
	def get_format(self):
		return self.format_get()
	def set_format(self, format):
		self.format_set(format)
	format = property(get_format, set_format)
	%}
	
	void objtype_set(const char *objtype) {
		osync_change_set_objtype_string(self, objtype);
	}
	
	const char *objtype_get() {
		OSyncObjType *objtype = osync_change_get_objtype(self);
		if (!objtype)
			return NULL;
		return osync_objtype_get_name(objtype);
	}
	
	%pythoncode %{
	def get_objtype(self):
		return self.objtype_get()
	def set_objtype(self, objtype):
		self.objtype_set(objtype)
	objtype = property(get_objtype, set_objtype)
	%}
	
	void changetype_set(int changetype) {
		osync_change_set_changetype(self, changetype);
	}
	
	int changetype_get() {
		return osync_change_get_changetype(self);
	}
	
	%pythoncode %{
	def get_changetype(self):
		return self.changetype_get()
	def set_changetype(self, changetype):
		self.changetype_set(changetype)
	changetype = property(get_changetype, set_changetype)
	%}
	
	void hash_set(const char *hash) {
		osync_change_set_hash(self, hash);
	}
	
	const char *hash_get() {
		return osync_change_get_hash(self);
	}
	
	%pythoncode %{
	def get_hash(self):
		return self.hash_get()
	def set_hash(self, hash):
		self.hash_set(hash)
	hash = property(get_hash, set_hash)
	%}
};

%extend OSyncContext {
	OSyncContext(PyObject *obj) {
		OSyncContext *ctx = (OSyncContext *)PyCObject_AsVoidPtr(obj);
		return ctx;
	}
	
	void report_success(void) {
		osync_context_report_success(self);
	}
	
	void report_error(int type, const char *msg) {
		osync_context_report_error(self, type, msg);
	}
};

%extend OSyncPlugin {
	OSyncPlugin(OSyncEnv *env) {
		OSyncPlugin *plugin = osync_plugin_new(env);
		return plugin;
	}
	
	~OSyncPlugin() {
		osync_plugin_free(self);
	}
	
	const char *name_get() {
		return osync_plugin_get_name(self);
	}
	
	%pythoncode %{
	def get_name(self):
		return self.name_get()
	name = property(get_name)
	%}
};

%extend OSyncPluginInfo {
	OSyncPluginInfo(PyObject *obj) {
		OSyncPluginInfo *exinfo = (OSyncPluginInfo *)PyCObject_AsVoidPtr(obj);
		return exinfo;
	}
	
	void accept_objtype(const char *objtype) {
		osync_plugin_accept_objtype(self, objtype);
	}
	
	void accept_objformat(const char *objtype, const char *objformat, const char *extension = NULL) {
		osync_plugin_accept_objformat(self, objtype, objformat, extension);
	}
	
	void name_set(const char *name) {
		self->name = g_strdup(name);
	}
	
	const char *name_get() {
		return self->name;
	}
	
	%pythoncode %{
	def get_name(self):
		return self.name_get()
	def set_name(self, name):
		self.name_set(name)
	name = property(get_name, set_name)
	%}
};

%extend OSyncEnv {
	OSyncEnv(PyObject *obj) {
		osync_trace(TRACE_INTERNAL, "env!: %p", obj);
		if (obj) {
			OSyncEnv *exenv = (OSyncEnv *)PyCObject_AsVoidPtr(obj);
			osync_trace(TRACE_INTERNAL, "exenv!: %p", exenv);
			return exenv;
		}
		OSyncEnv *env = osync_env_new();
		return env;
	}
	
	~OSyncEnv() {
		osync_env_free(self);
	}
	
	int initialize() {
		return osync_env_initialize(self, NULL);
	}
	
	int finalize() {
		return osync_env_finalize(self, NULL);
	}
	
	int num_plugins() {
		return osync_env_num_plugins(self);
	}

	OSyncPlugin *get_nth_plugin(int nth) {
		OSyncPlugin *plugin = osync_env_nth_plugin(self, nth);
		return plugin;
	}
};

%extend OSyncHashTable {
	OSyncHashTable() {
		return osync_hashtable_new();
	}

	~OSyncHashTable() {
		osync_hashtable_free(self);
	}

	void forget() {
		osync_hashtable_forget(self);
	}

	osync_bool load(OSyncMember *member) {
		OSyncError *error;
		/* FIXME: do something with the error! */
		return osync_hashtable_load(self, member, &error);
	}

	void close() {
		osync_hashtable_close(self);
	}

	void update_hash(OSyncChange *change) {
		osync_hashtable_update_hash(self, change);
	}

	void report(const char *uid) {
		osync_hashtable_report(self, uid);
	}

	void report_deleted(OSyncContext *context, const char *objtype) {
		osync_hashtable_report_deleted(self, context, objtype);
	}

	OSyncChangeType get_changetype(const char *uid, const char *hash) {
		return osync_hashtable_get_changetype(self, uid, hash);
	}

	osync_bool detect_change(OSyncChange *change) {
		return osync_hashtable_detect_change(self, change);
	}

	void set_slow_sync(const char *objtype) {
		osync_hashtable_set_slow_sync(self, objtype);
	}
};

/* pull in constants from opensync_error.h without all the functions */
%constant int NO_ERROR = OSYNC_NO_ERROR;
%constant int ERROR_GENERIC = OSYNC_ERROR_GENERIC;
%constant int ERROR_IO_ERROR = OSYNC_ERROR_IO_ERROR;
%constant int ERROR_NOT_SUPPORTED = OSYNC_ERROR_NOT_SUPPORTED;
%constant int ERROR_TIMEOUT = OSYNC_ERROR_TIMEOUT;
%constant int ERROR_DISCONNECTED = OSYNC_ERROR_DISCONNECTED;
%constant int ERROR_FILE_NOT_FOUND = OSYNC_ERROR_FILE_NOT_FOUND;
%constant int ERROR_EXISTS = OSYNC_ERROR_EXISTS;
%constant int ERROR_CONVERT = OSYNC_ERROR_CONVERT;
%constant int ERROR_MISCONFIGURATION = OSYNC_ERROR_MISCONFIGURATION;
%constant int ERROR_INITIALIZATION = OSYNC_ERROR_INITIALIZATION;
%constant int ERROR_PARAMETER = OSYNC_ERROR_PARAMETER;
%constant int ERROR_EXPECTED = OSYNC_ERROR_EXPECTED;
%constant int ERROR_NO_CONNECTION = OSYNC_ERROR_NO_CONNECTION;
%constant int ERROR_TEMPORARY = OSYNC_ERROR_TEMPORARY;
%constant int ERROR_LOCKED = OSYNC_ERROR_LOCKED;
%constant int ERROR_PLUGIN_NOT_FOUND = OSYNC_ERROR_PLUGIN_NOT_FOUND;
