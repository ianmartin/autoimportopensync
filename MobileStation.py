#!/usr/bin/env python

#   Copyright (C) 2007 by Daniel Gollub <dgollub@suse.de>
#                                                                       
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License.
#
#   This program is distributed in the hope that it will be useful,  
#   but WITHOUT ANY WARRANTY; without even the implied warranty of   
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    
#   GNU General Public License for more details.                     
#                                                                    
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the                    
#   Free Software Foundation, Inc.,                                  
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.     


"""              !!! This is only PROOF OF CONCEPT !!!              """

import dbus
import dbus.service
import dbus.glib
import gobject
import opensync

class SyncMember(dbus.service.Object):

    def __init__(self, bus_name, group_id, member, member_id):
        dbus.service.Object.__init__(self, bus_name, str("/org/opensync/group%i/member%i" % (group_id, member_id)))
	self.member = member
	# FIXME: Copy the PluginName since the member object got lost... 
	self.plugin_name = member.get_pluginname()		    
	print self.member.get_pluginname()		    

    @dbus.service.method("org.opensync.SyncMember", in_signature='', out_signature='s')
    def GetPluginName(self):
	#print self.member.get_pluginname() # This triggers a reference problem in the opensync bindings

	return self.plugin_name

class SyncGroup(dbus.service.Object):

    def __init__(self, bus_name, group, group_id):
	self.group = group
	self.group_id = group_id
        dbus.service.Object.__init__(self, bus_name, str("/org/opensync/group%i/Manager" % group_id))

    	self.syncMembers = []

	num_member = self.group.num_members
	i = 0 
	while i < num_member:
	    member = group.nth_member(i)
	    i += 1
	    syncMember = SyncMember(bus_name, group_id, member, i)
	    self.syncMembers.append(syncMember._object_path)


    @dbus.service.method("org.opensync.SyncGroup", in_signature='', out_signature='as')
    def ListMembers(self):
	return self.syncMembers	  

class MobileStationManager(dbus.service.Object):

    def __init__(self, bus_name, object_path="/org/opensync/MobileStation"):
        dbus.service.Object.__init__(self, bus_name, object_path)

	""" Load all local OpenSync groups """
	group_env = opensync.GroupEnv()
	group_env.load_groups(None)
	num_group = group_env.num_groups
	i = 0
	while i < num_group:
	    group = group_env.nth_group(i)
	    i += 1
	    syncGroup = SyncGroup(bus_name, group, i)
	    self.syncGroups.append(syncGroup._object_path)

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='', out_signature='as')
    def ListDevices(self):
	print "ListDevices " + str(self.fakeDevices)
	return self.fakeDevices

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='ss', out_signature='s')
    def CreateDevice(self, interface, uid):
 	print "CreateDevice(interface:" + str(interface) + " uid: " + str(uid) +")"
 	return "/org/opensync/MobileStation/FakeDevice" 

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='s', out_signature='')
    def RemoveDevice(self, value):
	print "RemoveDevice(path: " + str(value) + ")"
	return None

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='', out_signature='as')
    def ListSyncGroups(self):
	return self.syncGroups

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='s', out_signature='s')
    def CreateSyncGroup(self, value):
	print "CreateSyncGroup " + str(value)
	return "/org/opensync/" + str(value)

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='s', out_signature='')
    def RemoveSyncGroup (self, value):
	print "RemoveSyncGroup" + str(value)
	return None 


		    

if __name__ == "__main__":

   session_bus = dbus.SessionBus()
   name = dbus.service.BusName("org.opensync", bus=session_bus)
   manager = MobileStationManager(name)
   mainloop = gobject.MainLoop()
   mainloop.run()

