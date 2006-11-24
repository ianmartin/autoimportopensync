import sys
import os
sys.path.append('build')
from osync_support import *
from substin import *



#Define the default values for some variables. Take note, that they might
#get overwriten by the scons files in the build directory


class BuildConfig:
	version = "0.30"
	plugin_version = 1
	path_sep = r"/"
	plugindir = r"$prefix/lib/opensync/plugins"
	formatdir = r"$prefix/lib/opensync/formats"
	configdir = r"$prefix/share/opensync/defaults"
	capabilitiesdir = r"$prefix/share/opensync/capabilities"
	descriptionsdir = r"$prefix/share/opensync/descriptions"
	schemasdir = r"$prefix/share/opensync/schemas"
        headerdir= r"$prefix/include/opensync-1.0/opensync"


config = BuildConfig()

# Get our configuration options:
env = Environment(ENV = os.environ, tools=('default', TOOL_SUBST)) 
opts = Options('libopensync.conf')
opts.Add(BoolOption('debug', 'Should debugging be enabled?', 1))
opts.Add(BoolOption('enable_trace', 'Should tracing be enabled?', 1))
opts.Add(BoolOption('enable_tests', 'Should the unit tests be enabled', 1))
opts.Add(BoolOption('enable_tools', 'Should the developer tools be build', 1))
opts.Add(BoolOption('enable_profiling', 'Should code profiling be enabled', 0))

target_dir = SelectBuildDir('build')
sys.path.append(target_dir)
from osync_build import *
target_dir = '#' + target_dir
configure(opts)
SConsignFile()

opts.Update(env)
opts.Save('libopensync.conf', env)

env.Append(CCFLAGS = r'-DENABLE_TRACE=$enable_trace')
env.Append(CCFLAGS = r'-DENABLE_TESTS=$enable_tests')
env.Append(CCFLAGS = r'-DENABLE_TOOLS=$enable_tools')
env.Append(CCFLAGS = r'-DENABLE_PROFILING=$enable_profiling')


# pkg config files
subst_dict={'@prefix@': '$prefix',
	    '@exec_prefix@': '${prefix}',
	    '@libdir@': '${prefix}/lib',
	    '@includedir@': '${prefix}/include',
	    '@OPENSYNC_PLUGINDIR@': config.plugindir,
	    '@OPENSYNC_CONFIGDIR@': config.configdir,
	    '@OPENSYNC_FORMATSDIR@': config.formatdir,
	    '@OPENSYNC_HEADERDIR@': config.headerdir,
            '@VERSION@': '$version'
}

env.SubstInFile('opensync-1.0.pc', 'opensync-1.0.pc.in', SUBST_DICT=subst_dict)
env.SubstInFile('osengine-1.0.pc', 'osengine-1.0.pc.in', SUBST_DICT=subst_dict)

env.Install('$prefix/lib/pkgconfig', 'opensync-1.0.pc') 
env.Install('$prefix/lib/pkgconfig', 'osengine-1.0.pc') 


Help("""
++++++++++++++++++++++++++++++++++++
Welcome to the OpenSync Help System!


You can set the following options:
""" + opts.GenerateHelpText(env))

testenv = check(env, config)

install_prefix = '$prefix'
install_lib    = '$prefix/lib'
install_bin    = '$prefix/bin'
install_inc    = '$prefix/include'
install_format    = '$prefix/lib/opensync/formats'
install_plugin    = '$prefix/lib/opensync/plugins'
install_config    = '$prefix/share/opensync/defaults'
install_capabilities = '$prefix/share/opensync/capabilities'
install_descriptions = '$prefix/share/opensync/descriptions'
install_schemas = '$prefix/share/opensync/schemas'

Export('env opts testenv install_prefix install_lib install_bin install_inc install_format install_plugin install_config install_capabilities install_descriptions install_schemas')

SConscript(['opensync/SConscript', 'tools/SConscript', 'tests/SConscript', 'formats/SConscript', 'misc/SConscript'])
BuildDir(target_dir, 'opensync', duplicate=0)
