from opensync import *

class A:
	def connect(self, ctx):
		print "Connect called!!"
		ctx.report_success()

	def get_changeinfo(self, ctx):
		print "get_changeinfo called!!"
		ctx.report_error(0, "Not implemented")

	def commit_change(self, ctx, chg):
		print "commit called!!"
		ctx.report_success()

	def access(self, ctx, chg):
		print "access called!!"
		ctx.report_success()

	def finalize(self, ctx):
		print "finalize called!"
		ctx.report_success()

	def disconnect(self, ctx):
		print "disconnect called!"
		ctx.report_success()

	def get_data(self, ctx, chg):
		print "get_data called!"

def initialize():
	print "Initialize called!!"
	return A()

def get_info(info):
	info.accept_objtype("contact")
	info.accept_objformat("contact", "vcard30")
	info.set_name("testmodule", 1)

# vim: ts=4:sw=4:noet
