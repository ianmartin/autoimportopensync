plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\" -Werror

EXTRA_DIST = evo-sync

config_DATA = evo-sync

opensyncheader_HEADERS = \
	evo_sync.h

plugin_LTLIBRARIES = evo_sync.la

evo_sync_la_SOURCES = \
	evolution_sync.c \
	evolution_ebook.c \
	evolution_xml.c
	
#	evolution_ecal.c
#	evolution_etodo.c

evo_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
evo_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ -lpthread