#Linux Makefile

ARCHIVE = InbEap.lib
OBJS = InbEap.o

#INCLUDE += -I. -I../../common/inc -I../inc 
INCLUDE += -I. -I../../common/MasterControl/ -I../../common/inc/ -I../inc -I../../StateMachine

CXXFLAGS += $(INCLUDE)

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) 

InbEap.o: ../../common/InbEap/InbEap.cpp ../../common/inc/InbEap.h 
	$(CXX) -c $(CXXFLAGS) -o InbEap.o ../../common/InbEap/InbEap.cpp

clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

