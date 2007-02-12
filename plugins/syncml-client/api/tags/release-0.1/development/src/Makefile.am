AUTOMAKE_OPTIONS              = gnu

AM_CPPFLAGS                   = -I./include @SYNCML_CFLAGSS@ -gdwarf-2 -O0 -g3
libsyncmlclient               = @SYNCML_LIBS@ -version-info 0:0:0

lib_LTLIBRARIES               = libsyncmlclient.la
libsyncmlclient_la_SOURCES    = 

SUBDIRS                       = c++/common                      \
                                c++/http/common                 \
                                c++/http/linux                  \
                                c++/ptypes/                     \
                                c++/spdm/common                 \
                                c++/spdm/gconf                  \
                                c++/spds/common                 \
                                c++/examples/common             \
                                include

libsyncmlclient_la_LIBADD     =  \
                                c++/common/libcommon.la         \
                                c++/http/common/libhttp.la      \
                                c++/http/linux/libhttplinux.la  \
                                c++/ptypes/libptypes.la         \
                                c++/spdm/common/libspdm.la      \
                                c++/spdm/gconf/libspdmgconf.la  \
                                c++/spds/common/libspds.la

EXTRA_DIST                    = syncml-client.pc.in             \
                                syncml-client.spec              \
                                syncml-client.spec.in

configdir                     = $(datadir)/@PACKAGE@
dist_config_DATA              = gconf-schema.xml

pkgconfigdir                  = $(libdir)/pkgconfig
pkgconfig_DATA                = syncml-client.pc

#
#  RPM
#

rpm:	dist
	rpmbuild -ta @PACKAGE@-@VERSION@.tar.gz

#
#    Gconf install/uninstall
#
install-data-local:
	env GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source` \
	gconftool-2 --makefile-install-rule gconf-schema.xml

install-gconf-schema: install-data-local

uninstall-gconf-schema:
	env GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source` \
	gconftool-2 --makefile-uninstall-rule gconf-schema.xml
