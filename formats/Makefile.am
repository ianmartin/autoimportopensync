## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Werror -DOPENSYNC_FORMATSDIR=\"@OPENSYNC_FORMATSDIR@\" -Wall

formatsdir=@OPENSYNC_FORMATSDIR@

INCLUDES = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src

formats_LTLIBRARIES = calendar.la todo.la contact.la

calendar_la_SOURCES = calendar.c
calendar_la_LDFLAGS = -avoid-version -export-dynamic -module
calendar_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

todo_la_SOURCES = todo.c
todo_la_LDFLAGS = -avoid-version -export-dynamic -module
todo_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

contact_la_SOURCES = contact.c
contact_la_LDFLAGS = -avoid-version -export-dynamic -module
contact_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
