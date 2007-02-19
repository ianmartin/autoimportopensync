/* Python module for OpenSync
 * Copyright (C) 2005  Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 * Copyright (C) 2007  Andrew Baumann <andrewb@cse.unsw.edu.au>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * @author Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 * @author Andrew Baumann <andrewb@cse.unsw.edu.au>
 *
 * Additional changes by Armin Bauer <armin.bauer@desscon.com>
 */

#include <Python.h>
#include <opensync/opensync.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>
#include <signal.h>
#include <glib.h>
#include "config.h"

/* change this define for python exception output on stderr */
//#define PYERR_CLEAR() PyErr_Clear()
#define PYERR_CLEAR() PyErr_Print()

typedef struct MemberData {
	PyThreadState *interp_thread;
	PyObject *osync_module;
	PyObject *module;
	GSList *sinks;
} MemberData;

static PyObject *pm_load_opensync(OSyncError **error)
{
	PyObject *osync_module = PyImport_ImportModule("opensync");
	if (!osync_module) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't load OpenSync module");
		PYERR_CLEAR();
		return NULL;
	}
	return osync_module;
}

static PyObject *pm_make_change(PyObject *osync_module, OSyncChange *change, OSyncError **error)
{
	PyObject *pychg_cobject = PyCObject_FromVoidPtr(change, NULL);
	if (!pychg_cobject) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldnt make pychg cobject");
		PYERR_CLEAR();
		return NULL;
	}
	
	PyObject *pychg = PyObject_CallMethod(osync_module, "Change", "O", pychg_cobject);
	Py_DECREF(pychg_cobject);
	if (!pychg) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot create Python OSyncChange");
		PYERR_CLEAR();
		return NULL;
	}
	return pychg;
}

static PyObject *pm_make_context(PyObject *osync_module, OSyncContext *ctx, OSyncError **error)
{
	PyObject *pyctx_cobject = PyCObject_FromVoidPtr(ctx, NULL);
	if (!pyctx_cobject) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldnt make pyctx cobject");
		PYERR_CLEAR();
		return NULL;
	}
	
	PyObject *pyctx = PyObject_CallMethod(osync_module, "Context", "O", pyctx_cobject);
	Py_DECREF(pyctx_cobject);
	if (!pyctx) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot create Python OSyncContext");
		PYERR_CLEAR();
		return NULL;
	}
	return pyctx;
}

static PyObject *pm_make_info(PyObject *osync_module, OSyncPluginInfo *info, OSyncError **error)
{
	PyObject *pyinfo_cobject = PyCObject_FromVoidPtr(info, NULL);
	if (!pyinfo_cobject) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldnt make pyinfo cobject");
		PYERR_CLEAR();
		return NULL;
	}
	
	PyObject *pyinfo = PyObject_CallMethod(osync_module, "PluginInfo", "O", pyinfo_cobject);
	Py_DECREF(pyinfo_cobject);
	if (!pyinfo) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot create Python OSyncPluginInfo");
		PYERR_CLEAR();
		return NULL;
	}
	return pyinfo;
}

/* convert a python exception to an OSyncError containing the traceback of the exception */
static void pm_pyexcept_to_oserror(PyObject *pytype, PyObject *pyvalue, PyObject *pytraceback, OSyncError **error)
{
	const char *errmsg = NULL;
	PyObject *tracebackmod = NULL, *stringmod = NULL;
	PyObject *pystrs = NULL, *pystr = NULL;
	
	tracebackmod = PyImport_ImportModule("traceback");
	if (!tracebackmod) {
		errmsg = "import traceback";
		goto error;
	}

	pystrs = PyObject_CallMethod(tracebackmod, "format_exception", "OOO", pytype, pyvalue, pytraceback);
	if (!pystrs) {
		errmsg = "traceback.format_exception";
		goto error;
	}

	stringmod = PyImport_ImportModule("string");
	if (!stringmod) {
		errmsg = "import string";
		goto error;
	}

	pystr = PyObject_CallMethod(stringmod, "join", "Os", pystrs, "");
	if (!pystr) {
		errmsg = "string.join";
		goto error;
	}

	osync_error_set(error, OSYNC_ERROR_GENERIC, "%s", PyString_AsString(pystr));

error:
	Py_XDECREF(tracebackmod);
	Py_XDECREF(stringmod);
	Py_XDECREF(pystrs);
	Py_XDECREF(pystr);

	if (errmsg) {
		PYERR_CLEAR();
		osync_error_set(error, OSYNC_ERROR_GENERIC, "pm_pyexcept_to_oserror: failed to report error: exception in %s", errmsg);
	}
}

