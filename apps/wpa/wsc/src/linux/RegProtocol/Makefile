#Linux Makefile

ARCHIVE  = RegProtocol.lib
OBJS1 = RegProtocol.o  
OBJS2 = ProtoUtils.o 
OBJS3 = RegProtoMsgs.o 

COMMON = ../../common
COMINC = $(COMMON)/inc/
COMSRC = $(COMMON)/RegProtocol/

INCLUDE += -I. -I../inc -I../../common/inc -I/usr/include

CXXFLAGS += $(INCLUDE) 

default: $(OBJS2) $(OBJS1) $(OBJS3)
	$(LD) -r -o $(ARCHIVE) $(OBJS1) $(OBJS2) $(OBJS3)

RegProtocol.o: $(COMSRC)RegProtocol.cpp 
	$(CXX) -c $(CXXFLAGS) -o RegProtocol.o $(COMSRC)RegProtocol.cpp

ProtoUtils.o: $(COMSRC)ProtoUtils.cpp 
	$(CXX) -c $(CXXFLAGS) -o ProtoUtils.o $(COMSRC)ProtoUtils.cpp

RegProtoMsgs.o: $(COMSRC)RegProtoMsgs.cpp 
	$(CXX) -c $(CXXFLAGS) -o RegProtoMsgs.o $(COMSRC)RegProtoMsgs.cpp

clean:
	rm -f $(OBJS1)
	rm -f $(OBJS2)
	rm -f $(OBJS3)
	rm -f $(ARCHIVE)

debug: default

