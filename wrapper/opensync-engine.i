typedef enum {} EngineCmd;
%constant int ENGINE_COMMAND_CONNECT = OSYNC_ENGINE_COMMAND_CONNECT;
%constant int ENGINE_COMMAND_READ = OSYNC_ENGINE_COMMAND_READ;
%constant int ENGINE_COMMAND_WRITE = OSYNC_ENGINE_COMMAND_WRITE;
%constant int ENGINE_COMMAND_SYNC_DONE = OSYNC_ENGINE_COMMAND_SYNC_DONE;
%constant int ENGINE_COMMAND_DISCONNECT = OSYNC_ENGINE_COMMAND_DISCONNECT;
%constant int ENGINE_COMMAND_SOLVE = OSYNC_ENGINE_COMMAND_SOLVE;
%constant int ENGINE_COMMAND_DISCOVER = OSYNC_ENGINE_COMMAND_DISCOVER;

typedef enum {} EngineState;
%constant int ENGINE_STATE_UNINITIALIZED = OSYNC_ENGINE_STATE_UNINITIALIZED;
%constant int ENGINE_STATE_INITIALIZED = OSYNC_ENGINE_STATE_INITIALIZED;
%constant int ENGINE_STATE_WAITING = OSYNC_ENGINE_STATE_WAITING;
%constant int ENGINE_STATE_CONNECTING = OSYNC_ENGINE_STATE_CONNECTING;
%constant int ENGINE_STATE_READING = OSYNC_ENGINE_STATE_READING;
%constant int ENGINE_STATE_WRITING = OSYNC_ENGINE_STATE_WRITING;
%constant int ENGINE_STATE_DISCONNECTING = OSYNC_ENGINE_STATE_DISCONNECTING;

typedef enum {} EngineEvent;
%constant int ENGINE_EVENT_CONNECTED = OSYNC_ENGINE_EVENT_CONNECTED;
%constant int ENGINE_EVENT_ERROR = OSYNC_ENGINE_EVENT_ERROR;
%constant int ENGINE_EVENT_READ = OSYNC_ENGINE_EVENT_READ;
%constant int ENGINE_EVENT_WRITTEN = OSYNC_ENGINE_EVENT_WRITTEN;
%constant int ENGINE_EVENT_SYNC_DONE = OSYNC_ENGINE_EVENT_SYNC_DONE;
%constant int ENGINE_EVENT_DISCONNECTED = OSYNC_ENGINE_EVENT_DISCONNECTED;
%constant int ENGINE_EVENT_SUCCESSFUL = OSYNC_ENGINE_EVENT_SUCCESSFUL;
%constant int ENGINE_EVENT_END_CONFLICTS = OSYNC_ENGINE_EVENT_END_CONFLICTS;
%constant int ENGINE_EVENT_PREV_UNCLEAN = OSYNC_ENGINE_EVENT_PREV_UNCLEAN;

typedef enum {} MemberEvent;
%constant int CLIENT_EVENT_CONNECTED = OSYNC_CLIENT_EVENT_CONNECTED;
%constant int CLIENT_EVENT_ERROR = OSYNC_CLIENT_EVENT_ERROR;
%constant int CLIENT_EVENT_READ = OSYNC_CLIENT_EVENT_READ;
%constant int CLIENT_EVENT_WRITTEN = OSYNC_CLIENT_EVENT_WRITTEN;
%constant int CLIENT_EVENT_SYNC_DONE = OSYNC_CLIENT_EVENT_SYNC_DONE;
%constant int CLIENT_EVENT_DISCONNECTED = OSYNC_CLIENT_EVENT_DISCONNECTED;
%constant int CLIENT_EVENT_DISCOVERED = OSYNC_CLIENT_EVENT_DISCOVERED;

typedef enum {} ChangeEvent;
%constant int CHANGE_EVENT_READ = OSYNC_CHANGE_EVENT_READ;
%constant int CHANGE_EVENT_WRITTEN = OSYNC_CHANGE_EVENT_WRITTEN;
%constant int CHANGE_EVENT_ERROR = OSYNC_CHANGE_EVENT_ERROR;

typedef enum {} MappingEvent;
%constant int MAPPING_EVENT_SOLVED = OSYNC_MAPPING_EVENT_SOLVED;
%constant int MAPPING_EVENT_ERROR = OSYNC_MAPPING_EVENT_ERROR;

/* modified from SWIG docs, typemap to pass a python callable (function) object */
%typemap(in) PyObject *pyfunc {
	if (!PyCallable_Check($input)) {
		PyErr_SetString(PyExc_TypeError, "Need a callable object!");
		return NULL;
	}
	$1 = $input;
}

