INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -Werror @XML_CFLAGS@

EXTRA_DIST = mock_sync.h

lib_LTLIBRARIES = mock_sync.la mockformat.la

mock_sync_la_SOURCES = mock_sync.c
mock_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
mock_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ @XML_LIBS@

mockformat_la_SOURCES = mock_format.c
mockformat_la_LDFLAGS = -avoid-version -export-dynamic -module
mockformat_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
