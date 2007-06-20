import os
import sys
from SCons.Script.SConscript import SConsEnvironment

def CheckPKGConfig(context, version):
	context.Message( 'Checking for pkg-config... ' )
	ret = context.TryAction('pkg-config --atleast-pkgconfig-version=%s' % version)[0]
	context.Result( ret )
	return ret

def CheckPKG(context, name):
	context.Message( 'Checking for %s... ' % name )
	ret = context.TryAction('pkg-config --exists \'%s\'' % name)[0]
	context.Result( ret )
	return ret

def SelectBuildDir(build_dir, platform=None):

	# if no platform is specified, then default to sys.platform
	if not(platform):
		platform = sys.platform

	print "Looking for build directory for platform '%s'" % platform

	# setup where we start looking at first
	test_dir = build_dir + os.sep + platform
	default_dir = build_dir + os.sep + 'linux'


	# we look for a directory named exactly after the
	# platform so that very specific builds can be done
	if os.path.exists(test_dir):
		# make sure it is a directory
		target_dir = test_dir
	else:
		print "Exact match not found, finding closest guess"

		# looks like there isn't an exact match
		# find the closest matching directory
		dirs = os.listdir(build_dir)
		found_match = 0
		for dir in dirs:
			if platform.find(dir) != -1:
				# found a match (hopefully the right one)
				target_dir = build_dir + os.sep + dir
				found_match = 1
				break
		if not(found_match):
			print "No match found, looking for 'default' directory"
			# looks like this platform isn't available
			# try the default target
			if os.path.exists(default_dir):
				target_dir = default_dir
			else:
				# bad, nothing is available, tell the user
				print "No build directories found for your platform '%s'" % platform
			return None

	print "Found directory %s, will build there" % target_dir
	return target_dir

def dist(env, appname=None, version=None):
	"dist target - should be portable"
	import shutil, tarfile
	if not appname: appname=config["name"]
	if not version: version=config["version"]

	# Our temporary folder where to put our files
	TMPFOLDER=appname+'-'+str(version)

	# Remove an old package directory
	if os.path.exists(TMPFOLDER): shutil.rmtree(TMPFOLDER)

	# Copy everything into the new folder
	shutil.copytree('.', TMPFOLDER)

	# Enter into it and remove unnecessary files
	os.chdir(TMPFOLDER)
	#clean up build in tmpdir
	os.popen('scons -Q -c').read()
	for (root, dirs, filenames) in os.walk('.'):
		clean_dirs = []
		for d in dirs:
			if d in ['CVS', 'cache','.svn', '{arch}']:
				shutil.rmtree(os.path.join(root,d))
			elif d.startswith('.'):
				shutil.rmtree(os.path.join(root,d))
			else:
				clean_dirs += d
		dirs = clean_dirs

		to_remove = False
		for f in list(filenames):
			if f.startswith('.'): to_remove = True
			elif f.endswith('~'): to_remove = True
			elif f.endswith('.pyc'): to_remove = True
			elif f.endswith('.pyo'): to_remove = True
			elif f.endswith('.bak'): to_remove = True
			elif f.endswith('.orig'): to_remove = True
			elif f in ['config.log']: to_remove = True
			elif f.endswith('.tar.bz2'): to_remove = True
			elif f.endswith('.zip'): to_remove = True
			elif f.endswith('Makefile'): to_remove = True

			if to_remove:
				os.remove(os.path.join(root, f))
				to_remove = False

	# go back to the root directory
	os.chdir('..')

	tar = tarfile.open(TMPFOLDER+'.tar.bz2','w:bz2')
	tar.add(TMPFOLDER)
	tar.close()
	print 'Your archive is ready -> '+TMPFOLDER+'.tar.bz2'

	if os.path.exists(TMPFOLDER): shutil.rmtree(TMPFOLDER)

	sys.exit(0)

