typedef struct {} Capability;
%extend Capability {
	Capability(Capabilities *capabilities, const char *objtype, const char *name) {
		Error *err = NULL;
		Capability *cap = osync_capability_new(capabilities, objtype, name, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return cap;
	}

	~Capability() {
		/* FIXME: no way to free a capability??? */
	}

	const char *get_name() {
		return osync_capability_get_name(self);
	}

	Capability *get_next() {
		return osync_capability_get_next(self);
	}

	bool has_key() {
		return osync_capability_has_key(self);
	}

	int get_key_count() {
		return osync_capability_get_key_count(self);
	}

	const char *get_nth_key(int nth) {
		return osync_capability_get_nth_key(self, nth);
	}

	void add_key(const char *name) {
		osync_capability_add_key(self, name);
	}

%pythoncode %{
	name = property(get_name)
%}
}


typedef struct {} Capabilities;
%extend Capabilities {
	Capabilities() {
		Error *err = NULL;
		Capabilities *caps = osync_capabilities_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return caps;
	}

	~Capabilities() {
		osync_capabilities_unref(self);
	}

	Capability *get_first(const char *objtype) {
		return osync_capabilities_get_first(self, objtype);
	}

	/* returns a python string object */
	PyObject *assemble() {
		char *buf;
		int size;
		osync_bool ret = osync_capabilities_assemble(self, &buf, &size);
		if (!ret) {
			wrapper_exception("osync_capabilities_assemble failed\n")
			return NULL;
		}
		PyObject *obj = PyString_FromStringAndSize(buf, size);
		free(buf);
		return obj;
	}

	void sort() {
		osync_capabilities_sort(self);
	}
}

%inline %{
	%cstring_input_binary(const char *buffer, unsigned int size);
	Capabilities *capabilities_parse(const char *buffer, unsigned int size) {
		Error *err = NULL;
		Capabilities *caps = osync_capabilities_parse(buffer, size, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return caps;
	}

	Capabilities *capabilities_load(const char *file) {
		Error *err = NULL;
		Capabilities *caps = osync_capabilities_load(file, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return caps;
	}

	Capabilities *capabilities_member_get_capabilities(Member *member) {
		Error *err = NULL;
		Capabilities *ret = osync_capabilities_member_get_capabilities(member, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return ret;
	}

	void capabilities_member_set_capabilities(Member *member, Capabilities *caps) {
		Error *err = NULL;
		osync_bool ret = osync_capabilities_member_set_capabilities(member, caps, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_capabilities_member_set_capabilities failed but did not set error code");
	}
%}


%{
/* convert an XMLFieldList to a python list */
static PyObject *xmlfieldlist_to_pylist(const XMLFieldList *list)
{
	PyObject *ret = PyList_New(0);
	if (ret == NULL)
		return NULL;
	int n, max = osync_xmlfieldlist_get_length(list);
	for (n = 0; n < max; n++) {
		PyObject *obj = SWIG_NewPointerObj(osync_xmlfieldlist_item(list, n), SWIG_XMLField_p, 0);
		if (!obj || PyList_Append(ret, obj) != 0) {
			Py_DECREF(ret);
			return NULL;
		}
	}
	return ret;
}
%}

typedef struct {} XMLFormat;
%extend XMLFormat {
	XMLFormat(const char *objtype) {
		Error *err = NULL;
		XMLFormat *xmlformat = osync_xmlformat_new(objtype, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return xmlformat;
	}

	~XMLFormat() {
		osync_xmlformat_unref(self);
	}

	const char *get_objtype() {
		return osync_xmlformat_get_objtype(self);
	}

	XMLField *get_first_field() {
		return osync_xmlformat_get_first_field(self);
	}

	/* FIXME: this is a varargs function, it should take a list of tuples or similar */
	PyObject *search_field(const char *name) {
		OSyncError *err = NULL;
		XMLFieldList *list = osync_xmlformat_search_field(self, name, &err, NULL);
		if (raise_exception_on_error(err))
			return NULL;
		if (!list) {
			wrapper_exception("osync_xmlformat_search_field failed but did not set error code");
			return NULL;
		}

		PyObject *ret = xmlfieldlist_to_pylist(list);
		/* FIXME: osync_xmlfieldlist_free frees the list structure and the nodes as well,
		 * but we are returning references to the nodes, so here we just want to free the list.
		 * So, we reach around the API and free the list object directly. */
		free(list);
		return ret;
	}

	/* returns a python string object */
	PyObject *assemble() {
		char *buf;
		int size;
		osync_bool ret = osync_xmlformat_assemble(self, &buf, &size);
		if (!ret) {
			wrapper_exception("osync_xmlformat_assemble failed\n")
			return NULL;
		}
		PyObject *obj = PyString_FromStringAndSize(buf, size);
		free(buf);
		return obj;
	}

	bool validate() {
		return osync_xmlformat_validate(self);
	}

	void sort() {
		osync_xmlformat_sort(self);
	}

	/* TODO: compare */

%pythoncode %{
	objtype = property(get_objtype)
%}
}

%inline %{
	%cstring_input_binary(const char *buffer, unsigned int size);
	XMLFormat *xmlformat_parse(const char *buffer, unsigned int size) {
		Error *err = NULL;
		XMLFormat *xmlformat = osync_xmlformat_parse(buffer, size, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return xmlformat;
	}

%}


typedef struct {} XMLField;
%extend XMLField {
	XMLField(XMLFormat *xmlformat, const char *name) {
		Error *err = NULL;
		XMLField *xmlfield = osync_xmlfield_new(xmlformat, name, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return xmlfield;
	}

	~XMLField() {
		osync_xmlfield_delete(self);
	}

	/* TODO: lots of methods */
}


typedef struct {} Merger;
%extend Merger {
	Merger(Capabilities *capabilities) {
		Error *err = NULL;
		Merger *merger = osync_merger_new(capabilities, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return merger;
	}

	~Merger() {
		osync_merger_unref(self);
	}

	void merge(XMLFormat *xmlformat, XMLFormat *entire) {
		osync_merger_merge(self, xmlformat, entire);
	}

	void demerge(XMLFormat *xmlformat) {
		osync_merger_demerge(self, xmlformat);
	}
}