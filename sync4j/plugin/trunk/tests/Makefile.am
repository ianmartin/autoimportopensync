# Rules for the test code (use `make check` to execute)
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src 
TESTS = pluginTest
check_PROGRAMS = $(TESTS)

pluginTest_SOURCES = Sync4jClientTest.cpp \
										SmcSyncSourceTest.cpp \
										PluginTest.cpp

check-am check: test-files
.PHONY: test-files
test-files:
	@ [ -d data ] || ln -s $(top_srcdir)/tests/data .

pluginTest_CXXFLAGS = $(CPPUNIT_CFLAGS)
pluginTest_LDFLAGS = $(CPPUNIT_LIBS) -rpath '$(libdir)'
pluginTest_LDADD = ../src/syncml_client_plugin.la
