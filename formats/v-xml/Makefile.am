## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Werror -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\" -Wall @XML_CFLAGS@ 

formatsdir=@OPENSYNC_FORMATSDIR@

EXTRA_DIST = e-vcard.h

INCLUDES = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src -I../

formats_LTLIBRARIES = xml-vcard.la xml-evolution.la

xml_vcard_la_SOURCES = xml-vcard.c e-vcard.c
xml_vcard_la_LDFLAGS = -avoid-version -export-dynamic -module
xml_vcard_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

xml_evolution_la_SOURCES = xml-evolution.c
xml_evolution_la_LDFLAGS = -avoid-version -export-dynamic -module
xml_evolution_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
