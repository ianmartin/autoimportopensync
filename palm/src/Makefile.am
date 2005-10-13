plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

AM_CFLAGS = @PACKAGE_CFLAGS@ -Wall -Werror

EXTRA_DIST = \
	palm-sync

config_DATA = palm-sync

opensyncheader_HEADERS = palm_sync.h palm_format.h

formats_LTLIBRARIES = palm.la

palm_la_SOURCES = palm_format.c
palm_la_LDFLAGS = -avoid-version -export-dynamic -module
palm_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

plugin_LTLIBRARIES = palm_sync.la

palm_sync_la_SOURCES = palm_sync.c
palm_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
palm_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
