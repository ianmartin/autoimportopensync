# Copyright Matthias Jahn 2007
# BSD license (see COPYING)

def exists(env):
	return true

def generate(env):
	have_sqlite  = env.pkgConfig_findPackage('SQLITE', 'sqlite3', '3.3')
	# if the config worked, read the necessary variables and cache them
	if not have_sqlite:
		print'package \'sqlite\' not found. http://sqlite.org'
		env.Exit(1)

