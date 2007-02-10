%module opensync
%include "cdata.i"
%include "cstring.i"
%include "typemaps.i"

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

	osync_bool objtype_enabled(const char *objtype) {
		return osync_member_objtype_enabled(self, objtype);
	}
	
	const char *get_configdir() {
		return osync_member_get_configdir(self);
	}

	void set_configdir(const char *configdir) {
		osync_member_set_configdir(self, configdir);
	}

	%pythoncode %{
	configdir = property(get_configdir, set_configdir);
	%}

	/* This SWIG code is really confusing, so I'd better explain it:
	 * The %apply directive means that any argument called int *sizeout is actually a return value;
	 * this means that __get_config (from python) returns two values, a pointer and a length.
	 * The char * is returned as a void * to avoid SWIG converting it to a string automatically
	 * (because theoretically it might not be null-terminated).
	 */
	%apply int *OUTPUT {int *sizeout};
	void * __get_config(int *sizeout) {
		char *data;
		OSyncError *error;
		/* FIXME: do something with the error! */
		if (osync_member_get_config(self, &data, sizeout, &error))
			return data;
		else
			return NULL;
	}

	void __set_config(void *data, int size) {
		osync_member_set_config(self, data, size);
	}

	%pythoncode %{
	def get_config(self):
		data, len = self.__get_config()
		return cdata(data, len)
	def set_config(self, data):
		self.__set_config(data, len(data))
	config = property(get_config, set_config)
	%}
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
	
	%cstring_input_binary(char *data, int datasize);
	void __set_data(char *data, int datasize) {
		osync_change_set_data(self, data, datasize, TRUE);
	}
	
	void *__get_data() {
		return osync_change_get_data(self);
	}
	
	int __get_datasize() {
		return osync_change_get_datasize(self);
	}
	
	%pythoncode %{
	def get_data(self):
		try:
			return self.__data
		except AttributeError:
			self.__data = cdata(self.__get_data(), self.__get_datasize())
			# FIXME: despite passing the size around, sometimes the data
			# seems also to be null-terminated; remove this.
			if self.__data[-1] == '\0':
				self.__data = self.__data[:-1]
		return self.__data

	def set_data(self, data):
		self.__data = data
		self.__set_data(data)

	data = property(get_data,set_data)
	%}
	
	void report(OSyncContext *ctx)
	{
		/* take a copy of the data, so python does not try to reclaim it
		 * this memory should be freed by opensync after the change is written
		 */
		if (osync_change_has_data(self)) {
			void *data = osync_change_get_data(self);
			int datasize = osync_change_get_datasize(self);
			void *copy = memcpy(malloc(datasize), data, datasize);
			osync_change_set_data(self, copy, datasize, TRUE);
		}
		osync_context_report_change(ctx, self);
	}
	
	void set_uid(const char *uid) {
		osync_change_set_uid(self, uid);
	}
	
	const char *get_uid() {
		return osync_change_get_uid(self);
	}
	
	%pythoncode %{
	uid = property(get_uid, set_uid)
	%}
	
	void set_format(const char *format) {
		osync_change_set_objformat_string(self, format);
	}
	
	const char *get_format() {
		OSyncObjFormat *format = osync_change_get_objformat(self);
		if (!format)
			return NULL;
		return osync_objformat_get_name(format);
	}
	
	%pythoncode %{
	format = property(get_format, set_format)
	%}
	
	void set_objtype(const char *objtype) {
		osync_change_set_objtype_string(self, objtype);
	}
	
	const char *get_objtype() {
		OSyncObjType *objtype = osync_change_get_objtype(self);
		if (!objtype)
			return NULL;
		return osync_objtype_get_name(objtype);
	}
	
	%pythoncode %{
	objtype = property(get_objtype, set_objtype)
	%}
	
	void set_changetype(int changetype) {
		osync_change_set_changetype(self, changetype);
	}
	
	int get_changetype() {
		return osync_change_get_changetype(self);
	}
	
	%pythoncode %{
	changetype = property(get_changetype, set_changetype)
	%}
	
	void set_hash(const char *hash) {
		osync_change_set_hash(self, hash);
	}
	
	const char *get_hash() {
		return osync_change_get_hash(self);
	}
	
	%pythoncode %{
	hash = property(get_hash, set_hash)
	%}

	void set_member(OSyncMember *member) {
		osync_change_set_member(self, member);
	}

	OSyncMember *get_member() {
		return osync_change_get_member(self);
	}

	%pythoncode %{
	member = property(get_member, set_member);
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
	
	const char *get_name() {
		return osync_plugin_get_name(self);
	}
	
	%pythoncode %{
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
	
	void set_name(const char *name) {
		self->name = g_strdup(name);
	}
	const char *get_name() {
		return self->name;
	}
	
	void set_longname(const char *name) {
		self->longname = g_strdup(name);
	}
	const char *get_longname() {
		return self->longname;
	}
	
	void set_description(const char *desc) {
		self->description = g_strdup(desc);
	}
	const char *get_description() {
		return self->description;
	}
	
	%pythoncode %{
	name = property(get_name, set_name)
	longname = property(get_longname, set_longname)
	description = property(get_description, set_description)
	%}
};

%extend OSyncEnv {
	OSyncEnv(PyObject *obj) {
		return (OSyncEnv *)PyCObject_AsVoidPtr(obj);
	}

	OSyncEnv() {
		return osync_env_new();
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

	OSyncChangeType get_changetype(const char *uid, const char *hash, const char *objtype) {
		return osync_hashtable_get_changetype(self, uid, hash, objtype);
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
