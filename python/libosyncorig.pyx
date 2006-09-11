cdef extern from "../src/osync.h":

	ctypedef struct OSyncPlugin
	ctypedef struct OSyncStruct
	ctypedef int osync_bool
	
	OSyncStruct *osync_new	(void)
	osync_bool osync_init(OSyncStruct *osstruct)
	int osync_num_plugins (OSyncStruct *osstruct)
	OSyncPlugin *osync_get_nth_plugin(OSyncStruct *osstruct, int nth)
	
cdef class OSync:
	cdef OSyncStruct *osync

	property name:
		def __get__(self):
			return msync_syncmember_get_name(self.member)
			
		def __set__(self, name):
			msync_syncmember_set_name(self.member, name)
	
cdef class MSCsyncgroup:
	cdef MSyncGroup *group

	def __init__(self, _create=1):
		if _create:
			self.group = msync_syncgroup_new()

	property name:
		def __get__(self):
			return msync_syncgroup_get_name(self.group)
			
		def __set__(self, name):
			msync_syncgroup_set_name(self.group, name)
		
	def load_syncmembers(self):
		return msync_syncgroup_load_members(self.group)
		
	def count_syncmembers(self):
		return msync_syncgroup_count_members(self.group)
		
	def get_nth_syncmember(self, number):
		cdef MSCsyncmember s
		s = MSCsyncmember()
		s.member = msync_syncgroup_get_nth_member(self.group, number)
		if s.member == NULL:
			print "error!"
			return
		return s
	
cdef class MSCserver:
	cdef MSyncServer *server
	
	property name:
		def __get__(self):
			if self.server == NULL:
				return ""
			return self.server.name
			
		def __set__(self, name):
			self.server.name = name
	
	def __init__(self, _create=1):
		if _create:
			self.server = msync_server_new()
		
	def __dealloc__(self):
		pass

	def get_configfile_path(self):
		return msync_server_get_configfile_path(self.server)
	
	def set_configfile_path(self, path):
		msync_server_set_configfile_path(self.server, path)
		
	def load_syncgroups(self):
		return msync_server_load_syncgroups(self.server)
		
	def count_syncgroups(self):
		return msync_server_count_syncgroups(self.server)
		
	def get_nth_syncgroup(self, number):
		cdef MSCsyncgroup s
		s = MSCsyncgroup()
		s.group = msync_server_get_nth_syncgroup(self.server, number)
		if s.group == NULL:
			print "error!"
			return
		return s
		
	def add_syncgroup(self, MSCsyncgroup group):
		return msync_server_add_syncgroup(self.server, group.group)

cdef class MSCclient:
	cdef MSyncClient *client
	
	def __new__(self):
		self.client = msync_client_new()
		
	def __dealloc__(self):
		pass
		
	def set_configfile_path(self, path):
		msync_client_set_configfile_path(self.client, path)
		
	def configure(self):
		return msync_client_configure(self.client)
		
	def set_type_controlplugin(self):
		msync_client_set_controlplugin(self.client)
		
	def init(self):
		return msync_client_init(self.client)
	
	def install_wellknown_handler(self):
		return msync_client_install_wellknown_handler(self.client)
	
	def send_connected(self):
		return msync_client_call_connected(self.client)
	
	def get_server(self):
		cdef MSCserver s
		s = MSCserver(_create=0)
		s.server = msync_client_get_server(self.client)
		return s
	