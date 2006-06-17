## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror @GCOV_CFLAGS@

INCLUDES = @PACKAGE_CFLAGS@ -I..

noinst_LTLIBRARIES = libopensync-ipc.la

opensyncincludedir = @OPENSYNC_HEADERDIR@/ipc

opensyncinclude_HEADERS = \
	opensync_message.h \
	opensync_queue.h \
	opensync_serializer.h

EXTRA_DIST = \
	opensync_message_internals.h \
	opensync_queue_internals.h

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da
	
libopensync_ipc_la_SOURCES = opensync_message.c opensync_queue.c opensync_serializer.c
libopensync_ipc_la_LDFLAGS = @PACKAGE_LIBS@ @GCOV_LDFLAGS@
