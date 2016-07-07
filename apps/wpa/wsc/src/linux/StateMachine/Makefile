#Linux Makefile

ARCHIVE = StateMachine.lib 
OBJ1 = StateMachine.o 
OBJ2 = RegistrarSM.o 
OBJ3 = EnrolleeSM.o

COMMON = ../../common
COMINC = $(COMMON)/inc/
COMSRC = $(COMMON)/StateMachine/

INCLUDE += -I. -I../../common/inc/ -I../inc -I../StateMachine
CXXFLAGS += $(INCLUDE) -D__LINUX 

default: $(OBJ1) $(OBJ2) $(OBJ3)
	$(LD) -r -o $(ARCHIVE) $(OBJ1) $(OBJ2) $(OBJ3)

StateMachine.o: $(COMSRC)StateMachine.cpp $(COMINC)StateMachine.h
	$(CXX) -c $(CXXFLAGS) -o StateMachine.o $(COMSRC)StateMachine.cpp

RegistrarSM.o: $(COMSRC)RegistrarSM.cpp $(COMINC)StateMachine.h
	$(CXX) -c $(CXXFLAGS) -o RegistrarSM.o $(COMSRC)RegistrarSM.cpp

EnrolleeSM.o: $(COMSRC)EnrolleeSM.cpp $(COMINC)StateMachine.h
	$(CXX) -c $(CXXFLAGS) -o EnrolleeSM.o $(COMSRC)EnrolleeSM.cpp

clean:
	rm -f $(OBJ1)
	rm -f $(OBJ2)
	rm -f $(OBJ3)
	rm -f $(ARCHIVE)

debug: default

