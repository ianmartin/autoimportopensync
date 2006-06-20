## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror @GCOV_CFLAGS@ 

INCLUDES = @PACKAGE_CFLAGS@ @XML_CFLAGS@ -I..

noinst_LTLIBRARIES = libopensync-merger.la

opensyncincludedir = @OPENSYNC_HEADERDIR@/merger

opensyncinclude_HEADERS = \
	opensync_capabilities.h \
	opensync_capability.h \
	opensync_xmlformat.h \
	opensync_xmlfield.h \
	opensync_xmlfieldlist.h

EXTRA_DIST = \
	opensync_capabilities_internals.h \
	opensync_capability_internals.h \
	opensync_xmlformat_internals.h \
	opensync_xmlfield_internals.h \
	opensync_xmlfieldlist_internals.h

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da

libopensync_merger_la_SOURCES = opensync_capabilities.c opensync_capability.c opensync_xmlformat.c opensync_xmlfield.c opensync_xmlfieldlist.c
libopensync_merger_la_LDFLAGS = @PACKAGE_LIBS@ @GCOV_LDFLAGS@
