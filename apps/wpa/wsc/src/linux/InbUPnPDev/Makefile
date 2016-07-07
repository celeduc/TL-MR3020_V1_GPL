#Linux Makefile

ARCHIVE = InbUPnPDev.lib
OBJS = InbUPnPDev.o

INCLUDE += -I. -I../../common/inc -I../inc 
CXXFLAGS += $(INCLUDE)

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) 

InbUPnPDev.o: ../../common/InbUPnPDev/InbUPnPDev.cpp ../../common/inc/InbUPnPDev.h 
	$(CXX) -c $(CXXFLAGS) -o InbUPnPDev.o ../../common/InbUPnPDev/InbUPnPDev.cpp

clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

