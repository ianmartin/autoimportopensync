## Process this file with automake to produce Makefile.in ../dbus/glib/libdbus-glib-1.la
# not a nice way to do it
libdir=/home/azrael/test/plugins

INCLUDES = @PACKAGE_CFLAGS@

lib_LTLIBRARIES = evo2_sync.la

evo2_sync_la_SOURCES = \
	evolution_sync.c \
	evolution_ebook.c \
	evolution_ecal.c \
	evolution_etodo.c \
	xml.c

evo2_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
evo2_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ -lopensync

noinst_PROGRAMS = evo_test
evo_test_SOURCES = evo_test.c
evo_test_LDADD = @PACKAGE_LIBS@ @LIBS@ -lopensync
