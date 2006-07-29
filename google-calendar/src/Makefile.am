plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall

EXTRA_DIST = google-calendar.cfg

config_DATA = google-calendar

plugin_LTLIBRARIES = gcalendar.la

bin_PROGRAMS = gchelper.py

gchelper_py_SOURCES = gchelper.py

gcalendar_la_SOURCES = gcalendar.c
gcalendar_la_LDFLAGS = -avoid-version -export-dynamic -module
gcalendar_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ -lopensync-xml

google-calendar: $(srcdir)/google-calendar.cfg
	cp $(srcdir)/google-calendar.cfg google-calendar
