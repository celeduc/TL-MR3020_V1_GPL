CONFIGURE_OPTIONS = \
	--prefix=/tmp/snmp \
	--target=$(ARCH)-linux  \
	--host=$(ARCH) \
	--with-cc=$(ARCH)-gcc \
	--with-ar=$(ARCH)-ar \
	--with-endianness=big \
	--with-cflags="$(COPTS) \
	-DCAN_USE_SYSCTL=1 \
	-ffunction-sections \
	-fdata-sections \
	-Wl,--gc-sections" \
	--enable-mini-agent \
	--disable-debugging \
	--disable-privacy \
	--without-opaque-special-types \
	--with-persistent-directory=/tmp/snmp/persist \
	--with-default-snmp-version=1 \
	--with-sys-contact=root \
	--with-sys-location=Unknown \
	--with-logfile=/dev/null \
	--with-out-transports=UDPIPv6,TCPIPv6,AAL5PVC,IPX,TCP,Unix \
	--enable-shared=no \
#	--enable-static \
	--with-gnu-ld \
	--enable-internal-md5 \
	--with-copy-persistent-files=no \
	--without-openssl \
	-sysconfdir=/tmp/snmp \
	--with-mib-modules=mibII,mibII/ip,mibII/tcp,mibII/udp,mibII/icmp,mibII/var_route,mibII/kernel_linux \
	--with-out-mib-modules=agent_mips,host,agentx,ieee802dot11,notification,utilities,target,ucd_snmp,snmpv3mibs \
	--disable-ipv6 \
	--with-defaults \
	--without-efence \
	--without-rsaref \
	--without-kmem-usage \
	--without-dmalloc\
	--disable-applications \
	--disable-ipv6 \
	--disable-manuals \
	--disable-mib-loading \
	--disable-mibs \
	--disable-scripts \
	--disable-embedded-perl \
	--disable-snmptrapd-subagent \
	--without-opaque-special-types \
	--without-perl-modules \
	--without-kmem-usage \
	--without-libwrap \
	--without-rpm \
	--without-zlib \
	



SNMP_MIB_MODULES_INCLUDED = \
	mibII/system_mib \
	mibII/ip \
	mibII/tcp \
	mibII/udp \
	mibII/icmp \

SNMP_MIB_MODULES_EXCLUDED = \
	agent_mibs \
	agentx \
	host \
	ieee802dot11 \
	notification \
	mibII \
	snmpv3mibs \
	ucd_snmp \
	utilities \
	target \

SNMP_TRANSPORTS_INCLUDED = Callback UDP

SNMP_TRANSPORTS_EXCLUDED = TCP TCPv6 UDPv6 Unix

ARCH=mips-linux
PREFIX=/tmp/snmp
SRCDIR=$(TOPDIR)/apps/net-snmp-5.4.2.1


SNMP_CONFIGURE_OPTIONS = \
	--prefix=$(PREFIX) \
	--build=i686-linux \
	--host=$(ARCH) \
	--target=$(ARCH)-linux \
	--with-cc=$(ARCH)-gcc \
	--with-ar=$(ARCH)-ar \
	--with-endianness=big \
	--enable-shared=no \
	--enable-static=yes \
	--enable-internal-md5 \
	--enable-mini-agent \
	--disable-applications \
	--disable-debugging \
	--disable-ipv6 \
	--disable-manuals \
	--disable-mib-loading \
	--disable-mibs \
	--disable-scripts \
	--disable-ucd-snmp-compatibility \
	--disable-privacy \
	--disable-embedded-perl \
	--disable-snmptrapd-subagent \
	--with-default-snmp-version="1" \
	--with-out-mib-modules="$(SNMP_MIB_MODULES_EXCLUDED)" \
	--with-mib-modules="$(SNMP_MIB_MODULES_INCLUDED)" \
	--with-out-transports="$(SNMP_TRANSPORTS_EXCLUDED)" \
	--with-transports="$(SNMP_TRANSPORTS_INCLUDED)" \
	--without-opaque-special-types \
	--without-perl-modules \
	--without-kmem-usage \
	--without-openssl \
	--without-libwrap \
	--without-rpm \
	--without-zlib \
	--without-efence \
	--without-rsaref \
	--without-kmem-usage \
	--without-dmalloc\
	--with-sys-contact=root@localhost \
	--with-sys-location=Unknown \
	--with-logfile=/dev/null \
	--with-persistent-directory=$(PREFIX)/persist \
	-sysconfdir=$(PREFIX) \

snmp:
	#cd $(SRCDIR) && make clean
	cd $(SRCDIR) && ./configure $(SNMP_CONFIGURE_OPTIONS)
	cd $(SRCDIR) && make
	#$(TOOLPREFIX)strip $(PREFIX)/sbin/snmpd
	#$(TOOLPREFIX)strip $(PREFIX)/lib/*so*
	cp $(SRCDIR)/agent/snmpd $(TOPDIR)/$(PRODUCT_TYPE)/images/$(BOARD_TYPE)/


snmp-configure:
	cd $(SRCDIR) && ./configure $(SNMP_CONFIGURE_OPTIONS) 

	
snmp-config:
	cd $(SRCDIR) && ./configure $(CONFIGURE_OPTIONS)

snmp-clean:
	cd $(SRCDIR) && make clean


        
