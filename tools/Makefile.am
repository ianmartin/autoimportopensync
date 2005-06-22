## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror @GCOV_CFLAGS@

INCLUDES = -I$(top_srcdir) @PACKAGE_CFLAGS@ @XML_CFLAGS@ -I$(top_srcdir)/osengine

if ENABLE_TOOLS
bin_PROGRAMS = osyncdump osyncstress osyncplugin osynctest
endif

osyncdump_SOURCES = osyncdump.c
osyncdump_LDFLAGS = @PACKAGE_LIBS@ @XML_LIBS@ -R $(libdir) @GCOV_LDFLAGS@
osyncdump_LDADD = $(top_builddir)/opensync/libopensync.la

osyncstress_SOURCES = osyncstress.c
osyncstress_LDFLAGS = @PACKAGE_LIBS@ -R $(libdir) @GCOV_LDFLAGS@
osyncstress_LDADD = $(top_builddir)/osengine/libosengine.la $(top_builddir)/opensync/libopensync.la

osyncplugin_SOURCES = osyncplugin.c
osyncplugin_LDFLAGS = @PACKAGE_LIBS@ -R $(libdir) @GCOV_LDFLAGS@
osyncplugin_LDADD = $(top_builddir)/opensync/libopensync.la

osynctest_SOURCES = osynctest.c
osynctest_LDFLAGS = @PACKAGE_LIBS@ -R $(libdir) @GCOV_LDFLAGS@
osynctest_LDADD = $(top_builddir)/osengine/libosengine.la $(top_builddir)/opensync/libopensync.la

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da
