typedef enum {} ConverterType;

%constant int CONVERTER_CONV = OSYNC_CONVERTER_CONV;
%constant int CONVERTER_ENCAP = OSYNC_CONVERTER_ENCAP;
%constant int CONVERTER_DECAP = OSYNC_CONVERTER_DECAP;
%constant int CONVERTER_DETECTOR = OSYNC_CONVERTER_DETECTOR;

typedef struct {} FormatConverter;
%extend FormatConverter {
	FormatConverter(PyObject *obj) {
		FormatConverter *conv = PyCObject_AsVoidPtr(obj);
		osync_converter_ref(conv);
		return conv;
	}

	~FormatConverter() {
		osync_converter_unref(self);
	}

	ObjFormat *get_sourceformat() {
		return osync_converter_get_sourceformat(self);
	}

	ObjFormat *get_targetformat() {
		return osync_converter_get_targetformat(self);
	}

	ConverterType get_type() {
		return osync_converter_get_type(self);
	}

	ObjFormat *detect(Data *data) {
		return osync_converter_detect(self, data);
	}

	void invoke(Data *data, const char *config) {
		Error *err = NULL;
		bool ret = osync_converter_invoke(self, data, config, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_converter_invoke failed but did not set error code");
	}

	bool matches(Data *data) {
		return osync_converter_matches(self, data);
	}

%pythoncode %{
	sourceformat = property(get_sourceformat)
	targetformat = property(get_targetformat)
	type = property(get_type)
%}
}


typedef struct {} FormatConverterPath;
%extend FormatConverterPath {
	FormatConverterPath(PyObject *obj) {
		FormatConverterPath *conv_path = PyCObject_AsVoidPtr(obj);
		osync_converter_path_ref(conv_path);
		return conv_path;
	}

	FormatConverterPath() {
		Error *err = NULL;
		FormatConverterPath *ret = osync_converter_path_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return ret;
	}

	~FormatConverterPath() {
		osync_converter_path_unref(self);
	}

	void add_edge(FormatConverter *edge) {
		osync_converter_path_add_edge(self, edge);
	}

	int num_edges() {
		return osync_converter_path_num_edges(self);
	}

	FormatConverter *nth_edge(unsigned int nth) {
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


typedef enum {} FilterAction;

%constant int FILTER_IGNORE = OSYNC_FILTER_IGNORE;
%constant int FILTER_ALLOW = OSYNC_FILTER_ALLOW;
%constant int FILTER_DENY = OSYNC_FILTER_DENY;

typedef struct {} Filter;
%extend Filter {
	Filter(PyObject *obj) {
		Filter *filter = PyCObject_AsVoidPtr(obj);
		osync_filter_ref(filter);
		return filter;
	}

	Filter(const char *objtype, FilterAction action) {
		Error *err = NULL;
		Filter *filter = osync_filter_new(objtype, action, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return filter;
	}

	~Filter() {
		osync_filter_unref(self);
	}

	void set_config(const char *config) {
		osync_filter_set_config(self, config);
	}

	const char *get_config() {
		return osync_filter_get_config(self);
	}

	FilterAction invoke(Data *data) {
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


typedef struct {} FormatEnv;
%extend FormatEnv {
	FormatEnv(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

	FormatEnv() {
		Error *err = NULL;
		FormatEnv *env = osync_format_env_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return env;
	}

	~FormatEnv() {
		/* FIXME: need to free here, but only if we created it! */
		/* osync_format_env_free(self); */
	}

	void load_plugins(const char *path) {
		Error *err = NULL;
		bool ret = osync_format_env_load_plugins(self, path, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_format_env_load_plugins failed but did not set error code");
	}

	void register_objformat(ObjFormat *format) {
		osync_format_env_register_objformat(self, format);
	}

	ObjFormat *find_objformat(const char *name) {
		return osync_format_env_find_objformat(self, name);
	}

	int num_objformats() {
		return osync_format_env_num_objformats(self);
	}

	ObjFormat *nth_objformat(int nth) {
		return osync_format_env_nth_objformat(self, nth);
	}

	void register_converter(FormatConverter *converter) {
		osync_format_env_register_converter(self, converter);
	}

	FormatConverter *find_converter(ObjFormat *sourceformat, ObjFormat *targetformat) {
		return osync_format_env_find_converter(self, sourceformat, targetformat);
	}

	int num_converters() {
		return osync_format_env_num_converters(self);
	}

	FormatConverter *nth_converter(int nth) {
		return osync_format_env_nth_converter(self, nth);
	}

	void register_filter(CustomFilter *filter) {
		osync_format_env_register_filter(self, filter);
	}

	int num_filters() {
		return osync_format_env_num_filters(self);
	}

	CustomFilter *nth_filter(int nth) {
		return osync_format_env_nth_filter(self, nth);
	}

	ObjFormat *detect_objformat(Data *data) {
		return osync_format_env_detect_objformat(self, data);
	}

	ObjFormat *detect_objformat_full(Data *input) {
		Error *err = NULL;
		ObjFormat *ret = osync_format_env_detect_objformat_full(self, input, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_format_env_detect_objformat_full failed but did not set error code");
		return ret;
	}

	void convert(FormatConverterPath *path, Data *data) {
		Error *err = NULL;
		bool ret = osync_format_env_convert(self, path, data, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_format_env_convert failed but did not set error code");
	}

	FormatConverterPath *find_path(ObjFormat *sourceformat, ObjFormat *targetformat) {
		Error *err = NULL;
		FormatConverterPath *ret = osync_format_env_find_path(self, sourceformat, targetformat, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_format_env_find_path failed but did not set error code");
		return ret;
	}

	FormatConverterPath *find_path_formats(ObjFormat *sourceformat, PyObject *targets) {
		/* convert second argument from a python list to a NULL-terminated array of ObjFormat pointers */
		if (!PyList_Check(targets)) {
			PyErr_SetString(PyExc_TypeError, "wrapper error: argument is not a list");
			return NULL;
		}

		int i, len = PyList_Size(targets);
		ObjFormat *real_targets[len + 1];

		for (i = 0; i < len; i++) {
			PyObject *o = PyList_GetItem(targets, i);
			if (SWIG_ConvertPtr(o, (void **) &real_targets[i], SWIGTYPE_p_ObjFormat, SWIG_POINTER_EXCEPTION) != 0)
				return NULL;
		}
		real_targets[len] = NULL;

		Error *err = NULL;
		FormatConverterPath *ret = osync_format_env_find_path_formats(self, sourceformat, real_targets, &err);
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


typedef struct {} ObjFormat;
%extend ObjFormat {
	ObjFormat(PyObject *obj) {
		ObjFormat *objformat = PyCObject_AsVoidPtr(obj);
		osync_objformat_ref(objformat);
		return objformat;
	}

	ObjFormat(const char *name, const char *objtype_name) {
		Error *err = NULL;
		ObjFormat *format = osync_objformat_new(name, objtype_name, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return format;
	}

	~ObjFormat() {
		osync_objformat_unref(self);
	}

	const char *get_name() {
		return osync_objformat_get_name(self);
	}

	const char *get_objtype() {
		return osync_objformat_get_objtype(self);
	}

	/* TODO: compare, duplicate, create, print, get_revision, destroy, copy */

	bool is_equal(ObjFormat *rightformat) {
		return osync_objformat_is_equal(self, rightformat);
	}

	bool must_marshal() {
		return osync_objformat_must_marshal(self);
	}

	/* TODO: marshal, demarshal */

%pythoncode %{
	name = property(get_name)
	objtype = property(get_objtype)
	must_marshal = property(must_marshal)
%}
}
