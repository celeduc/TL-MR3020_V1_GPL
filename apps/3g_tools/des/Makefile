# $Id: Makefile,v 1.40 2006/02/13 03:06:25 quozl Exp $
VERSION=0.9.0
RELEASE=

CC = gcc
RM	= rm -f
CFLAGS  = -Wall -Wno-trigraphs -O2 -fomit-frame-pointer

DES_BIN = des

DES_OBJS = des_min_enc.o des.o 
DES_DEPS = des_min_enc.h des_min_inc.h
MERGE_OBJ = merge_config_files.o log/log.o link/link.o des_min_enc.o

all: $(DES_BIN)

$(DES_BIN): $(DES_OBJS) $(DES_DEPS) $(MERGE_OBJ)
	$(RM) *.O $(DES_BIN) && \
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(DES_BIN) $(DES_OBJS) $(LIBS)
	$(CC) -o sort $(MERGE_OBJ) $(CFLAGS)	

.PHONY: clean
# [pangxing] Fix rm conflict.
clean:
	find . -name "*.o" | xargs rm -rf
	find . -name "*.swo" | xargs rm -rf
	find . -name "*.lst" | xargs rm -rf
	find . -name "*.bin" | xargs rm -rf
	$(RM) *.o $(DES_BIN) 
	if [ -e sort ]; then rm sort; fi
