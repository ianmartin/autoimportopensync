typedef enum {} ConverterType;

%constant int CONVERTER_CONV = OSYNC_CONVERTER_CONV;
%constant int CONVERTER_ENCAP = OSYNC_CONVERTER_ENCAP;
%constant int CONVERTER_DECAP = OSYNC_CONVERTER_DECAP;
%constant int CONVERTER_DETECTOR = OSYNC_CONVERTER_DETECTOR;

typedef struct {} FormatConverter;
%extend FormatConverter {
	FormatConverter() {
		wrapper_exception("cannot create FormatConverter objects");
		return NULL;
	}

	~FormatConverter() {
		osync_converter_unref(self);
	}

	ObjFormat *get_sourceformat() {
		ObjFormat *ret = osync_converter_get_sourceformat(self);
		if (ret)
			osync_objformat_ref(ret);
		return ret;
	}

	ObjFormat *get_targetformat() {
		ObjFormat *ret = osync_converter_get_targetformat(self);
		if (ret)
			osync_objformat_ref(ret);
		return ret;
	}

	ConverterType get_type() {
		return osync_converter_get_type(self);
	}

	ObjFormat *detect(Data *data) {
		ObjFormat *ret = osync_converter_detect(self, data);
		if (ret)
			osync_objformat_ref(ret);
		return ret;
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
		FormatConverter *ret = osync_converter_path_nth_edge(self, nth);
		if (ret)
			osync_converter_ref(ret);
		return ret;
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
	FormatEnv() {
		Error *err = NULL;
		FormatEnv *env = osync_format_env_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return env;
	}

	~FormatEnv() {
		osync_format_env_free(self);
	}

	void load_plugins(const char *path = NULL) {
		Error *err = NULL;
		bool ret = osync_format_env_load_plugins(self, path, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_format_env_load_plugins failed but did not set error code");
	}

	void register_objformat(ObjFormat *format) {
		osync_format_env_register_objformat(self, format);
	}

	ObjFormat *find_objformat(const char *name) {
		ObjFormat *ret = osync_format_env_find_objformat(self, name);
		if (ret)
			osync_objformat_ref(ret);
		return ret;
	}

	int num_objformats() {
		return osync_format_env_num_objformats(self);
	}

	ObjFormat *nth_objformat(int nth) {
		ObjFormat *ret = osync_format_env_nth_objformat(self, nth);
		if (ret)
			osync_objformat_ref(ret);
		return ret;
	}

	void register_converter(FormatConverter *converter) {
		osync_format_env_register_converter(self, converter);
	}

	FormatConverter *find_converter(ObjFormat *sourceformat, ObjFormat *targetformat) {
		FormatConverter *ret = osync_format_env_find_converter(self, sourceformat, targetformat);
		if (ret)
			osync_converter_ref(ret);
		return ret;
	}

	int num_converters() {
		return osync_format_env_num_converters(self);
	}

	FormatConverter *nth_converter(int nth) {
		FormatConverter *ret = osync_format_env_nth_converter(self, nth);
		if (ret)
			osync_converter_ref(ret);
		return ret;
	}

	void register_filter(CustomFilter *filter) {
		osync_format_env_register_filter(self, filter);
	}

	int num_filters() {
		return osync_format_env_num_filters(self);
	}

	CustomFilter *nth_filter(int nth) {
		CustomFilter *ret = osync_format_env_nth_filter(self, nth);
		if (ret)
			osync_custom_filter_ref(ret);
		return ret;
	}

	ObjFormat *detect_objformat(Data *data) {
		ObjFormat *ret = osync_format_env_detect_objformat(self, data);
		if (ret)
			osync_objformat_ref(ret);
		return ret;
	}

	ObjFormat *detect_objformat_full(Data *input) {
		Error *err = NULL;
		ObjFormat *ret = osync_format_env_detect_objformat_full(self, input, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_format_env_detect_objformat_full failed but did not set error code");
		if (ret)
			osync_objformat_ref(ret);
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
		if (ret)
			osync_converter_path_ref(ret);
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
		if (ret)
			osync_converter_path_ref(ret);
		return ret;
	}

%pythoncode %{
	@property
	def objformats(self):
		return _ListWrapper(self.num_objformats, self.nth_objformat, self.register_objformat)

	@property
	def converters(self):
		return _ListWrapper(self.num_converters, self.nth_converter, self.register_converter)

	@property
	def filters(self):
		return _ListWrapper(self.num_filters, self.nth_filter, self.register_filter)
%}
}


typedef struct {} ObjFormat;
%extend ObjFormat {
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