/** Call a python method, report any exception it raises as an error, if no exception was raised report success
 *
 * Methods called using this function can
 * have one of these formats:
 *
 * - function(info, context)
 * - function(info, context, change)
 */
static osync_bool pm_call_module_method(MemberData *data, char *name, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %p, %p, %p)", __func__, name, info, ctx, chg);
	PyObject *ret = NULL;
	OSyncError *error = NULL;
	osync_bool report_error = TRUE;

	PyEval_AcquireThread(data->interp_thread);

	PyObject *pyinfo = pm_make_info(data->osync_module, info, &error);
	if (!pyinfo)
		goto error;

	PyObject *pycontext = pm_make_context(data->osync_module, ctx, &error);
	if (!pycontext) {
		Py_DECREF(pyinfo);
		goto error;
	}

	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	PyObject *sink_pyobject = osync_objtype_sink_get_userdata(sink);

	if (chg) {
		PyObject *pychange = pm_make_change(data->osync_module, chg, &error);
		if (!pychange) {
			Py_DECREF(pyinfo);
			Py_DECREF(pycontext);
			goto error;
		}
		
		ret = PyObject_CallMethod(sink_pyobject, name, "OOO", pyinfo, pycontext, pychange);
		
		Py_DECREF(pychange);
	} else {
		ret = PyObject_CallMethod(sink_pyobject, name, "OO", pyinfo, pycontext);
	}

	Py_DECREF(pyinfo);

	if (ret) {
		Py_DECREF(pycontext);
		Py_DECREF(ret);
		PyEval_ReleaseThread(data->interp_thread);
		osync_context_report_success(ctx);
		osync_trace(TRACE_EXIT, "%s", __func__);
		return TRUE;
	}

	/* an exception occurred. get the python exception data */
	PyObject *pytype, *pyvalue, *pytraceback;
	PyErr_Fetch(&pytype, &pyvalue, &pytraceback);
	
	PyObject *osyncerror = NULL;
	osyncerror = PyObject_GetAttrString(data->osync_module, "Error");
	if (!osyncerror) {
		PYERR_CLEAR();
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to get OSyncError class object");
		goto out;
	}
	
	if (PyErr_GivenExceptionMatches(pytype, osyncerror)) {
		/* if it's an OSyncError, just report that up on the context object */
		PyObject *obj = PyObject_CallMethod(pyvalue, "report", "OO", pyvalue, pycontext);
		if (!obj) {
			PYERR_CLEAR();
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed reporting OSyncError");
			goto out;
		}
		
		Py_DECREF(obj);
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Reported OSyncError");
		report_error = FALSE;
	} else if (PyErr_GivenExceptionMatches(pytype, PyExc_IOError)
	           || PyErr_GivenExceptionMatches(pytype, PyExc_OSError)) {
		/* for IOError or OSError, we just report the &error message */
		PyObject *pystr = PyObject_Str(pyvalue);
		if (!pystr) {
			PYERR_CLEAR();
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed reporting IOError/OSError");
			goto out;
		}

		osync_error_set(&error, OSYNC_ERROR_IO_ERROR, "%s", PyString_AsString(pystr));
		Py_DECREF(pystr);
	} else {
		/* for other exceptions, we report a full traceback */
		pm_pyexcept_to_oserror(pytype, pyvalue, pytraceback, &error);
	}

out:
	Py_DECREF(pycontext);
	Py_XDECREF(pytype);
	Py_XDECREF(pyvalue);
	Py_XDECREF(pytraceback);
	Py_XDECREF(osyncerror);

error:
	PyEval_ReleaseThread(data->interp_thread);
	if (report_error)
		osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	return FALSE;
}

static void pm_connect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	pm_call_module_method(data, "connect", info, ctx, NULL);
}

static void pm_disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	pm_call_module_method(data, "disconnect", info, ctx, NULL);
}

