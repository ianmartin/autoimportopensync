## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Werror -Wall @GCOV_CFLAGS@

INCLUDES = -I$(top_srcdir) @PACKAGE_CFLAGS@ 

if BUILD_ENGINE
lib_LTLIBRARIES = libosengine.la
endif

if BUILD_ENGINE
osengineincludedir = @OPENSYNC_ENGINEHEADERDIR@
osengineinclude_HEADERS = \
	engine.h \
	osengine_mapping.h \
	osengine_engine.h \
	osengine_status.h \
	osengine_debug.h
endif

libosengine_la_SOURCES = \
	osengine_client.c \
	osengine_engine.c \
	osengine_status.c \
	osengine_debug.c \
	osengine_mapping.c \
	osengine_mapcmds.c \
	osengine_deciders.c \
	osengine_flags.c

EXTRA_DIST = \
	engine_internals.h \
	osengine_client_internals.h \
	osengine_engine_internals.h \
	osengine_flags_internals.h \
	osengine_mapping_internals.h \
	osengine_mapcmds_internals.h \
	osengine_deciders_internals.h \
	osengine_debug_internals.h

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da

libosengine_la_LDFLAGS = $(PACKAGE_LIBS) -R $(libdir) @GCOV_LDFLAGS@
libosengine_la_LIBADD = $(top_builddir)/opensync/libopensync.la
