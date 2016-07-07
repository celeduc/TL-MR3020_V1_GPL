#Linux Makefile

ARCHIVE = InbWlan.lib
OBJS = InbWlan.o

INCLUDE += -I. -I../../common/inc -I../inc 
CXXFLAGS += $(INCLUDE)

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) 

InbWlan.o: ../../common/InbWlan/InbWlan.cpp ../../common/inc/InbWlan.h 
	$(CXX) -c $(CXXFLAGS) -o InbWlan.o ../../common/InbWlan/InbWlan.cpp

clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

