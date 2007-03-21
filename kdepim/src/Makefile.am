plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@

INCLUDES = $(OSYNC_CFLAGS) $(XML_CFLAGS) $(GLIB_CFLAGS) $(KDE_INCLUDES) $(QT_INCLUDES) -I$(top_srcdir) -I$(top_srcdir)/src
# KDE has broken classes so -Werror cannot be used (lots of non-virtual
# destructors in polymorphic classes).
AM_CXXFLAGS = -Wall

#FIXME: Is there a KDE macro to set this?
LIB_KCAL = -lkcal

config_DATA = kdepim-sync

plugin_LTLIBRARIES = kdepim_sync.la kdepim_lib.la

EXTRA_DIST = \
	kaddrbook.h \
	kcal.h \
	kdepim-sync \
	osyncbase.h \
	knotes.h \
	KNotesIface.h
	
CLEANFILES = \
	KNotesIface_stub.h \
	KNotesIface_skel.cpp \
	KNotesIface_stub.cpp \
	KNotesIface.kidl

kdepim_sync_la_SOURCES = kdepim_sync.cpp
kdepim_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
# ugly hack to make the c++ library work
kdepim_sync_la_LIBADD = -lstdc++ @OSYNC_LIBS@

$(srcdir)/kdepim_impl.cpp: KNotesIface_stub.h
KNotesIface_stub.cpp: KNotesIface_stub.h

KNotesIface_stub.cpp:
	dcopidl $(srcdir)/KNotesIface.h > KNotesIface.kidl
	dcopidl2cpp KNotesIface.kidl
KNotesIface_stub.h:
	dcopidl $(srcdir)/KNotesIface.h > KNotesIface.kidl
	dcopidl2cpp KNotesIface.kidl

dist_kdepim_lib_la_SOURCES = \
	kdepim_impl.cpp \
	kcal.cpp \
	knotes.cpp \
	kaddrbook.cpp 
nodist_kdepim_lib_la_SOURCES = KNotesIface_stub.cpp
kdepim_lib_la_LDFLAGS = $(KDE_LDFLAGS) -avoid-version -export-dynamic -module
kdepim_lib_la_LIBADD = $(KDE_RPATH) $(LIB_KABC) $(LIB_KCAL) -L$(libdir) @OSYNC_LIBS@ -lopensync-xml
