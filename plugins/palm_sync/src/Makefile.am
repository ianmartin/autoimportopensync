## Process this file with automake to produce Makefile.in ../dbus/glib/libdbus-glib-1.la
# not a nice way to do it
plugindir = $(libdir)/opensync/plugins

AM_CFLAGS = @XML_CFLAGS@

INCLUDES = @PACKAGE_CFLAGS@

plugin_LTLIBRARIES = libpalm_sync.la

libpalm_sync_la_SOURCES = \
	palm_sync.c \
	xml.c

libpalm_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
libpalm_sync_la_LIBADD = @PACKAGE_LIBS@ @XML_LIBS@ @LIBS@ -lopensync

