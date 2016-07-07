#Linux Makefile

ARCHIVE  = UdpLib.lib
OBJS = UdpLib.o

INCLUDE += -I. -I../../common/inc

CXXFLAGS += $(INCLUDE)

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS)

UdpLib.o: ../../common/UdpLib/UdpLib.cpp ../../common/inc/UdpLib.h
	$(CXX) -c $(CXXFLAGS) -o UdpLib.o ../../common/UdpLib/UdpLib.cpp

clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

