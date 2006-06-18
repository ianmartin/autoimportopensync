## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror @GCOV_CFLAGS@ -DOPENSYNC_PLUGINDIR=\"@OPENSYNC_PLUGINDIR@\"

INCLUDES = @PACKAGE_CFLAGS@ -I..

noinst_LTLIBRARIES = libopensync-plugin.la

opensyncincludedir = @OPENSYNC_HEADERDIR@/plugin

opensyncinclude_HEADERS = \
	opensync_plugin.h \
	opensync_sink.h \
	opensync_plugin_env.h \
	opensync_plugin_info.h

EXTRA_DIST = \
	opensync_plugin_internals.h \
	opensync_sink_internals.h \
	opensync_plugin_env_internals.h \
	opensync_plugin_info_internals.h

libopensync_plugin_la_SOURCES = opensync_plugin.c opensync_plugin_env.c opensync_plugin_info.c opensync_sink.c
libopensync_plugin_la_LDFLAGS = @PACKAGE_LIBS@ @GCOV_LDFLAGS@

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da
