## Process this file with automake to produce Makefile.in

SUBDIRS = src $(enginedir) formats $(toolsdir) $(testdir)

if BUILD_ENGINE
OSENGINE_PC = osengine-1.0.pc
endif

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = opensync-1.0.pc $(OSENGINE_PC)

DISTCLEANFILES = \
	opensync-1.0.pc  \
	osengine-1.0.pc

EXTRA_DIST = \
	autogen.sh \
	buildtest.sh \
	plugins \
	opensync-1.0.pc.in \
	osengine-1.0.pc.in

dist-hook:
	find $(distdir) -name .svn -print0 | xargs -0 rm -rf
	find $(distdir) -name autom4te.cache -print0 | xargs -0 rm -rf
	find $(distdir) -name .libs -print0 | xargs -0 rm -rf
	find $(distdir) -name .deps -print0 | xargs -0 rm -rf
	find $(distdir) -name *.la -print0 | xargs -0 rm -rf
	find $(distdir) -name *.lo -print0 | xargs -0 rm -rf
	find $(distdir) -name Makefile -print0 | xargs -0 rm -rf
