ifndef BOARD_TYPE
export BOARD_TYPE=ap121
endif

ifndef PID
export PID=302001
endif

export WEB_SERVER = web_server
export ETHNET_PATH = ethernet
export WLAN_PATH = wlan
export FIRMWARE_RECOVERY=0
export FIRMWARE_RECOVERY_NAME=tp_recovery.bin
export WEBDIR=../webs/web_pages

include Makefile.$(BOARD_TYPE)

	
######## build fakeroot host tool ########
fakeroot_build:
	cd $(TOPDIR)/util/fakeroot && make -f make.fakeroot fakeroot
	
toolchain_prep:
	@echo Uncompressing toolchain, please wait for a few minutes ...
	cd $(TOPDIR)/toolchain && \
	tar -jxf build0.9.29.pb42.tar.bz2 && \
	tar -jxf gcc-4.3.3.tar.bz2 && \
	(if [ -e $(TOPDIR)/build/gcc-3.4.4-2.16.1 ]; then rm -rf $(TOPDIR)/build/gcc-3.4.4-2.16.1; fi) && \
	mv ./build/gcc-3.4.4-2.16.1 $(TOPDIR)/build && \
	rm -rf $(TOPDIR)/toolchain/build && \
	(if [ -e $(TOPDIR)/build/gcc-4.3.3 ]; then rm -rf $(TOPDIR)/build/gcc-4.3.3; fi) && \
	mv gcc-4.3.3 $(TOPDIR)/build

#
#  CONFIG setting
#

# modules used by all boards
# [pangxing] happy to refactor git to repo.
export BOARD_KERNEL_MODULES = netfilter netsched ts_kmp #art
ifneq ($(wildcard $(TOPDIR)/kernel_modules/stats), )
BOARD_KERNEL_MODULES += stats
endif
ifneq ($(wildcard $(TOPDIR)/kernel_modules/wlan_warn), )
BOARD_KERNEL_MODULES += wlan_warn
endif
ifneq ($(wildcard $(TOPDIR)/kernel_modules/br_hook), )
BOARD_KERNEL_MODULES += br_filter
endif
ifneq ($(wildcard $(TOPDIR)/kernel_modules/pppol2tp-kmod-0.17), )
BOARD_KERNEL_MODULES += pppol2tp
endif
ifneq ($(wildcard $(TOPDIR)/kernel_modules/pptp_driver-0.8.3), )
BOARD_KERNEL_MODULES += pptp_module
endif
ifneq ($(wildcard $(TOPDIR)/kernel_modules/tp_domain), )
BOARD_KERNEL_MODULES += tp_domain
endif
ifneq ($(wildcard $(TOPDIR)/kernel_modules/kernel_debug), )
BOARD_KERNEL_MODULES += kernel_debug
endif
ifneq ($(wildcard $(TOPDIR)/kernel_modules/ht_mcu), )
BOARD_KERNEL_MODULES += ht_mcu
endif

ifeq ($(BOARD_TYPE),ap121)
BOARD_KERNEL_MODULES += usb_serial usb_storage usb_net
endif

ifeq ($(CONFIG_USB_SERIAL_MODULE),y)
BOARD_KERNEL_MODULES += usb_serial
endif

# applications used by all board types 
BOARD_APPLICATIONS = busybox pppoe l2tp bpa iptables tc_build lltd net_ioctl arp

ifeq ($(CONFIG_3G_TOOLS),1)
BOARD_APPLICATIONS += 3g_tools
endif

export NO_CHECK_RSA_SIGNATURE=1
export MAKE_FLASH_TOOL=make_flash


$(warning xtest, BOARD_KERNEL_MODULES is $(BOARD_KERNEL_MODULES))


fs_prep:install_root_clean util_chmod check_tftp libs_copy etc_prep empty_dir_prep

