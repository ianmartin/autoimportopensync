noinst_LTLIBRARIES = libspds.la

AM_CPPFLAGS          = -gdwarf-2 -O0 -g3

libspds_la_SOURCES = \
    Config.cpp          SyncItemStatusHolder.cpp  SyncMLProcessor.cpp     \
    SPDSUtils.cpp       SyncManager.cpp           SyncSourceConfig.cpp    \
    SyncItem.cpp        SyncManagerFactory.cpp    SyncSource.cpp          \
    SyncItemHolder.cpp  SyncMap.cpp                                       \
    SyncItemStatus.cpp  SyncMLBuilder.cpp

libspds = @SYNCML_LIBS@ -version-info 0:0:0

INCLUDES  = -I@top_srcdir@/include  @SYNCML_CFLAGS@

