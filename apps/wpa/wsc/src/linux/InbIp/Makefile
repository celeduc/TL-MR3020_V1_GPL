#Linux Makefile

ARCHIVE = InbIp.lib
OBJS = InbIp.o

INCLUDE += -I. -I../../common/inc/ -I../inc 
CXXFLAGS += $(INCLUDE)

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) 

InbIp.o: InbIp.cpp ../../common/inc/InbIp.h 
	$(CXX) -c $(CXXFLAGS) -o InbIp.o InbIp.cpp

clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

