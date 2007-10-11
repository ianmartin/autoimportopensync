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

%define                 version    0.01
%define                 pkg        syncml-client-plugin

Summary:                A SyncML client opensync plugin
Name:                   %{pkg}
Version:                %{version}
Release:                trunk

License:		GPL
Group:			Development/Libraries
Source:			%{name}-%{version}.tar.gz
BuildRoot:		%{_tmppath}/%{name}-%{version}
Requires:		glib2 libopensync  syncml-client
BuildRequires:	        glib2-devel libopensync-devel syncml-client-devel

%description
This plugin allows applications using OpenSync to synchronise to and from
SyncML servers. It's build upon the API from http://www.funambol.org.

%prep
%setup -q

%build
%configure
make

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install
rm -f %{buildroot}%{_libdir}/opensync/plugins/*.la

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_libdir}/opensync/plugins/syncml_client_plugin.so
%{_datadir}/opensync/defaults/syncml-client

%changelog
* Sun Mar 26 2006 Michael Kolmodin <mk[at]kolmodin.net>
- Shamelessly stole it, adapted for syncml-client-plugin.

* Mon Sep 26 2005 Pierre Ossman <drzeus@drzeus.cx> 0.18-1
- Removed file format.

* Fri Apr  8 2005 Pierre Ossman <drzeus@drzeus.cx> 0.15-1
- Initial package
