
plugindir=$(OPENSYNC_PLUGINDIR)


AM_CPPFLAGS = $(SYNCML_CFLAGS) $(OPENSYNC_CFLAGS)

plugin_LTLIBRARIES = syncml.la

syncml_la_SOURCES = src/syncml_plugin.c
syncml_la_LDFLAGS = -avoid-version -export-dynamic -module

