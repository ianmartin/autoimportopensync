%define         version    0.1
%define         pkg        syncml-client
%define         configdir  %{_datadir}/syncml-client

Summary:   	A SyncML API
Name: 	   	%{pkg}
Version:   	%{version}
Release:   	trunk
License:   	GPL
Group:     	Applications/Communications
URL:            http://www.sync4j.org      

Packager:   	Michael Kolmodin <michael@kolmodin.net>
Vendor:     	Michael Kolmodin <michael@kolmodin.net>

Source:     	%{name}-%{version}.tar.gz
BuildRoot:  	%{_tmppath}/root-%{name}-%{version}
Prefix:     	%{_prefix}


#
#  Base Fedora 
#
BuildRequires:  GConf2-devel, curl-devel
Requires:       curl, GConf2


%description
An API to access SyncML servers, based on a port from sync4j.org

%package devel
Summary:        Development tools for programs which will use the syncml lib
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}

%description devel
The syncml-client-devel package includes the header files necessary for
developing programs which use the syncml-client library.


%prep
%setup
autoreconf -sfi

%build
%configure --prefix=/usr 
%{__make} %{?_smp_mflags}

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -fr $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
%makeinstall 

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -fr $RPM_BUILD_ROOT

%post
/sbin/ldconfig
env GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source` \
      gconftool-2 --makefile-install-rule %{configdir}/gconf-schema.xml

%preun
env GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source` \
      gconftool-2 --makefile-uninstall-rule  %{configdir}/gconf-schema.xml

%postun
/sbin/ldconfig



%files
%defattr(-, root, root, 0755)
%doc AUTHORS ChangeLog COPYING NEWS README
%attr(755,root,root) %{_libdir}/libsyncmlclient*.so*
%{configdir}/gconf-schema.xml

%files devel
%defattr(644,root,root,755)
%{_libdir}/lib*.la
%{_libdir}/lib*.a
%{_libdir}/pkgconfig/syncml-client.pc
%{_includedir}/syncml-client


%changelog
* Sun Jan 30 2006 Michael Kolmodin <michael@kolmodin.net>
  - Initial attempt to create a spec file
