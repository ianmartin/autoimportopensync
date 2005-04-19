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

typedef struct MemberData {
	PyThreadState *interp_thread;
	PyObject *module;
	PyObject *object;
} MemberData;

static PyObject *pm_load_opensync(OSyncError **error)
{
	PyObject *osync_module = PyImport_ImportModule("opensync");
	if (!osync_module) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't load OpenSync module");
		PyErr_Print();
		return NULL;
	}
	return osync_module;
}

static PyObject *pm_load_script(const char *filename, OSyncError **error)
{
	FILE *fp = fopen(filename, "r");
	if (!fp) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open file %s", filename);
		return NULL;
	}
	
	if (PyRun_SimpleFile(fp, filename) == -1) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't run module from file %s", filename);
		PyErr_Print();
		return NULL;
	}
	
	PyObject *module = PyImport_AddModule("__main__");
	if (!module) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't load module from file %s", filename);
		PyErr_Print();
		return NULL;
	}
	return module;
}

static PyObject *pm_make_change(PyObject *module, OSyncChange *change, OSyncError **error)
{
	PyObject *pychg_cobject = PyCObject_FromVoidPtr(change, NULL);
	if (!pychg_cobject) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldnt make pychg cobject");
		PyErr_Print();
		return NULL;
	}
	
	PyObject *pychg = PyObject_CallMethod(module, "OSyncChange", "O", pychg_cobject);
	if (!pychg) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot create Python OSyncChange");
		PyErr_Print();
		Py_XDECREF(pychg_cobject);
		return NULL;
	}
	return pychg;
}

static PyObject *pm_make_context(PyObject *module, OSyncContext *ctx, OSyncError **error)
{
	PyObject *pyctx_cobject = PyCObject_FromVoidPtr(ctx, NULL);
	if (!pyctx_cobject) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldnt make pyctx cobject");
		PyErr_Print();
		return NULL;
	}
	
	PyObject *pyctx = PyObject_CallMethod(module, "OSyncContext", "O", pyctx_cobject);
	if (!pyctx) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot create Python OSyncContext");
		PyErr_Print();
		Py_XDECREF(pyctx_cobject);
		return NULL;
	}
	return pyctx;
}

static PyObject *pm_make_member(PyObject *module, OSyncMember *member, OSyncError **error)
{
	PyObject *pymember_cobject = PyCObject_FromVoidPtr(member, NULL);
	if (!pymember_cobject) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldnt make pymember cobject");
		PyErr_Print();
		return NULL;
	}
	
	PyObject *pymember = PyObject_CallMethod(module, "OSyncMember", "O", pymember_cobject);
	if (!pymember) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot create Python OSyncMember");
		PyErr_Print();
		Py_XDECREF(pymember_cobject);
		return NULL;
	}
	return pymember;
}

/** Calls the method initialize function
 *
 * The python initialize() function should return an object that
 * has the other plugin methods (get_changeinfo, commit, etc.)
 */
static void *pm_initialize(OSyncMember *member, OSyncError **error)
{
	printf("Init called!\n");

	const char *name = osync_member_get_plugindata(member);
	if (!name) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No script name was set");
		return NULL;
	}

	MemberData *data = g_malloc(sizeof(MemberData));

	data->interp_thread = Py_NewInterpreter();
	if (!data->interp_thread) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't initialize python sub interpreter");
		goto error_free_data;
	}
	
	if (!pm_load_opensync(error))
		goto error_free_interp;
	
	if (!(data->module = pm_load_script(name, error)))
		goto error_free_interp;
	
	PyObject *pymember = pm_make_member(data->module, member, error);
	if (!pymember)
		goto error_unload_module;
	
	data->object = PyObject_CallMethod(data->module, "initialize", "O", pymember);
	if (!data->object) {
		osync_debug("python", 1, "Error during initialize()");
		PyErr_Print();
		PyErr_Clear();
		goto error_unload_module;
	}

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

static void pm_finalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	MemberData *mydata = data;
	PyEval_AcquireThread(mydata->interp_thread);
	{
		PyObject *ret = PyObject_CallMethod(mydata->object, "finalize", NULL);
		if (!ret) {
			osync_trace(TRACE_INTERNAL, "Error during finalize()");
			PyErr_Print();
		} else
			Py_DECREF(ret);
	}
	Py_DECREF(mydata->object);
	Py_DECREF(mydata->module);
	Py_EndInterpreter(mydata->interp_thread);
	
	free(mydata);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** Call a python method
 *
 * Methods called using this function can
 * have one of these formats:
 *
 * - function(context)
 * - function(context, change)
 */
