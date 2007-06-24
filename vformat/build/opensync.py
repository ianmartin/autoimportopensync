# Copyright Matthias Jahn 2007
# BSD license (see COPYING)

"""
Find and load the opensync necessary compilation and link flags
"""
def pkgconfig_fetch_variable(env, pkgname, variable):
	import os
	pkgbin='pkg-config'
	return os.popen('%s --variable=%s %s' % (pkgbin, variable, pkgname)).read().strip()

def exists(env):
	return true

def generate(env):
	have_xml  = env.pkgConfig_findPackage('OPENSYNC', 'opensync-1.0', '0.30')
	# if the config worked, read the necessary variables and cache them
	if not have_xml:
		print'opensync-1.0 >= 0.30 not found.'
		env.Exit(1)
	variables="plugindir formatsdir configdir capabilitiesdir descriptionsdir schemasdir"
	for variable in variables.split():
		var_defname = 'OPENSYNC_' + variable.upper()
		var_value = pkgconfig_fetch_variable(env, 'opensync-1.0', variable)
		env.add_define("%s" % var_defname, "%s" % var_value)
		env["%s" % var_defname] = "%s" % var_value
	

