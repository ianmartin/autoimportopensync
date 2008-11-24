typedef enum {} TraceType;

%constant int TRACE_ENTRY = TRACE_ENTRY;
%constant int TRACE_EXIT = TRACE_EXIT;
%constant int TRACE_INTERNAL = TRACE_INTERNAL;
%constant int TRACE_SENSITIVE = TRACE_SENSITIVE;
%constant int TRACE_EXIT_ERROR = TRACE_EXIT_ERROR;
%constant int TRACE_ERROR = TRACE_ERROR;

%inline %{
        /*
	static char *rand_str(int maxlength) {
		return osync_rand_str(maxlength);
	}
        */

	static void trace_reset_indent() {
		osync_trace_reset_indent();
	}

	static void trace(TraceType type, const char *message) {
		osync_trace(type, "%s", message);
	}

	static void trace_disable(void) {
		osync_trace_disable();
	}

	static void trace_enable(void) {
		osync_trace_enable();
	}

	static const char *get_version() {
		return osync_get_version();
	}
%}

%pythoncode %{
# this won't change, so just call it once at module load time
version = get_version()
%}
