## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall @XML_CFLAGS@ -Werror @GCOV_CFLAGS@ -DOPENSYNC_PLUGINDIR=\"@OPENSYNC_PLUGINDIR@\" -DOPENSYNC_CONFIGDIR=\"@OPENSYNC_CONFIGDIR@\" -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\"

INCLUDES = @PACKAGE_CFLAGS@

lib_LTLIBRARIES = libopensync.la

opensyncincludedir = @OPENSYNC_HEADERDIR@

opensyncinclude_HEADERS = \
	opensync.h \
	opensync_env.h \
	opensync_plugin.h \
	opensync_group.h \
	opensync_member.h \
	opensync_change.h \
	opensync_error.h \
	opensync_hashtable.h \
	opensync_context.h \
	opensync_convert.h \
	opensync_convreg.h \
	opensync_anchor.h \
	opensync_filter.h \
	opensync_time.h \
	opensync_xml.h \
	opensync_debug.h \
	opensync_changecmds.h \
	opensync_serializer.h

EXTRA_DIST = \
	opensync_internals.h \
	opensync_anchor.h \
	opensync_db_bdb.h \
	opensync_db_internals.h \
	opensync_format_internals.h \
	opensync_member_internals.h \
	opensync_message_internals.h \
	opensync_plugin_internals.h \
	opensync_queue_internals.h \
	opensync_env_internals.h \
	opensync_error_internals.h \
	opensync_filter_internals.h \
	opensync_change_internals.h \
	opensync_context_internals.h \
	opensync_group_internals.h \
	opensync_hashtable_internals.h \
	opensync_support.h \
	opensync_user_internals.h

libopensync_la_SOURCES = \
	opensync_debug.c \
	opensync_env.c \
	opensync_db.c \
	opensync_plugin.c \
	opensync_group.c \
	opensync_user.c \
	opensync_member.c \
	opensync_change.c \
	opensync_hashtable.c \
	opensync_error.c \
	opensync_context.c \
	opensync_convert.c \
	opensync_changecmds.c \
	opensync_convreg.c \
	opensync_anchor.c \
	opensync_time.c \
	opensync_xml.c \
	opensync_filter.c \
	opensync_queue.c \
	opensync_message.c \
	opensync_serializer.c

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da

libopensync_la_LDFLAGS = @PACKAGE_LIBS@ @GCOV_LDFLAGS@
libopensync_la_LIBADD = @XML_LIBS@
