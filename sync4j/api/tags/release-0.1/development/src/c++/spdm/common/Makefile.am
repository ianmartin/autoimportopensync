noinst_LTLIBRARIES = libspdm.la

AM_CPPFLAGS        = -gdwarf-2 -O0 -g3



libspdm_la_SOURCES = Base64.cpp          \
                     DeviceManager.cpp   \
                     ManagementNode.cpp  \
                     SPDMUtils.cpp

libspdm             = @SYNCML_LIBS@ -version-info 0:0:0

INCLUDES            = -I@top_srcdir@/include  @SYNCML_CFLAGS@

