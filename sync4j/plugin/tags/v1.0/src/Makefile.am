plugindir = @OPENSYNC_PLUGINDIR@
configdir = @OPENSYNC_CONFIGDIR@
formatsdir = @OPENSYNC_FORMATSDIR@
xsltdir = @OPENSYNC_SCHEMASDIR@

dist_config_DATA                  = syncml-client
dist_xslt_DATA	= xslt/sifn2xmlformat-note.xsl xslt/sift2xmlformat-todo.xsl \
				xslt/xmlformat-note2sifn.xsl xslt/xmlformat-todo2sift.xsl \
				xslt/helper.xsl


plugin_LTLIBRARIES                = syncml_client_plugin.la
syncml_client_plugin_la_CXXFLAGS  = @opensync_CFLAGS@ @glib_CFLAGS@ \
									@libxml_CFLAGS@ @sync4j_CFLAGS@
syncml_client_plugin_la_SOURCES = SmcSyncSource.cpp Sync4jClient.cpp \
	syncml-client-plugin.cpp
                                 
syncml_client_plugin_la_LDFLAGS   = -avoid-version -export-dynamic -module
syncml_client_plugin_la_LIBADD    = @opensync_LIBS@ @glib_LIBS@ @libxml_LIBS@ \
									@sync4j_LIBS@ @LIBS@

formats_LTLIBRARIES		= libSIFformat.la
libSIFformat_la_CFLAGS		= @opensync_CFLAGS@ @glib_CFLAGS@ @libxml_CFLAGS@ \
						  @libxslt_CFLAGS@
libSIFformat_la_SOURCES = SIFNformat.c SIFTformat.c SIFformat-utils.c SIFformat.c
libSIFformat_la_LDFLAGS	= -avoid-version -export-dynamic -module
libSIFformat_la_LIBADD		= @opensync_LIBS@ @glib_LIBS@ @libxml_LIBS@ \
						  @libxslt_LIBS@ @LIBS@

noinst_HEADERS = SIFNformat.h SIFTformat.h SIFformat-utils.h SIFformat.h \
	SmcSyncSource.h Sync4jClient.h syncml-client-plugin.h




