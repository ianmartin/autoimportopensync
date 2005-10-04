SUBDIRS = bfb

plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall @PACKAGE_CFLAGS@ -DHAVE_BT_OBEX

EXTRA_DIST = irmc-sync irmc_sync.h

config_DATA = irmc-sync

if BLUETOOTH
bluetooth_lib = libirmc_bluetooth.la
endif

if SDP
sdp_lib = -lsdp
endif

noinst_LTLIBRARIES = libirmc_bluetooth.la
libirmc_bluetooth_la_SOURCES = irmc_bluetooth.c irmc_bluetooth.h
libirmc_bluetooth_la_LIBADD = -lbluetooth -lpthread $(sdp_lib)

opensyncheader_HEADER = irmc_sync.h

plugin_LTLIBRARIES = irmc_sync.la

irmc_sync_la_SOURCES = cobex_bfb.c cobex_bfb.h \
	irmc_irda.c irmc_irda.h \
	irmc_obex.c irmc_obex.h \
	irmc_sync.c irmc_sync.h \
	sync_vtype.c sync_vtype.h
irmc_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
irmc_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ -lopenobex -lpthread bfb/libbfb.la $(bluetooth_lib)
