from support import *
from os import *

version = "0.30"
plugin_version = 1

path_sep = r"\\"

#pkg_config = ARGUMENTS.get('with-pkg-config', 'pkg-config')
#pkg_config_path = ARGUMENTS.get('pkg-config-path', '/usr/lib/pkgconfig')

env = Environment()

# Get our configuration options:
opts = Options('libopensync.conf')
opts.Add('prefix', 'Directory to install under', '/usr/local')
opts.Add(BoolOption('enable_trace', 'Should tracing be enabled?', 1))
opts.Add('sqlite_headers', 'Path to location of sqlite header', '/usr/include')
opts.Add('sqlite_libs', 'Path to location of sqlite libs', '/usr/lib')
opts.Update(env)
opts.Save('libopensync.conf', env)

Help(opts.GenerateHelpText(env))

#conf = Configure(env, custom_tests = { 'CheckPKGConfig' : CheckPKGConfig,
#                                       'CheckPKG' : CheckPKG })

#if not conf.CheckPKGConfig(pkg_config, '0.15.0'):
#     print 'pkg-config (' + pkg_config + ') >= 0.15.0 not found.'
#     Exit(1)

#if not conf.CheckPKG('glib-2.0 >= 2.4'):
#     print 'glib-2.0 >= 2.4 not found.'
#     Exit(1)

#env = conf.Finish()

install_prefix = '$prefix'
install_lib    = '$prefix/lib'
install_bin    = '$prefix/bin'
install_inc    = '$prefix/include'

env.Append(CCFLAGS = '-DOPENSYNC_PLUGINDIR=\"\\"$prefix/libs/opensync/plugins\\"\"')
env.Append(CCFLAGS = r'-DOPENSYNC_FORMATSDIR="\"$prefix/libs/opensync/formats\""')
env.Append(CCFLAGS = '-DOPENSYNC_CONFIGDIR="\\"$prefix' + path_sep + 'share' + path_sep + 'opensync' + path_sep + 'defaults\\"\"')
env.Append(CCFLAGS = '-DVERSION=\"\\"' + version + '\"\\"')
env.Append(CCFLAGS = r'-DENABLE_TRACE=$enable_trace')
env.Append(CCFLAGS = '-DOPENSYNC_PLUGINVERSION=\"' + str(plugin_version) + '\"')
env.Append(CCFLAGS = '/I"C:\Documents and Settings\\abauer\Desktop\glib\include"' )
env.Append(CCFLAGS = '/I.' )
env.Append(CCFLAGS = '/Iopensync' )
env.Append(CCFLAGS = '/I"C:\Documents and Settings\\abauer\Desktop\glib\include\glib-2.0"' )
env.Append(CCFLAGS = '/I"C:\Documents and Settings\\abauer\Desktop\glib\lib\glib-2.0\include"' )
env.Append(CPPPATH = '.' )

#env.ParseConfig('pkg-config --cflags --libs glib-2.0')
#env.ParseConfig('pkg-config --cflags --libs libxml-2.0')

sources = []

Export('env install_prefix install_lib install_bin install_inc sources')


target_dir = '#' + SelectBuildDir('build')
SConscript('opensync/SConscript')
BuildDir(target_dir, 'opensync', duplicate=0)
Default('opensync')

#SConscript(['opensync/SConscript'], build_dir=SelectBuildDir("build"))
