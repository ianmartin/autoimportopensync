#FIXME kdepimlibdir=@OPENSYNC_LIBDIR@/kdepim
plugindir=@OPENSYNC_PLUGINDIR@
#configdir=@OPENSYNC_CONFIGDIR@

#FIXME: set EXTRA_DIST

INCLUDES = @PACKAGE_CFLAGS@ $(KDE_INCLUDES) $(QT_INCLUDES)

#FIXME: Is there a KDE macro to set this?
LIB_KCAL = -lkcal

plugin_LTLIBRARIES = kdepim_sync.la kdepim_lib.la
# format_LTLIBRARIES = kcal_format.la

EXTRA_DIST = \
			src/kaddrbook.h \
			src/kcal.h \
			src/osyncbase.h

# kcal_format_la_SOURCES = src/kcal_format.cpp
# kcal_format_la_LDFLAGS = -avoid-version -export-dynamic -module
# kcal_format_la_LIBADD = $(KDE_RPATH) $(LIB_KABC)

kdepim_sync_la_SOURCES = src/kdepim_sync.cpp
kdepim_sync_la_CXXFLAGS = -DPLUGINDIR=\"$(plugindir)\" -DKDEPIM_LIBDIR=\"$(plugindir)\"
kdepim_sync_la_LDFLAGS = -avoid-version -export-dynamic -module

# ugly hack to make the c++ library work
kdepim_sync_la_LIBADD = -lstdc++

kdepim_lib_la_SOURCES = src/kdepim_impl.cpp \
                         src/kcal.cpp \
                         src/knotes.cpp
kdepim_lib_la_CXXFLAGS = -DPLUGINDIR=\"$(plugindir)\" -DKDEPIM_LIBDIR=\"$(plugindir)\"
kdepim_lib_la_LDFLAGS = $(KDE_LDFLAGS) -avoid-version -export-dynamic -module
kdepim_lib_la_LIBADD = $(KDE_RPATH) $(LIB_KABC) $(LIB_KCAL)


