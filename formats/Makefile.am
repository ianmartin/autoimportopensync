## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Werror -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\" -Wall

formatsdir=@OPENSYNC_FORMATSDIR@

INCLUDES = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src

formats_LTLIBRARIES = data.la event.la todo.la contact.la note.la

data_la_SOURCES = data.c
data_la_LDFLAGS = -avoid-version -export-dynamic -module
data_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

event_la_SOURCES = event.c
event_la_LDFLAGS = -avoid-version -export-dynamic -module
event_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

note_la_SOURCES = note.c
note_la_LDFLAGS = -avoid-version -export-dynamic -module
note_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

todo_la_SOURCES = todo.c
todo_la_LDFLAGS = -avoid-version -export-dynamic -module
todo_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

contact_la_SOURCES = contact.c
contact_la_LDFLAGS = -avoid-version -export-dynamic -module
contact_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

SUBDIRS = xml
