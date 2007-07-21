plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall

EXTRA_DIST = google-calendar google-cal-helper.py

config_DATA = google-calendar

plugin_LTLIBRARIES = gcalendar.la

libexec_PROGRAMS = google-cal-helper

google_cal_helper_SOURCES = google-cal-helper.py

gcalendar_la_SOURCES = gcalendar.c
gcalendar_la_CFLAGS = -DGCAL_HELPER=\"$(libexecdir)/google-cal-helper\"
gcalendar_la_LDFLAGS = -avoid-version -export-dynamic -module
gcalendar_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

google-cal-helper: $(srcdir)/google-cal-helper.py
	cp $(srcdir)/google-cal-helper.py google-cal-helper
