## Process this file with automake to produce Makefile.in
plugindir=@OPENSYNC_PLUGINDIR@/plugins

PLUGINDIR = $(libdir)
AM_CFLAGS = -DPLUGINDIR=\"$(PLUGINDIR)\"

INCLUDES = $(KDE_INCLUDES) $(QT_INCLUDES)

plugin_LTLIBRARIES = kdepim_sync.la 

kdepim_sync_la_SOURCES = \
	kaddrbook.cpp kaddrbook.h \
	kdeplugin.c

kdepim_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
kdepim_sync_la_LIBADD = @PACKAGE_LIBS@ $(KDE_RPATH) $(LIB_KABC)

