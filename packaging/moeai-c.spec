Name:           moeai-c
Version:        0.1.0
Release:        1%{?dist}
Summary:        智能内核助手模块

License:        GPL
URL:            https://gitlab.com/yourusername/moeai-c
Source0:        %{name}.tar.gz

BuildRequires:  kernel-devel, gcc, make
Requires:       kernel >= 4.0

%description
MoeAI-C 是一个智能内核助手模块，提供内存监控和管理功能，
通过 procfs 接口与用户空间通信。

%prep
%setup -q

%build
make %{?_smp_mflags}
gcc -Wall -Iinclude cli/moectl.c -o build/bin/moectl

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_sbindir}
mkdir -p %{buildroot}%{_sysconfdir}/moeai
mkdir -p %{buildroot}%{_sysconfdir}/modules-load.d
mkdir -p %{buildroot}/lib/modules/%{kernel_version}/extra

install -m 755 build/bin/moectl %{buildroot}%{_sbindir}/
install -m 644 moeai.ko %{buildroot}/lib/modules/%{kernel_version}/extra/
echo "moeai" > %{buildroot}%{_sysconfdir}/modules-load.d/moeai.conf

%post
/sbin/depmod -a

%postun
/sbin/depmod -a

%files
%{_sbindir}/moectl
/lib/modules/%{kernel_version}/extra/moeai.ko
%{_sysconfdir}/modules-load.d/moeai.conf

%changelog
* Fri Apr 25 2025 Your Name <your.email@example.com> - 0.1.0-1
- 首个 MVP 版本发布