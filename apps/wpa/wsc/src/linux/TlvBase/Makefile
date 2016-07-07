#Linux Makefile

ARCHIVE = WscTlvBase.lib 
OBJ1 = WscTlvBase.o 
OBJ2 = RegProtoTlv.o 

COMMON = ../../common
COMINC = $(COMMON)/inc/
COMSRC = $(COMMON)/TlvBase/

INCLUDE += -I. -I../../common/inc/ -I../inc -I../TlvBase
CXXFLAGS += $(INCLUDE)

default: $(OBJ1) $(OBJ2)
	$(LD) -r -o $(ARCHIVE) $(OBJ1) $(OBJ2)

WscTlvBase.o: $(COMSRC)WscTlvBase.cpp 
	$(CXX) -c $(CXXFLAGS) -o WscTlvBase.o $(COMSRC)WscTlvBase.cpp

RegProtoTlv.o: $(COMSRC)RegProtoTlv.cpp 
	$(CXX) -c $(CXXFLAGS) -o RegProtoTlv.o $(COMSRC)RegProtoTlv.cpp

clean:
	rm -f $(OBJ1)
	rm -f $(OBJ2)
	rm -f $(ARCHIVE)

debug: default

