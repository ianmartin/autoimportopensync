## Process this file with automake to produce Makefile.in ../dbus/glib/libdbus-glib-1.la
# not a nice way to do it
plugindir=@OPENSYNC_PLUGINDIR@/plugins

configdir=@OPENSYNC_CONFIGDIR@/defaults

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\" -Werror

config_DATA = file-sync

plugin_LTLIBRARIES = file_sync.la

file_sync_la_SOURCES = file_sync.c
file_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
file_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ @FAM_LIBS@

formatsdir=@OPENSYNC_FORMATSDIR@
formats_LTLIBRARIES = file.la

file_la_SOURCES = file.c
file_la_LDFLAGS = -avoid-version -export-dynamic -module
file_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
