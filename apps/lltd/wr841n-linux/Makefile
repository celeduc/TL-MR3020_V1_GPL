VPATH = ../src

#CC = gcc
#LD = gcc
#INC_PATH = ../../build/gcc-3.4.4-2.16.1/build_mips/include
#CFLAGS += -I$(INC_PATH) -O3 -pipe -mips32 -mtune=mips32 -Wall -Wno-unused -s -g -fpack-struct -Wpadded
#LDFLAGS = -L../../rootfs.build/lib

OS_LAYER = osl-linux.c

all: lld2d lld2test

include ../src/common.mk

depend:
	$(CC) $(CFLAGS) -M $(DCFILES) >.depend
	$(CC) $(CFLAGS) -M $(TCFILES) >>.depend

-include .depend
