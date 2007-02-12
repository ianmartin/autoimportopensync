%{
#include <opensync/opensync-error.h>
%}

typedef struct {} OSyncError;

%feature("ref")   OSyncError "osync_error_ref($this);"
%feature("unref") OSyncError "osync_error_unref($this);"
%feature("exceptionclass") OSyncError;

%extend OSyncError {
	OSyncError(PyObject *obj) {
		OSyncError *error = PyCObject_AsVoidPtr(obj);
		return error;
	}

	OSyncError(const char *msg, OSyncErrorType type=OSYNC_ERROR_GENERIC) {
		OSyncError *error = NULL;
		osync_error_set(&error, type, "%s", msg);
		return error;
	}

	const char *get_name() {
		return osync_error_get_name(&self);
	}

	osync_bool is_set() {
		return osync_error_is_set(&self);
	}

	OSyncErrorType get_type() {
		return osync_error_get_type(&self);
	}

	void set_type(OSyncErrorType type) {
		osync_error_set_type(&self, type);
	}

	const char *get_msg() { // 'print' is a reserved word
		return osync_error_print(&self);
	}

	char *print_stack() {
		return osync_error_print_stack(&self);
	}

	void set_from_error(OSyncError *source) {
		osync_error_set_from_error(&self, &source);
	}

	void set(OSyncErrorType type, const char *msg) {
		osync_error_set(&self, type, "%s", msg);
	}

	void stack(OSyncError *child) {
		osync_error_stack(&self, &child);
	}

	OSyncError *get_child() {
		return osync_error_get_child(&self);
	}

%pythoncode %{
	# for some reason the OpenSync API only allows setting the msg with a type
	def __set_msg(self, msg):
		self.set(self.num, msg)

	def __str__(self):
		return self.get_name() + ": " + self.get_msg()

	def report(self, context):
		"""Report myself as an error to the given OSyncContext object."""
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
static osync_bool
raise_exception_on_error(OSyncError *oserr)
{
	if (!osync_error_is_set(&oserr)) {
		return FALSE;
	}

	PyObject *obj = SWIG_NewPointerObj(oserr, SWIGTYPE_p_OSyncError, 0);
	PyErr_SetObject(SWIG_Python_ExceptionType(SWIGTYPE_p_OSyncError), obj);
	Py_DECREF(obj);

	return TRUE;
}

static void
wrapper_exception(const char *msg)
{
	OSyncError *err = NULL;
	osync_error_set(&err, OSYNC_ERROR_GENERIC, "internal wrapper error: %s", msg);
	raise_exception_on_error(err);
}
%}


typedef enum {} OSyncErrorType;

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
