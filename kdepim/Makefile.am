
SUBDIRS = tests

plugindir=@OPENSYNC_PLUGINDIR@

INCLUDES = $(OSYNC_CFLAGS) $(XML_CFLAGS) $(GLIB_CFLAGS) $(KDE_INCLUDES) $(QT_INCLUDES)
AM_CXXFLAGS = -Wall -Werror

#FIXME: Is there a KDE macro to set this?
LIB_KCAL = -lkcal

plugin_LTLIBRARIES = kdepim_sync.la kdepim_lib.la

EXTRA_DIST = \
	src/kaddrbook.h \
	src/kcal.h \
	src/osyncbase.h \
	src/knotes.h \
	src/KNotesIface.h
	
CLEANFILES = \
	src/KNotesIface_stub.h \
	src/KNotesIface_skel.cpp \
	src/KNotesIface_stub.cpp \
	src/KNotesIface.kidl

kdepim_sync_la_SOURCES = src/kdepim_sync.cpp
kdepim_sync_la_CXXFLAGS = -DPLUGINDIR=\"$(plugindir)\" -DKDEPIM_LIBDIR=\"$(plugindir)\" -Wall -Werror
kdepim_sync_la_LDFLAGS = -avoid-version -export-dynamic -module

# ugly hack to make the c++ library work
kdepim_sync_la_LIBADD = -lstdc++ -L@OPENSYNC_LIBDIR@

kdepim_lib_la_SOURCES = \
	src/KNotesIface_stub.cpp \
	src/kdepim_impl.cpp \
	src/kcal.cpp \
	src/knotes.cpp \
	src/kaddrbook.cpp
kdepim_lib_la_CXXFLAGS = -DPLUGINDIR=\"$(plugindir)\" -DKDEPIM_LIBDIR=\"$(plugindir)\" -Wall -Werror
kdepim_lib_la_LDFLAGS = $(KDE_LDFLAGS) -avoid-version -export-dynamic -module
kdepim_lib_la_LIBADD = $(KDE_RPATH) $(LIB_KABC) $(LIB_KCAL) -L$(libdir)  -L@OPENSYNC_LIBDIR@ -lopensync-xml

src/KNotesIface_stub.cpp:
	dcopidl src/KNotesIface.h > src/KNotesIface.kidl
	dcopidl2cpp src/KNotesIface.kidl
