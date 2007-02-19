typedef struct {} Error;
%feature("exceptionclass") Error;
%extend Error {
	Error(PyObject *obj) {
		Error *error = PyCObject_AsVoidPtr(obj);
		osync_error_ref(&error);
		return error;
	}

	Error(const char *msg, ErrorType type=OSYNC_ERROR_GENERIC) {
		Error *error = NULL;
		osync_error_set(&error, type, "%s", msg);
		return error;
	}

	~Error() {
		osync_error_unref(&self);
	}

	const char *get_name() {
		return osync_error_get_name(&self);
	}

	bool is_set() {
		return osync_error_is_set(&self);
	}

	ErrorType get_type() {
		return osync_error_get_type(&self);
	}

	void set_type(ErrorType type) {
		osync_error_set_type(&self, type);
	}

	const char *get_msg() { // 'print' is a reserved word
		return osync_error_print(&self);
	}

	char *print_stack() {
		return osync_error_print_stack(&self);
	}

	void set_from_error(Error *source) {
		osync_error_set_from_error(&self, &source);
	}

	void set(ErrorType type, const char *msg) {
		osync_error_set(&self, type, "%s", msg);
	}

	void stack(Error *child) {
		osync_error_stack(&self, &child);
	}

	Error *get_child() {
		return osync_error_get_child(&self);
	}

%pythoncode %{
	# for some reason the OpenSync API only allows setting the msg with a type
	def __set_msg(self, msg):
		self.set(self.num, msg)

	def __str__(self):
		return self.get_name() + ": " + self.get_msg()

	def report(self, context):
		"""Report myself as an error to the given Context object."""
		context.report_osyncerror(self)

	name = property(get_name)
	is_set = property(is_set)
	num = property(get_type, set_type) # 'type' is a reserved word
	msg = property(get_msg, __set_msg)
%}
}

%{
/* If the given opensync error is set, raise a matching exception.
 * Returns TRUE iff an exception was raised. */
static bool
raise_exception_on_error(Error *oserr)
{
	if (!osync_error_is_set(&oserr)) {
		return FALSE;
	}

	PyObject *obj = SWIG_NewPointerObj(oserr, SWIGTYPE_p_Error, 0);
	PyErr_SetObject(SWIG_Python_ExceptionType(SWIGTYPE_p_Error), obj);
	Py_DECREF(obj);

	return TRUE;
}

static void
wrapper_exception(const char *msg)
{
	Error *err = NULL;
	osync_error_set(&err, OSYNC_ERROR_GENERIC, "internal wrapper error: %s", msg);
	raise_exception_on_error(err);
}
%}


typedef enum {} ErrorType;

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
