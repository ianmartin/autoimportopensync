plugindir=@OPENSYNC_LIBDIR@/plugins
kdepimlibdir=@OPENSYNC_LIBDIR@/kdepim


INCLUDES = $(KDE_INCLUDES) $(QT_INCLUDES)

plugin_LTLIBRARIES = kdepim_sync.la 
kdepimlib_LTLIBRARIES = kdepim_impl.la

kdepim_sync_la_SOURCES = src/kdepim_sync.cpp

kdepim_impl_la_SOURCES = src/kaddrbook.cpp

kdepim_sync_la_CXXFLAGS = -DPLUGINDIR=\"$(plugindir)\" -DKDEPIM_LIBDIR=\"$(kdepimlibdir)\"
kdepim_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
kdepim_sync_la_LIBADD = @PACKAGE_LIBS@

kdepim_impl_la_LDFLAGS = -avoid-version -export-dynamic -module
kdepim_impl_la_LIBADD = @PACKAGE_LIBS@ $(KDE_RPATH) $(LIB_KABC)
