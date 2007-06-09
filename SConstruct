import sys
import os
sys.path.append('build')
from osync_support import *
from doxygen import *
from substin import TOOL_SUBST

#Define the default values for some variables. Take note, that they might
#get overwriten by the scons files in the build directory
config = {
	'version': "0.30",
	'major': 1,
	'minor': 0,
	'micro': 0,
	'plugin_version': 1,
	'path_sep': r"/",
	'plugindir': r"$prefix/$libsuffix/opensync/plugins",
	'formatdir': r"$prefix/$libsuffix/opensync/formats",
	'configdir': r"$prefix/share/opensync/defaults",
	'capabilitiesdir': r"$prefix/share/opensync/capabilities",
	'descriptionsdir': r"$prefix/share/opensync/descriptions",
	'schemasdir': r"$prefix/share/opensync/schemas",
	'headerdir': r"$prefix/include/opensync-1.0/opensync"
}

# Get our configuration options:
env = Environment(ENV = os.environ, tools=("default", TOOL_SUBST)) 
opts = Options('libopensync.conf')
A = opts.Add
A('debug', 'Should debugging be enabled?', 1)
A('enable_trace', 'Should tracing be enabled?', 1)
A(BoolOption('enable_tests', 'Should the unit tests be enabled', 0))
A(BoolOption('enable_tools', 'Should the developer tools be build', 1))
A(BoolOption('enable_python', 'Build python wrapper? (swig required)', 0))
A(BoolOption('debug_modules', 'Should unloading of shared modules be avoided (DEBUGGING ONLY!)', 0))
A(BoolOption('enable_doxygen', 'Generating OpenSync API with doxygen?', 0))
A(('APPEND_LDFLAGS', 'Linker flags'))
A(('APPEND_CCFLAGS', 'Compiler flags'))
A(('DESTDIR', 'Set the root directory to install into ( /path/to/DESTDIR )', ''))

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

env.Append(CCFLAGS = Split('$APPEND_CCFLAGS'))
env.Append(LDFLAGS = Split('$APPEND_LDFLAGS'))

add_define(env, "OPENSYNC_PLUGINDIR", config['plugindir'])
add_define(env, "OPENSYNC_FORMATSDIR", config['formatdir'])
add_define(env, "OPENSYNC_CONFIGDIR", config['configdir'])
add_define(env, "OPENSYNC_CAPABILITIESDIR", config['capabilitiesdir'])
add_define(env, "OPENSYNC_DESCRIPTIONSDIR", config['descriptionsdir'])
add_define(env, "OPENSYNC_SCHEMASDIR", config['descriptionsdir'])
add_define(env, "VERSION", config['version'])
add_define(env, "OPENSYNC_PLUGINVERSION", config['plugin_version'])
if env['debug_modules'] == 1:
	add_define(env, "DEBUG_MODULES")
add_define(env, "ENABLE_TRACE", env['enable_trace'])

env.Replace(
	CC = env['CC'],
	CXX = env['CXX']
)

if env['enable_doxygen'] == 1:
	doxygen = Builder(action = 'doxygen ' + 'Doxyfile')
	env.Append(BUILDERS = {'DoxygenBuilder' : doxygen})
	env.DoxygenBuilder(target = 'documentation', source =[])

testenv = check(env, config)
if not env.GetOption('clean'):
	write_config_header(env, "config.h")
else:
	try: os.unlink("config.h")
	except OSError: pass

#define some shortcuts for common used methodes
p_j = os.path.join
if env['DESTDIR']: install_prefix = p_j('${DESTDIR}', '$prefix')
else: install_prefix = p_j('$prefix')
install_lib = p_j(install_prefix, '$libsuffix')
install_bin = p_j(install_prefix, 'bin')
install_inc = p_j(install_prefix, 'include')
install_format = p_j(install_lib, 'opensync', 'formats')
install_plugin = p_j(install_lib, 'opensync', 'plugins')
install_data = p_j(install_prefix, 'share', 'opensync')
install_config = p_j(install_data, 'defaults')
install_capabilities = p_j(install_data, 'capabilities')
install_descriptions = p_j(install_data, 'descriptions')
install_schemas = p_j(install_data, 'schemas')
install_pythonlib = p_j(install_lib, 'python%d.%d' % sys.version_info[:2], 'site-packages')

# pkg config files
subst_dict={'@prefix@': '$prefix',
	'@exec_prefix@': '${prefix}',
	'@libdir@': '${prefix}/${libsuffix}',
	'@includedir@': '${prefix}/include',
	'@OPENSYNC_PLUGINDIR@': config['plugindir'],
	'@OPENSYNC_CONFIGDIR@': config['configdir'],
	'@OPENSYNC_FORMATSDIR@': config['formatdir'],
	'@OPENSYNC_HEADERDIR@': config['headerdir'],
	'@VERSION@': config['version']
}

env.SubstInFile('opensync-1.0.pc', 'opensync-1.0.pc.in', SUBST_DICT=subst_dict)
env.SubstInFile('osengine-1.0.pc', 'osengine-1.0.pc.in', SUBST_DICT=subst_dict)
env.SubstInFile('Doxyfile', 'Doxyfile.in', SUBST_DICT=subst_dict)

env.Install(p_j(install_lib, 'pkgconfig'), 'opensync-1.0.pc') 
env.Install(p_j(install_lib, 'pkgconfig'), 'osengine-1.0.pc') 


Export('env opts testenv install_prefix install_lib install_bin install_inc install_format install_plugin install_config install_capabilities install_descriptions install_schemas install_pythonlib')

SConscript(['opensync/SConscript', 'tools/SConscript', 'tests/SConscript', 'formats/SConscript', 'misc/SConscript', 'wrapper/SConscript'])
BuildDir(target_dir, 'opensync', duplicate=0)
