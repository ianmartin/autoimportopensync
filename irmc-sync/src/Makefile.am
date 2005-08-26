## Process this file with automake to produce Makefile.in
# not a nice way to do it
libdir=$(prefix)/lib/multisync

SUBDIRS = bfb

PLUGINDIR = $(libdir)
AM_CFLAGS = -DPLUGINDIR=\"$(PLUGINDIR)\"

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)/multisync"\" \
	@PACKAGE_CFLAGS@ -I$(top_srcdir)/include  \
	-I../../../include

if BLUETOOTH
bluetooth_lib = libirmc_bluetooth.la
endif

lib_LTLIBRARIES = libirmc_sync.la $(bluetooth_lib)

libirmc_bluetooth_la_SOURCES = \
	irmc_bluetooth.c irmc_bluetooth.h

if SDP
sdp_lib = -lsdp
endif

libirmc_bluetooth_la_LIBADD = -lbluetooth -lpthread $(sdp_lib)

libirmc_sync_la_SOURCES = \
	interface.c interface.h \
	callbacks.c callbacks.h \
	irmc_obex.c irmc_obex.h \
	irmc_sync.c irmc_sync.h \
	support.c support.h \
	gui.c gui.h \
	irmc_irda.c irmc_irda.h \
	cobex_bfb.c cobex_bfb.h

libirmc_sync_la_LIBADD = @PACKAGE_LIBS@ -lopenobex -lpthread bfb/libbfb.la

