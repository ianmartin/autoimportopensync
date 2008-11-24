typedef enum {} ConvCmpResult;

%constant int CONV_DATA_UNKNOWN = OSYNC_CONV_DATA_UNKNOWN;
%constant int CONV_DATA_MISMATCH = OSYNC_CONV_DATA_MISMATCH;
%constant int CONV_DATA_SIMILAR = OSYNC_CONV_DATA_SIMILAR;
%constant int CONV_DATA_SAME = OSYNC_CONV_DATA_SAME;


typedef struct {} Data;
%extend Data {
	/* FIXME: cstring_input_binary is broken in my version of swig, so I've recreated it here */
	%typemap(in) (char *buf, size_t size) {
		int alloc = SWIG_NEWOBJ;
		int res = SWIG_AsCharPtrAndSize($input, &$1, &$2, &alloc);
		if (!SWIG_IsOK(res)) {
			%argument_fail(res, "(char *buf, size_t size)", $symname, $argnum);
		}
	}
	Data(char *buf, size_t size, ObjFormat *format) {
		Error *err = NULL;
		Data *data = osync_data_new(buf, (unsigned int)size, format, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return data;
	}

	~Data() {
		osync_data_unref(self);
	}

	ObjFormat *get_objformat() {
		ObjFormat *ret = osync_data_get_objformat(self);
		if (ret)
			osync_objformat_ref(ret);
		return ret;
	}

	void set_objformat(ObjFormat *objformat) {
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

        /*
	%cstring_output_allocate_size(char **buffer, unsigned int *size, );
	void steal_data(char **buffer, unsigned int *size) {
		osync_data_steal_data(self, buffer, size);
	}
        */

	%cstring_input_binary(char *buf, unsigned int size);
	void set_data(char *buffer, unsigned int size) {
		osync_data_set_data(self, buffer, size);
	}

	bool has_data() {
		return osync_data_has_data(self);
	}

	Data *clone() {
		Error *err = NULL;
		Data *data = osync_data_clone(self, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return data; /* new object, no need to inc ref */
	}

        /*
	ConvCmpResult compare(Data *data) {
		return osync_data_compare(self, data);
	}
        */

	char *get_printable() {
		return osync_data_get_printable(self);
	}

	time_t get_revision() {
		Error *err = NULL;
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


typedef enum {} ChangeType;

%constant int CHANGE_TYPE_UNKNOWN = OSYNC_CHANGE_TYPE_UNKNOWN;
%constant int CHANGE_TYPE_ADDED = OSYNC_CHANGE_TYPE_ADDED;
%constant int CHANGE_TYPE_UNMODIFIED = OSYNC_CHANGE_TYPE_UNMODIFIED;
%constant int CHANGE_TYPE_DELETED = OSYNC_CHANGE_TYPE_DELETED;
%constant int CHANGE_TYPE_MODIFIED = OSYNC_CHANGE_TYPE_MODIFIED;


typedef struct {} Change;
%extend Change {
	/* called by python-module plugin */
	Change(PyObject *obj) {
		Change *change = PyCObject_AsVoidPtr(obj);
		osync_change_ref(change);
		return change;
	}

	Change() {
		Error *err = NULL;
		Change *change = osync_change_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return change;
	}

	~Change() {
		osync_change_unref(self);
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

	void set_changetype(ChangeType changetype) {
		osync_change_set_changetype(self, changetype);
	}

	ChangeType get_changetype() {
		return osync_change_get_changetype(self);
	}

	void set_data(Data *data) {
		osync_change_set_data(self, data);
	}

	Data *get_data() {
		return osync_change_get_data(self);
	}

	ConvCmpResult compare(Change *change) {
		return osync_change_compare(self, change);
	}

        /*
	bool duplicate() {
		Error *err = NULL;
		bool dirty = FALSE;
		bool ret = osync_change_duplicate(self, &dirty, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_change_duplicate failed but did not set error code");
		return dirty;
	}
        */

	ObjFormat *get_objformat() {
		ObjFormat *ret = osync_change_get_objformat(self);
		if (ret)
			osync_objformat_ref(ret);
		return ret;
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
	data = property(get_data, set_data)
	objformat = property(get_objformat)
	objtype = property(get_objtype, set_objtype)
%}
};
