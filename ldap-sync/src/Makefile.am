plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall

EXTRA_DIST = ldap_plugin

config_DATA = ldap-sync

plugin_LTLIBRARIES = ldap_plugin.la

ldap_plugin_la_SOURCES = md5_crypt.c xml_convert.c ldap_connect.c ldap_plugin.c
ldap_plugin_la_CFLAGS = 
ldap_plugin_la_LDFLAGS = -avoid-version -export-dynamic -module
ldap_plugin_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ -lopensync -lldap -lgcrypt

