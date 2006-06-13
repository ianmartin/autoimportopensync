## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror @GCOV_CFLAGS@ -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\"

INCLUDES = @PACKAGE_CFLAGS@ -I..

noinst_LTLIBRARIES = libopensync-format.la

opensyncincludedir = @OPENSYNC_HEADERDIR@

opensyncinclude_HEADERS = \
	opensync_converter.h \
	opensync_filter.h \
	opensync_format_env.h \
	opensync_sink.h \
	opensync_objformat.h

EXTRA_DIST = \
	opensync_converter_internals.h \
	opensync_filter_internals.h \
	opensync_format_env_internals.h \
	opensync_sink_internals.h \
	opensync_objformat_internals.h

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da

libopensync_format_la_SOURCES = opensync_converter.c opensync_objformat.c opensync_format_env.c opensync_filter.c opensync_sink.c
libopensync_format_la_LDFLAGS = @PACKAGE_LIBS@ @GCOV_LDFLAGS@
