typedef struct {} Context;
%extend Context {
	/* called by python-module plugin */
	Context(PyObject *obj) {
		Context *context = PyCObject_AsVoidPtr(obj);
		osync_context_ref(context);
		return context;
	}

	Context() {
		Error *err = NULL;
		Context *context = osync_context_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return context;
	}

	~Context() {
		osync_context_unref(self);
	}

	/* TODO: set_{,changes,warning}_callback */

	void report_error(ErrorType type, const char *msg) {
		osync_context_report_error(self, type, "%s", msg);
	}

	void report_success() {
		osync_context_report_success(self);
	}

	void report_osyncerror(Error *error) {
		osync_context_report_osyncerror(self, error);
	}

	void report_osyncwarning(Error *error) {
		osync_context_report_osyncwarning(self, error);
	}

	void report_change(Change *change) {
		osync_context_report_change(self, change);
	}
};