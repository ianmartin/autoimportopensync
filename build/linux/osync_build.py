from osync_support import *
from SCons.Options import *

def configure(opts):
	opts.Add(PathOption('prefix', 'Directory, where opensync should be installed', '/usr/local'))
	opts.Add(PathOption('with_sqlite', 'Path to location of sqlite', '/usr'))
	opts.Add(PathOption('with_check', 'Path to location of sqlite', '/usr'))
	
def check(env, config):
	conf = env.Configure(custom_tests = {'CheckPKGConfig' : CheckPKGConfig, 'CheckPKG' : CheckPKG})
	
	if not conf.CheckPKGConfig('0.15.0'):
		print 'pkg-config >= 0.15.0 not found.'
		Exit(1)
		
	if not conf.CheckPKG('glib-2.0 >= 2.4'):
		print 'glib-2.0 >= 2.4 not found.'
		Exit(1)
		
	env = conf.Finish()
	
	if env['debug'] == 1:
		env.Append(CCFLAGS = r'-g')
	if env['debug_modules'] == 1:
		env.Append(CCFLAGS = r'-DDEBUG_MODULES')
	
	env.ParseConfig('pkg-config --cflags --libs glib-2.0')
	env.ParseConfig('pkg-config --cflags --libs libxml-2.0')
	env.Append(CCFLAGS = r'-I.')
	env.Append(CCFLAGS = [r'-Wall', r'-Werror'])
	
	testenv = env.Copy()
	testenv.Append(CCFLAGS = r'-I' + testenv.GetLaunchDir() + '/tests')
	testenv.Append(CCFLAGS = r'-I$with_check/include')
	testenv.Append(LINKFLAGS = r'-L$with_check/lib')
	testenv.Append(LINKFLAGS = [r'-Wl,--rpath', r'-Wl,$with_check/lib'])
	testenv.Append(LINKFLAGS = [r'-Wl,--rpath', r'-Wl,' + testenv.GetLaunchDir() + r'/opensync/'])
	testenv.Append(CCFLAGS = r'-DOPENSYNC_TESTDATA="\"' + env.GetLaunchDir() + r'/tests/data\""')
	
	env.Append(LINKFLAGS = [r'-Wl,--rpath', r'-Wl,$prefix/lib'])
	env.Append(CCFLAGS = r'-DOPENSYNC_PLUGINDIR="\"' + config.plugindir + r'\""')
	env.Append(CCFLAGS = r'-DOPENSYNC_FORMATSDIR="\"' + config.formatdir + r'\""')
	env.Append(CCFLAGS = r'-DOPENSYNC_CONFIGDIR="\"' + config.configdir + r'\""')
	env.Append(CCFLAGS = r'-DOPENSYNC_CAPABILITIESDIR="\"' + config.capabilitiesdir + r'\""')
	env.Append(CCFLAGS = r'-DOPENSYNC_DESCRIPTIONSDIR="\"' + config.descriptionsdir + r'\""')
	env.Append(CCFLAGS = r'-DOPENSYNC_SCHEMASDIR="\"' + config.descriptionsdir + r'\""')
	env.Append(CCFLAGS = r'-DVERSION="\"' + config.version + r'\""')
	env.Append(CCFLAGS = r'-DOPENSYNC_PLUGINVERSION=' + str(config.plugin_version) + '')
	
	return testenv
