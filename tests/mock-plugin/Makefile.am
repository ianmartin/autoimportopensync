plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -Werror

EXTRA_DIST = mock_sync.h

noinst_LTLIBRARIES = mock_sync.la

mock_sync_la_SOURCES = mock_sync.c
mock_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
mock_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ @FAM_LIBS@
