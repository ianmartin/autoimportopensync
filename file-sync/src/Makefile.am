plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall

EXTRA_DIST = \
	file-sync \
	file_sync.h

config_DATA = file-sync

plugin_LTLIBRARIES = file_sync.la

file_sync_la_SOURCES = file_sync.c file_sync_conf.c
file_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
file_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ @FAM_LIBS@
