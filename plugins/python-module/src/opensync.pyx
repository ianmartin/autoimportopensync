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
	
	#objformat methods
	char *osync_objformat_get_name(OSyncObjFormat *format)

	#context methods
	void osync_context_report_change(OSyncContext *context, OSyncChange *change)
	void osync_context_report_error(OSyncContext *context, OSyncErrorType type, char *format, ...)
	void osync_context_report_success(OSyncContext *context)

cdef extern from "pywrap.h":
	OSyncMember *osync_member_from_void(void *m)
	OSyncContext *osync_context_from_void(void *c)

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
		self.memb = osync_member_from_void(PyCObject_AsVoidPtr(m))

cdef class Change

cdef class Context:
	"""opensync OSyncContext object"""
	cdef OSyncContext *ctx
	cdef void *v
	def __new__(self, long c):
		"""Never call this method from python code.
		
		Objects of this class are created only internally by OpenSync
		"""
		# Another hack like the OSyncMember hack above,
		# to be able to convert a python object
		# to OSyncContext *
		self.ctx = osync_context_from_void(PyCObject_AsVoidPtr(c))

	def report_change(self, Change chg):
		osync_context_report_change(self.ctx, chg.chg)

	def report_success(self):
		osync_context_report_success(self.ctx)

	def report_error(self, type, msg):
		#TODO: convert error type from python to C
		osync_context_report_error(self.ctx, OSYNC_ERROR_GENERIC, msg)

cdef class Change:
	cdef OSyncChange *chg

	def __new__(self, Member member):
		self.chg = osync_change_new()
		osync_change_set_member(self.chg, member.memb)

	def __dealloc__(self):
		osync_change_free(self.chg)

	def set_objformat(self, name):
		osync_change_set_objformat_string(self.chg, name)

	def get_objformat(self):
		cdef OSyncObjFormat *f
		f = osync_change_get_objformat(self.chg)
		if f: return osync_objformat_get_name(f)
		else: return None


# vim:ft=python