typedef struct {} Engine;
%extend Engine {
	Engine(Group *group) {
		Error *err = NULL;
		Engine *engine = osync_engine_new(group, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return engine;
	}

	~Engine() {
		osync_engine_unref(self);
	}

	void set_plugindir(const char *dir) {
		osync_engine_set_plugindir(self, dir);
	}

	void set_formatdir(const char *dir) {
		osync_engine_set_formatdir(self, dir);
	}

	Group *get_group() {
		Group *ret = osync_engine_get_group(self);
		if (ret)
			osync_group_ref(ret);
		return ret;
	}

/* TODO: Archive not wrapped yet
	Archive *get_archive() {
		Archive *ret = osync_engine_get_archive(self);
		if (ret)
			osync_archive_ref(ret);
		return ret;
	}
*/

	void initialize() {
		Error *err = NULL;
		bool ret;
		PyEval_InitThreads();
		Py_BEGIN_ALLOW_THREADS
		ret = osync_engine_initialize(self, &err);
		Py_END_ALLOW_THREADS
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_engine_initialize failed but did not set error code");
	}

	void finalize() {
		Error *err = NULL;
		bool ret;
		PyEval_InitThreads();
		Py_BEGIN_ALLOW_THREADS
		ret = osync_engine_finalize(self, &err);
		Py_END_ALLOW_THREADS
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_engine_finalize failed but did not set error code");
	}

	void synchronize() {
		Error *err = NULL;
		bool ret;
		PyEval_InitThreads();
		Py_BEGIN_ALLOW_THREADS
		ret = osync_engine_synchronize(self, &err);
		Py_END_ALLOW_THREADS
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_engine_synchronize failed but did not set error code");
	}

	void synchronize_and_block() {
		Error *err = NULL;
		bool ret;
		PyEval_InitThreads();
		Py_BEGIN_ALLOW_THREADS
		ret = osync_engine_synchronize_and_block(self, &err);
		Py_END_ALLOW_THREADS
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_engine_synchronize_and_block failed but did not set error code");
	}

	void wait_sync_end() {
		Error *err = NULL;
		bool ret;
		PyEval_InitThreads();
		Py_BEGIN_ALLOW_THREADS
		ret = osync_engine_wait_sync_end(self, &err);
		Py_END_ALLOW_THREADS
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_engine_wait_sync_end failed but did not set error code");
	}

	void discover(Member *member) {
		Error *err = NULL;
		bool ret;
		PyEval_InitThreads();
		Py_BEGIN_ALLOW_THREADS
		ret = osync_engine_discover(self, member, &err);
		Py_END_ALLOW_THREADS
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_engine_discover failed but did not set error code");
	}

	void discover_and_block(Member *member) {
		Error *err = NULL;
		bool ret;
		PyEval_InitThreads();
		Py_BEGIN_ALLOW_THREADS
		ret = osync_engine_discover_and_block(self, member, &err);
		Py_END_ALLOW_THREADS
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_engine_discover_and_block failed but did not set error code");
	}

	%{
		static void enginestatus_cb_wrapper(OSyncEngineUpdate *arg, void *clientdata) {
			PyGILState_STATE pystate = PyGILState_Ensure();

			PyObject *pyfunc = clientdata;
			PyObject *errobj = SWIG_NewPointerObj(arg->error, SWIGTYPE_p_Error, 0);
			PyObject *args = Py_BuildValue("(iO)", arg->type, errobj);
			PyObject *result = PyEval_CallObject(pyfunc, args);
			Py_DECREF(args);
			Py_XDECREF(result);

			/* there's no way to report an error to OpenSync, so we just print it */
			if (PyErr_Occurred())
				PyErr_Print();

			PyGILState_Release(pystate);
		}
	%}

	/* FIXME: we'll leak a reference to the function object if this callback is overwritten */
	void set_enginestatus_callback(PyObject *pyfunc) {
		Py_INCREF(pyfunc);
		PyEval_InitThreads();
		osync_engine_set_enginestatus_callback(self, enginestatus_cb_wrapper, pyfunc);
	}

	/* TODO: other callbacks */

	void set_group_slowsync(bool isslowsync) {
		osync_engine_set_group_slowsync(self, isslowsync);
	}

	bool get_group_slowsync() {
		return osync_engine_get_group_slowsync(self);
	}

	void slowsync_objtype(const char *objtype) {
		osync_engine_slowsync_objtype(self, objtype);
	}

	void event(EngineEvent ev) {
		PyEval_InitThreads();
		Py_BEGIN_ALLOW_THREADS
		osync_engine_event(self, ev);
		Py_END_ALLOW_THREADS
	}

	bool check_get_changes() {
		return osync_engine_check_get_changes(self);
	}

	/* TODO: proxies, mappings */

%pythoncode %{
	group = property(get_group)
	# archive = property(get_archive)
	group_slowsync = property(get_group_slowsync, set_group_slowsync)
%}
}
