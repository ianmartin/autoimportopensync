## Process this file with automake to produce Makefile.in

SUBDIRS = module data ipc plugin format helper

AM_CFLAGS = -Wall @XML_CFLAGS@ -Werror @GCOV_CFLAGS@ -DOPENSYNC_CONFIGDIR=\"@OPENSYNC_CONFIGDIR@\"

INCLUDES = @PACKAGE_CFLAGS@

lib_LTLIBRARIES = libopensync.la

opensyncincludedir = @OPENSYNC_HEADERDIR@

opensyncinclude_HEADERS = \
	opensync.h \
	opensync-data.h \
	opensync-plugin.h \
	opensync-format.h \
	opensync-error.h \
	opensync-support.h \
	opensync-context.h \
	opensync-db.h

EXTRA_DIST = \
	opensync_internals.h \
	opensync_error_internals.h \
	opensync_support_internals.h \
	opensync_context_internals.h \
	opensync_db_internals.h

libopensync_la_SOURCES = \
	opensync_support.c \
	opensync_error.c \
	opensync_context.c \
	opensync_db.c
	
CLEANFILES = \
	*.bb \
	*.bbg \
	*.da

libopensync_la_LDFLAGS = @PACKAGE_LIBS@ @GCOV_LDFLAGS@
libopensync_la_LIBADD = @XML_LIBS@ \
	module/libopensync-module.la \
	data/libopensync-data.la \
	format/libopensync-format.la \
	ipc/libopensync-ipc.la \
	plugin/libopensync-plugin.la \
	helper/libopensync-helper.la
