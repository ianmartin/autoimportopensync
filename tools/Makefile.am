## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror

INCLUDES = @PACKAGE_CFLAGS@ @XMLCFLAGS@ -I$(top_srcdir) -I$(top_srcdir)/src -I$(top_srcdir)/engine

bin_PROGRAMS = osyncdump osyncstress

osyncdump_SOURCES = osyncdump.c
osyncdump_LDFLAGS = @PACKAGE_LIBS@ @XMLLDFLAGS@
osyncdump_LDADD = $(top_builddir)/src/libopensync.la

osyncstress_SOURCES = osyncstress.c
osyncstress_LDFLAGS = @PACKAGE_LIBS@
osyncstress_LDADD = $(top_builddir)/engine/libosengine.la