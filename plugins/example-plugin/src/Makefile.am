plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -Werror

EXTRA_DIST = \
	<plugin-name>

config_DATA = <plugin-name>

opensyncheader_HEADER = <your-header>.h

plugin_LTLIBRARIES = <plugin-name>.la

<plugin-name>_la_SOURCES = <plugin-name>.c
<plugin-name>_la_LDFLAGS = -avoid-version -export-dynamic -module
<plugin-name>_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

## if you plan to install format libraries: ##
formats_LTLIBRARIES = <formatname>.la

<formatname>_la_SOURCES = <formatname>.c
<formatname>_la_LDFLAGS = -avoid-version -export-dynamic -module
<formatname>_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
