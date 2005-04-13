/** @file Python module proxy plugin
 *
 * (c) Copyright 2005 Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 *
 * @author Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 */

#include <Python.h>

#include <opensync/opensync.h>
#include <signal.h>
#include <glib.h>
#include "config.h"

struct MemberData {
	PyThreadState *interp_thread;
	PyObject *module;
	PyObject *object;
	PyObject *osync_module;
};

#if 0
/** Insert plugin search path in sys.path
 *
 * Isn't there an easier way of setting it?
 *
 *FIXME: It would be better if we just load modules from
 * their filename, not setting the module search path
 */
/*static int change_sys_path()
{
	int rv = -1;
	PyObject *sys = NULL;
	PyObject *path = NULL;
	PyObject *dir = NULL;

	sys = PyImport_ImportModule("sys");
	if (!sys) goto error;

	path = PyObject_GetAttrString(sys, "path");
	if (!path) goto error;

	dir = PyString_FromString(OPENSYNC_PYTHONPLG_DIR);
	if (!dir) goto error;

	if (PyList_Insert(path, 0, dir) < 0)
		goto error;

	rv = 0;

error:
	Py_XDECREF(dir);
	Py_XDECREF(path);
	Py_XDECREF(sys);
	return rv;
}*/

/** Calls the method initialize function
 *
 * The python initialize() function should return an object that
 * has the other plugin methods (get_changeinfo, commit, etc.)
 */
static void *py_initialize(OSyncMember *member, OSyncError **error)
{
	char *name;
	struct MemberData *data = malloc(sizeof(struct MemberData));
	if (!data)
		/*FIXME: set error info */
		return NULL;

	/* The plugin name was set on register_plugin() on plugin_data */
	name = osync_plugin_get_plugin_data(osync_member_get_plugin(member));
	if (!name)
		/*FIXME: set error info */
		return NULL;

	data->interp_thread = Py_NewInterpreter();
	if (0 && !data->interp_thread) {
		osync_debug("python", 1, "Couldn't initialize python interpreter");
		goto error_free_data;
	}
	if (change_sys_path() < 0) {
		osync_debug("python", 1, "Exception when initializing python intepreter");
		PyErr_Print();
		PyErr_Clear();
		goto error_free_interp;
	}
	data->osync_module = PyImport_ImportModule("opensync");
	if (!data->module) {
		osync_debug("python", 1, "Couldn't load OpenSync module");
		PyErr_Print();
		PyErr_Clear();
		goto error_free_interp;
	}
	data->module = PyImport_ImportModule(name);
	if (!data->module) {
		osync_debug("python", 1, "Couldn't load testmodule");
		PyErr_Print();
		PyErr_Clear();
		goto error_unload_osync_mod;
	}

	/* Call the method */
	data->object = PyObject_CallMethod(data->module, "initialize", NULL);
	if (!data->object) {
		osync_debug("python", 1, "Error during initialize()");
		PyErr_Print();
		PyErr_Clear();
		goto error_unload_module;
	}

	/* Done */
	PyEval_ReleaseThread(data->interp_thread);

	return data;

error_unload_module:
	Py_DECREF(data->module);
error_unload_osync_mod:
	Py_DECREF(data->osync_module);
error_free_interp:
	Py_EndInterpreter(data->interp_thread);
error_free_data:
	free(data);
	return NULL;
}

static void py_finalize(void *data)
{
	struct MemberData *mydata = data;
	PyEval_AcquireThread(mydata->interp_thread);
	{
		PyObject *ret = PyObject_CallMethod(mydata->object, "finalize", NULL);
		if (!ret) {
			osync_debug("python", 1, "Error during finalize()");
			PyErr_Print();
			PyErr_Clear();
		} else
			Py_DECREF(ret);
	}
	Py_DECREF(mydata->object);
	Py_DECREF(mydata->module);
	Py_DECREF(mydata->osync_module);
	free(mydata);

	Py_EndInterpreter(mydata->interp_thread);
}

/** Create a new opensync.Change object for a given change */
static PyObject *new_pychange(struct MemberData *mydata, OSyncChange *chg)
{
	PyObject *cobject = NULL;
	PyObject *ret = NULL;

	cobject = PyCObject_FromVoidPtr(chg, NULL);
	if (!cobject)
		goto error;

	/* ret = opensync.Context(member=None, chg=cobject) */
	ret = PyObject_CallMethod(mydata->osync_module, "Change", "OO", Py_None, cobject);

	/* Done, drop reference even on success (the opensync.Change object
	 * will hold a reference to the cobject)
	 */
	Py_XDECREF(cobject);

	return ret;

error:
	Py_XDECREF(ret);
	Py_XDECREF(cobject);
	return NULL;
}

