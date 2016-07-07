#Linux Makefile

ARCHIVE = WscCmd.lib
OBJS = WscCmd.o Led.o

INCLUDE += -I. -I../../common/WscCmd/ -I../../common/MasterControl/ -I../../common/inc/ -I../inc -I../../StateMachine
CXXFLAGS += $(INCLUDE)

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) 

#WscCmd.o: WscCmdL.cpp 
WscCmd.o: ../../common/WscCmd/WscCmd.cpp 
	$(CXX) -c $(CXXFLAGS) -o WscCmd.o ../../common/WscCmd/WscCmd.cpp 
Led.o: ../../common/WscCmd/Led.cpp 
	$(CXX) -c $(CXXFLAGS) -o Led.o ../../common/WscCmd/Led.cpp 

WscCfg: ../../common/WscCmd/WscCfg.cpp 
	$(CXX) -c $(CXXFLAGS) -o WscCfg.o ../../common/WscCmd/WscCfg.cpp 
clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