static void pm_get_changes(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	pm_call_module_method(data, "get_changes", info, ctx, NULL);
}

static void pm_commit(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{	
	pm_call_module_method(data, "commit", info, ctx, change);
}

static void pm_committed_all(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{	
	pm_call_module_method(data, "committed_all", info, ctx, NULL);
}

static osync_bool pm_write(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{	
	return pm_call_module_method(data, "write", info, ctx, change);
}

static osync_bool pm_read(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{	
	return pm_call_module_method(data, "read", info, ctx, change);
}

static void pm_sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	pm_call_module_method(data, "sync_done", info, ctx, NULL);
}

static OSyncObjTypeSinkFunctions pm_sink_functions = {
	.connect = pm_connect,
	.disconnect = pm_disconnect,
	.get_changes = pm_get_changes,
	.commit = pm_commit,
	.write = pm_write,
	.committed_all = pm_committed_all,
	.read = pm_read,
	.batch_commit = NULL, /* not (yet) supported for python plugins */
	.sync_done = pm_sync_done
};

/** Calls the method initialize function
 *
 * The python initialize() function register one or more sink objects
 * that have the other plugin methods (get_changeinfo, commit, etc.)
 */
static void *pm_initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	MemberData *data = g_malloc0(sizeof(MemberData));
	char *modulename;

	if (!(modulename = osync_plugin_get_data(plugin)))
		return NULL;
	osync_plugin_set_data(plugin, NULL);

	data->interp_thread = Py_NewInterpreter();
	if (!data->interp_thread) {
		PYERR_CLEAR();
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't initialize python sub interpreter");
		free(modulename);
		goto error;
	}

	if (!(data->module = PyImport_ImportModule(modulename))) {
		PYERR_CLEAR();
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't load module %s", modulename);
		free(modulename);
		goto error;
	}

	free(modulename);

	if (!(data->osync_module = pm_load_opensync(error)))
		goto error;
	
	PyObject *pyinfo = pm_make_info(data->osync_module, info, error);
	if (!pyinfo)
		goto error;
	
	PyObject *ret = PyObject_CallMethod(data->module, "initialize", "O", pyinfo);
	Py_DECREF(pyinfo);
	if (!ret) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't initialize module");
		PYERR_CLEAR();
		goto error;
	}
	Py_DECREF(ret);

	/* loop through all objtype sinks, set up function pointers */
	int n, max = osync_plugin_info_num_objtypes(info);
	for (n = 0; n < max; n++) {
		OSyncObjTypeSink *sink = osync_plugin_info_nth_objtype(info, n);
		PyObject *sinkobj = osync_objtype_sink_get_userdata(sink);
		osync_objtype_sink_set_functions(sink, pm_sink_functions, sinkobj);
		Py_INCREF(sinkobj);
		data->sinks = g_slist_prepend(data->sinks, sinkobj);
	}

	PyEval_ReleaseThread(data->interp_thread);

	return data;

error:
	Py_XDECREF(data->module);
	Py_XDECREF(data->osync_module);
	if (data->interp_thread)
		Py_EndInterpreter(data->interp_thread);
	free(data);
	return NULL;
}

static osync_bool pm_discover(void *data_in, OSyncPluginInfo *info, OSyncError **error)
{
	MemberData *data = data_in;

	PyEval_AcquireThread(data->interp_thread);

	PyObject *pyinfo = pm_make_info(data->osync_module, info, error);
	if (!pyinfo)
		goto error;

	PyObject *ret = PyObject_CallMethod(data->module, "discover", "O", pyinfo);
	Py_DECREF(pyinfo);
	if (!ret)
		goto error;

	Py_DECREF(ret);
	PyEval_ReleaseThread(data->interp_thread);
	return TRUE;

error:
	osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't call discover method");
	PYERR_CLEAR();
	PyEval_ReleaseThread(data->interp_thread);
	return FALSE;
}

static void pm_finalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	MemberData *mydata = data;
	PyEval_AcquireThread(mydata->interp_thread);

	/* free all sink objects */
	while (mydata->sinks) {
		Py_DECREF((PyObject *)mydata->sinks->data);
		mydata->sinks = g_slist_delete_link(mydata->sinks, mydata->sinks);
	}

	Py_DECREF(mydata->module);
	Py_DECREF(mydata->osync_module);
	Py_EndInterpreter(mydata->interp_thread);
	free(mydata);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** Register a new plugin from python module called name.
 *
 * @todo We need to avoid the loading of the python runtime
 *       on get_info() somehow, but then we need to store all
 *       plugin information on another place (including
 *       accepted objtypes/formats info)
 */
static osync_bool register_plugin(OSyncPluginEnv *env, PyObject *osync_module,
                                  char *modulename, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, modulename, error);

	PyObject *module = NULL, *pyplugin_cobj = NULL, *pyplugin = NULL;

	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
		return FALSE;

	module = PyImport_ImportModule(modulename);
	if (!module) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't load module %s", modulename);
		goto error;
	}

	pyplugin_cobj = PyCObject_FromVoidPtr(plugin, NULL);
	if (!pyplugin_cobj) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldnt make pyplugin cobject");
		goto error;
	}
	
	pyplugin = PyObject_CallMethod(osync_module, "Plugin", "O", pyplugin_cobj);
	if (!pyplugin) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot create Python OSyncPlugin");
		goto error;
	}
	
	PyObject *pyret = PyObject_CallMethod(module, "get_sync_info", "O", pyplugin);
	if (!pyret) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error calling get_sync_info");
		goto error;
	}
	Py_DECREF(pyret);

	if (!osync_plugin_get_name(plugin)) {
		osync_trace(TRACE_INTERNAL, "%s: the plugin %s didn't set its name", __func__, modulename);
	}

	osync_plugin_set_initialize(plugin, pm_initialize);
	osync_plugin_set_discover(plugin, pm_discover);
	osync_plugin_set_finalize(plugin, pm_finalize);
	osync_plugin_set_data(plugin, g_strdup(modulename));
	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	PYERR_CLEAR();
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	Py_XDECREF(module);
	Py_XDECREF(pyplugin_cobj);
	Py_XDECREF(pyplugin);
	return FALSE;
}

