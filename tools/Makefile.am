## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror -g -O0

INCLUDES = -I$(top_srcdir) @PACKAGE_CFLAGS@ @XML_CFLAGS@ -I$(top_srcdir)/osengine

bin_PROGRAMS = osyncdump osyncstress osyncplugin

osyncdump_SOURCES = osyncdump.c
osyncdump_LDFLAGS = @PACKAGE_LIBS@ @XML_LIBS@ -R $(libdir)
osyncdump_LDADD = $(top_builddir)/opensync/libopensync.la

osyncstress_SOURCES = osyncstress.c
osyncstress_LDFLAGS = @PACKAGE_LIBS@ -R $(libdir)
osyncstress_LDADD = $(top_builddir)/osengine/libosengine.la $(top_builddir)/opensync/libopensync.la

osyncplugin_SOURCES = osyncplugin.c
osyncplugin_LDFLAGS = @PACKAGE_LIBS@ -R $(libdir)
osyncplugin_LDADD = $(top_builddir)/opensync/libopensync.la