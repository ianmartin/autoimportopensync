## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall @XML_CFLAGS@ -Werror -DOPENSYNC_PLUGINDIR=\"@OPENSYNC_PLUGINDIR@\" -DOPENSYNC_CONFIGDIR=\"@OPENSYNC_CONFIGDIR@\" -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\"

INCLUDES = @PACKAGE_CFLAGS@

lib_LTLIBRARIES = libopensync.la

opensyncincludedir = $(includedir)/opensync

opensyncinclude_HEADERS = \
	opensync.h \
	opensync_env.h \
	opensync_plugin.h \
	opensync_group.h \
	opensync_member.h \
	opensync_change.h \
	opensync_mapping.h \
	opensync_error.h \
	opensync_hashtable.h \
	opensync_context.h \
	opensync_convert.h \
	opensync_anchor.h

EXTRA_DIST = \
	opensync_internals.h \
	opensync_anchor.h \
	opensync_db_bdb.h \
	opensync_db_internals.h \
	opensync_format_internals.h \
	opensync_member_internals.h \
	opensync_plugin_internals.h

libopensync_la_SOURCES = \
	opensync_debug.c \
	opensync_env.c \
	opensync_db.c \
	opensync_plugin.c \
	opensync_group.c \
	opensync_user.c \
	opensync_member.c \
	opensync_change.c \
	opensync_mapping.c \
	opensync_hashtable.c \
	opensync_error.c \
	opensync_context.c \
	opensync_convert.c \
	opensync_anchor.c
	
libopensync_la_LDFLAGS = @PACKAGE_LIBS@
libopensync_la_LIBADD = @XML_LIBS@
