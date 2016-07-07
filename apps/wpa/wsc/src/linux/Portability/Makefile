#Linux Makefile

ARCHIVE = Portability.lib 
OBJ1 = Portability.o 

COMMON = ../../common
COMINC = $(COMMON)/inc/

INCLUDE += -I. -I../../common/inc/ -I../inc -I../Portability
CFLAGS += $(INCLUDE) -g

default: $(OBJ1) 
	$(LD) -r -o $(ARCHIVE) $(OBJ1) 

Portability.o: Portability.c 
	$(CC) -c $(CFLAGS) -o Portability.o Portability.c

clean:
	rm -f $(OBJ1)
	rm -f $(ARCHIVE)

debug: default