static osync_bool pm_call_module_method(OSyncContext *ctx, OSyncChange *chg, char *name, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, %p)", __func__, ctx, chg, name, error);
	PyObject *pycontext = NULL;
	PyObject *ret = NULL;

	MemberData *data = osync_context_get_plugin_data(ctx);
	PyEval_AcquireThread(data->interp_thread);

	pycontext = pm_make_context(data->module, ctx, error);
	if (!pycontext) {
		PyEval_ReleaseThread(data->interp_thread);
		osync_context_report_osyncerror(ctx, error);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (chg) {
		PyObject *pychange = pm_make_change(data->module, chg, error);
		if (!pychange) {
			PyEval_ReleaseThread(data->interp_thread);
			osync_context_report_osyncerror(ctx, error);
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
			return FALSE;
		}
		
		ret = PyObject_CallMethod(data->object, name, "OO", pycontext, pychange);
		
		Py_XDECREF(pychange);
	} else {
		ret = PyObject_CallMethod(data->object, name, "O", pycontext);
	}
	
	if (!ret) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error during %s() method", name);
		PyErr_Print();
		PyEval_ReleaseThread(data->interp_thread);
		osync_context_report_osyncerror(ctx, error);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	Py_XDECREF(ret);
	PyEval_ReleaseThread(data->interp_thread);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

static void pm_connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	OSyncError *error = NULL;
	pm_call_module_method(ctx, NULL, "connect", &error);
	osync_trace(TRACE_EXIT, "%s", __func__);
}


static void pm_get_changeinfo(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	OSyncError *error = NULL;
	pm_call_module_method(ctx, NULL, "get_changeinfo", &error);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

#if 0
static void py_get_data(OSyncContext *ctx, OSyncChange *change)
{
	call_module_method(ctx, change, "get_data");
}
#endif

static osync_bool pm_access(OSyncContext *ctx, OSyncChange *change)
{	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);
	OSyncError *error = NULL;
	pm_call_module_method(ctx, change, "access", &error);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

static osync_bool pm_commit_change(OSyncContext *ctx, OSyncChange *change)
{	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);
	OSyncError *error = NULL;
	pm_call_module_method(ctx, change, "commit_change", &error);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

static void pm_sync_done(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	OSyncError *error = NULL;
	pm_call_module_method(ctx, NULL, "sync_done", &error);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void pm_disconnect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	OSyncError *error = NULL;
	pm_call_module_method(ctx, NULL, "disconnect", &error);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** Register a new plugin from python module called name.
 *
 * @todo We need to avoid the loading of the python runtime
 *       on get_info() somehow, but then we need to store all
 *       plugin information on another place (including
 *       accepted objtypes/formats info)
 */
static osync_bool register_plugin(OSyncEnv *env, const char *filename, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, filename, error);

	PyObject *module = pm_load_script(filename, error);
	if (!module) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	OSyncPluginInfo *info = osync_plugin_new_info(env);
	info->functions.initialize = pm_initialize;
	info->functions.connect = pm_connect;
	info->functions.get_changeinfo = pm_get_changeinfo;
	info->functions.sync_done = pm_sync_done;
	info->functions.disconnect = pm_disconnect;
	info->functions.finalize = pm_finalize;
	
	info->plugin_data = g_strdup(filename);
	
	PyObject *pyinfo_cobject = PyCObject_FromVoidPtr(info, NULL);
	if (!pyinfo_cobject) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldnt make pyinfo cobject");
		PyErr_Print();
		PyErr_Clear();
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	PyObject *pyinfo = PyObject_CallMethod(module, "OSyncPluginInfo", "O", pyinfo_cobject);
	osync_trace(TRACE_INTERNAL, "pyinfo: %p\n", pyinfo);
	if (!pyinfo) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot create Python OSyncPluginInfo");
		PyErr_Print();
		PyErr_Clear();
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	if (!PyObject_CallMethod(module, "get_info", "O", pyinfo)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error calling get_info");
		PyErr_Print();
		PyErr_Clear();
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	if (!info->name) {
		osync_debug("python", 1, "The plugin didn't set its name!");
	}

	osync_plugin_set_access_objformat(info, NULL, NULL, pm_access);
	osync_plugin_set_commit_objformat(info, NULL, NULL, pm_commit_change);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

static osync_bool scan_for_plugins(OSyncEnv *env)
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
		if (!register_plugin(env, filename, &error))
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

	if (!pm_load_opensync(NULL))
		return;

	scan_for_plugins(env);
}
