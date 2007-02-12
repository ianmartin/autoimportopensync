%{
#include <opensync/opensync-context.h>
%}

typedef struct {} OSyncContext;

%feature("ref")   OSyncContext "osync_context_ref($this);"
%feature("unref") OSyncContext "osync_context_unref($this);"

%extend OSyncContext {
	OSyncContext(PyObject *obj) {
		OSyncContext *context = PyCObject_AsVoidPtr(obj);
		osync_context_ref(context);
		return context;
	}

	OSyncContext() {
		OSyncError *err = NULL;
		OSyncContext *context = osync_context_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return context;
	}

	/* TODO: set_{,changes,warning}_callback */

	void report_error(OSyncErrorType type, const char *msg) {
		osync_context_report_error(self, type, "%s", msg);
	}

	void report_success() {
		osync_context_report_success(self);
	}

	void report_osyncerror(OSyncError *error) {
		osync_context_report_osyncerror(self, error);
	}

	void report_osyncwarning(OSyncError *error) {
		osync_context_report_osyncwarning(self, error);
	}

	void report_change(OSyncChange *change) {
		osync_context_report_change(self, change);
	}
};