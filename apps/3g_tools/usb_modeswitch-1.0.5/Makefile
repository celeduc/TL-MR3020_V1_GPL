PROG        = usb_modeswitch
VERS        = 1.0.4
STRIP	    = mips-linux-strip
CC          = mips-linux-gcc
CCFLAGS     = -I../install_libusb_mips/include/ -L../install_libusb_mips/lib/ -lusb
RM          = /bin/rm -f
OBJS        = usb_modeswitch.c
PREFIX	    = $(DSTDIR)/usr
ETCDIR	    = $(DSTDIR)/etc
SBINDIR	    = $(PREFIX)/sbin

.PHONY:     clean
.PHONY:     all
all:        $(PROG)
$(PROG):    $(OBJS)
	      $(CC) $(CCFLAGS) -o $(PROG) $(OBJS)
	      $(STRIP) $(PROG)

clean:
	      $(RM) ./usb_modeswitch

install: all
	mkdir -p $(SBINDIR)
	install ./usb_modeswitch $(SBINDIR)
	mkdir -p $(ETCDIR)
	install --mode=644 ./usb_modeswitch.conf $(ETCDIR)

uninstall: clean
	$(RM) /usr/sbin/usb_modeswitch
	$(RM) /etc/usb_modeswitch.conf

.PHONY:     clean install uninstall

integrated:
	make -f Makefile.integrated

integrated_install:
	make -f Makefile.integrated install

