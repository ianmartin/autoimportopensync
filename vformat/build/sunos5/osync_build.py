from osync_support import *
from SCons.Options import *

def configure(opts):
	opts.add(PathOption('prefix', 'Directory, where opensync should be installed', '/usr'))
	opts.add(('libsuffix', 'Library suffic. lib64 for 64 bit systems', 'lib'))
	opts.add(BoolOption('enable_rpath', 'Build with -rpath?', 0))
	
def check(env, config):
	conf = env.Configure(custom_tests = {'CheckPKGConfig' : CheckPKGConfig, 'CheckPKG' : CheckPKG})
	
	if not conf.CheckPKGConfig('0.15.0'):
		print 'pkg-config >= 0.15.0 not found.'
		env.Exit(1)
		
	if not conf.CheckPKG('glib-2.0 >= 2.4'):
		print 'glib-2.0 >= 2.4 not found.'
		env.Exit(1)

	if not conf.CheckPKG('opensync-1.0 >= 0.30'):
		print 'opensync-1.0 >= 0.30 not found.'
		env.Exit(1)


	env = conf.Finish()
	
	if env['debug'] == 1:
		env.Append(CCFLAGS = r'-g')

	if env['debug_modules'] == 1:
		env.Append(CCFLAGS = r'-DDEBUG_MODULES')
	
	env.ParseConfig('pkg-config --cflags --libs glib-2.0')
	env.ParseConfig('pkg-config --cflags --libs libxml-2.0')
	env.ParseConfig('pkg-config --cflags --libs opensync-1.0')
	env.Append(CCFLAGS = r'-I.')
	env.Append(CCFLAGS = r'-DHAVE_CONFIG_H')
	env.add_define("VERSION", config['version'])
	
	testenv = env.Copy()
	testenv.Append(CCFLAGS = r'-I' + testenv.GetLaunchDir() + '/tests')

	if env['enable_tests'] == 1:
		testenv.Append(CCFLAGS = r'-DOPENSYNC_TESTDATA="\"' + env.GetLaunchDir() + r'/tests/data\""')

	if env['enable_rpath'] == 1:
		testenv.Append(LINKFLAGS = [r'-Wl,--rpath', r'-Wl,' + testenv.GetLaunchDir() + r'/opensync/'])
		env.Append(LINKFLAGS = [r'-Wl,--rpath', r'-Wl,$prefix/$libsuffix'])

	
	return testenv
