## Process this file with automake to produce Makefile.in
# not a nice way to do it
plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

SUBDIRS = bfb

AM_CFLAGS = @PACKAGE_CFLAGS@ -Wall

if BLUETOOTH
bluetooth_lib = libirmc_bluetooth.la
endif

plugin_LTLIBRARIES = libirmc_sync.la $(bluetooth_lib)

libirmc_sync_la_SOURCES = irmc_obex.c irmc_obex.h irmc_sync.c irmc_sync.h \
                          irmc_irda.c irmc_irda.h cobex_bfb.c cobex_bfb.h
libirmc_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
libirmc_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ -lopenobex -lpthread bfb/libbfb.la

libirmc_bluetooth_la_SOURCES = \
	irmc_bluetooth.c irmc_bluetooth.h

if SDP
sdp_lib = -lsdp
endif

libirmc_bluetooth_la_LIBADD = -lbluetooth -lpthread $(sdp_lib)


