cdef extern from "../src/opensync.h":
	cdef struct OSyncPluginInfo
	cdef struct OSyncInfo
	cdef struct OSyncGroup
	cdef struct OSyncUserInfo
	cdef struct OSyncMember
	
cdef class OpenSync:
	cdef OSyncInfo *osync
	
cdef class OpenSyncGroup:
	cdef OSyncGroup *group
	cdef OpenSync parent