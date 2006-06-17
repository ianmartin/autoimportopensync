## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror @GCOV_CFLAGS@ @XML_CFLAGS@ -DOPENSYNC_CONFIGDIR=\"@OPENSYNC_CONFIGDIR@\"

INCLUDES = @PACKAGE_CFLAGS@ -I..

noinst_LTLIBRARIES = libopensync-group.la

opensyncincludedir = @OPENSYNC_HEADERDIR@/group

opensyncinclude_HEADERS = \
	opensync_group.h \
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

libopensync_group_la_SOURCES = opensync_group_env.c opensync_xml.c opensync_group.c opensync_member.c
libopensync_group_la_LDFLAGS = @PACKAGE_LIBS@ @GCOV_LDFLAGS@ @XML_LIBS@
