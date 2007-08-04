from osync_support import *
from SCons.Options import *

def CheckCURL(context, name): 
	context.Message('Checking for %s... ' % name) 
	ret = context.TryAction('curl-config --version')[0] 
	context.Result(ret) 
	return ret 

def configure(opts):
	opts.Add(PathOption('prefix', 'Directory, where opensync should be installed', '/usr/local'))
	opts.Add(('libsuffix', 'Library suffic. lib64 for 64 bit systems', 'lib'))
	opts.Add(BoolOption('enable_rpath', 'Build with -rpath?', 0))
	
def check(env, config):
	conf = env.Configure(custom_tests = {'CheckPKGConfig' : CheckPKGConfig, 'CheckPKG' : CheckPKG, 'CheckCURL' : CheckCURL})
	
	if not conf.CheckPKGConfig('0.15.0'):
		print 'pkg-config >= 0.15.0 not found.'
		env.Exit(1)
		
	if not conf.CheckPKG('glib-2.0 >= 2.4'):
		print 'glib-2.0 >= 2.4 not found.'
		env.Exit(1)

	if not conf.CheckPKG('libxml-2.0 >= 2.6'):
		print 'libxml-2.0 >= 2.6 not found.'
		env.Exit(1)

	if not conf.CheckCURL('libcurl >= 7.13'):
		print 'libcurl >= 7.13 not found.'
		env.Exit(1)

	if not conf.CheckPKG('openssl >= 0.9.5'):
		print 'openssl >= 0.9.5 not found.'
		env.Exit(1)

	if not conf.CheckPKG('opensync-1.0 >= 0.30'):
		print 'opensync-1.0 >= 0.30 not found.'
		env.Exit(1)


	env = conf.Finish()
	
	if env['debug'] == 1:
		env.Append(CCFLAGS = r'-g')

	env.ParseConfig('pkg-config --cflags --libs glib-2.0')
	env.ParseConfig('pkg-config --cflags --libs libxml-2.0')
	env.ParseConfig('pkg-config --cflags --libs openssl')
	env.ParseConfig('curl-config --cflags --libs')
	env.ParseConfig('pkg-config --cflags --libs opensync-1.0')
	env.Append(CCFLAGS = r'-I.')
#	env.Append(CCFLAGS = [r'-Wall', r'-Werror', r'-O2'])
	env.Append(CCFLAGS = [r'-Wall', r'-O2'])
	env.Append(CCFLAGS = r'-DVERSION="\"' + config.version + r'\""')
	
	testenv = env.Copy()
	testenv.Append(CCFLAGS = r'-I' + testenv.GetLaunchDir() + '/tests')

	if env['enable_rpath'] == 1:
		testenv.Append(LINKFLAGS = [r'-Wl,--rpath', r'-Wl,' + testenv.GetLaunchDir() + r'/opensync/'])
		env.Append(LINKFLAGS = [r'-Wl,--rpath', r'-Wl,$prefix/$libsuffix'])

	
	return testenv
