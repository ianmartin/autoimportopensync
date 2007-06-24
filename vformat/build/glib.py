# Copyright Matthias Jahn 2007
# BSD license (see COPYING)

def exists(env):
	return true

def generate(env):
	have_glib  = env.pkgConfig_findPackage('GLIB', 'glib-2.0', '2.10')
	# if the config worked, read the necessary variables and cache them
	if not have_glib:
		print'glib-2.0 >= 2.10 not found.'
		env.Exit(1)

