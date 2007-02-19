import opensync

class DummySink(opensync.ObjTypeSink):
	def __init__(self, objtype):
		opensync.ObjTypeSink.__init__(self, objtype, self)
		self.add_objformat("file")

	def connect(self, info, ctx):
		print "Connect called!"

	def get_changes(self, info, ctx):
		print "get_changes called!"
		#if self.__member.get_slow_sync("data"):
			#print "Slow-sync requested"
		#change = opensync.Change()
		#change.uid = "testuid"
		#change.data = "testdata"
		#change.format = "plain"
		#change.objtype = "data"
		#change.changetype = opensync.CHANGE_ADDED
		#change.report(ctx)
		#print "done with get_changeinfo"
	
	def commit(self, info, ctx, chg):
		print "commit called!"
		print "Opensync wants me to write data for UID", chg.uid
	
	def committed_all(self, info, ctx):
		print "committed_all called!"

	def read(self, info, ctx, chg):
		print "read called!"
		print "OpenSync wants me to read the data for UID", chg.uid

	def write(self, info, ctx, chg):
		print "write called!"
		print "Opensync wants me to write data for UID", chg.uid
	
	def disconnect(self, info, ctx):
		print "disconnect called!"

	def sync_done(self, info, ctx):
		print "sync_done called!"

def initialize(info):
	print "initialize called!"
	print "My config is:", info.config
	newsink = DummySink("data")
	print "Adding new sink"
	info.add_objtype(newsink)
	print "Done"

def discover(info):
	print "discover called!"
	for sink in info.objtypes:
		print "setting sink available:", sink
		sink.available = True
	info.version = opensync.Version()
	info.version.plugin = "python-sample"
	print "done"

def get_sync_info(plugin):
	plugin.name = "python-sample"
	plugin.longname = "Sample sync plugin for the Python module"
	plugin.description = "This plugin only shows what must be implemented."
	plugin.config_type = opensync.PLUGIN_NO_CONFIGURATION
