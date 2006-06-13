## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror @GCOV_CFLAGS@

INCLUDES = @PACKAGE_CFLAGS@

noinst_LTLIBRARIES = libopensync-group.la

opensyncincludedir = @OPENSYNC_HEADERDIR@

opensyncinclude_HEADERS = \
	opensync-group.h \
	opensync_group_env.h \
	opensync_member.h

EXTRA_DIST = \
	opensync_group_internals.h \
	opensync_group_env_internals.h \
	opensync_member_internals.h

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da

libopensync_group_la_SOURCES = opensync_group.c opensync_group_env.c opensync_member.c
libopensync_group_la_LDFLAGS = @PACKAGE_LIBS@ @GCOV_LDFLAGS@
