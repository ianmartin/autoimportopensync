%{
#include <opensync/opensync-data.h>
%}

typedef struct {} OSyncData;

%feature("ref")   OSyncData "osync_data_ref($this);"
%feature("unref") OSyncData "osync_data_unref($this);"

%extend OSyncData {
	OSyncData(PyObject *obj) {
		return (OSyncData *)PyCObject_AsVoidPtr(obj);
	}

	/* FIXME: cstring_input_binary is broken in my version of swig, so I've recreated it here */
	%typemap(in) (char *buf, unsigned int size) {
		int alloc = 0;
		int res = SWIG_AsCharPtrAndSize($input, &$1, &$2, &alloc);
		if (!SWIG_IsOK(res)) {
			%argument_fail(res, "(char *buf, unsigned int size)", $symname, $argnum);
		}
	}
	OSyncData(char *buf, unsigned int size, OSyncObjFormat *format) {
		OSyncError *err = NULL;
		OSyncData *data = osync_data_new(buf, size, format, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return data;
	}

	OSyncObjFormat *get_objformat() {
		return osync_data_get_objformat(self);
	}

	void set_objformat(OSyncObjFormat *objformat) {
		osync_data_set_objformat(self, objformat);
	}

	const char *get_objtype() {
		return osync_data_get_objtype(self);
	}

	void set_objtype(const char *objtype) {
		osync_data_set_objtype(self, objtype);
	}

	%cstring_output_allocate_size(char **buffer, unsigned int *size, );
	void get_data(char **buffer, unsigned int *size) {
		osync_data_get_data(self, buffer, size);
	}

	%cstring_output_allocate_size(char **buffer, unsigned int *size, );
	void steal_data(char **buffer, unsigned int *size) {
		osync_data_steal_data(self, buffer, size);
	}

	%cstring_input_binary(char *buf, unsigned int size);
	void set_data(char *buffer, unsigned int size) {
		osync_data_set_data(self, buffer, size);
	}

	osync_bool has_data() {
		return osync_data_has_data(self);
	}

	OSyncData *clone() {
		OSyncError *err = NULL;
		OSyncData *data = osync_data_clone(self, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return data;
	}

	OSyncConvCmpResult compare(OSyncData *data) {
		return osync_data_compare(self, data);
	}

	char *get_printable() {
		return osync_data_get_printable(self);
	}

	time_t get_revision() {
		OSyncError *err = NULL;
		time_t ret = osync_data_get_revision(self, &err);
		if (raise_exception_on_error(err))
			return -1;
		else
			return ret;
	}

%pythoncode %{
	objformat = property(get_objformat, set_objformat)
	objtype = property(get_objtype, set_objtype)
	data = property(get_data, set_data)
	revision = property(get_revision)
%}
};


typedef struct {} OSyncChange;

%feature("ref")   OSyncChange "osync_change_ref($this);"
%feature("unref") OSyncChange "osync_change_unref($this);"

%extend OSyncChange {
	OSyncChange(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

	OSyncChange() {
		OSyncError *err = NULL;
		OSyncChange *change = osync_change_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return change;
	}

	void set_hash(const char *hash) {
		osync_change_set_hash(self, hash);
	}
	
	const char *get_hash() {
		return osync_change_get_hash(self);
	}

	void set_uid(const char *uid) {
		osync_change_set_uid(self, uid);
	}
	
	const char *get_uid() {
		return osync_change_get_uid(self);
	}
	
	void set_changetype(OSyncChangeType changetype) {
		osync_change_set_changetype(self, changetype);
	}
	
	OSyncChangeType get_changetype() {
		return osync_change_get_changetype(self);
	}

	void set_data(OSyncData *data) {
		osync_change_set_data(self, data);
	}

	OSyncData *get_data() {
		return osync_change_get_data(self);
	}

	OSyncConvCmpResult compare(OSyncChange *change) {
		return osync_change_compare(self, change);
	}

	osync_bool duplicate() {
		OSyncError *err = NULL;
		osync_bool dirty = FALSE;
		osync_bool ret = osync_change_duplicate(self, &dirty, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_change_duplicate failed but did not set error code");
		return dirty;
	}

	OSyncObjFormat *get_objformat() {
		return osync_change_get_objformat(self);
	}

	void set_objtype(const char *objtype) {
		osync_change_set_objtype(self, objtype);
	}
	
	const char *get_objtype() {
		return osync_change_get_objtype(self);
	}
	
%pythoncode %{
	hash = property(get_hash, set_hash)
	uid = property(get_uid, set_uid)
	changetype = property(get_changetype, set_changetype)
	data = property(get_data,set_data)
	objformat = property(get_objformat)
	objtype = property(get_objtype, set_objtype)
%}
};