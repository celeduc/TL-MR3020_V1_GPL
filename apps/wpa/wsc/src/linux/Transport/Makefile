#Linux Makefile

ARCHIVE = Transport.lib
OBJS = Transport.o

INCLUDE += -I. -I../../common/inc -I../inc 
CXXFLAGS += $(INCLUDE)

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) 

Transport.o: ../../common/Transport/Transport.cpp ../../common/inc/Transport.h ../../common/inc/TransportBase.h
	$(CXX) -c $(CXXFLAGS) -o Transport.o ../../common/Transport/Transport.cpp

clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

