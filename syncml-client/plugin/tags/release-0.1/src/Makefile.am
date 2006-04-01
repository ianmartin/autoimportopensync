plugindir                         = @OPENSYNC_PLUGINDIR@
configdir                         = @OPENSYNC_CONFIGDIR@
formatsdir                        = @OPENSYNC_FORMATSDIR@

INCLUDES                          = @PACKAGE_CFLAGS@
AM_CFLAGS                         = -Wall -Werror @PACKAGE_CFLAGS@
dist_config_DATA                  = syncml-client

plugin_LTLIBRARIES                = syncml_client_plugin.la
syncml_client_plugin_la_SOURCES   = syncml-client-plugin.cpp \
                                    SmcChangeFactory.cpp \
                                    ItemFactory.cpp \
                                    SmcSyncSource.cpp \
                                    SmcConfig.cpp
syncml_client_plugin_la_LDFLAGS   = -avoid-version -export-dynamic -module
syncml_client_plugin_la_LIBADD    = @PACKAGE_LIBS@ @LIBS@

noinst_HEADERS                    = ItemFactory.h  \
                                    SmcChangeFactory.h  \
                                    SmcConfig.h  \
                                    SmcSyncSource.h  \
                                    syncml-client-plugin.h

