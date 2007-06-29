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
	#print self.member.get_pluginname()		    

    @dbus.service.method("org.opensync.SyncMember", in_signature='', out_signature='s')
    def GetPluginName(self):
	#print self.member.get_pluginname() # This triggers a reference problem in the opensync bindings

	return self.plugin_name

class SyncGroup(dbus.service.Object):

    syncMembers = []

    def __init__(self, bus_name, group, group_id):
	self.group = group
	self.group_id = group_id
        dbus.service.Object.__init__(self, bus_name, str("/org/opensync/group%i/Manager" % group_id))

	num_member = self.group.num_members
	i = 0 
	while i < num_member:
	    member = group.nth_member(i)
	    i += 1
	    syncMember = SyncMember(bus_name, group_id, member, i)
	    self.syncMembers.append(syncMember._object_path)

    def EngineStatusCallback(self, status):
"""	    
	if opensync.ENGINE_COMMAND_SYNC_DONE == status:
		self.engine.finalize()
"""

    @dbus.service.method("org.opensync.SyncGroup", in_signature='', out_signature='')
    def Sync(self):
"""
	self.engine = opensync.Engine(self.group)
	self.engine.initialize()

	self.engine.set_enginestatus_callback(self.EngineStatusCallback)

	self.engine.synchronize()
"""


    @dbus.service.method("org.opensync.SyncGroup", in_signature='', out_signature='as')
    def ListMembers(self):
	return self.syncMembers	  


class MobileStationDevice(dbus.service.Object):
    
    isConnected = False
    name = "Fake Device"

    def __init__(self, bus_name, type, address):
        dbus.service.Object.__init__(self, bus_name, str("/org/opensync/MobileStation/Device/%s" % address))
	self.type = type
	self.address = address

    @dbus.service.method("org.opensync.MobileStation.Device", in_signature='', out_signature='b')
    def IsConnected(self):
	return self.isConnected

    @dbus.service.method("org.opensync.MobileStation.Device", in_signature='', out_signature='s')
    def GetConnectionType(self):
	return self.type

    @dbus.service.method("org.opensync.MobileStation.Device", in_signature='', out_signature='s')
    def GetName(self):
	return self.name

    @dbus.service.method("org.opensync.MobileStation.Device", in_signature='', out_signature='s')
    def GetAddress(self):
	return self.address

    @dbus.service.method("org.opensync.MobileStation.Device", in_signature='', out_signature='b')
    def IsPaired(self):
	if selft.type != "bluetooth":
		return True 
	""" TODO: request bonding status via BlueZ of the default adapter """
	return False

class MobileStationManager(dbus.service.Object):

    registeredPims = []	
    syncGroups = []
    bluetoothDevices = []
    usbDevices = []

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

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='s', out_signature='s')
    def CreateBluetoothDevice(self, address):
 	print "CreateBluetoothDevice(address: " + address + ")"
 	return "/org/opensync/MobileStation/Device/" + address 

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='s', out_signature='s')
    def CreateUsbDevice(self, udi):
 	print "CreateUsbDevice(udi:" + udi +")"
 	return "/org/opensync/MobileStation/Device/0830:0061" 

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='s', out_signature='')
    def RemoveDevice(self, value):
	print "RemoveDevice(path: " + str(value) + ")"
	return None

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='', out_signature='as')
    def ListSyncGroups(self):
	return self.syncGroups

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='s', out_signature='s')
    def CreateSyncGroup(self, name):
	print "CreateSyncGroup " + str(name)
	return "/org/opensync/groupX"

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='ss', out_signature='s')
    def SetupSyncDevicePimGroup(self, devicepath, pim):
	print "SetupSyncDevicePimGroup: " + str(devicepath) + " pim: " + str(pim)
	return "/org/opensync/groupX"

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='s', out_signature='')
    def RemoveSyncGroup (self, path):
	print "RemoveSyncGroup" + str(path)
	return None 

    @dbus.service.method("org.opensync.MobileStation.Manager", in_signature='', out_signature='ss')
    def ListRegisteredPims (self, path):
	print "ListRegisteredPims"
	return self.registeredPims 

if __name__ == "__main__":

   session_bus = dbus.SessionBus()
   name = dbus.service.BusName("org.opensync", bus=session_bus)
   manager = MobileStationManager(name)
   mainloop = gobject.MainLoop()
   mainloop.run()