/** Create a new opensync.Context object for a given context */
static PyObject *new_pycontext(struct MemberData *mydata, OSyncContext *ctx)
{
	PyObject *cobject = NULL;
	PyObject *ret = NULL;

	cobject = PyCObject_FromVoidPtr(ctx, NULL);
	if (!cobject)
		goto error;

	/* ret = opensync.Context(cobject) */
	ret = PyObject_CallMethod(mydata->osync_module, "Context", "O", cobject);

	/* Done, drop reference even on success (the opensync.Context object
	 * will hold a reference to the cobject)
	 */
	Py_XDECREF(cobject);

	return ret;

error:
	Py_XDECREF(ret);
	Py_XDECREF(cobject);
	return NULL;
}

/** Call a python method
 *
 * Methods called using this function can
 * have one of these formats:
 *
 * - function(context)
 * - function(context, change)
 */
static void call_module_method(OSyncContext *ctx, OSyncChange *chg, char *name)
{
	PyObject *context = NULL;
	PyObject *ret = NULL;
	PyObject *change = NULL;

	struct MemberData *data = osync_context_get_plugin_data(ctx);
	PyEval_AcquireThread(data->interp_thread);

	if (chg) {
		change = new_pychange(data, chg);
		if (!change) {
			osync_debug("python", 1, "Can't create a change object");
			PyErr_Print();
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't create a opensync.Change object");
			PyErr_Clear();
			goto out;
		}
	}

	context = new_pycontext(data, ctx);
	if (!context) {
		osync_debug("python", 1, "Can't create a context object");
		PyErr_Print();
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't create a opensync.Context object");
		PyErr_Clear();
		goto out;
	}

	if (change)
		ret = PyObject_CallMethod(data->object, name, "OO", context, change);
	else
		ret = PyObject_CallMethod(data->object, name, "O", context);

	if (!ret) {
		osync_debug("python", 1, "Error during %s() method", name);
		PyErr_Print();
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't call python %s() method", name);
		PyErr_Clear();
		goto out;
	}

out:
	Py_XDECREF(ret);
	Py_XDECREF(change);
	Py_XDECREF(context);
	PyEval_ReleaseThread(data->interp_thread);
}

static void py_connect(OSyncContext *ctx)
{
	call_module_method(ctx, NULL, "connect");
}


static void py_get_changeinfo(OSyncContext *ctx)
{
	call_module_method(ctx, NULL, "get_changeinfo");
}


static void py_get_data(OSyncContext *ctx, OSyncChange *change)
{
	call_module_method(ctx, change, "get_data");
}

static osync_bool py_access(OSyncContext *ctx, OSyncChange *change)
{
	call_module_method(ctx, change, "access");
	return TRUE;
}

static osync_bool py_commit_change(OSyncContext *ctx, OSyncChange *change)
{
	call_module_method(ctx, change, "commit_change");
	return 0;
}

static void py_sync_done(OSyncContext *ctx)
{
	call_module_method(ctx, NULL, "sync_done");
}

static void py_disconnect(OSyncContext *ctx)
{
	call_module_method(ctx, NULL, "disconnect");
}
#endif

/** Register a new plugin from python module called name.
 *
 * @todo We need to avoid the loading of the python runtime
 *       on get_info() somehow, but then we need to store all
 *       plugin information on another place (including
 *       accepted objtypes/formats info)
 */
static osync_bool register_plugin(OSyncEnv *env, PyObject *osync_module, char *filename, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p. %p, %s, %p)", __func__, env, osync_module, filename, error);
	//PyObject *get_info_result = NULL;
	//PythonPluginInfo pyinfo;
	//PyObject *pyinfo_cobject = NULL;
	//PyObject *get_info_parm = NULL;
	//OSyncPluginInfo *info;

	osync_trace(TRACE_INTERNAL, "Opening file");
	FILE *fp = fopen(filename, "r");
	osync_trace(TRACE_INTERNAL, "Running file");
	PyImport_AddModule("sys");
	PyRun_SimpleFile(fp, filename);
	
	PyObject *module = PyImport_AddModule("__main__");
	
	if (!module) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't load module from file %s", filename);
		PyErr_Print();
		PyErr_Clear();
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	
	osync_trace(TRACE_INTERNAL, "Calling file");
	osync_trace(TRACE_INTERNAL, "Calling file %i", PyModule_Check(module));
	
	PyObject *pyret = PyObject_CallMethod(module, "blubbasd", "i", 1);
	if (PyErr_Occurred())
			PyErr_Print();
	if (pyret != NULL) {
			printf("Result of call: %ld\n", PyInt_AsLong(pyret));
			Py_DECREF(pyret);
		}
	
	PyObject *pDict = PyModule_GetDict(module);
	osync_trace(TRACE_INTERNAL, "Dict is %p", pDict);
	/* pDict is a borrowed reference */
	
	PyObject *pFunc = PyDict_GetItemString(pDict, "blubbasd");
	osync_trace(TRACE_INTERNAL, "func is %p", pFunc);
	/* pFun: Borrowed reference */

	if (pFunc && PyCallable_Check(pFunc)) {
		PyObject *pArgs = PyTuple_New(1);
		PyObject *pValue = PyInt_FromLong(2);
		if (!pValue) {
			Py_DECREF(pArgs);
			fprintf(stderr, "Cannot convert argument\n");
			return 1;
		}
		/* pValue reference stolen here: */
		PyTuple_SetItem(pArgs, 0, pValue);
		pValue = PyObject_CallObject(pFunc, pArgs);
		Py_DECREF(pArgs);
		if (pValue != NULL) {
			printf("Result of call: %ld\n", PyInt_AsLong(pValue));
			Py_DECREF(pValue);
		}
	} else {
		if (PyErr_Occurred())
			PyErr_Print();
		fprintf(stderr, "Cannot find function \"blubbasd\"\n");
	}
	