static osync_bool scan_for_plugins(OSyncPluginEnv *env, PyObject *osync_module, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);

	GError *gerror = NULL;
	GDir *dir = g_dir_open(OPENSYNC_PYTHONPLG_DIR, 0, &gerror);
	if (!dir) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to open directory %s: %s", __func__, OPENSYNC_PYTHONPLG_DIR, gerror ? gerror->message : "None");
		return FALSE;
	}

	/* scan through the plugin directory looking for python modules (*.py)
	 * for each matching file, drop the .py extension to get the module name, and try to register its plugin */
	const char *filename = NULL;
	while ((filename = g_dir_read_name(dir))) {
		if (g_str_has_suffix(filename, ".py")) {
			char *modulename = g_strndup(filename, strlen(filename) - 3);
			if (!register_plugin(env, osync_module, modulename, error))
				osync_trace(TRACE_INTERNAL, "Couldn't register python plugin \"%s\": %s", filename, osync_error_print(error));
			g_free(modulename);
		}
	}
	g_dir_close(dir);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	/* OpenSync likes to call this function multiple times.
	 * IMO this is a bug and should be fixed, however to work around it we have to:
	 *  * init python only once
	 *  * make sure we save and re-acquire the main thread before making any python API calls
	 */

	static PyThreadState *mainthread;

	if (!Py_IsInitialized() || !PyEval_ThreadsInitialized()) {
		/* set python search path to look in our module directory first */
		char *pypath = g_build_path(":", OPENSYNC_PYTHONPLG_DIR, getenv("PYTHONPATH"), NULL);
		if (pypath == NULL || setenv("PYTHONPATH", pypath, 1) != 0) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't set PYTHONPATH");
			return FALSE;
		}
		g_free(pypath);

		/* init python */
		Py_InitializeEx(0);
		PyEval_InitThreads();
		mainthread = PyThreadState_Get();
	} else {
		PyEval_AcquireThread(mainthread);
	}

	/* import opensync module */
	PyObject *osync_module = pm_load_opensync(error); 
	if (!osync_module)
		return FALSE;

	osync_bool ret = scan_for_plugins(env, osync_module, error);
	Py_DECREF(osync_module);

	PyEval_ReleaseThread(mainthread);
	return ret;
}

int get_version(void)
{
	return 1; /* opensync plugin API version we expect */
}
