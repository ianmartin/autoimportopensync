## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror -I$(top_srcdir)

INCLUDES = @PACKAGE_CFLAGS@ @XMLCFLAGS@

bin_PROGRAMS = osyncdump

osyncdump_SOURCES = osyncdump.c
osyncdump_LDFLAGS = @PACKAGE_LIBS@ @XMLLDFLAGS@
osyncdump_LDADD = $(top_builddir)/src/libopensync.la
