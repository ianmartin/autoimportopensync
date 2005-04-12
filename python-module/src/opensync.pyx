# OpenSync classes for python
cdef extern from "Python.h":
	void* PyCObject_AsVoidPtr(object self)

cdef extern from "opensync/opensync.h":
	#types
	ctypedef struct OSyncChange
	ctypedef struct OSyncObjFormat
	ctypedef struct OSyncMember
	ctypedef struct OSyncContext

	ctypedef enum OSyncErrorType:
		OSYNC_NO_ERROR
		OSYNC_ERROR_GENERIC
		OSYNC_ERROR_IO_ERROR
		OSYNC_ERROR_NOT_SUPPORTED
		OSYNC_ERROR_TIMEOUT
		OSYNC_ERROR_DISCONNECTED
		OSYNC_ERROR_FILE_NOT_FOUND
		OSYNC_ERROR_EXISTS
		OSYNC_ERROR_CONVERT
		OSYNC_ERROR_MISCONFIGURATION
		OSYNC_ERROR_INITIALIZATION

	#change methods
	OSyncChange *osync_change_new()
	void osync_change_free(OSyncChange *change)
	void osync_change_set_objformat_string(OSyncChange *change, char *name)
	OSyncObjFormat *osync_change_get_objformat(OSyncChange *change)
	void osync_change_set_member(OSyncChange *change, OSyncMember *member)
	void osync_change_ref(OSyncChange *change)
	void osync_change_decref(OSyncChange *change)
	
	#objformat methods
	char *osync_objformat_get_name(OSyncObjFormat *format)

	#context methods
	void osync_context_report_change(OSyncContext *context, OSyncChange *change)
	void osync_context_report_error(OSyncContext *context, OSyncErrorType type, char *format, ...)
	void osync_context_report_success(OSyncContext *context)

cdef extern from "pywrap.h":
	ctypedef struct PythonPluginInfo

	# pyrex doesn't handle typecasts gracefully,
	# so the functions below are just macros that
	# do typecasts
	OSyncMember *osync_member_from_void(void *m)
	OSyncContext *osync_context_from_void(void *c)
	OSyncChange *osync_change_from_void(void *c)
	PythonPluginInfo *osync_plginfo_from_void(void *v)


	void pywrap_accept_objtype(PythonPluginInfo *info, char *objtype)
	void pywrap_accept_objformat(PythonPluginInfo *info, char *objtype, char *objformat, char *extension)
	void pywrap_set_name_and_version(PythonPluginInfo *info, char *name, int version)

cdef class Member:
	"""opensync OSyncMember object"""
	cdef OSyncMember *memb
	def __new__(self, object m):
		"""Never call this method from python code.
		
		Objects of this class are created only internally by OpenSync
		"""
		# This is a hack to be able to pass a OSyncMember pointer
		# to the constructor: we need something that converts a python
		# object to (OSyncMember *), we can wrap the pointer in
		# a CObject, but we still need something that converts
		# (void *) to (OSyncMember *)
		cdef void *v
		v = PyCObject_AsVoidPtr(m)
		self.memb = osync_member_from_void(v)

# forward declaration
cdef class Change

cdef class Context:
	"""opensync OSyncContext object"""
	cdef OSyncContext *ctx
	def __new__(self, c):
		"""Never call this method from python code.
		
		Objects of this class are created only internally by OpenSync
		"""
		cdef void *v
		v = PyCObject_AsVoidPtr(c)
		self.ctx = osync_context_from_void(v)

	def report_change(self, Change chg):
		# call _ref() to avoid the change
		# from being free()d after destroying
		# the python opensync.Change object
		osync_change_ref(chg.chg)
		#FIXME: Are we supposed to call ref() here,
		# or osync_context_report_change() implies
		# on a _ref() call?

		osync_context_report_change(self.ctx, chg.chg)

	def report_success(self):
		osync_context_report_success(self.ctx)

	def report_error(self, type, msg):
		#TODO: convert error type from python to C
		osync_context_report_error(self.ctx, OSYNC_ERROR_GENERIC, msg)

cdef class Change:
	cdef OSyncChange *chg

	def __new__(self, Member member, chg = None):
		"""Creates a new change object

		The chg parameter can be used only internally
		by OpenSync. It is a CObject containing
		an existing OSyncChange"""
		cdef void *v
		if chg is None:
			self.chg = osync_change_new()
			osync_change_set_member(self.chg, member.memb)
		else:
			v = PyCObject_AsVoidPtr(chg)
			self.chg = osync_change_from_void(v)
			osync_change_ref(self.chg)

	def __dealloc__(self):
		osync_change_decref(self.chg)

	def set_objformat(self, name):
		osync_change_set_objformat_string(self.chg, name)

	def get_objformat(self):
		cdef OSyncObjFormat *f
		f = osync_change_get_objformat(self.chg)
		if f: return osync_objformat_get_name(f)
		else: return None

	#TODO: Implement set_data and get_data
	#TODO: Write some wrappers to common formats having structs,
	#      such as fs_info. In the case of known object formats,
	#      return a python object wrapping the object sotred
	#      on the change data



cdef class PluginInfo:
	"""opensync OSyncPluginInfo object"""

	# python_module.c should be able to tell,
	# somehow, which functions should be registered,
	# pyinfo is used for that
	cdef PythonPluginInfo *info

	def __new__(self, info):
		"""Never call this method from python code.
		
		Objects of this class are created only internally by OpenSync
		"""
		cdef void *v
		v = PyCObject_AsVoidPtr(info)
		self.info = osync_plginfo_from_void(v)

	def accept_objtype(self, objtype):
		pywrap_accept_objtype(self.info, objtype)
	
	def accept_objformat(self, objtype, objformat, extension = None):
		cdef char *ext
		if extension is None:
			ext = NULL
		else:
			ext = extension
		pywrap_accept_objformat(self.info, objtype, objformat, ext)

	def set_name(self, name, version):
		pywrap_set_name_and_version(self.info, name, version);

# vim:ft=python
