## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror

INCLUDES = @PACKAGE_CFLAGS@ @XMLCFLAGS@

bin_PROGRAMS = osyncdump

osyncdump_SOURCES = osyncdump.c
osyncdump_LDFLAGS = @PACKAGE_LIBS@ @XMLLDFLAGS@ -ldb-4.2
osyncdump_LDADD = $(top_srcdir)/src/libopensync.la
