#Linux Makefile

ARCHIVE  = slist.lib
OBJS = slist.o

INCLUDE += -I. -I../../common/inc

CFLAGS += $(INCLUDE)

default: $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS)

slist.o: ../../common/slist/slist.c ../../common/inc/slist.h
	$(CC) -c $(CFLAGS) -o slist.o ../../common/slist/slist.c

clean:
	rm -f $(OBJS)
	rm -f $(ARCHIVE)

debug: default