#if 0
	info = osync_plugin_new_info(env);
	/*info->functions.initialize = py_initialize;
	info->functions.connect = py_connect;
	info->functions.sync_done = py_sync_done;
	info->functions.disconnect = py_disconnect;
	info->functions.finalize = py_finalize;
	info->functions.get_changeinfo = py_get_changeinfo;
	info->functions.get_data = py_get_data;*/
	
	/* The plugin data is just the plugin name,
	 * to be used on py_initialize()
	 */
	info->plugin_data = strdup(filename);
	
	/** Build a PluginInfo object for use by get_info */
	//pyinfo.commit_fn = py_commit_change;
	//pyinfo.access_fn = py_access;
	//pyinfo.osync_info = info;

	pyinfo_cobject = PyCObject_FromVoidPtr(&pyinfo, NULL);
	if (!pyinfo_cobject) {
		osync_debug("python", 1, "Can't create pyinfo_cobject");
		PyErr_Print();
		PyErr_Clear();
		goto error_cancel_register;
	}

	/* get_info_parm = opensync.PluginInfo(pyinfo_cobject) */
	get_info_parm = PyObject_CallMethod(osync_module, "PluginInfo", "O", pyinfo_cobject);
	if (!get_info_parm) {
		osync_debug("python", 1, "Can't create get_info_parm");
		PyErr_Print();
		PyErr_Clear();
		goto error_cancel_register;
	}
	/* Call get_info */
	get_info_result = PyObject_CallMethod(module, "get_info", "O", get_info_parm);
	if (!get_info_result) {
		osync_debug("python", 1, "Error during initialize()");
		PyErr_Print();
		PyErr_Clear();
		goto error_cancel_register;
	}

	if (!info->name) {
		osync_debug("python", 1, "The plugin didn't set its name!");
		goto error_cancel_register;
	}
	osync_trace(TRACE_EXIT, "register_plugin");

	Py_XDECREF(get_info_result);
	Py_XDECREF(get_info_parm);
	Py_XDECREF(pyinfo_cobject);
	Py_XDECREF(module);
#endif

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

static osync_bool scan_for_plugins(OSyncEnv *env, PyObject *osync_module)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);

	char *path = OPENSYNC_PYTHONPLG_DIR;
	GError *gerror = NULL;
	GDir *dir = g_dir_open(path, 0, &gerror);
	if (!dir) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to open directory %s: %s", __func__, path, gerror ? gerror->message : "None");
		return FALSE;
	}

	const char *de = NULL;
	while ((de = g_dir_read_name(dir))) {
		char *filename = g_build_filename(path, de, NULL);
		OSyncError *error = NULL;
		if (!register_plugin(env, osync_module, filename, &error))
			osync_debug("python", 1, "Couldn't register plugin \"%s\": %s", filename, osync_error_print(&error));

		g_free(filename);
	}
	g_dir_close(dir);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

void get_info(OSyncEnv *env)
{
	/* Python initialization */
	struct sigaction old_sigint;

	/* Hack to make python not overwrite SIGINT */
	sigaction(SIGINT, NULL, &old_sigint);  /* Save old handler */
	Py_Initialize();
	sigaction(SIGINT, &old_sigint, NULL);  /* Restore it */
	PyEval_InitThreads();

	PyObject *osync_module = PyImport_ImportModule("opensync");
	if (!osync_module) {
		osync_debug("python", 1, "Couldn't load OpenSync module");
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	scan_for_plugins(env, osync_module);
}
