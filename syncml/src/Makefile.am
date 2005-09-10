plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -Werror @PACKAGE_CFLAGS@

EXTRA_DIST = \
	syncml-http-server \
	syncml_plugin.h

config_DATA = syncml-http-server syncml-obex-client

opensyncheader_HEADER = syncml_plugin.h

plugin_LTLIBRARIES = syncml_plugin.la

syncml_plugin_la_SOURCES = syncml_plugin.c
syncml_plugin_la_LDFLAGS = -avoid-version -export-dynamic -module
syncml_plugin_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
