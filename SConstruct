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
	major = 1
	minor = 0	       
	micro = 0		       
	plugin_version = 1
	path_sep = r"/"
	plugindir = r"$prefix/$libsuffix/opensync/plugins"
	formatdir = r"$prefix/$libsuffix/opensync/formats"
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
opts.Add(('CC', 'Path to Custom c compiler', 'gcc'))
opts.Add(('CXX', 'Path to Custom c++ compiler flags', 'g++'))
opts.Add(('APPEND_LDFLAGS', 'Linker flags'))
opts.Add(('APPEND_CCFLAGS', 'Compiler flags'))
opts.Add(('DESTDIR', 'Set the root directory to install into ( /path/to/DESTDIR )', ''))

target_dir = SelectBuildDir('build')
sys.path.append(target_dir)
from osync_build import *
target_dir = '#' + target_dir
configure(opts)
SConsignFile()

opts.Update(env)
opts.Save('libopensync.conf', env)
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
env.Append(CCFLAGS = Split('$APPEND_CCFLAGS'))
env.Append(LDFLAGS = Split('$APPEND_LDFLAGS'))

env.Replace(
       CC = env['CC'],
       CXX = env['CXX']
)

# pkg config files
subst_dict={'@prefix@': '$prefix',
	    '@exec_prefix@': '${prefix}',
	    '@libdir@': '${prefix}/${libsuffix}',
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

env.Install('${DESTDIR}$prefix/$libsuffix/pkgconfig', 'opensync-1.0.pc') 
env.Install('${DESTDIR}$prefix/$libsuffix/pkgconfig', 'osengine-1.0.pc') 


if env['enable_doxygen'] == 1:
	doxygen = Builder(action = 'doxygen ' + 'Doxyfile')
	env.Append(BUILDERS = {'DoxygenBuilder' : doxygen})
	env.DoxygenBuilder(target = 'documentation', source =[])



testenv = check(env, config)

install_prefix = '${DESTDIR}$prefix'
install_lib    = '${DESTDIR}$prefix/$libsuffix'
install_bin    = '${DESTDIR}$prefix/bin'
install_inc    = '${DESTDIR}$prefix/include'
install_format    = '${DESTDIR}$prefix/$libsuffix/opensync/formats'
install_plugin    = '${DESTDIR}$prefix/$libsuffix/opensync/plugins'
install_config    = '${DESTDIR}$prefix/share/opensync/defaults'
install_capabilities = '${DESTDIR}$prefix/share/opensync/capabilities'
install_descriptions = '${DESTDIR}$prefix/share/opensync/descriptions'
install_schemas = '${DESTDIR}$prefix/share/opensync/schemas'
install_pythonlib = '${DESTDIR}$prefix/$libsuffix/python%d.%d/site-packages' % sys.version_info[:2]

Export('env opts testenv install_prefix install_lib install_bin install_inc install_format install_plugin install_config install_capabilities install_descriptions install_schemas install_pythonlib')

SConscript(['opensync/SConscript', 'tools/SConscript', 'tests/SConscript', 'formats/SConscript', 'misc/SConscript', 'wrapper/SConscript'])
BuildDir(target_dir, 'opensync', duplicate=0)
