## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Werror -Wall

INCLUDES = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src

lib_LTLIBRARIES = libosengine.la

osengineincludedir = $(includedir)/opensync
osengineinclude_HEADERS = \
	engine.h \
	osengine_mapping.h \
	osengine_engine.h \
	osengine_status.h \
	osengine_debug.h

libosengine_la_SOURCES = \
	osengine_message.c \
	osengine_queue.c \
	osengine_client.c \
	osengine_engine.c \
	osengine_status.c \
	osengine_debug.c \
	osengine_mapping.c \
	osengine_flags.c

EXTRA_DIST = \
	engine_internals.h \
	osengine_client_internals.h \
	osengine_engine_internals.h \
	osengine_flags_internals.h \
	osengine_mapping_internals.h \
	osengine_message_internals.h \
	osengine_queue_internals.h

libosengine_la_LDFLAGS = $(top_builddir)/src/libopensync.la
libosengine_la_LIBADD = $(PACKAGE_LIBS)
