#Linux Makefile

ARCHIVE  = UPnPCpLib.lib
OBJS = ILibSSDPClient.o UPnPControlPoint.o

INCLUDE += -I. -I../../common/inc

CFLAGS += $(INCLUDE)

CFLAGS += -D_POSIX

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS)

ILibSSDPClient.o: ../../common/UPnPCpLib/ILibSSDPClient.c ../../common/inc/ILibSSDPClient.h
	$(CC) -c $(CFLAGS) -o ILibSSDPClient.o ../../common/UPnPCpLib/ILibSSDPClient.c

UPnPControlPoint.o: ../../common/UPnPCpLib/UPnPControlPoint.c ../../common/inc/UPnPControlPoint.h
	$(CC) -c $(CFLAGS) -o UPnPControlPoint.o ../../common/UPnPCpLib/UPnPControlPoint.c

clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

