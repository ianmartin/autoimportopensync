plugindir=@OPENSYNC_PLUGINDIR@/plugins
configdir=@OPENSYNC_CONFIGDIR@/defaults

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\" -Werror

config_DATA = evo2-sync

lib_LTLIBRARIES = evo2_sync.la

evo2_sync_la_SOURCES = \
	evolution_sync.c \
	evolution_ebook.c \
	evolution_ecal.c \
	evolution_etodo.c \
	evolution_xml.c

evo2_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
evo2_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
