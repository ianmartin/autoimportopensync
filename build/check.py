# Copyright Matthias Jahn 2007
# BSD license (see COPYING)

def exists(env):
	return true

def generate(env):
	from SCons.Options import Options, BoolOption
	env.get_opts().add(BoolOption('enable_tests', 'Should the unit tests be enabled', 0))
	env.get_opts().update()
	if env['enable_tests'] == 1 or env.has_key('HAVE_CHECK'):
		have_check  = env.pkgConfig_findPackage('CHECK', 'check')
		# if the config worked, read the necessary variables and cache them
		if not have_check:
			print 'package \'check\' not found. http://check.sourceforge.net'
			print "This is only needed for unit tests of the OpenSync framework (developing)"
			env.Exit(1)

