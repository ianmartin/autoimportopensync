/** @file Python module proxy plugin
 *
 * (c) Copyright 2005 Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 *
 * @author Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 */

#include <Python.h>

#include <opensync/opensync.h>
#include <signal.h>


struct MemberData {
	PyThreadState *interp_thread;
	PyObject *module;
	PyObject *object;
};

/*FIXME: Provide the opensync plugin API to python modules.
 * Probably only a OSyncContext class implementation will suffice
 *
 * (report_change, report_error, report_success, etc.)
 *
 * Then, add parameters to the initialize, connect, etc. functions
 * of the python module
 */

/** Insert plugin search path to sys.path
 *
 * Isn't there an easier way of setting it?
 */
static int change_sys_path()
{
	int rv = -1;
	PyObject *sys, *path, *dir;

	sys = PyImport_ImportModule("sys");
	if (!sys) goto error_sys;

	path = PyObject_GetAttrString(sys, "path");
	if (!path) goto error_path;

	dir = PyString_FromString(OPENSYNC_PYTHONPLG_DIR);
	if (!dir) goto error_dir;

	if (PyList_Insert(path, 0, dir) < 0)
		goto error_insert;

	/* Success */
	rv = 0;

error_insert:
	Py_DECREF(dir);
error_dir:
	Py_DECREF(path);
error_path:
	Py_DECREF(sys);
error_sys:
	return rv;
}

/** Calls the method initialize function
 *
 * The initialize() function should return an object that
 * has the other plugin methods (get_changeinfo, commit, etc.)
 */
static void *py_initialize(OSyncMember *member)
{
	struct MemberData *data = malloc(sizeof(struct MemberData));
	if (!data)
		return NULL;

	/*FIXME: the code loading should be on the get_info() function,
	 * if we support registering many plugins by a single
	 * loadable module.
	 *
	 * Suggestion: on get_info(), use the main interpreter, to avoid
	 * creating interpreter objects only to get plugin information.
	 * Then on initialize(), load the plugin using another interpreter
	 * (or thread state?) to keep the members on separated interpreters (or
	 * threads)
	 */
	data->interp_thread = Py_NewInterpreter();
	if (!data->interp_thread) {
		osync_debug("python", 1, "Couldn't initialize python interpreter");
		goto error_free_data;
	}
	if (change_sys_path() < 0) {
		osync_debug("python", 1, "Exception when initializing python intepreter");
		PyErr_Clear();
		goto error_free_interp;
	}
	data->module = PyImport_ImportModule("testmodule");
	if (!data->module) {
		osync_debug("python", 1, "Couldn't load testmodule");
		PyErr_Clear();
		goto error_free_interp;
	}
	/* Call the method */
	data->object = PyObject_CallMethod(data->module, "initialize", NULL);
	if (!data->object) {
		osync_debug("python", 1, "Error during initialize()");
		PyErr_Clear();
		goto error_unload_module;
	}

	/* Done */
	PyEval_ReleaseThread(data->interp_thread);

	return data;

error_unload_module:
	Py_DECREF(data->module);
error_free_interp:
	Py_EndInterpreter(data->interp_thread);
error_free_data:
	free(data);
	return NULL;
}

static void py_finalize(void *data)
{ /*FIXME: Implement me */
	struct MemberData *mydata = data;
	PyEval_AcquireThread(mydata->interp_thread);
	{
		PyObject *ret = PyObject_CallMethod(mydata->object, "finalize", NULL);
		if (!ret) {
			osync_debug("python", 1, "Error during finalize()");
			PyErr_Clear();
		} else
			Py_DECREF(ret);
	}
	Py_DECREF(mydata->object);
	Py_DECREF(mydata->module);
	Py_EndInterpreter(mydata->interp_thread);
}

static void call_module_method(OSyncContext *ctx, char *name)
{
	struct MemberData *data = osync_context_get_plugin_data(ctx);
	PyEval_AcquireThread(data->interp_thread);
	/*FIXME: send parameters */
	{
		PyObject *ret = PyObject_CallMethod(data->object, name, NULL);
		if (!ret) {
			osync_debug("python", 1, "Error during %s() method", name);
			PyErr_Clear();
		} else
			Py_DECREF(ret);
	}
	PyEval_ReleaseThread(data->interp_thread);
}

static void py_connect(OSyncContext *ctx)
{
	call_module_method(ctx, "connect");
}


static void py_get_changeinfo(OSyncContext *ctx)
{
	call_module_method(ctx, "get_changeinfo");
}


static void py_get_data(OSyncContext *ctx, OSyncChange *change)
{ /*FIXME: Implement me */
}

static osync_bool py_access(OSyncContext *ctx, OSyncChange *change)
{ /*FIXME: Implement me */
	return 0;
}

static osync_bool py_commit_change(OSyncContext *ctx, OSyncChange *change)
{ /*FIXME: Implement me */
	return 0;
}

static void py_sync_done(OSyncContext *ctx)
{ /*FIXME: Implement me */
}

static void py_disconnect(OSyncContext *ctx)
{ /*FIXME: Implement me */
}

void get_info(OSyncPluginInfo *info)
{
	/* Python initialization */
	struct sigaction old_sigint;

	/* Hack to make python not overwrite SIGINT */
	sigaction(SIGINT, NULL, &old_sigint);  /* Save old handler */
	Py_Initialize();
	sigaction(SIGINT, &old_sigint, NULL);  /* Restore it */
	PyEval_InitThreads();

	info->name = "python_module";
	info->version = 1;
	info->is_threadsafe = 1;
	
	info->functions.initialize = py_initialize;
	info->functions.connect = py_connect;
	info->functions.sync_done = py_sync_done;
	info->functions.disconnect = py_disconnect;
	info->functions.finalize = py_finalize;
	info->functions.get_changeinfo = py_get_changeinfo;
	info->functions.get_data = py_get_data;
	
	/*FIXME: We can't know all accepted objtypes here. Only after
	 * loading the python module. This is possible only after loading
	 * the python module.
	 *
	 * There are two approaches to fix this: the first one is
	 * getting the python module name from the configuration
	 * data, loading the module and registering the formats on
	 * initialize() time. Then we need to support registering
	 * the accepted formats on initialize(). Even if we choose
	 * the other approach described below, maybe it would be a good thing
	 * make possible to register the accepted formats on initialize
	 * (but I am not sure, maybe it is a good thing to force the
	 * format information to be available after get_info()
	 *
	 * Another possible fix: make possible to
	 * a single loadable module report any number of plugin info,
	 * so the get_info() function here just
	 * scans the OSYNC_LIBDIR/python-plugins directory, and reports
	 * all plugins. I think this is more elegant, and the better approach.
	 * It has the disadvantage of loading the python interpreter at plugin-listing
	 * time, but if we have any python plugins available, listing them
	 * is expected, anyway
	 */
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard");
	osync_plugin_set_commit_objformat(info, "contact", "vcard", py_commit_change);
	osync_plugin_set_access_objformat(info, "contact", "vcard", py_access);
}
