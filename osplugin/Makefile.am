## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall @XML_CFLAGS@ -Werror @GCOV_CFLAGS@ -DOPENSYNC_PLUGINDIR=\"@OPENSYNC_PLUGINDIR@\" -DOPENSYNC_CONFIGDIR=\"@OPENSYNC_CONFIGDIR@\" -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\"

INCLUDES = -I$(top_srcdir) @PACKAGE_CFLAGS@ @XML_CFLAGS@ -I$(top_srcdir)/osengine -I$(top_srcdir)/opensync

libexec_PROGRAMS = osplugin

ospluginincludedir = @OPENSYNC_HEADERDIR@

osplugin_SOURCES = osplugin.c
osplugin_LDFLAGS = @PACKAGE_LIBS@ @XML_LIBS@ -R $(libdir) @GCOV_LDFLAGS@
osplugin_LDADD = $(top_builddir)/opensync/libopensync.la

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da
