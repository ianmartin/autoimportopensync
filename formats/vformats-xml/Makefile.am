## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Werror -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\" -Wall @XML_CFLAGS@ 

formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

EXTRA_DIST = \
		e-vcard.h \
		opensync-xml.h

opensyncheader_HEADERS = opensync-xml.h

INCLUDES = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src

formats_LTLIBRARIES = xml-vcard.la xml-evolution.la xml-vnote.la

lib_LTLIBRARIES =  libopensync-xml.la

xml_vcard_la_SOURCES = xml-vcard.c e-vcard.c
xml_vcard_la_LDFLAGS = -avoid-version -export-dynamic -module
xml_vcard_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ libopensync-xml.la

xml_vnote_la_SOURCES = xml-vnote.c e-vcard.c
xml_vnote_la_LDFLAGS = -avoid-version -export-dynamic -module
xml_vnote_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ libopensync-xml.la

xml_evolution_la_SOURCES = xml-evolution.c
xml_evolution_la_LDFLAGS = -avoid-version -export-dynamic -module
xml_evolution_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ libopensync-xml.la

libopensync_xml_la_SOURCES = xml-support.c
libopensync_xml_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
