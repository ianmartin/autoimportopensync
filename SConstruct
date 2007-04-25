import sys
import os
sys.path.append('build')
from osync_support import *
from doxygen import *
from substin import TOOL_SUBST


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
env = Environment(ENV = os.environ, tools=("default", TOOL_SUBST)) 
opts = Options('libopensync.conf')
opts.Add(BoolOption('debug', 'Should debugging be enabled?', 1))
opts.Add(BoolOption('enable_trace', 'Should tracing be enabled?', 1))
opts.Add(BoolOption('enable_tests', 'Should the unit tests be enabled', 0))
opts.Add(BoolOption('enable_tools', 'Should the developer tools be build', 1))
opts.Add(BoolOption('enable_profiling', 'Should code profiling be enabled', 0))
opts.Add(BoolOption('enable_python', 'Build python wrapper? (swig required)', 0))
opts.Add(BoolOption('debug_modules', 'Should unloading of shared modules be avoided (DEBUGGING ONLY!)', 0))
opts.Add(BoolOption('enable_doxygen', 'Generating OpenSync API with doxygen?', 0))
opts.AddOptions(
('CC', 'Path to Custom c compiler', 'gcc'),
('CXX', 'Path to Custom c++ compiler flags', 'g++'),
('LDFLAGS', 'Linker flags', ''),
)

target_dir = SelectBuildDir('build')
sys.path.append(target_dir)
from osync_build import *
target_dir = '#' + target_dir
configure(opts)
SConsignFile()

opts.Update(env)
opts.Save('libopensync.conf', env)
opts.Add("DESTDIR", 'Set the root directory to install into ( /path/to/DESTDIR )', "")
opts.Update(env)

Help("""
++++++++++++++++++++++++++++++++++++
Welcome to the OpenSync Help System!


You can set the following options:
""" + opts.GenerateHelpText(env))

env.Append(CCFLAGS = r'-DENABLE_TRACE=$enable_trace')
env.Append(CCFLAGS = r'-DENABLE_TESTS=$enable_tests')
env.Append(CCFLAGS = r'-DENABLE_TOOLS=$enable_tools')
env.Append(CCFLAGS = r'-DENABLE_PROFILING=$enable_profiling')

env.Replace(
       CC = env['CC'],
       CXX = env['CXX'],
       LDFLAGS = env['LDFLAGS']
)

# pkg config files
subst_dict={'@prefix@': '$prefix',
	    '@exec_prefix@': '${prefix}',
	    '@libdir@': '${prefix}/lib',
	    '@includedir@': '${prefix}/include',
	    '@OPENSYNC_PLUGINDIR@': config.plugindir,
	    '@OPENSYNC_CONFIGDIR@': config.configdir,
	    '@OPENSYNC_FORMATSDIR@': config.formatdir,
	    '@OPENSYNC_HEADERDIR@': config.headerdir,
            '@VERSION@': config.version
}

env.SubstInFile('opensync-1.0.pc', 'opensync-1.0.pc.in', SUBST_DICT=subst_dict)
env.SubstInFile('osengine-1.0.pc', 'osengine-1.0.pc.in', SUBST_DICT=subst_dict)
env.SubstInFile('Doxyfile', 'Doxyfile.in', SUBST_DICT=subst_dict)

env.Install('${DESTDIR}$prefix/lib/pkgconfig', 'opensync-1.0.pc') 
env.Install('${DESTDIR}$prefix/lib/pkgconfig', 'osengine-1.0.pc') 


if env['enable_doxygen'] == 1:
	doxygen = Builder(action = 'doxygen ' + 'Doxyfile')
	env.Append(BUILDERS = {'DoxygenBuilder' : doxygen})
	env.DoxygenBuilder(target = 'documentation', source =[])



testenv = check(env, config)

install_prefix = '${DESTDIR}$prefix'
install_lib    = '${DESTDIR}$prefix/lib'
install_bin    = '${DESTDIR}$prefix/bin'
install_inc    = '${DESTDIR}$prefix/include'
install_format    = '${DESTDIR}$prefix/lib/opensync/formats'
install_plugin    = '${DESTDIR}$prefix/lib/opensync/plugins'
install_config    = '${DESTDIR}$prefix/share/opensync/defaults'
install_capabilities = '${DESTDIR}$prefix/share/opensync/capabilities'
install_descriptions = '${DESTDIR}$prefix/share/opensync/descriptions'
install_schemas = '${DESTDIR}$prefix/share/opensync/schemas'
install_pythonlib = '${DESTDIR}$prefix/lib/python%d.%d/site-packages' % sys.version_info[:2]

Export('env opts testenv install_prefix install_lib install_bin install_inc install_format install_plugin install_config install_capabilities install_descriptions install_schemas install_pythonlib')

SConscript(['opensync/SConscript', 'tools/SConscript', 'tests/SConscript', 'formats/SConscript', 'misc/SConscript', 'wrapper/SConscript'])
BuildDir(target_dir, 'opensync', duplicate=0)
