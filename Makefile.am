plugindir=@OPENSYNC_LIBDIR@/plugins
kdepimlibdir=@OPENSYNC_LIBDIR@/kdepim
# formatdir=@OPENSYNC_LIBDIR@/formats

#FIXME: set EXTRA_DIST

INCLUDES = $(KDE_INCLUDES) $(QT_INCLUDES)

#FIXME: Is there a KDE macro to set this?
LIB_KCAL = -lkcal


plugin_LTLIBRARIES = kdepim_sync.la 
kdepimlib_LTLIBRARIES = kdepim_impl.la
# format_LTLIBRARIES = kcal_format.la


# kcal_format_la_SOURCES = src/kcal_format.cpp
# kcal_format_la_LDFLAGS = -avoid-version -export-dynamic -module
# kcal_format_la_LIBADD = @PACKAGE_LIBS@ $(KDE_RPATH) $(LIB_KABC)

kdepim_sync_la_SOURCES = src/kdepim_sync.cpp
kdepim_sync_la_CXXFLAGS = -DPLUGINDIR=\"$(plugindir)\" -DKDEPIM_LIBDIR=\"$(kdepimlibdir)\"
kdepim_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
kdepim_sync_la_LIBADD = @PACKAGE_LIBS@

kdepim_impl_la_SOURCES = src/kdepim_impl.cpp \
                         src/kcal.cpp
kdepim_impl_la_CXXFLAGS = -O0 -DPLUGINDIR=\"$(plugindir)\" -DKDEPIM_LIBDIR=\"$(kdepimlibdir)\"
kdepim_impl_la_LDFLAGS = -avoid-version -export-dynamic -module
kdepim_impl_la_LIBADD = @PACKAGE_LIBS@ $(KDE_RPATH) $(LIB_KABC) $(LIB_KCAL)


