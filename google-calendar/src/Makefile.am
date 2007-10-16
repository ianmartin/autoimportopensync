plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@
descriptiondir=@OPENSYNC_DESCRIPTIONDIR@
capabilitiesdir=@OPENSYNC_CAPABILITIESDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall

config_DATA = google-calendar
description_DATA = google-calendar.xml
capabilities_DATA = google-calendar-gcal-2005.xml

plugin_LTLIBRARIES = gcalendar.la

libexec_PROGRAMS = google-cal-helper

google_cal_helper_SOURCES = google-cal-helper.py

gcalendar_la_SOURCES = gcalendar.c
gcalendar_la_CFLAGS = -DGCAL_HELPER=\"$(libexecdir)/google-cal-helper\"
gcalendar_la_LDFLAGS = -avoid-version -export-dynamic -module
gcalendar_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

google-cal-helper: $(srcdir)/google-cal-helper.py
	cp $(srcdir)/google-cal-helper.py google-cal-helper

EXTRA_DIST = \
	google-calendar \
	google-cal-helper.py \
	$(description_DATA) \
	$(capabilities_DATA)

