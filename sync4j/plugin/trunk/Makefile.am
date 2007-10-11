#
# Copyright (C) 2006 Michael Kolmodin
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

SUBDIRS = src tests

EXTRA_DIST = \
	autogen.sh syncml-client-plugin.spec.in syncml-client-plugin.spec
	
## The distclean files are just there so we can ship the example correctly. Remove these ##
DISTCLEANFILES = \
	aclocal.m4 \
	config.status \
	Makefile.in \
	stamp-h1 \
	config.h \
	configure \
	config.h.in \
	Makefile \
	config.log \
	src/Makefile \
	src/Makefile.in \
	tests/Makefile \
	tests/Makefile.in

distclean-local:
	rm -rf autom4te.cache

rpm: dist
	rpmbuild -ta @PACKAGE@-@VERSION@.tar.gz
