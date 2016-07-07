# Makefile for wpa2 software
#
# This Makefile may be run "standalone" to create native software
# for a Linux PC.
# For cross-compiling, some pre-definitions are needed,
# see "buildmips" script as an example.
# (You can still do "make clean" even if cross-compiling).

# Pseudo-targets:
# 	clean -- remove all(?) created files
# 	firsttime -- do clean, then initialize all subdirectories
# 	update (default if .firsttime exists) -- limited rebuild
# 			suitable for typical source file changes
# With no arguments, does update (and firsttime if .firsttime does not exist)
#
# Environmental variables used:
# INSTALL_ROOT=...  where to put output files
#       If missing, uses install subdirectory (useful for development
#       on a linux PC).
# BUILD_WPA2_DEBUG=y to turn off optimizations and staticly linked
# 	in order to ease debugging.
# BUILD_WPA2_STATIC_COMMON_LIBRARY=y to staticly link common library
#       instead of using shared object file.
# BUILD_WPA2_NO_HOSTAPD=y to disable building hostapd.
# BUILD_WPA2_NO_WPA_SUPPLICANT=y to disable building wpa_supplicant.
# BUILD_WPA2_NO_WPATALK=y to disable building wpatalk.
# BUILD_WPA2_NO_APSTART=y to disable building apstart.
# BUILD_WPA2_NO_EXAMPLES=y to disable installing example files to /etc/wpa2.
#
# Output files:
# 	.firsttime -- writtten after "firsttime" make
# 		If missing, "update" will do "firsttime"
# 	$(INSTALL_ROOT) -- important run-time files copied here,
# 		and also some files for cross-module compilation.
# 		(By default this is "install").

ifndef INSTALL_ROOT
INSTALL_ROOT=$(shell pwd)/install
endif

# If building only one of wpa_supplicant and hostapd, then static link
# makes most sense... else shared object file (default) makes most sense.
ifeq ($(BUILD_WPA2_NO_WPA_SUPPLICANT),y)
ifndef BUILD_WPA2_STATIC_COMMON_LIBRARY
export BUILD_WPA2_STATIC_COMMON_LIBRARY=y
endif
endif
ifeq ($(BUILD_WPA2_NO_HOSTAPD),y)
ifndef BUILD_WPA2_STATIC_COMMON_LIBRARY
export BUILD_WPA2_STATIC_COMMON_LIBRARY=y
endif
endif

ifndef ATHEROSPATH
export ATHEROSPATH=$(TOPDIR)/$(BOARD_TYPE)/$(WLAN_DIR)
endif

# Keep "update" first so it is default
update: 
	if [ ! -f .firsttime ] ; then echo MUST DO FIRSTTIME BUILD FIRST; $(MAKE) firsttime; fi
	@echo DOING UPDATE
	@echo UPDATING tags file
	@echo $(ATHEROSPATH)
	ctags `find . -name '*.h' -o -name '*.c' -o -name '*.cpp'`
	mkdir -p $(INSTALL_ROOT)
	mkdir -p $(INSTALL_ROOT)/sbin
	mkdir -p $(INSTALL_ROOT)/lib

	@echo UPDATING wireless_tools
	cd wireless_tools && $(MAKE) install
	@echo UPDATING common
	cd common && $(MAKE) install
ifeq ($(BUILD_WPA2_NO_WPA_SUPPLICANT),y)
else
	@echo UPDATING wpa_supplicant
	cd wpa_supplicant && $(MAKE) install
endif
ifeq ($(BUILD_WPA2_NO_HOSTAPD),y)
else
	@echo UPDATING hostapd
	cd hostapd && $(MAKE) install
endif
ifeq ($(BUILD_WPA2_NO_WPATALK),y)
else
	@echo UPDATING wpatalk
	cd wpatalk && $(MAKE) install
endif
ifeq ($(BUILD_WPA2_NO_APSTART),y)
else
	@echo UPDATING apstart
	cd apstart && $(MAKE) install
endif

firsttime:  clean 
	@echo BUILD_WPA2_DEBUG is $(BUILD_WPA2_DEBUG)
	echo Did first time inits >.firsttime
	@echo DONE with firsttime
	$(MAKE) update

clean:
	@echo DOING CLEAN
	-rm -rf install tags .firsttime
	-find . -name 'junk*' -exec rm -rf '{}' ';'
	-find . -name '.deps' -exec rm -rf '{}' ';'
	-find . -name '.dirstamp' -exec rm -rf '{}' ';'
	-find . -name '.obj' -exec rm -rf '{}' ';'
	-find . -name '.moc' -exec rm -rf '{}' ';'
	-find . -name '.ui' -exec rm -rf '{}' ';'
	-find . -name '.rnd' -exec rm -rf '{}' ';'
	-find . -name '.*.swp' -exec rm -rf '{}' ';'
	-find . -name '*.log' -exec rm -rf '{}' ';'
	-(cd wireless_tools && $(MAKE) -k clean)
	-(cd common && $(MAKE) -k clean)
	-(cd hostapd && $(MAKE) -k clean)
	-(cd wpa_supplicant && $(MAKE) -k clean)
	-(cd wpatalk && $(MAKE) -k clean)
	-(cd apstart && $(MAKE) -k clean)
	@echo All cleaned


