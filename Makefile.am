## Process this file with automake to produce Makefile.in

SUBDIRS = src $(enginedir) formats $(toolsdir) $(testdir)

if BUILD_ENGINE
OSENGINE_PC = osengine.pc
endif

pkgconfigdir = /usr/lib/pkgconfig
pkgconfig_DATA = opensync.pc $(OSENGINE_PC)

DISTCLEANFILES = \
	opensync.pc  \
	osengine.pc

EXTRA_DIST = \
	autogen.sh \
	buildtest.sh \
	plugins \
	opensync.pc.in \
	osengine.pc.in

dist-hook:
	find $(distdir) -name .svn -print0 | xargs -0 rm -rf
	find $(distdir) -name autom4te.cache -print0 | xargs -0 rm -rf
	find $(distdir) -name .libs -print0 | xargs -0 rm -rf
	find $(distdir) -name .deps -print0 | xargs -0 rm -rf
	find $(distdir) -name *.la -print0 | xargs -0 rm -rf
	find $(distdir) -name *.lo -print0 | xargs -0 rm -rf
	find $(distdir) -name Makefile -print0 | xargs -0 rm -rf
