cdef extern from "../src/opensync.h":
	cdef struct OSyncPluginInfo
	cdef struct OSyncInfo
	cdef struct OSyncGroup
	cdef struct OSyncUserInfo
	cdef struct OSyncMember
	ctypedef int osync_bool
	
	OSyncInfo *osync_new()
	osync_bool osync_init(OSyncInfo *osstruct)
	int osync_num_plugins (OSyncInfo *osstruct)
	OSyncPluginInfo *osync_get_nth_plugin(OSyncInfo *osstruct, int nth)
	osync_bool osync_remove_nth_group(OSyncInfo *osstruct, int nth)
	osync_bool osync_remove_group(OSyncInfo *osstruct, OSyncGroup *group)
	char *osync_get_configdir(OSyncInfo *osinfo)
	void osync_set_configdir(OSyncInfo *osinfo, char *path)
	int osync_num_groups (OSyncInfo *osinfo)
	OSyncGroup *osync_get_nth_group(OSyncInfo *osinfo, int nth)
	int osync_add_group(OSyncInfo *osinfo, OSyncGroup *group)
	
	osync_bool osync_user_set_confdir(OSyncUserInfo *user, char *path)
	char *osync_user_get_confdir(OSyncUserInfo *user)
	
	#opensync_group.h
	OSyncGroup *osync_group_new(OSyncInfo *osinfo)
	osync_bool osync_group_set_name(OSyncGroup *group, char *name)
	char *osync_group_get_name(OSyncGroup *group)
	osync_bool osync_group_save(OSyncGroup *group)
	osync_bool osync_group_load_dir(OSyncInfo *osyncinfo)
	osync_bool osync_group_add_member(OSyncGroup *group, OSyncMember *member)
	OSyncMember *osync_group_get_nth_member(OSyncGroup *group, int nth)
	int osync_group_num_members(OSyncGroup *group)
	char *osync_group_get_configdir(OSyncGroup *group)
	void osync_group_set_configdir(OSyncGroup *group, char *path)
	osync_bool osync_group_initialize(OSyncGroup *group)
	
	#opensync_member.h
	OSyncMember *osync_member_new(OSyncGroup *group)
	osync_bool osync_member_instance_plugin(OSyncMember *member, OSyncPluginInfo *plugin)
	osync_bool osync_member_set_name(OSyncMember *member, char *name)
	char *osync_member_get_name(OSyncMember *member)
	char *osync_member_get_configdir(OSyncMember *member)
	osync_bool osync_member_set_configdir(OSyncMember *member, char *path)
	char *osync_member_get_config(OSyncMember *member)
	void osync_member_set_config(OSyncMember *member, char *data)
	osync_bool osync_member_initialize(OSyncMember *member)
	osync_bool osync_member_get_changeinfo(OSyncMember *member)
	
	#opensync_plugin.h
	OSyncPluginInfo *osync_plugin_new()
	OSyncPluginInfo *osync_plugin_from_name(OSyncInfo *osinfo, char *name)
	osync_bool osync_plugin_load_info(OSyncPluginInfo *plugin, char *path)
	char *osync_plugin_get_name(OSyncPluginInfo *plugin)
	void osync_plugin_set_name(OSyncPluginInfo *plugin, char *name)

cdef class OpenSyncPluginInfo:
	cdef OSyncPluginInfo *plugin
	
	property name:
		def __get__(self):
			return osync_plugin_get_name(self.plugin)

cdef class OpenSyncMember:
	cdef OSyncMember *member
	cdef OpenSyncGroup _parent

	def __init__(self, OpenSyncGroup group, plugin=None, _create=1):
		self._parent = group
		if _create:
			self.member = osync_member_new(group.group)
		if plugin:
			self.instance_plugin(plugin)

	property parent:
		def __get__(self):
			return self._parent

	property name:
		def __get__(self):
			return osync_member_get_name(self.member)
		def __set__(self, name):
			osync_member_set_name(self.member, name)
	
	def instance_plugin(self, OpenSyncPluginInfo plugin):
		osync_member_instance_plugin(self.member, plugin.plugin)
	
	property configdir:
		def __get__(self):
			return osync_member_get_configdir(self.member)
		def __set__(self, name):
			osync_member_set_configdir(self.member, name)
			
	property config:
		def __get__(self):
			return osync_member_get_config(self.member)
		def __set__(self, config):
			osync_member_set_config(self.member, config)
			
	def get_changeinfo(self):
		osync_member_get_changeinfo(self.member)

cdef class OpenSyncGroup:
	#cdef OSyncGroup *group
	#cdef OpenSync parent
		
	def __init__(self, OpenSync osinfo, _create=1):
		self.parent = osinfo
		if _create:
			self.group = osync_group_new(osinfo.osync)
	
	property name:
		def __get__(self):
			return osync_group_get_name(self.group)
		def __set__(self, name):
			osync_group_set_name(self.group, name)
	
	property configdir:
		def __get__(self):
			return osync_group_get_configdir(self.group)
		def __set__(self, name):
			osync_group_set_configdir(self.group, name)
	
	def num_members(self):
		return osync_group_num_members(self.group)
		
	def get_nth_member(self, number):
		cdef OpenSyncMember s
		s = OpenSyncMember(self)
		s.member = osync_group_get_nth_member(self.group, number)
		if s.member == NULL:
			print "error!"
			return
		return s
		
	def add_member(self, OpenSyncMember member):
		osync_group_add_member(self.group, member.member)
		
	def save(self):
		osync_group_save(self.group)
		
	def initialize(self):
		osync_group_initialize(self.group)
	
cdef class OpenSync:
	#cdef OSyncInfo *osync
	
	def __init__(self, _create=1):
		self.osync = osync_new()
		osync_init(self.osync)

	def plugin_from_name(self, name):
		cdef OpenSyncPluginInfo s
		s = OpenSyncPluginInfo()
		s.plugin = osync_plugin_from_name(self.osync, name)
		if s.plugin == NULL:
			print "error!"
			return
		return s
	
	def get_nth_plugin(self, number):
		cdef OpenSyncPluginInfo s
		s = OpenSyncPluginInfo()
		s.plugin = osync_get_nth_plugin(self.osync, number)
		if s.plugin == NULL:
			print "error!"
			return
		return s
	
	def num_plugins (self):
		return osync_num_plugins(self.osync)
	
	def num_groups (self):
		return osync_num_groups(self.osync)
		
	def get_nth_group(self, number):
		cdef OpenSyncGroup s
		s = OpenSyncGroup(self)
		s.group = osync_get_nth_group(self.osync, number)
		if s.group == NULL:
			print "error!"
			return
		return s
		
	def load_group_dir(self):
		return osync_group_load_dir(self.osync)
		
	def add_group(self, OpenSyncGroup group):
		osync_add_group(self.osync, group.group)
		
	def delete_group(self, OpenSyncGroup group):
		osync_remove_group(self.osync, group.group)
		