plugindir=@OPENSYNC_PLUGINDIR@/plugins
configdir=@OPENSYNC_CONFIGDIR@/defaults
headerdir=$(includedir)/opensync/
INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\" -Werror

#FIXME: the 'evo2-sync' file is missing from subversion
config_DATA = evo2-sync

header_HEADERS = \
	evo2_sync.h

plugin_LTLIBRARIES = evo2_sync.la

evo2_sync_la_SOURCES = \
	evolution_sync.c \
	evolution_ebook.c \
	evolution_ecal.c \
	evolution_etodo.c \
	evolution_xml.c

evo2_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
evo2_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
