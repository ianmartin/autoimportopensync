noinst_LTLIBRARIES = libspdmgconf.la

AM_CPPFLAGS          = -gdwarf-2 -O0 -g3



libspdmgconf_la_SOURCES = DeviceManagerFactory.cpp  \
                          GconfDeviceManager.cpp    \
                          GconfManagementNode.cpp




libspdmgconf = @SYNCML_LIBS@ -version-info 0:0:0

INCLUDES  = -I@top_srcdir@/include  @SYNCML_CFLAGS@

