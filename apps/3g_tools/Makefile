#
# 3G tools Makefile
# this file is wrote based on yangxv's 3G tools
# Attention:
#     chat-2.4.3 need link libmsglog.so
#     handle_card and usb-modeswitch-1.1.3 depend on libusb-0.1.12 because they need libusb.so and usb.h     
# created by tiger 20091202

CURRENTDIR = $(3G_TOOLS_PATH)
LIBUSBMIPS = install_libusb_mips

# [pangxing start] make repo proprietary happy.
APPS_3G_TOOLS = des_build chat_build usb_modeswitch 3g_conifg_build
# [wukan start] Use telnet instead of cmdmanage.
#ifneq ($(wildcard $(CURRENTDIR)/cmd_manage), )
#APPS_3G_TOOLS += cmdmanage
#endif
# [wukan end]

ifneq ($(wildcard $(CURRENTDIR)/handle_card), )
APPS_3G_TOOLS += handlecard_build
endif
ifneq ($(wildcard $(CURRENTDIR)/ipad), )
APPS_3G_TOOLS += ipad_build
endif
ifneq ($(wildcard $(CURRENTDIR)/modem_scan), )
APPS_3G_TOOLS += modemscan_build
endif
$(warning xtest, APPS_3G_TOOLS are $(APPS_3G_TOOLS))

.PHONY: all
all: $(APPS_3G_TOOLS)
# [pangxing end]

3g_tools_prep:
	test -d $(CURRENTDIR)/$(LIBUSBMIPS) || mkdir -p $(CURRENTDIR)/$(LIBUSBMIPS) 

#changed by ZQQ 11.11.09 sort the mode_switch.conf before des
# [pangxing start] make repo proprietary happy.
des_build:
	cd $(CURRENTDIR)/des && \
	$(MAKE) CC=gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld && \
	(if [ -e ../mode_switch/mode_switch.conf ]; then ./sort -e -f ../mode_switch/mode_switch.conf -o ../mode_switch.conf.bin; fi)
# [pangxing end]

# [pangxing] Build: Fix libusb/Tpo build error on Ubuntu 11 and later (x86_64).
libusb_build: 3g_tools_prep
	cd $(CURRENTDIR)/libusb-0.1.12 && \
	if [ ! \( \( -e Makefile \) -a \( -e $(CURRENTDIR)/$(LIBUSBMIPS)/lib/libusb.so \) \) ]; then \
	./configure --prefix=$(CURRENTDIR)/$(LIBUSBMIPS) --build=i686-linux --host=mips-linux CC=$(TOOLPREFIX)gcc CXX=$(TOOLPREFIX)g++ AR=$(TOOLPREFIX)ar && \
	$(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld && \
	$(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld install ; \
	fi
	cd $(CURRENTDIR)/libusb-1.0.8 && \
	autoreconf -i && \
	./configure  --build=i686-linux --host=mips-linux CC=$(TOOLPREFIX)gcc CXX=$(TOOLPREFIX)g++ AR=$(TOOLPREFIX)ar && \
	$(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld	
	 
	cp $(CURRENTDIR)/$(LIBUSBMIPS)/lib/libusb-0.1.so.4.4.4 $(3G_PACKAGE_PATH)

ipad_build:libusb_build
	cd $(CURRENTDIR)/ipad && \
	cp ../libusb-1.0.8/libusb/*.o ./ && \
        $(MAKE) all CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld && \
	cp -f ipad_charge $(3G_PACKAGE_PATH)/
		
cmdmanage:
	echo cmdmanage
	cd $(CURRENTDIR)/cmd_manage && \
	$(MAKE) all CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld && \
	cp -f cmdmanage $(3G_PACKAGE_PATH)/
	
chat_build:
	cd $(CURRENTDIR)/chat-2.4.3 && \
	$(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld TP_WEB_LOG=y CFLAGS+="-I$(TOPDIR)/web_server/msglogd -lmsglog" LDFLAGS+="-L$(INSTALL_ROOT)/lib" && \
	cp -f chat $(3G_PACKAGE_PATH)/

usb_modeswitch:libusb_build
	cd $(CURRENTDIR)/usb_modeswitch-1.1.9 && \
	$(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld DSTDIR=$(3G_PACKAGE_PATH) install

handlecard_build:libusb_build des_build
	cd $(CURRENTDIR)/handle_card && \
	ls -l handle_card && $(TOOLPREFIX)strip handle_card && ls -l handle_card && \
	cp -f handle_card $(3G_PACKAGE_PATH)/


modemscan_build:
	cd $(CURRENTDIR)/modem_scan && \
	ls -l modem_scan && $(TOOLPREFIX)strip modem_scan && ls -l modem_scan && \
	cp -f modem_scan $(3G_PACKAGE_PATH)/

modemreboot_build:
	cd $(CURRENTDIR)/modem_reboot && \
	$(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld && \
	ls -l modem_reboot && $(TOOLPREFIX)strip modem_reboot && ls -l modem_reboot && \
	cp -f modem_reboot $(3G_PACKAGE_PATH)/

modemoperation_build:
	cd $(CURRENTDIR)/modem_operation && \
	$(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld && \
	ls -l modem_operation && $(TOOLPREFIX)strip modem_operation && ls -l modem_operation && \
	cp -f modem_operation $(3G_PACKAGE_PATH)/

# [pangxing start] make repo proprietary happy.
3g_conifg_build:
	(if [ -e $(CURRENTDIR)/ppp ]; then rm -rf $(3G_PACKAGE_PATH)/etc/ppp && cp -rf $(CURRENTDIR)/ppp $(3G_PACKAGE_PATH)/etc; fi;)
	(if [ -e $(CURRENTDIR)/3gISP ]; then rm -rf $(3G_PACKAGE_PATH)/etc/3gISP && cp -rf $(CURRENTDIR)/3gISP $(3G_PACKAGE_PATH)/etc; fi;)
	-cd $(CURRENTDIR) && \
	cp hotplug $(3G_PACKAGE_PATH)/ && \
	#cp -rf $(CURRENTDIR)/usb_modeswitch.d $(3G_PACKAGE_PATH)/etc && \
	#cp mode_switch.conf $(3G_PACKAGE_PATH)/ && \
	cp mode_switch.conf.bin $(3G_PACKAGE_PATH)/
# [pangxing end]

3g_tools_clean:
	-rm -rf $(CURRENTDIR)/$(LIBUSBMIPS) 
	cd $(CURRENTDIR)/chat-2.4.3 && $(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld clean
	cd $(CURRENTDIR)/libusb-0.1.12 &&  \
	if [ -e Makefile ]; then \
	$(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld distclean; \
	fi  
	cd $(CURRENTDIR)/usb_modeswitch-1.1.3 && $(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld clean
	-cd $(CURRENTDIR)/handle_card && $(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld clean
	-cd $(CURRENTDIR)/modem_scan && $(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld clean
	cd $(CURRENTDIR)/des && $(MAKE) CC=gcc clean
clean: 3g_tools_clean
	echo "do not clean"
	
	
