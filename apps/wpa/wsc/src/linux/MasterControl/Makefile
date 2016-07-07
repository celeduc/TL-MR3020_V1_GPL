#Linux Makefile

ARCHIVE = MasterControl.lib
OBJS = MasterControl.o Info.o

INCLUDE += -I. -I../../common/MasterControl/ -I../../common/inc/ -I../inc -I../../StateMachine
CXXFLAGS += $(INCLUDE)

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) 

MasterControl.o: ../../common/MasterControl/MasterControl.cpp 
	$(CXX) -c $(CXXFLAGS) -o MasterControl.o ../../common/MasterControl/MasterControl.cpp 

Info.o: ../../common/MasterControl/Info.cpp 
	$(CXX) -c $(CXXFLAGS) -o Info.o ../../common/MasterControl/Info.cpp 

clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

