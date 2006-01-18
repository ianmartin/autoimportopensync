plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

AM_CFLAGS = @PACKAGE_CFLAGS@ -Wall -Werror

EXTRA_DIST = \
	opie-sync

config_DATA = opie-sync

opensyncheader_HEADER = opie_sync.h

formats_LTLIBRARIES = opie.la

opie_la_SOURCES = opie_format.c
opie_la_LDFLAGS = -avoid-version -export-dynamic -module
opie_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

plugin_LTLIBRARIES = opie_sync.la

opie_sync_la_SOURCES = opie_sync.c opie_qcop.c opie_comms.c opie_xml.c
opie_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
opie_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
