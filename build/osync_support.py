import os
import sys

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

def make_dist(config):
	"dist target - should be portable"
	import shutil, tarfile

	# Our temporary folder where to put our files
	TMPFOLDER=config["name"]+'-'+config["version"]

	# Remove an old package directory
	if os.path.exists(TMPFOLDER): shutil.rmtree(TMPFOLDER)

	# Copy everything into the new folder
	shutil.copytree('.', TMPFOLDER)

	# Enter into it and remove unnecessary files
	os.chdir(TMPFOLDER)
	os.popen('scons -Q -c').read()
	for (root, dirs, filenames) in os.walk('.'):
		clean_dirs = []
		for d in dirs:
			if d in ['CVS', '.svn', '{arch}']:
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
