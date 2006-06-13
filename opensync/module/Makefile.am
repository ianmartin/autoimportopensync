## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror @GCOV_CFLAGS@ -DOPENSYNC_PLUGINVERSION=1

INCLUDES = @PACKAGE_CFLAGS@ -I..

noinst_LTLIBRARIES = libopensync-module.la

EXTRA_DIST = \
	opensync_module_internals.h

libopensync_module_la_SOURCES = opensync_module.c
libopensync_module_la_LDFLAGS = @PACKAGE_LIBS@ @GCOV_LDFLAGS@

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da
