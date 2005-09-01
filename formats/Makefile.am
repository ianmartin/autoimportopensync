## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Werror -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\" -Wall @XML_CFLAGS@ @GCOV_CFLAGS@

formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

opensyncheader_HEADERS = file.h

INCLUDES = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src

formats_LTLIBRARIES = data.la event.la todo.la contact.la note.la file.la

data_la_SOURCES = data.c
data_la_LDFLAGS = -avoid-version -export-dynamic -module @GCOV_LDFLAGS@
data_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

event_la_SOURCES = event.c
event_la_LDFLAGS = -avoid-version -export-dynamic -module @GCOV_LDFLAGS@
event_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

note_la_SOURCES = note.c
note_la_LDFLAGS = -avoid-version -export-dynamic -module @GCOV_LDFLAGS@
note_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

todo_la_SOURCES = todo.c
todo_la_LDFLAGS = -avoid-version -export-dynamic -module @GCOV_LDFLAGS@
todo_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

contact_la_SOURCES = contact.c
contact_la_LDFLAGS = -avoid-version -export-dynamic -module @GCOV_LDFLAGS@
contact_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

file_la_SOURCES = file.c
file_la_LDFLAGS = -avoid-version -export-dynamic -module @GCOV_LDFLAGS@
file_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

SUBDIRS = vformats-xml
