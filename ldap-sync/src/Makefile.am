plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall

EXTRA_DIST = ldap-sync \
	    ldap_connect.h \
	    ldap_plugin.h \
	    md5_crypt.h \
	    xml_convert.h

config_DATA = ldap-sync

plugin_LTLIBRARIES = ldap_plugin.la

ldap_plugin_la_SOURCES = md5_crypt.c xml_convert.c ldap_connect.c ldap_plugin.c
ldap_plugin_la_CFLAGS = @PACKAGE_CFLAGS@
ldap_plugin_la_LDFLAGS = -avoid-version -export-dynamic -module
ldap_plugin_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ @LIBGCRYPT_LIBS@

