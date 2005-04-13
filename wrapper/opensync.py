# This file was created automatically by SWIG.
# Don't modify this file, modify the SWIG interface instead.

import _opensync

def _swig_setattr_nondynamic(self,class_type,name,value,static=1):
    if (name == "this"):
        if isinstance(value, class_type):
            self.__dict__[name] = value.this
            if hasattr(value,"thisown"): self.__dict__["thisown"] = value.thisown
            del value.thisown
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    if (not static) or hasattr(self,name) or (name == "thisown"):
        self.__dict__[name] = value
    else:
        raise AttributeError("You cannot add attributes to %s" % self)

def _swig_setattr(self,class_type,name,value):
    return _swig_setattr_nondynamic(self,class_type,name,value,0)

def _swig_getattr(self,class_type,name):
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0
del types


def _swig_setattr_nondynamic_method(set):
    def set_attr(self,name,value):
        if hasattr(self,name) or (name in ("this", "thisown")):
            set(self,name,value)
        else:
            raise AttributeError("You cannot add attributes to %s" % self)
    return set_attr


TRUE = _opensync.TRUE
FALSE = _opensync.FALSE
class OSyncEnv(object):
    def __repr__(self):
        return "<%s.%s; proxy of C OSyncEnv instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    def __init__(self, *args):
        newobj = _opensync.new_OSyncEnv(*args)
        self.this = newobj.this
        self.thisown = 1
        del newobj.thisown
    def __del__(self, destroy=_opensync.delete_OSyncEnv):
        try:
            if self.thisown: destroy(self)
        except: pass

    def initialize(*args): return _opensync.OSyncEnv_initialize(*args)
    def finalize(*args): return _opensync.OSyncEnv_finalize(*args)
    def num_plugins(*args): return _opensync.OSyncEnv_num_plugins(*args)
    def get_nth_plugin(*args): return _opensync.OSyncEnv_get_nth_plugin(*args)

class OSyncEnvPtr(OSyncEnv):
    def __init__(self, this):
        self.this = this
        if not hasattr(self,"thisown"): self.thisown = 0
        self.__class__ = OSyncEnv
_opensync.OSyncEnv_swigregister(OSyncEnvPtr)

class OSyncPlugin(object):
    def __repr__(self):
        return "<%s.%s; proxy of C OSyncPlugin instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    def __init__(self, *args):
        newobj = _opensync.new_OSyncPlugin(*args)
        self.this = newobj.this
        self.thisown = 1
        del newobj.thisown
    def __del__(self, destroy=_opensync.delete_OSyncPlugin):
        try:
            if self.thisown: destroy(self)
        except: pass

    def name_set(*args): return _opensync.OSyncPlugin_name_set(*args)
    def get_name(self):
    	return self.name_get()
    def set_name(self, name):
    	self.name_set(name)
    name = property(get_name, set_name)

    def name_get(*args): return _opensync.OSyncPlugin_name_get(*args)

class OSyncPluginPtr(OSyncPlugin):
    def __init__(self, this):
        self.this = this
        if not hasattr(self,"thisown"): self.thisown = 0
        self.__class__ = OSyncPlugin
_opensync.OSyncPlugin_swigregister(OSyncPluginPtr)


