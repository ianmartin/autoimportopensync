plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -Werror @PACKAGE_CFLAGS@

EXTRA_DIST = plugin-name

config_DATA = plugin-name

opensyncheader_HEADER = plugin.h

plugin_LTLIBRARIES = plugin.la

plugin_la_SOURCES = plugin.c
plugin_la_LDFLAGS = -avoid-version -export-dynamic -module
plugin_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

## if you plan to install format libraries: ##
formats_LTLIBRARIES = format.la

format_la_SOURCES = format.c
format_la_LDFLAGS = -avoid-version -export-dynamic -module
format_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