def add_define(env, define, value = 1, quote = -1):
	"""store a single define and its state into an internal list
		for later writing to a config header file"""
	try:
		tbl = env['defines']
	except KeyError:
		tbl = {}
	if not define: raise "define must be .. defined"

	value = env.subst(value) #substitute construction values
	# the user forgot to tell if the value is quoted or not
	if quote < 0:
		if isinstance(value, basestring):
			tbl[define] = '"%s"' % str(value)
		else:
			tbl[define] = value
	elif not quote:
		if isinstance(value, int):
			tbl[define] = int(value)
		else:
			tbl[define] = value
	else:
		tbl[define] = '"%s"' % str(value)
	env['defines'] = tbl

def is_defined(env, define):
	try: return env['defines'].has_key(define)
	except: return None

def get_define(env, define):
	"get the value of a previously stored define"
	try: return env['defines'][define]
	except: return 0

def write_config_header(env, configfile='config.h'):
	"save the defines into a file"
	import re, os
	reg=re.compile('[/\\\\.-]', re.M)

	include_name = '_'.join(reg.split(configfile)).upper()
	inclusion_guard_name = '_%s_OPENSYNC' % (include_name)

	dest = open(configfile, 'w')
	dest.write('/* configuration created by OpenSync build */\n')
	dest.write('#ifndef %s\n#define %s\n\n' % (inclusion_guard_name, inclusion_guard_name))
	try:
		for key, value in env['defines'].iteritems():
			if value is None:
				dest.write('#define %s\n' % key)
			elif isinstance(value, int):
				dest.write('#define %s %i\n' % (key, value))
			elif value:
				dest.write('#define %s %s\n' % (key, value))
			else:
				dest.write('/* #undef %s */\n' % key)
	except KeyError:
		pass
	dest.write('\n#endif /* %s */\n' % (inclusion_guard_name,))
	dest.close()

class opts:
	def __init__(self, env, option_file="config_cache.py", cache_dir="cache"):
		from SCons.Options import Options
		self.env=env
		self.args=self.__makeHashTable(sys.argv)
		self.option_file=os.path.join(cache_dir, option_file)
		self.env['CACHEDIR']=cache_dir
		if not os.path.isdir(env['CACHEDIR']): os.mkdir(env['CACHEDIR'])
		self.opts=Options(self.option_file)
	
	#TODO: this parser is silly, fix scons command-line handling instead
	def __makeHashTable(self, args):
		table = { }
		for arg in args:
			if len(arg) > 1:
				lst=arg.split('=')
				if len(lst) < 2: continue
				key=lst[0]
				value=lst[1]
				if len(key) > 0 and len(value) >0: table[key] = value
		return table
	
	def update(self):
		self.opts.Update(self.env, self.args)
	
	def save(self):
		self.opts.Save(self.option_file, self.env)
	
	def add(self, key, help="", default=None, validator=None, converter=None, **kw):
		self.opts.Add(key, help, default, validator, converter, **kw)
	
	def add_options(self, *optlist):
		self.opts.AddOptions(*optlist)
	
	def generate_help_text(self):
		return self.opts.GenerateHelpText(self.env)

#wrapper methode to initalise and access opts_class object
def get_opts(env, option_file="config_cache.py", cache_dir="cache"):
	if not env.has_key('OPTIONS'):
		env['OPTIONS'] = env.opts_class(env, option_file, cache_dir)
	return env['OPTIONS']

