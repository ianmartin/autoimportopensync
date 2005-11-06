INCLUDES = @PACKAGE_CFLAGS@ -I$(top_srcdir)

AM_CFLAGS = -Wall -Werror @XML_CFLAGS@ @GCOV_CFLAGS@
AM_LDFLAGS = -rpath @libdir@

EXTRA_DIST = mock_sync.h Makefile.in

if ENABLE_TESTS
noinst_LTLIBRARIES = mock_sync.la mockformat.la
endif

mock_sync_la_SOURCES = mock_sync.c
mock_sync_la_LDFLAGS = -avoid-version -export-dynamic -module @GCOV_LDFLAGS@
mock_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ @XML_LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la

mockformat_la_SOURCES = mock_format.c
mockformat_la_LDFLAGS = -avoid-version -export-dynamic -module @GCOV_LDFLAGS@
mockformat_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la
