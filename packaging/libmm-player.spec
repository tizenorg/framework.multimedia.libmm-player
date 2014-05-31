Name:       libmm-player
Summary:    Multimedia Framework Player Library
%if 0%{?tizen_profile_mobile}
Version:    0.2.27
Release:    0
%else
Version:    0.4.51
Release:    0
%endif
VCS:        magnolia/framework/multimedia/libmm-player#libmm-player-0.3.68-0-210-gf6b0a076a779eaddfd94fcdd0ccb5473f1c6ab39
Group:      System/Libraries
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Requires(post):  /sbin/ldconfig
Requires(postun):  /sbin/ldconfig
BuildRequires:  pkgconfig(mm-ta)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(gstreamer-0.10)
BuildRequires:  pkgconfig(gstreamer-plugins-base-0.10)
BuildRequires:  pkgconfig(gstreamer-interfaces-0.10)
BuildRequires:  pkgconfig(gstreamer-app-0.10)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(mm-session)
BuildRequires:  pkgconfig(mmutil-imgp)
BuildRequires:  pkgconfig(audio-session-mgr)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(iniparser)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(icu-i18n)

BuildRoot:  %{_tmppath}/%{name}-%{version}-build

%description

%package devel
Summary:    Multimedia Framework Player Library (DEV)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel

%prep
%setup -q

%build
%if 0%{?tizen_profile_wearable}
cd wearable
%else
cd mobile
%endif
./autogen.sh

CFLAGS+="  -Wall -D_LITEW_OPT_ -D_MM_PLAYER_ALP_PARSER -D_FILE_OFFSET_BITS=64 -DMMFW_DEBUG_MODE -DGST_EXT_TIME_ANALYSIS -DUSE_AUDIO_EFFECT -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" "; export CFLAGS
LDFLAGS+="-Wl,--rpath=%{_prefix}/lib -Wl,--hash-style=both -Wl,--as-needed"; export LDFLAGS

# always enable sdk build. This option should go away
CFLAGS=$CFLAGS LDFLAGS=$LDFLAGS ./configure --enable-sdk --prefix=%{_prefix} --disable-static

# Call make instruction with smp support
make -j1 

%install
%if 0%{?tizen_profile_wearable}
cd wearable
%else
cd mobile
%endif
rm -rf %{buildroot}
mkdir -p %{buildroot}/%{_datadir}/license
cp LICENSE.APLv2 %{buildroot}/%{_datadir}/license/%{name}
%make_install


#rm -f %{buildroot}/usr/bin/test_alarmdb

%clean
rm -rf %{buildroot}



%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%if 0%{?tizen_profile_wearable}
%manifest wearable/libmm-player.manifest
%else
%manifest mobile/libmm-player.manifest
%endif
%defattr(-,root,root,-)
%{_libdir}/*.so.*
%if 0%{?tizen_profile_wearable}
%{_bindir}/*
%endif
%{_datadir}/license/%{name}

%files devel
%defattr(-,root,root,-)
%{_libdir}/*.so
%{_includedir}/mmf/*.h
%{_libdir}/pkgconfig/*

