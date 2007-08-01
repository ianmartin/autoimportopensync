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

	%{
		static void context_callback_wrapper(void *clientdata, OSyncError *err) {
			PyGILState_STATE pystate = PyGILState_Ensure();

			PyObject *handler = clientdata;
			PyObject *errobj = SWIG_NewPointerObj(err, SWIGTYPE_p_Error, 0);
			PyObject *result = PyObject_CallMethod(handler, "callback", "(O)", errobj);
			Py_XDECREF(result);

			/* there's no way to report an error to OpenSync, so we just print it */
			if (PyErr_Occurred())
				PyErr_Print();

			PyGILState_Release(pystate);
		}

		static void context_changes_wrapper(OSyncChange *change, void *clientdata) {
			PyGILState_STATE pystate = PyGILState_Ensure();

			PyObject *handler = clientdata;
			PyObject *chgobj = SWIG_NewPointerObj(change, SWIGTYPE_p_Change, 0);
			PyObject *result = PyObject_CallMethod(handler, "changes", "(O)", chgobj);
			Py_XDECREF(result);

			/* there's no way to report an error to OpenSync, so we just print it */
			if (PyErr_Occurred())
				PyErr_Print();

			PyGILState_Release(pystate);
		}

		static void context_warning_wrapper(void *clientdata, OSyncError *err) {
			PyGILState_STATE pystate = PyGILState_Ensure();

			PyObject *handler = clientdata;
			PyObject *errobj = SWIG_NewPointerObj(err, SWIGTYPE_p_Error, 0);
			PyObject *result = PyObject_CallMethod(handler, "warning", "(O)", errobj);
			Py_XDECREF(result);

			/* there's no way to report an error to OpenSync, so we just print it */
			if (PyErr_Occurred())
				PyErr_Print();

			PyGILState_Release(pystate);
		}
	%}

	/* this should be passed an instance of a subclass of the ContextCallbacks class below
	 * FIXME: we'll leak a reference to the handler object if this callback is replaced */
	void set_callback_object(PyObject *handler) {
		Py_INCREF(handler);
		PyEval_InitThreads();
		osync_context_set_callback(self, context_callback_wrapper, handler);
		osync_context_set_changes_callback(self, context_changes_wrapper);
		osync_context_set_warning_callback(self, context_warning_wrapper);
	}
};

%pythoncode %{
class ContextCallbacks:
	"""A purely-Python class that should be subclassed by code that wishes to handle context callbacks."""
	def callback(self, err):
		"""If err is set, this call indicates that an error was reported. Otherwise success."""
		pass

	def changes(self, change):
		"""Called to report a change object."""
		pass

	def warning(self, err):
		"""Called to report a non-fatal error."""
		pass
%}
