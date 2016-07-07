#Linux Makefile

ARCHIVE  = UPnPDevLib.lib
OBJS = UPnPMicroStack.o

INCLUDE += -I. -I../../common/inc

CFLAGS += $(INCLUDE)

CFLAGS += -D_POSIX

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS)

UPnPMicroStack.o: ../../common/UPnPDevLib/UPnPMicroStack.c ../../common/inc/UPnPMicroStack.h
	$(CC) -c $(CFLAGS) -o UPnPMicroStack.o ../../common/UPnPDevLib/UPnPMicroStack.c

clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

