plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall

EXTRA_DIST = jescs

config_DATA = jescs-sync

plugin_LTLIBRARIES = jescs.la

libexec_PROGRAMS = wcaptool

wcaptool_SOURCES = wcaptool

jescs_la_SOURCES = jescs.c
jescs_la_CFLAGS = -DWCAPTOOL=\"$(libexecdir)/wcaptool\"
jescs_la_LDFLAGS = -avoid-version -export-dynamic -module
jescs_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ -lopensync-xml

