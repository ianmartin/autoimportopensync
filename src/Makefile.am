plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

INCLUDES = @PACKAGE_CFLAGS@ @EPACKAGE_CFLAGS@

# Please remove '-Wno-return-type' when Evo fixes the warning in their headers.
AM_CFLAGS = -Wall -Werror -Wno-return-type

config_DATA = evo2-sync

opensyncheader_HEADERS = evo2_sync.h

plugin_LTLIBRARIES = evo2_sync.la

EXTRA_DIST = \
	evolution2_sync.h \
	evolution2_ebook.h \
	evolution2_ecal.h \
	evolution2_etodo.h \
	evolution2_xml.h \
	evo2-sync
	
evo2_sync_la_SOURCES = \
	evolution2_sync.c \
	evolution2_ebook.c \
	evolution2_ecal.c \
	evolution2_etodo.c \
	evolution2_xml.c

evo2_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
evo2_sync_la_LIBADD = @PACKAGE_LIBS@ @EPACKAGE_LIBS@ @LIBS@
