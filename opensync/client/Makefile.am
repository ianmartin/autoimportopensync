## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror @GCOV_CFLAGS@

INCLUDES = @PACKAGE_CFLAGS@ -I..

noinst_LTLIBRARIES = libopensync-client.la

ospluginincludedir = @OPENSYNC_HEADERDIR@/client

EXTRA_DIST = \
	opensync_client_internals.h

libopensync_client_la_SOURCES = opensync_client.c
libopensync_client_la_LDFLAGS = @PACKAGE_LIBS@ @GCOV_LDFLAGS@

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da
