## Process this file with automake to produce Makefile.in

SUBDIRS = src $(enginedir) formats tools $(testdir)

EXTRA_DIST = \
	autogen.sh \
	buildtest.sh \
	plugins

dist-hook:
	find $(distdir) -name .svn -print0 | xargs -0 rm -rf
	find $(distdir) -name autom4te.cache -print0 | xargs -0 rm -rf
	find $(distdir) -name .libs -print0 | xargs -0 rm -rf
	find $(distdir) -name .deps -print0 | xargs -0 rm -rf
	find $(distdir) -name *.la -print0 | xargs -0 rm -rf
	find $(distdir) -name *.lo -print0 | xargs -0 rm -rf
	find $(distdir) -name Makefile -print0 | xargs -0 rm -rf
