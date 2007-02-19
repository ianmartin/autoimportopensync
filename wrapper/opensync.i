%module(docstring="Python bindings for the OpenSync library") opensync
%feature("autodoc", "1");
%include "cstring.i"

%{
#include <opensync/opensync.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-error.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-group.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-merger.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-version.h>

typedef OSyncCapabilities Capabilities;
typedef OSyncCapability Capability;
typedef OSyncChange Change;
typedef OSyncChangeType ChangeType;
typedef OSyncConfigurationType ConfigurationType;
typedef OSyncConflictResolution ConflictResolution;
typedef OSyncContext Context;
typedef OSyncConvCmpResult ConvCmpResult;
typedef OSyncCustomFilter CustomFilter;
typedef OSyncData Data;
typedef OSyncError Error;
typedef OSyncErrorType ErrorType;
typedef OSyncFilterAction FilterAction;
typedef OSyncFilter Filter;
typedef OSyncFormatConverter FormatConverter;
typedef OSyncFormatConverterPath FormatConverterPath;
typedef OSyncFormatEnv FormatEnv;
typedef OSyncGroupEnv GroupEnv;
typedef OSyncGroup Group;
typedef OSyncHashTable HashTable;
typedef OSyncLockState LockState;
typedef OSyncMember Member;
typedef OSyncMerger Merger;
typedef OSyncObjFormat ObjFormat;
typedef OSyncObjTypeSinkFunctions ObjTypeSinkFunctions;
typedef OSyncObjTypeSink ObjTypeSink;
typedef OSyncPluginEnv PluginEnv;
typedef OSyncPluginInfo PluginInfo;
typedef OSyncPlugin Plugin;
typedef OSyncStartType StartType;
typedef OSyncVersion Version;

/* make SWIG treat osync_bool as real Python booleans */
typedef osync_bool bool;
#define true TRUE
#define false FALSE
%}

/* macro to define arbitrary output argument pointer typemaps, based on:
 * http://embedded.eecs.berkeley.edu/Alumni/pinhong/scriptEDA/pyTypemapFAQ.html#22
 */
%define define_ptr_argout(T)
%typemap(argout) T* OUTPUT {
	PyObject *o = SWIG_NewPointerObj((void *)$1, SWIGTYPE_p_##T);
	$result = l_output_helper($result, o);
}
%enddef

/* define output typemaps for pointer types we need */
define_ptr_argout(ConflictResolution);

%{
/* convert an List linked list to a python list of the given type */
static PyObject *osynclist_to_pylist(const OSyncList *elt, swig_type_info *type)
{
	PyObject *ret = PyList_New(0);
	if (ret == NULL)
		return NULL;
	for (; elt != NULL; elt = elt->next) {
		PyObject *obj = SWIG_NewPointerObj(elt->data, type, 0);
		if (!obj || PyList_Append(ret, obj) != 0) {
			Py_DECREF(ret);
			return NULL;
		}
	}
	return ret;
}
%}

/* force all functions to return NULL if a python exception has been raised */
%exception {
	$action
	if (PyErr_Occurred())
		return NULL;
}

%include "opensync-error.i"
%include "opensync-context.i"
%include "opensync-data.i"
%include "opensync-format.i"
%include "opensync-group.i"
%include "opensync-helper.i"
%include "opensync-merger.i"
%include "opensync-plugin.i"
%include "opensync-version.i"
