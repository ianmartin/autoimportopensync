## Process this file with automake to produce Makefile.in

SUBDIRS = src $(enginedir) formats tools $(testdir)

EXTRA_DIST = \
	autogen.sh

dist-hook:
	find $(distdir) -name .svn -print0 | xargs -0 rm -rf
