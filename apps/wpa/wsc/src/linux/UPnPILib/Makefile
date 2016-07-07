#Linux Makefile

ARCHIVE  = UPnPILib.lib
OBJS = ILibAsyncServerSocket.o \
       ILibAsyncSocket.o \
       ILibAsyncUDPSocket.o \
       ILibParsers.o \
       ILibWebClient.o \
       ILibWebServer.o

INCLUDE += -I. -I../../common/inc

CFLAGS += $(INCLUDE)

CFLAGS += -D_POSIX

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS)

ILibAsyncServerSocket.o: ../../common/UPnPILib/ILibAsyncServerSocket.c ../../common/inc/ILibAsyncServerSocket.h
	$(CC) -c $(CFLAGS) -o ILibAsyncServerSocket.o ../../common/UPnPILib/ILibAsyncServerSocket.c

ILibAsyncSocket.o: ../../common/UPnPILib/ILibAsyncSocket.c ../../common/inc/ILibAsyncSocket.h
	$(CC) -c $(CFLAGS) -o ILibAsyncSocket.o ../../common/UPnPILib/ILibAsyncSocket.c

ILibAsyncUDPSocket.o: ../../common/UPnPILib/ILibAsyncUDPSocket.c ../../common/inc/ILibAsyncUDPSocket.h
	$(CC) -c $(CFLAGS) -o ILibAsyncUDPSocket.o ../../common/UPnPILib/ILibAsyncUDPSocket.c

ILibParsers.o: ../../common/UPnPILib/ILibParsers.c ../../common/inc/ILibParsers.h
	$(CC) -c $(CFLAGS) -o ILibParsers.o ../../common/UPnPILib/ILibParsers.c

ILibWebClient.o: ../../common/UPnPILib/ILibWebClient.c ../../common/inc/ILibWebClient.h
	$(CC) -c $(CFLAGS) -o ILibWebClient.o ../../common/UPnPILib/ILibWebClient.c

ILibWebServer.o: ../../common/UPnPILib/ILibWebServer.c ../../common/inc/ILibWebServer.h
	$(CC) -c $(CFLAGS) -o ILibWebServer.o ../../common/UPnPILib/ILibWebServer.c

clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

