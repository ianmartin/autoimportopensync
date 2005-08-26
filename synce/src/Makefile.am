plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@

AM_CFLAGS = -Wall -Werror @PACKAGE_CFLAGS@

EXTRA_DIST = synce-plugin

config_DATA = synce-plugin

plugin_LTLIBRARIES = synce_plugin.la

synce_plugin_la_SOURCES = \
	synce_plugin.c \
	synce_plugin.h \
	synce_conf.c \
	synce_file.c \
	synce_file.h

synce_plugin_la_LDFLAGS = -avoid-version -export-dynamic -module
synce_plugin_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ -lrapi -lsynce -lrra

noinst_PROGRAMS = ft

ft_SOURCES=	synce_file.c synce_file.h
ft_CFLAGS = -DTEST_FILE -Wall -Werror @PACKAGE_CFLAGS@
ft_LDADD = @PACKAGE_LIBS@ @LIBS@ -lrapi -lsynce -lrra
