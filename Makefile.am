
plugindir=$(OPENSYNC_PLUGINDIR)


AM_CPPFLAGS = $(SYNCML_CFLAGS) $(OPENSYNC_CFLAGS) $(SYNCML_DEPS_CFLAGS)

plugin_LTLIBRARIES = syncml.la

syncml_la_SOURCES = src/syncml_plugin.c
syncml_la_LDFLAGS = -avoid-version -export-dynamic -module
syncml_la_LIBADD = $(SYNCML_LIBS) $(OPENSYNC_LIBS) $(SYNCML_DEPS_LIBS)
