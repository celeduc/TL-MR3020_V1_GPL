#Linux Makefile

ARCHIVE = OobUfd.lib
OBJS = OobUfd.o

INCLUDE += -I. -I../../common/inc/ -I../inc 
CXXFLAGS += $(INCLUDE)

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) 

OobUfd.o: OobUfd.cpp ../../common/inc/OobUfd.h 
	$(CXX) -c $(CXXFLAGS) -o OobUfd.o OobUfd.cpp

clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

