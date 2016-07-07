#Linux Makefile

ARCHIVE = InbUPnPCp.lib
OBJS = InbUPnPCp.o

INCLUDE += -I. -I../../common/inc -I../inc 
CXXFLAGS += $(INCLUDE)

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) 

InbUPnPCp.o: ../../common/InbUPnPCp/InbUPnPCp.cpp ../../common/inc/InbUPnPCp.h 
	$(CXX) -c $(CXXFLAGS) -o InbUPnPCp.o ../../common/InbUPnPCp/InbUPnPCp.cpp

clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

