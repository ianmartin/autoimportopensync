plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -Werror

config_DATA = evo2-sync

opensyncheader_HEADERS = evo2_sync.h

plugin_LTLIBRARIES = evo2_sync.la

evo2_sync_la_SOURCES = \
	evolution_sync.c \
	evolution_ebook.c \
	evolution_ecal.c \
	evolution_etodo.c \
	evolution_xml.c

evo2_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
evo2_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