install_root_clean:
	-rm -rf $(INSTALL_ROOT)/*
	
util_chmod:
	-cd $(TOPDIR)/util && chmod 777 cplibs md5_des mkdev mksquashfs pad
	-chmod 777 $(TOPDIR)/webs/web_pages/utils/getini/gather
	-chmod 777 $(TOPDIR)/webs/web_pages/utils/mkrc/mkrc
	-chmod 777 $(TOPDIR)/webs/web_pages/utils/mkrc/rmtag
	-chmod 777 $(TOPDIR)/webs/web_pages/utils/web_collector/webc
	
check_tftp:
	mkdir -p $(IMAGEPATH)
	if test -d $(TFTPPATH); then echo $(TFTPPATH) exists; else mkdir -p $(TFTPPATH); fi;

libs_copy:
	$(TOPDIR)/util/cplibs
	
dev_create:
	rm -rf $(INSTALL_ROOT)/dev/*
	$(TOPDIR)/util/mkdev

etc_prep:
	rm -rf $(INSTALL_ROOT)/etc
	cp -r $(TOPDIR)/filesystem/ap93fus/etc $(INSTALL_ROOT)/

empty_dir_prep:
	if test -d $(INSTALL_ROOT)/mnt; then echo $(INSTALL_ROOT)/mnt exists; else mkdir $(INSTALL_ROOT)/mnt; fi;
	if test -d $(INSTALL_ROOT)/proc; then echo $(INSTALL_ROOT)/proc exists; else mkdir $(INSTALL_ROOT)/proc; fi;
	if test -d $(INSTALL_ROOT)/root; then echo $(INSTALL_ROOT)/root exists; else mkdir $(INSTALL_ROOT)/root; fi;
	if test -d $(INSTALL_ROOT)/tmp; then echo $(INSTALL_ROOT)/mnt exists; else mkdir $(INSTALL_ROOT)/tmp; fi;
	if test -d $(INSTALL_ROOT)/var; then echo $(INSTALL_ROOT)/var exists; else mkdir -p $(INSTALL_ROOT)/var/run; fi;
	
#
# kernel modules
#

KERNEL_MODULE_PATH=$(TOPDIR)/images/$(BOARD_TYPE)/kernel_modules/

kernel_modules: kernel_modules_prep $(BOARD_KERNEL_MODULES)

kernel_modules_warning:
	echo xtest, kernel_modules src NOT exist, skip kernel_modules build.

kernel_modules_prep:
	test -d $(KERNEL_MODULE_PATH) || mkdir -p $(KERNEL_MODULE_PATH)

kernel_modules_install:
	mkdir -p $(INSTALL_ROOT_FOR_BOARD)/lib/modules/$(KERNELVER)/kernel/
	cp -Rf $(KERNEL_MODULE_PATH)*.ko $(INSTALL_ROOT_FOR_BOARD)/lib/modules/$(KERNELVER)/kernel/
	
stats:
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/stats modules
	cp $(TOPDIR)/kernel_modules/stats/statistics.ko $(KERNEL_MODULE_PATH)
	
kernel_debug:
	#$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/kernel_debug modules
	cp $(TOPDIR)/kernel_modules/kernel_debug/kernel_debug.ko $(KERNEL_MODULE_PATH)	

netfilter:
	if [ "$(KERNELVER)" = "2.6.15" ]; then \
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=net/ipv4/netfilter modules ; \
	else \
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=net/netfilter modules ; \
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=net/ipv4/netfilter modules ; \
	fi;

	-cp $(KERNELPATH)/net/ipv4/netfilter/*.ko $(KERNEL_MODULE_PATH)
	-cp $(KERNELPATH)/net/netfilter/*.ko $(KERNEL_MODULE_PATH)	

wlan_warn:
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/wlan_warn modules
	cp $(TOPDIR)/kernel_modules/wlan_warn/wlan_warn.ko $(KERNEL_MODULE_PATH)
	
tp_domain:
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/tp_domain modules
	cp $(TOPDIR)/kernel_modules/tp_domain/tp_domain.ko $(KERNEL_MODULE_PATH)

netsched:
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=net/sched modules	
	cp $(KERNELPATH)/net/sched/*.ko $(KERNEL_MODULE_PATH)

ts_kmp:
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=lib modules	
	cp $(KERNELPATH)/lib/ts_kmp.ko $(KERNEL_MODULE_PATH)
	
br_filter:
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/br_hook modules
	cp $(TOPDIR)/kernel_modules/br_hook/br_filter.ko $(KERNEL_MODULE_PATH)
	
flashid:
ifneq ($(wildcard $(TOPDIR)/kernel_modules/flash_id), )
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/flash_id modules
	cp $(TOPDIR)/kernel_modules/flash_id/flashid.ko $(KERNEL_MODULE_PATH)
else
	@echo xtest, flash_id src NOT exist, skip flash_id build.
endif
	
antind:
ifneq ($(wildcard $(TOPDIR)/kernel_modules/anti_nat_detect modules), )
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/anti_nat_detect modules
	cp $(TOPDIR)/kernel_modules/anti_nat_detect/ipt_ANTIND.ko /tftpboot/tftp/
else
	@echo xtest, antind src NOT exist, skip antind build.
endif

ht_mcu:
ifeq ($(MANAGE_BATTERY_BY_MCU),y)
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/ht_mcu modules
	cp $(TOPDIR)/kernel_modules/ht_mcu/ht_mcu.ko $(KERNEL_MODULE_PATH)
else
	@echo ht mcu not enabled, skip ht_mcu build.
endif

gpio:
ifneq ($(wildcard $(TOPDIR)/kernel_modules/gpio), )
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/gpio modules
	cp $(TOPDIR)/kernel_modules/gpio/gpio.ko $(KERNEL_MODULE_PATH)
else
	@echo xtest, gpio src NOT exist, skip gpio build.
endif

cone_udp:
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/cone_udp modules
	cp $(TOPDIR)/kernel_modules/cone_udp/cone_udp.ko $(KERNEL_MODULE_PATH) 
	cp $(TOPDIR)/kernel_modules/cone_udp/cone_udp.ko /tftpboot/tftp/
	
starcraft_helper:
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/starcraft_helper modules
	cp $(TOPDIR)/kernel_modules/starcraft_helper/ip_conntrack_helper_starcraft.ko /tftpboot/tftp/

harmony:
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/harmony modules
	
pppol2tp:	
	if [ "$(KERNELVER)" = "2.6.15" ]; then \
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/pppol2tp-kmod-0.17 modules && \
	cp $(TOPDIR)/kernel_modules/pppol2tp-kmod-0.17/*.ko $(KERNEL_MODULE_PATH) ; \
	else \
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=drivers/net modules && \
	cp $(KERNELPATH)/drivers/net/pppol2tp.ko $(KERNEL_MODULE_PATH) ; \
	fi;
	
generic_ppp:	
ifeq ($(KERNELVER), "2.6.15")
	echo "Nothing to do..."
else
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=drivers/net modules    
	cp $(KERNELPATH)/drivers/net/ppp_generic.ko $(KERNEL_MODULE_PATH) && \
	cp $(KERNELPATH)/drivers/net/ppp_generic.ko $(TOPDIR)/util/mkfirmware/bin
endif

pptp_module:
	 $(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/pptp_driver-0.8.3 modules
	cp $(TOPDIR)/kernel_modules/pptp_driver-0.8.3/*.ko $(KERNEL_MODULE_PATH)

ap_vlan:
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/ap_vlan/forWard modules
	$(MAKEARCH) -C $(KERNELPATH) SUBDIRS=$(TOPDIR)/kernel_modules/ap_vlan/passUp modules
	cp $(TOPDIR)/kernel_modules/ap_vlan/forWard/*.ko $(KERNEL_MODULE_PATH)
	cp $(TOPDIR)/kernel_modules/ap_vlan/passUp/*.ko $(KERNEL_MODULE_PATH)
#
# applications
#

apps_build: $(BOARD_APPLICATIONS)


ifeq ($(BUSYBOX), busybox-1.01)
busybox: # easy build for busybox, no clean
ifeq ($(CONFIG_BETA),1)
	cp $(TOPDIR)/apps/$(BUSYBOX)/configs/.debug_config $(TOPDIR)/apps/$(BUSYBOX)/.config
else
	cp $(TOPDIR)/apps/$(BUSYBOX)/configs/.release_config $(TOPDIR)/apps/$(BUSYBOX)/.config
endif
	cd $(TOPDIR)/apps/$(BUSYBOX); $(MAKE) PREFIX=$(INSTALL_ROOT) uninstall; \
	find . -name "*.o" | xargs rm -rf; \
	$(MAKE); \
	$(MAKE) PREFIX=$(INSTALL_ROOT) install
else ifeq ($(BUSYBOX), busybox-1.21.1)
busybox:
	cd $(TOPDIR)/apps/$(BUSYBOX); \
	find . -name "*.o" | xargs rm -rf; \
	if test -e ./configs/tp_dbg_defconfig; then \
		$(MAKE) tp_dbg_defconfig; \
	elif test -e ./configs/$(PID)_defconfig; then \
		$(MAKE) $(PID)_defconfig; \
	else \
		$(MAKE) tp_defconfig; \
	fi; \
	$(MAKE) CONFIG_PREFIX=$(INSTALL_ROOT) install;
endif

ifndef COUNTRY_SPEC
export COUNTRY_SPEC=COUNTRY_UN
endif

pppoe:
	cd $(TOPDIR)/apps/ppp-2.4.3/pppd && make clean && make CC=$(TOOLPREFIX)gcc LD=$(TOOLPREFIX)ld AR=$(TOOLPREFIX)ar && \
	cp pppd $(INSTALL_ROOT)/usr/sbin

pptp:
	cd $(TOPDIR)/apps/pptp-1.7.1 && make clean && make CC=$(TOOLPREFIX)gcc LD=$(TOOLPREFIX)ld AR=$(TOOLPREFIX)ar && \
	cp pptp $(INSTALL_ROOT)/usr/sbin

l2tp:
	cd $(TOPDIR)/apps/xl2tpd-1.1.12 && make clean &&  make CC=$(TOOLPREFIX)gcc LD=$(TOOLPREFIX)ld AR=$(TOOLPREFIX)ar && \
	cp xl2tpd $(INSTALL_ROOT)/usr/sbin

bpa:
	cd $(TOPDIR)/apps/bpalogin-2.0.2 && make clean &&  make CC=$(TOOLPREFIX)gcc LD=$(TOOLPREFIX)ld AR=$(TOOLPREFIX)ar && \
	cp bpalogin $(INSTALL_ROOT)/usr/sbin
	
iptables:
ifeq ($(KERNELVER), 2.6.15)
	cd $(TOPDIR)/apps/iptables-1.3.7/ && \
	$(MAKE) CC=$(TOOLPREFIX)gcc LD=$(TOOLPREFIX)ld DO_IPV6=0 KERNEL_DIR=$(KERNELPATH) LIBDIR=/lib && \
	cp iptables $(INSTALL_ROOT)/sbin/ && \
	cd extensions && \
	test -d $(INSTALL_ROOT)/lib/iptables/ || mkdir -p $(INSTALL_ROOT)/lib/iptables/ && \
	mv -f libipt_DNAT.so libipt_MASQUERADE.so libipt_SNAT.so libipt_TCPMSS.so libipt_TRIGGER.so \
	libipt_comment.so libipt_icmp.so libipt_iprange.so libipt_mac.so \
	libipt_multiport.so libipt_standard.so libipt_state.so libipt_string.so \
	libipt_tcp.so libipt_time.so libipt_udp.so libipt_MARK.so libipt_multiurl.so libipt_wlanwarn.so $(INSTALL_ROOT)/lib/iptables/
else ifeq ($(KERNELVER), 2.6.31)
	cd $(TOPDIR)/apps/iptables-1.4.5 && \
	if test -e Makefile ; then make distclean ; fi && \
	autoreconf -i && \
	./configure --prefix=$(INSTALL_ROOT) \
	--build=i386 --host=mips-linux  \
	--with-ksource=$(KERNELPATH) \
	--disable-devel --disable-libipq \
	--disable-ipv6 && \
	$(MAKE) CC=$(TOOLPREFIX)gcc LD=$(TOOLPREFIX)ld AR=$(TOOLPREFIX)ar RANLIB=$(TOOLPREFIX)ranlib && \
	$(MAKE) CC=$(TOOLPREFIX)gcc LD=$(TOOLPREFIX)ld AR=$(TOOLPREFIX)ar RANLIB=$(TOOLPREFIX)ranlib install && \
	./rm_ext.sh
endif
	rm -rf $(INSTALL_ROOT)/lib/libexec
	-mv $(INSTALL_ROOT)/libexec $(INSTALL_ROOT)/lib
	rm -rf $(INSTALL_ROOT)/share
	rm -rf $(INSTALL_ROOT)/man
	

tc_build:
	cd $(TOPDIR)/apps/iproute2-$(KERNELVER) && make CC=$(TOOLPREFIX)gcc && \
	cp $(TOPDIR)/apps/iproute2-$(KERNELVER)/tc/tc $(INSTALL_ROOT)/sbin
	
lltd:
	@echo ********** making WinRally ************
	cd $(TOPDIR)/apps/lltd/wr841n-linux && \
	make clean && $(MAKE) CC=$(TOOLPREFIX)gcc LD=$(TOOLPREFIX)gcc && \
	cp lld2d $(INSTALL_ROOT)/usr/bin/

net_ioctl:
	cd $(TOPDIR)/debug_tools/net_ioctl && \
	cp net_ioctl $(INSTALL_ROOT)/usr/
	
wlanreg:
	cd $(MADWIFIPATH)/tools/hal_diag && make && cp -f reg $(INSTALL_ROOT)/usr/bin/wlanregss
	
arp:
	cd $(TOPDIR)/apps/arp && \
	$(TOOLPREFIX)gcc -o arp arp.c && cp arp $(INSTALL_ROOT)/usr/
	

3gtools_build:
	cd $(TOPDIR)/apps/3g_tools && \
	$(MAKE) clean && $(MAKE) CC=$(TOOLPREFIX)gcc LD=$(TOOLPREFIX)ld AR=$(TOOLPREFIX)ar 
	
httpd_build:
	echo -n "BUILD_TIME = " > $(TOPDIR)/$(WEB_SERVER)/sys/buildtime.h;
	expr `date +%H` \* 3600 + `date +%M` \* 60 + `date +%S` >> $(TOPDIR)/$(WEB_SERVER)/sys/buildtime.h;
	echo -n "BUILD_DATE = " >> $(TOPDIR)/$(WEB_SERVER)/sys/buildtime.h;
	@date +%D >> $(TOPDIR)/$(WEB_SERVER)/sys/buildtime.h;
	cd $(TOPDIR)/$(WEB_SERVER) && \
	ls -l httpd && \
	cp httpd $(IMAGEPATH) && \
	cp ./msglogd/libmsglog.so $(INSTALL_ROOT)/lib/
	
#
# utils
#

make_flash:
	cd $(TOPDIR)/util/make_flash_src/ && make && \
	cp make_flash $(TOPDIR)/util/

md5_des:
	cd $(TOPDIR)/util/md5_des_src/ && gcc main.c md5_des.c -o md5_des && \
	cp md5_des $(TOPDIR)/util/
		
#
# fs and image
#	
strip_lib:
	$(TOOLPREFIX)strip $(INSTALL_ROOT_FOR_BOARD)/lib/libwpa_common.so
	$(TOOLPREFIX)strip $(INSTALL_ROOT_FOR_BOARD)/lib/libxtables.so.2.1.0
	$(TOOLPREFIX)strip $(INSTALL_ROOT_FOR_BOARD)/lib/libxtables.so.2
	$(TOOLPREFIX)strip $(INSTALL_ROOT_FOR_BOARD)/lib/libxtables.so
	$(TOOLPREFIX)strip $(INSTALL_ROOT_FOR_BOARD)/lib/libip6tc.so.0.0.0
	$(TOOLPREFIX)strip $(INSTALL_ROOT_FOR_BOARD)/lib/libiw.so.29	
	$(TOOLPREFIX)strip $(INSTALL_ROOT_FOR_BOARD)/lib/libiw.so
	$(TOOLPREFIX)strip $(INSTALL_ROOT_FOR_BOARD)/lib/libip4tc.so.0.0.0
	$(TOOLPREFIX)strip $(INSTALL_ROOT_FOR_BOARD)/lib/libip4tc.so.0
	$(TOOLPREFIX)strip $(INSTALL_ROOT_FOR_BOARD)/lib/libip4tc.so

strip_all:strip_lib
	cd $(INSTALL_ROOT_FOR_BOARD)/sbin/ && \
	ls -l hostapd && $(TOOLPREFIX)strip hostapd && ls -l hostapd && \
	ls -l tc && $(TOOLPREFIX)strip tc && ls -l tc && \
	ls -l iptables && $(TOOLPREFIX)strip iptables && ls -l iptables && \
	ls -l wlanconfig && $(TOOLPREFIX)strip wlanconfig && ls -l wlanconfig && \
	ls -l iwlist && $(TOOLPREFIX)strip iwlist && ls -l iwlist && \
	ls -l iwconfig && $(TOOLPREFIX)strip iwconfig && ls -l iwconfig && \
	ls -l athstats && $(TOOLPREFIX)strip athstats && ls -l athstats && \
	ls -l iwpriv && $(TOOLPREFIX)strip iwpriv && ls -l iwpriv && \
	ls -l wpa_supplicant && $(TOOLPREFIX)strip wpa_supplicant && ls -l wpa_supplicant

	cd $(INSTALL_ROOT_FOR_BOARD)/bin/ && \
	ls -l busybox && $(TOOLPREFIX)strip busybox && ls -l busybox 
	
	cd $(INSTALL_ROOT_FOR_BOARD)/usr/sbin/ && \
	ls -l bpalogin && $(TOOLPREFIX)strip bpalogin && ls -l bpalogin && \
	ls -l pppd && $(TOOLPREFIX)strip pppd && ls -l pppd && \
	ls -l xl2tpd && $(TOOLPREFIX)strip xl2tpd && ls -l xl2tpd
ifeq ($(CONFIG_AP_TOOLS),1)
	cd $(INSTALL_ROOT_FOR_BOARD)/usr/sbin/ && ls -l snmpd && $(TOOLPREFIX)strip snmpd && ls -l snmpd
endif
	
	cd $(INSTALL_ROOT_FOR_BOARD)/usr/bin/ && \
	ls -l httpd && $(TOOLPREFIX)strip httpd && ls -l httpd && \
	ls -l lld2d && $(TOOLPREFIX)strip lld2d && ls -l lld2d
strip_usb_all:
	cd $(INSTALL_ROOT_FOR_BOARD)/usr/sbin/ && \
	ls -l handle_card && $(TOOLPREFIX)strip handle_card && ls -l handle_card && \
	ls -l usb_modeswitch && $(TOOLPREFIX)strip usb_modeswitch && ls -l usb_modeswitch && \
	ls -l modem_scan && $(TOOLPREFIX)strip modem_scan && ls -l modem_scan	
	$(TOOLPREFIX)strip $(INSTALL_ROOT_FOR_BOARD)/lib/libusb.so
	$(TOOLPREFIX)strip $(INSTALL_ROOT_FOR_BOARD)/lib/libusb-0.1.so.4.4.4
	$(TOOLPREFIX)strip $(INSTALL_ROOT_FOR_BOARD)/lib/libusb-0.1.so.4
	
ifndef lzma
encode = -nolzma
endif

apxxfus_fs_prep:
	-cd $(INSTALL_ROOT_FOR_BOARD) && rm -Rf `ls -I sbin`
	(if [ -h $(INSTALL_ROOT_FOR_BOARD)/etc/passwd ]; then rm -rf $(INSTALL_ROOT_FOR_BOARD)/etc/passwd; fi;)
	(if [ -e $(INSTALL_ROOT_FOR_BOARD) ]; then cp -Rf $(INSTALL_ROOT)/* $(INSTALL_ROOT_FOR_BOARD); else cp -Rf $(INSTALL_ROOT) $(INSTALL_ROOT_FOR_BOARD); fi;)
	make kernel_modules_install
	make wireless_install
ifeq ($(BOARD_TYPE), ap83fus)
	make nas_package
endif
ifeq ($(BOARD_TYPE), pb92)
	make nas_package
endif
ifeq ($(CONFIG_3G_TOOLS),1)
	make 3g_tools_install
	make strip_usb_all
endif
	cp $(IMAGEPATH)/httpd $(INSTALL_ROOT_FOR_BOARD)/usr/bin/
ifeq ($(CONFIG_AP_TOOLS),1)
	cp $(IMAGEPATH)/snmpd $(INSTALL_ROOT_FOR_BOARD)/usr/sbin/
endif
	make strip_all
	

	mkdir -p $(INSTALL_ROOT_FOR_BOARD)/web
	rm -rf $(INSTALL_ROOT_FOR_BOARD)/web/*
	cp -r $(WEBDIR)/$(lang)/common/* $(INSTALL_ROOT_FOR_BOARD)/web

ifeq ($(CONFIG_3G_TOOLS),1)
	-cp -r $(WEBDIR)/$(lang)/common_3g/* $(INSTALL_ROOT_FOR_BOARD)/web
endif

ifeq ($(CONFIG_AP_TOOLS),1)
	-cp -r $(WEBDIR)/$(lang)/common_ap/* $(INSTALL_ROOT_FOR_BOARD)/web
endif

ifeq ($(CONFIG_PHONE_WEB_CN),1)
	-cp -r $(WEBDIR)/$(lang)/common_phone/* $(INSTALL_ROOT_FOR_BOARD)/web
endif

ifeq ($(CONFIG_PRODUCT_MIFI),y)
	-cp -r $(WEBDIR)/$(lang)/common_mifi/* $(INSTALL_ROOT_FOR_BOARD)/web
endif

	-cp -r $(WEBDIR)/$(lang)/oem/$(oem_dir)/* $(INSTALL_ROOT_FOR_BOARD)/web
	$(WEBDIR)/utils/web_collector/webc -s $(INSTALL_ROOT_FOR_BOARD)/web
# [wuzhong end]

	test -d $(INSTALL_ROOT_FOR_BOARD)/web/oem/ || mkdir -p $(INSTALL_ROOT_FOR_BOARD)/web/oem/
	cp $(TOPDIR)/webs/models/$(conf_file) $(TOPDIR)/webs/models/model_tmp.conf

mk_squashfs_build:
	echo "" >> $(TOPDIR)/webs/models/model_tmp.conf
	cat $(TOPDIR)/$(WEB_SERVER)/sys/buildtime.h >> $(TOPDIR)/webs/models/model_tmp.conf
	$(TOPDIR)/util/md5_des $(TOPDIR)/webs/models/model_tmp.conf $(INSTALL_ROOT_FOR_BOARD)/web/oem/model.conf enc
	find $(INSTALL_ROOT_FOR_BOARD) -name ".svn" | xargs rm -rf
	find $(INSTALL_ROOT_FOR_BOARD) -name ".git" | xargs rm -rf
	
	if [ $(BOARD_TYPE) = ap143 ]; then $(TOPDIR)/util/fakeroot/fakeroot/fakeroot $(TOPDIR)/util/buildFS_ap143_LZ; else $(TOPDIR)/util/fakeroot/fakeroot/fakeroot $(TOPDIR)/util/buildFS_LZ; fi;
	ls -l $(IMAGEPATH)/$(BOARD_TYPE)-squashfs	
	$(TOPDIR)/util/pad -i $(IMAGEPATH)/$(BOARD_TYPE)-squashfs -p $(fs_size)
	cp $(IMAGEPATH)/$(BOARD_TYPE)-squashfs $(TFTPPATH)
	ls -l $(TFTPPATH)/$(BOARD_TYPE)-squashfs
	
squashfs_build:	
	$(TOPDIR)/util/mksquashfs $(INSTALL_ROOT) $(IMAGEPATH)/$(BOARD_TYPE)-squashfs -noappend -be -always-use-fragments $(encode)
	ls -l $(IMAGEPATH)/$(BOARD_TYPE)-squashfs
	$(TOPDIR)/util/pad -i $(IMAGEPATH)/$(BOARD_TYPE)-squashfs -p 0x2C0000
	cp $(IMAGEPATH)/$(BOARD_TYPE)-squashfs $(TFTPPATH)
	cp $(IMAGEPATH)/$(BOARD_TYPE)-squashfs $(TOPDIR)/make_flash
	ls -l $(TFTPPATH)/$(BOARD_TYPE)-squashfs
	
image_build:
	find $(IMAGEPATH) -name ".git" | xargs rm -rf
	cd $(IMAGEPATH)/ && \
	$(TOPDIR)/util/$(MAKE_FLASH_TOOL) -b u-boot.bin -k vmlinux.bin.gz -f $(BOARD_TYPE)-squashfs \
	-o $(prefix) -c $(TOPDIR)/webs/models/model_tmp.conf -t $(TOPDIR)/$(WEB_SERVER)/sys/buildtime.h && \
	chmod 777 *

linux_build_all: uboot kernel_build kernel_modules wireless apps_build
# refer to Makefile.product for action of $(PID).
product_build:$(PID)

