# Copyright Thomas Nagy 2005
# edit Matthias Jahn 2007
# BSD license (see COPYING)

def exists(env):
	return true

def generate(env):
	import SCons.Util, os
	from SCons.Options import Options
	env.get_opts().add_options(('HAVE_PKGCONFIG', 'whether to use pkg-config'))
	env.get_opts().update()

	# This funtion detects pkg-config
	from SCons.Script.SConscript import SConsEnvironment
	def Check_pkg_config(context, version):
		context.Message('Checking for pkg-config ... ')
			
		pkg_config_command = 'pkg-config'
		if os.environ.has_key("PKG_CONFIG_PATH"):
			pkg_config_command = "PKG_CONFIG_PATH="+os.environ["PKG_CONFIG_PATH"]+" pkg-config "
		ret = context.TryAction(pkg_config_command+' --atleast-pkgconfig-version=%s' % version)[0]
			
		context.Result(ret)
		
		return ret

	# This function detects a package using pkg-config
	def Check_package(context, pkgname, module, version):
		context.Message('Checking for %s >= %s ... ' % (module, version))
		pkg_config_command = 'pkg-config'
		if os.environ.has_key("PKG_CONFIG_PATH"):
			pkg_config_command = "PKG_CONFIG_PATH="+os.environ["PKG_CONFIG_PATH"]+" pkg-config "
		ret = context.TryAction(pkg_config_command+' %s --atleast-version=%s' % (module, version))[0]
		if ret:
			env.ParseConfig(pkg_config_command+' %s --cflags --libs' % module);
			env['CCFLAGS_'+pkgname] = SCons.Util.CLVar( 
				os.popen(pkg_config_command+" %s --cflags 2>/dev/null" % module).read().strip() );
			env['CXXFLAGS_'+pkgname] = env['CCFLAGS_'+pkgname]
			env['LINKFLAGS_'+pkgname] = SCons.Util.CLVar( 
					os.popen(pkg_config_command+" %s --libs 2>/dev/null" % module).read().strip() );
		context.Result(ret)
		return ret

	def pkgConfig_findPackage(env, pkgname, module, version="0"):
		env.get_opts().add_options(
				('HAVE_'+pkgname, 'whether '+pkgname+' was found'),
				('CCFLAGS_'+pkgname, 'additional compilation flags'),
				('CXXFLAGS_'+pkgname, 'additional compilation flags'),
				('LINKFLAGS_'+pkgname, 'link flags')
				)
		env.get_opts().update()
		if not env.has_key('HAVE_'+pkgname):
			conf = env.Configure(custom_tests =
					     { 'Check_pkg_config' : Check_pkg_config,
					       'Check_package' : Check_package }
					     )
			for i in ['CXXFLAGS_'+pkgname, 'LINKFLAGS_'+pkgname, 'CCFLAGS_'+pkgname]:
				if env.has_key(i): env.__delitem__(i)
			if not env.has_key('HAVE_PKGCONFIG'):
				if not conf.Check_pkg_config('0.15'):
					print 'pkg-config >= 0.15 not found.'
					env.Exit(1)
			haveModule = conf.Check_package(pkgname, module, version)
			env = conf.Finish()
			env['HAVE_'+pkgname] = haveModule
		else:
			haveModule = env['HAVE_'+pkgname]
		return haveModule
	
	if not env.has_key('HAVE_PKGCONFIG'):
		conf = env.Configure(custom_tests =
				     { 'Check_pkg_config' : Check_pkg_config  }
				     )

		env['HAVE_PKGCONFIG'] = conf.Check_pkg_config('0.15')
		
		env = conf.Finish()

	SConsEnvironment.pkgConfig_findPackage = pkgConfig_findPackage