# Since SCons doesn't support versioning, we need to do it for SCons. 
# Unfortunately, that means having to do things differently depending 
# on platform... 
def build_shlib(env, target_name, sources, vnum=None, **kw): 
	import SCons.Util
	import re
	platform = env.subst('$PLATFORM') 
	shlib_pre_action = None 
	shlib_suffix = env.subst('$SHLIBSUFFIX') 
	shlib_post_action = None 
	shlink_flags = SCons.Util.CLVar(env.subst('$SHLINKFLAGS')) 

	if platform == 'posix': 
		shlib_post_action = [ 'rm -f $TARGET', 'ln -s ${SOURCE.file} $TARGET' ] 
		shlib_post_action_output_re = [ 
			'%s\\.[0-9\\.]*$' % re.escape(shlib_suffix), 
			shlib_suffix ] 
		shlib_suffix += '.' + vnum 
		shlink_flags += [ '-Wl,-Bsymbolic', '-Wl,-soname=${TARGET}' ] 
	elif platform == 'aix': 
		shlib_pre_action = [ 
			"nm -Pg $SOURCES > ${TARGET}.tmp1", 
			"grep ' [BDT] ' < ${TARGET}.tmp1 > ${TARGET}.tmp2", 
			"cut -f1 -d' ' < ${TARGET}.tmp2 > ${TARGET}", 
			"rm -f ${TARGET}.tmp[12]" ] 
		shlib_pre_action_output_re = [ '$', '.exp' ] 
		shlib_post_action = [ 'rm -f $TARGET', 'ln -s $SOURCE $TARGET' ] 
		shlib_post_action_output_re = [ 
			'%s\\.[0-9\\.]*' % re.escape(shlib_suffix), 
			shlib_suffix ] 
		shlib_suffix += '.' + vnum
		shlink_flags += ['-G', '-bE:${TARGET}.exp', '-bM:SRE'] 
	elif platform == 'cygwin': 
		shlink_flags += [ '-Wl,-Bsymbolic', 
				'-Wl,--out-implib,${TARGET.base}.a' ] 
	elif platform == 'darwin': 
		shlib_suffix = '.' + vnum + shlib_suffix 
		shlink_flags += [ '-dynamiclib', 
				'-current-version %s' % vnum ] 

	lib = env.SharedLibrary(target_name, sources, 
				SHLIBSUFFIX=shlib_suffix, 
				SHLINKFLAGS=shlink_flags, **kw) 

	if shlib_pre_action: 
		shlib_pre_action_output = re.sub(shlib_pre_action_output_re[0], 
						shlib_pre_action_output_re[1], 
						str(lib[0])) 
		env.Command(shlib_pre_action_output, [ lib_objs ], 
				shlib_pre_action) 
		env.Depends(lib, shlib_pre_action_output) 
	if shlib_post_action: 
		shlib_post_action_output = re.sub(shlib_post_action_output_re[0], 
						shlib_post_action_output_re[1], 
						str(lib[0])) 
		env.Command(shlib_post_action_output, lib, shlib_post_action) 
	return lib 

def install_shlib(env, destination, lib): 
	import re
	platform = env.subst('$PLATFORM') 
	shlib_suffix = env.subst('$SHLIBSUFFIX') 
	shlib_install_pre_action = None 
	shlib_install_post_action = None 

	if platform == 'posix': 
		shlib_post_action = [ 'rm -f $TARGET', 
					'ln -s ${SOURCE.file} $TARGET' ] 
		shlib_post_action_output_re = [ 
			'%s\\.[0-9\\.]*$' % re.escape(shlib_suffix), 
			shlib_suffix ] 
		shlib_install_post_action = shlib_post_action 
		shlib_install_post_action_output_re = shlib_post_action_output_re 

	ilib = env.Install(destination,lib) 

	if shlib_install_pre_action: 
		shlib_install_pre_action_output = re.sub(shlib_install_pre_action_output_re[0], 
							shlib_install_pre_action_output_re[1], 
							str(ilib[0])) 
		env.Command(shlib_install_pre_action_output, ilib, 
				shlib_install_pre_action) 
		env.Depends(shlib_install_pre_action_output, ilib) 
	if shlib_install_post_action: 
		shlib_install_post_action_output = re.sub(shlib_install_post_action_output_re[0], 
							shlib_install_post_action_output_re[1], 
							str(ilib[0])) 
		env.Command(shlib_install_post_action_output, ilib, 
				shlib_install_post_action)

def exists(env):
	return true

def generate(env):
	SConsEnvironment.dist = dist
	SConsEnvironment.write_config_header = write_config_header
	SConsEnvironment.add_define = add_define
	SConsEnvironment.is_defined = is_defined
	SConsEnvironment.get_define = get_define
	SConsEnvironment.opts_class = opts
	SConsEnvironment.get_opts = get_opts
	SConsEnvironment.build_shlib = build_shlib
	SConsEnvironment.install_shlib = install_shlib
	
	env.get_opts()
