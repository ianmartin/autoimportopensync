%{
#include <opensync/opensync-format.h>
%}

typedef enum {} ConverterType;

%constant int CONVERTER_CONV = OSYNC_CONVERTER_CONV;
%constant int CONVERTER_ENCAP = OSYNC_CONVERTER_ENCAP;
%constant int CONVERTER_DECAP = OSYNC_CONVERTER_DECAP;
%constant int CONVERTER_DETECTOR = OSYNC_CONVERTER_DETECTOR;

typedef struct {} OSyncFormatConverter;

%feature("ref")   OSyncFormatConverter "osync_converter_ref($this);"
%feature("unref") OSyncFormatConverter "osync_converter_unref($this);"

%extend OSyncFormatConverter {
	OSyncFormatConverter(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

	OSyncObjFormat *get_sourceformat() {
		return osync_converter_get_sourceformat(self);
	}

	OSyncObjFormat *get_targetformat() {
		return osync_converter_get_targetformat(self);
	}

	ConverterType get_type() {
		return osync_converter_get_type(self);
	}

	OSyncObjFormat *detect(OSyncData *data) {
		return osync_converter_detect(self, data);
	}

	void invoke(OSyncData *data, const char *config) {
		OSyncError *err = NULL;
		osync_bool ret = osync_converter_invoke(self, data, config, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_converter_invoke failed but did not set error code");
	}

	osync_bool matches(OSyncData *data) {
		return osync_converter_matches(self, data);
	}

%pythoncode %{
	sourceformat = property(get_sourceformat)
	targetformat = property(get_targetformat)
	type = property(get_type)
%}
}


typedef struct {} OSyncFormatConverterPath;

%feature("ref")   OSyncFormatConverterPath "osync_converter_path_ref($this);"
%feature("unref") OSyncFormatConverterPath "osync_converter_path_unref($this);"

%extend OSyncFormatConverterPath {
	OSyncFormatConverterPath(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

	OSyncFormatConverterPath() {
		OSyncError *err = NULL;
		OSyncFormatConverterPath *ret = osync_converter_path_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return ret;
	}

	void add_edge(OSyncFormatConverter *edge) {
		osync_converter_path_add_edge(self, edge);
	}

	int num_edges() {
		return osync_converter_path_num_edges(self);
	}

	OSyncFormatConverter *nth_edge(unsigned int nth) {
		return osync_converter_path_nth_edge(self, nth);
	}

	const char *get_config() {
		return osync_converter_path_get_config(self);
	}

	void set_config(const char *config) {
		osync_converter_path_set_config(self, config);
	}

%pythoncode %{
	num_edges = property(num_edges)
	config = property(get_config, set_config)
%}
}


typedef enum {} OSyncFilterAction;

%constant int FILTER_IGNORE = OSYNC_FILTER_IGNORE;
%constant int FILTER_ALLOW = OSYNC_FILTER_ALLOW;
%constant int FILTER_DENY = OSYNC_FILTER_DENY;

typedef struct {} OSyncFilter;

%feature("ref")   OSyncFilter "osync_filter_ref($this);"
%feature("unref") OSyncFilter "osync_filter_unref($this);"

%extend OSyncFilter {
	OSyncFilter(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

	OSyncFilter(const char *objtype, OSyncFilterAction action) {
		OSyncError *err = NULL;
		OSyncFilter *filter = osync_filter_new(objtype, action, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return filter;
	}

	void set_config(const char *config) {
		osync_filter_set_config(self, config);
	}

	const char *get_config() {
		return osync_filter_get_config(self);
	}

	OSyncFilterAction invoke(OSyncData *data) {
		return osync_filter_invoke(self, data);
	}

	const char *get_objtype() {
		return osync_filter_get_objtype(self);
	}

%pythoncode %{
	config = property(get_config, set_config)
	objtype = property(get_objtype)
%}
}


typedef struct {} OSyncFormatEnv;

%extend OSyncFormatEnv {
	OSyncFormatEnv(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

	OSyncFormatEnv() {
		OSyncError *err = NULL;
		OSyncFormatEnv *env = osync_format_env_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return env;
	}

	~OSyncFormatEnv() {
		osync_format_env_free(self);
	}

	void load_plugins(const char *path) {
		OSyncError *err = NULL;
		osync_bool ret = osync_format_env_load_plugins(self, path, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_format_env_load_plugins failed but did not set error code");
	}

	void register_objformat(OSyncObjFormat *format) {
		osync_format_env_register_objformat(self, format);
	}

	OSyncObjFormat *find_objformat(const char *name) {
		return osync_format_env_find_objformat(self, name);
	}

	int num_objformats() {
		return osync_format_env_num_objformats(self);
	}

	OSyncObjFormat *nth_objformat(int nth) {
		return osync_format_env_nth_objformat(self, nth);
	}

	void register_converter(OSyncFormatConverter *converter) {
		osync_format_env_register_converter(self, converter);
	}

	OSyncFormatConverter *find_converter(OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat) {
		return osync_format_env_find_converter(self, sourceformat, targetformat);
	}

	int num_converters() {
		return osync_format_env_num_converters(self);
	}

	OSyncFormatConverter *nth_converter(int nth) {
		return osync_format_env_nth_converter(self, nth);
	}

	void register_filter(OSyncCustomFilter *filter) {
		osync_format_env_register_filter(self, filter);
	}

	int num_filters() {
		return osync_format_env_num_filters(self);
	}

	OSyncCustomFilter *nth_filter(int nth) {
		return osync_format_env_nth_filter(self, nth);
	}

	OSyncObjFormat *detect_objformat(OSyncData *data) {
		return osync_format_env_detect_objformat(self, data);
	}

	OSyncObjFormat *detect_objformat_full(OSyncData *input) {
		OSyncError *err = NULL;
		OSyncObjFormat *ret = osync_format_env_detect_objformat_full(self, input, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_format_env_detect_objformat_full failed but did not set error code");
		return ret;
	}

	void convert(OSyncFormatConverterPath *path, OSyncData *data) {
		OSyncError *err = NULL;
		osync_bool ret = osync_format_env_convert(self, path, data, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_format_env_convert failed but did not set error code");
	}

	OSyncFormatConverterPath *find_path(OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat) {
		OSyncError *err = NULL;
		OSyncFormatConverterPath *ret = osync_format_env_find_path(self, sourceformat, targetformat, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_format_env_find_path failed but did not set error code");
		return ret;
	}

	OSyncFormatConverterPath *find_path_formats(OSyncObjFormat *sourceformat, PyObject *targets) {
		/* convert second argument from a python list to a NULL-terminated array of ObjFormat pointers */
		if (!PyList_Check(targets)) {
			PyErr_SetString(PyExc_TypeError, "wrapper error: argument is not a list");
			return NULL;
		}

		int i, len = PyList_Size(targets);
		OSyncObjFormat *real_targets[len + 1];

		for (i = 0; i < len; i++) {
			PyObject *o = PyList_GetItem(targets, i);
			if (SWIG_ConvertPtr(o, (void **) &real_targets[i], SWIGTYPE_p_OSyncObjFormat, SWIG_POINTER_EXCEPTION) != 0)
				return NULL;
		}
		real_targets[len] = NULL;

		OSyncError *err = NULL;
		OSyncFormatConverterPath *ret = osync_format_env_find_path_formats(self, sourceformat, real_targets, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_format_env_find_path_formats failed but did not set error code");
		return ret;
	}

%pythoncode %{
	num_objformats = property(num_objformats)
	num_converters = property(num_converters)
	num_filters = property(num_filters)
%}
}


typedef struct {} OSyncObjFormat;

%feature("ref")   OSyncObjFormat "osync_objformat_ref($this);"
%feature("unref") OSyncObjFormat "osync_objformat_unref($this);"

%extend OSyncObjFormat {
	OSyncObjFormat(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

	OSyncObjFormat(const char *name, const char *objtype_name) {
		OSyncError *err = NULL;
		OSyncObjFormat *format = osync_objformat_new(name, objtype_name, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return format;
	}

	const char *get_name() {
		return osync_objformat_get_name(self);
	}

	const char *get_objtype() {
		return osync_objformat_get_objtype(self);
	}

	/* TODO: compare, duplicate, create, print, get_revision, destroy, copy */

	osync_bool is_equal(OSyncObjFormat *rightformat) {
		return osync_objformat_is_equal(self, rightformat);
	}

	osync_bool must_marshal() {
		return osync_objformat_must_marshal(self);
	}

	/* TODO: marshal, demarshal */

%pythoncode %{
	name = property(get_name)
	objtype = property(get_objtype)
	must_marshal = property(must_marshal)
%}
}