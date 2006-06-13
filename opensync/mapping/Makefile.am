## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror @GCOV_CFLAGS@

INCLUDES = @PACKAGE_CFLAGS@

noinst_LTLIBRARIES = libopensync-mapping.la

opensyncincludedir = @OPENSYNC_HEADERDIR@

opensyncinclude_HEADERS = \
	opensync_mapping.h

EXTRA_DIST = \
	opensync_mapping_internals.h

libopensync_mapping_la_SOURCES = opensync_mapping.c
libopensync_mapping_la_LDFLAGS = @PACKAGE_LIBS@ @GCOV_LDFLAGS@

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da
