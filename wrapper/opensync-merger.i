typedef struct {} Capability;
%extend Capability {
	Capability(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

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
	Capabilities(PyObject *obj) {
		Capabilities *caps = PyCObject_AsVoidPtr(obj);
		osync_capabilities_ref(caps);
		return caps;
	}

	Capabilities() {
		Error *err = NULL;
		Capabilities *caps = osync_capabilities_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return caps;
	}

	/* TODO: capabilities_parse() */

	~Capabilities() {
		osync_capabilities_unref(self);
	}

	Capability *get_first(const char *objtype) {
		return osync_capabilities_get_first(self, objtype);
	}
	
	/* TODO: lots! */
}


typedef struct {} Merger;
%extend Merger {
	Merger(PyObject *obj) {
		Merger *merger = PyCObject_AsVoidPtr(obj);
		osync_merger_ref(merger);
		return merger;
	}

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

	/* TODO: merge, demerge */
}


/* TODO: xmlformat, xmlfield, xmlfieldlist */
