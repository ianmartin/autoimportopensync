## Process this file with automake to produce Makefile.in

SUBDIRS = opensync osengine formats tests tools wrapper

if BUILD_ENGINE
OSENGINE_PC = osengine-1.0.pc
endif

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = opensync-1.0.pc $(OSENGINE_PC)

DISTCLEANFILES = \
	opensync-1.0.pc  \
	opensync-1.0-uninstalled.pc \
	osengine-1.0.pc  \
	osengine-1.0-uninstalled.pc \
	py-compile

EXTRA_DIST = \
	opensync-1.0.pc.in \
	osengine-1.0.pc.in \
	misc/opensync.spec.in \
	misc/opensync.spec \
	misc/buildtest.sh \
	misc/doxygen.css

dist-hook:
	find $(distdir) -name .svn -print0 | xargs -0 rm -rf
	find $(distdir) -name autom4te.cache -print0 | xargs -0 rm -rf
	find $(distdir) -name .libs -print0 | xargs -0 rm -rf
	find $(distdir) -name .deps -print0 | xargs -0 rm -rf
	find $(distdir) -name *.la -print0 | xargs -0 rm -rf
	find $(distdir) -name *.lo -print0 | xargs -0 rm -rf
	find $(distdir) -name Makefile -print0 | xargs -0 rm -rf
