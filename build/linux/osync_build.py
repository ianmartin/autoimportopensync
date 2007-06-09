from osync_support import *
from SCons.Options import *
import distutils.sysconfig

def configure(opts):
	opts.Add(PathOption('prefix', 'Directory, where opensync should be installed', '/usr/local'))
	opts.Add(('libsuffix', 'Library suffic. lib64 for 64 bit systems', 'lib'))
	opts.Add(BoolOption('enable_rpath', 'Build with -rpath?', 1))
	opts.Add(BoolOption('enable_profiling', 'Should code profiling be enabled (GCOV/LCOV, ...)', 0))
	opts.Add(('CC', 'Path to Custom c compiler', 'gcc'))
	opts.Add(('CXX', 'Path to Custom c++ compiler flags', 'g++'))
	
def check(env, config):
	conf = env.Configure(custom_tests = {'CheckPKGConfig' : CheckPKGConfig, 'CheckPKG' : CheckPKG})
	
	if not conf.CheckPKGConfig('0.15.0'):
		print 'pkg-config >= 0.15.0 not found.'
		env.Exit(1)
		
	if not conf.CheckPKG('glib-2.0 >= 2.10'):
		print 'glib-2.0 >= 2.10 not found.'
		env.Exit(1)

	if env['enable_tests'] == 1 and not conf.CheckPKG('check'):
		print 'package \'check\' not found. http://check.sourceforge.net'
		print 'This is only needed for unit tests of the OpenSync framework (developing)'
		env.Exit(1)

	if not conf.CheckPKG('sqlite3 >= 3.3'):
		print 'package \'sqlite\' not found. http://sqlite.org'
		env.Exit(1)

	if not conf.CheckPKG('libxml-2.0 >= 2.6'):
		print 'package \'libxml2\' not found. Version 2.6 or greater nwwsws. http://xmlsoft.org'
		env.Exit(1)

	env = conf.Finish()
	env.Append(CCFLAGS = r'-DHAVE_CONFIG_H')

	add_define(env, "OPENSYNC_PLUGINDIR", config.plugindir)
	add_define(env, "OPENSYNC_FORMATSDIR", config.formatdir)
	add_define(env, "OPENSYNC_CONFIGDIR", config.configdir)
	add_define(env, "OPENSYNC_CAPABILITIESDIR", config.capabilitiesdir)
	add_define(env, "OPENSYNC_DESCRIPTIONSDIR", config.descriptionsdir)
	add_define(env, "OPENSYNC_SCHEMASDIR", config.descriptionsdir)
	add_define(env, "VERSION", config.version)
	add_define(env, "OPENSYNC_PLUGINVERSION", config.plugin_version)
	if conf.env['debug_modules'] == 1:
		add_define(env, "DEBUG_MODULES")
	
	env.Append(CCFLAGS = r'-I.')
	env.Append(CCFLAGS = [r'-Wall', r'-Werror', r'-O2'])
	
	if env['debug'] == 1:
		env.Append(CCFLAGS = r'-g3')
	
	if env['enable_tests'] == 1:
		env.ParseConfig('pkg-config --cflags --libs check')

	if env['enable_profiling'] == 1:
		env.Append(CCFLAGS = [r'-O0', r'-fprofile-arcs', r'-ftest-coverage'])
		env.Append(LINKFLAGS = [r'-fprofile-arcs', r'-ftest-coverage', r'-lgcov'])

	env.ParseConfig('pkg-config --cflags --libs glib-2.0')
	env.ParseConfig('pkg-config --cflags --libs libxml-2.0')
	env.ParseConfig('pkg-config --cflags --libs sqlite3')

	testenv = env.Copy()
	testenv.Append(CCFLAGS = r'-I' + testenv.GetLaunchDir() + '/tests')
	testenv.Append(CCFLAGS = r'-DOPENSYNC_TESTDATA="\"' + env.GetLaunchDir() + r'/tests/data\""')

	if env['enable_rpath'] == 1:
		testenv.Append(LINKFLAGS = [r'-Wl,--rpath', r'-Wl,' + testenv.GetLaunchDir() + r'/opensync/'])
		env.Append(LINKFLAGS = [r'-Wl,--rpath', r'-Wl,$prefix/$libsuffix'])

	env.Append(CCFLAGS = r'-I' + distutils.sysconfig.get_python_inc()) 
	
	return testenv
