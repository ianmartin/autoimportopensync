NEON_LIBS = `neon-config --libs`

plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -Werror

EXTRA_DIST = \
	sunbird-sync \
	sunbird_sync.h

config_DATA = sunbird-sync

plugin_LTLIBRARIES = sunbird_sync.la

sunbird_sync_la_SOURCES = sunbird_sync.c tools.c webdav.c
sunbird_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
sunbird_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ @NEON_LIBS@
