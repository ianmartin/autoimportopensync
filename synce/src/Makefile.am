plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -Werror 

EXTRA_DIST = synce-plugin synce_plugin.h

config_DATA = synce-plugin

plugin_LTLIBRARIES = synce_plugin.la

synce_plugin_la_SOURCES = synce_plugin.c
synce_plugin_la_LDFLAGS = -avoid-version -export-dynamic -module
synce_plugin_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ -lrapi -lsynce -lrra

## if you plan to install format libraries: ##
##formats_LTLIBRARIES = synce-format.la

##format_la_SOURCES = format.c
##format_la_LDFLAGS = -avoid-version -export-dynamic -module
##format_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
