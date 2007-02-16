%module(docstring="Python bindings for the OpenSync library") opensync
%feature("autodoc", "1");
// %include "typemaps.i"
%include "cstring.i"

%{
#include <opensync/opensync.h>
%}

// macro to define arbitrary output argument pointer typemaps, based on:
// http://embedded.eecs.berkeley.edu/Alumni/pinhong/scriptEDA/pyTypemapFAQ.html#22
%define define_ptr_argout(T)
%typemap(argout) T* OUTPUT {
	PyObject *o = SWIG_NewPointerObj((void *)$1, SWIGTYPE_p_##T);
	$result = l_output_helper($result, o);
}
%enddef

// define output typemaps for pointer types we need
define_ptr_argout(OSyncConflictResolution);

%{
/* convert an OSyncList linked list to a python list of the given type */
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
%include "opensync-plugin.i"
%include "opensync-version.i"
