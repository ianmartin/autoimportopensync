# Copyright Thomas Nagy 2005
# BSD license (see COPYING)

"""
Find and load the libxml2 and libxslt necessary compilation and link flags
"""

def exists(env):
	return true

def generate(env):
	have_xml  = env.pkgConfig_findPackage('XML', 'libxml-2.0', '2.6.0')
	# if the config worked, read the necessary variables and cache them
	if not have_xml:
		print'libxml-2.0 >= 2.6.0 was not found (mandatory).'
		env.Exit(1)

