## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Werror -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\" -Wall @XML_CFLAGS@  @GCOV_CFLAGS@

formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

EXTRA_DIST = \
	vformat.h \
	opensync-xml.h \
	xml-vcard.h \
	xml-vcal.h \
	xml-vnote.h

opensyncheader_HEADERS = opensync-xml.h

INCLUDES = -I$(top_srcdir) @PACKAGE_CFLAGS@

formats_LTLIBRARIES = xml-vcard.la xml-vcal.la xml-evolution.la xml-vnote.la xml-kde.la

lib_LTLIBRARIES =  libopensync-xml.la

xml_vcard_la_SOURCES = xml-vcard.c vformat.c
xml_vcard_la_LDFLAGS = -avoid-version -export-dynamic -module @GCOV_LDFLAGS@
xml_vcard_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ libopensync-xml.la

xml_vcal_la_SOURCES = xml-vcal.c vformat.c
xml_vcal_la_LDFLAGS = -avoid-version -export-dynamic -module @GCOV_LDFLAGS@
xml_vcal_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ libopensync-xml.la

xml_vnote_la_SOURCES = xml-vnote.c vformat.c
xml_vnote_la_LDFLAGS = -avoid-version -export-dynamic -module @GCOV_LDFLAGS@
xml_vnote_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ libopensync-xml.la

xml_evolution_la_SOURCES = xml-evolution.c vformat.c
xml_evolution_la_LDFLAGS = -avoid-version -export-dynamic -module @GCOV_LDFLAGS@
xml_evolution_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ libopensync-xml.la

xml_kde_la_SOURCES = xml-kde.c vformat.c
xml_kde_la_LDFLAGS = -avoid-version -export-dynamic -module @GCOV_LDFLAGS@
xml_kde_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ libopensync-xml.la

libopensync_xml_la_SOURCES = xml-support.c
libopensync_xml_la_LDFLAGS = @GCOV_LDFLAGS@
libopensync_xml_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
