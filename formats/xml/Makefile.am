## Process this file with automake to produce Makefile.in

SUBDIRS = libversit

AM_CFLAGS = -Werror -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\" -Wall @XML_CFLAGS@

formatsdir=@OPENSYNC_FORMATSDIR@

EXTRA_DIST = \
	xml.h \
	e-vcard.h

INCLUDES = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src -Ilibversit

formats_LTLIBRARIES = xml-vcard21.la xml-vcard30.la xml.la

xml_la_SOURCES = xml.c
xml_la_LDFLAGS = -avoid-version -export-dynamic -module
xml_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

xml_vcard21_la_SOURCES = xml-vcard21.c
xml_vcard21_la_LDFLAGS = -avoid-version -export-dynamic -module
xml_vcard21_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ libversit/libversit.la

xml_vcard30_la_SOURCES = xml-vcard30.c e-vcard.c
xml_vcard30_la_LDFLAGS = -avoid-version -export-dynamic -module
xml_vcard30_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

