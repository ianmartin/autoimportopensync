## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror @GCOV_CFLAGS@

INCLUDES = @PACKAGE_CFLAGS@ -I..

noinst_LTLIBRARIES = libopensync-helper.la

opensyncincludedir = @OPENSYNC_HEADERDIR@

opensyncinclude_HEADERS = \
	opensync_anchor.h \
	opensync_hashtable.h

EXTRA_DIST = \
	opensync_anchor_internals.h \
	opensync_hashtable_internals.h

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da

libopensync_helper_la_SOURCES = opensync_anchor.c opensync_hashtable.c
libopensync_helper_la_LDFLAGS = @PACKAGE_LIBS@ @GCOV_LDFLAGS@
