## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror @GCOV_CFLAGS@

INCLUDES = @PACKAGE_CFLAGS@ -I..

noinst_LTLIBRARIES = libopensync-data.la

opensyncincludedir = @OPENSYNC_HEADERDIR@/data

opensyncinclude_HEADERS = \
	opensync_data.h \
	opensync_change.h

EXTRA_DIST = \
	opensync_data_internals.h \
	opensync_change_internals.h

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da

libopensync_data_la_SOURCES = opensync_data.c opensync_change.c
libopensync_data_la_LDFLAGS = @PACKAGE_LIBS@ @GCOV_LDFLAGS@
