#Linux Makefile

BLDTYPE = release
#BLDTYPE = debug

DIR_WSC=.

ifneq ($(strip $(TOOLPREFIX)),)
CROSS:=$(TOOLPREFIX)
endif

export CXX = $(CROSS)g++
export CC = $(CROSS)gcc
ifeq ($(BLDTYPE),debug)
export CXXFLAGS += -Wall -D_REENTRANT -D_TUDEBUGTRACE -O1  
export CFLAGS += -Wall -Wnested-externs -D_REENTRANT -D_TUDEBUGTRACE -O1 
else
export CXXFLAGS += -Wall -D_REENTRANT -O1
export CFLAGS += -Wall -Wnested-externs -D_REENTRANT -O1
endif
export LD = $(CROSS)ld
export LDFLAGS = -r
export LIBPATH = -L$(TOOLPATH)/lib -L../../lib/openssl-0.9.8a
export INCLUDE = -I$(TOOLPATH)/include -I../../../lib/openssl-0.9.8a/include

LIBS =  trace/tutrace.lib \
	slist/slist.lib \
	Portability/Portability.lib \
	UdpLib/UdpLib.lib \
	InbEap/InbEap.lib \
	InbIp/InbIp.lib \
	InbWlan/InbWlan.lib \
	UPnPILib/UPnPILib.lib \
	UPnPCpLib/UPnPCpLib.lib \
	InbUPnPCp/InbUPnPCp.lib \
	UPnPDevLib/UPnPDevLib.lib \
	InbUPnPDev/InbUPnPDev.lib \
	Transport/Transport.lib \
	StateMachine/StateMachine.lib \
	RegProtocol/RegProtocol.lib \
	MasterControl/MasterControl.lib \
	TlvBase/WscTlvBase.lib\
	WscCmd/WscCmd.lib \
	-static -lpthread -lc -lssl -lcrypto

WSC_CFG_OBJ = WscCmd/WscCfg.o

default: 
	cd trace && $(MAKE)
	cd slist && $(MAKE)
	cd Portability && $(MAKE)
	cd TlvBase && $(MAKE)
	cd UdpLib && $(MAKE)
	cd InbEap && $(MAKE)
	cd InbIp && $(MAKE)
	cd InbWlan && $(MAKE)
	cd UPnPILib && $(MAKE)
	cd UPnPCpLib && $(MAKE)
	cd InbUPnPCp && $(MAKE)
	cd UPnPDevLib && $(MAKE)
	cd InbUPnPDev && $(MAKE)
	cd Transport && $(MAKE)
	cd StateMachine && $(MAKE)
	cd RegProtocol && $(MAKE)
	cd MasterControl && $(MAKE)
	cd WscCmd && $(MAKE)
	$(CXX) $(OBJS) $(LIBS) $(LIBPATH) -o wsccmd
	$(CROSS)strip wsccmd
	cd WscCmd && $(MAKE) WscCfg
	$(CXX) $(WSC_CFG_OBJ) -static -lc $(LIBPATH) -o wsc_cfg
	$(CROSS)strip wsc_cfg

clean:
	cd trace && $(MAKE) clean
	cd slist && $(MAKE) clean
	cd Portability && $(MAKE) clean
	cd TlvBase && $(MAKE) clean
	cd UdpLib && $(MAKE) clean
	cd InbEap && $(MAKE) clean
	cd InbIp && $(MAKE) clean
	cd InbWlan && $(MAKE) clean
	cd UPnPILib && $(MAKE) clean
	cd UPnPCpLib && $(MAKE) clean
	cd InbUPnPCp && $(MAKE) clean
	cd UPnPDevLib && $(MAKE) clean
	cd InbUPnPDev && $(MAKE) clean
	cd Transport && $(MAKE) clean
	cd StateMachine && $(MAKE) clean
	cd RegProtocol && $(MAKE) clean
	cd MasterControl && $(MAKE) clean
	cd WscCmd && $(MAKE) clean
	rm -f wsccmd
install: default
	cp -f wsccmd $(PREFIX)/wsccmd; \
	cp -f wsc_cfg $(PREFIX)/wsc_cfg
	cp -f wsc_config.txt $(CONFIG_PATH)/wsc_config.txt
	cp -f default_wsc_cfg.txt $(DEFAULT_CFG)/default_wsc_cfg.txt
