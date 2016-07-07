ifneq ($(KERNELRELEASE),)
	obj-m := pptp.o
	
else
	KERNELDIR ?= /usr/src/redhat/BUILD/kernel-2.6.9/linux-2.6.9
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

clean:
	rm -f pptp.tmp* pptp.ksyms
	rm -fr *.mod.c .*cmd *.o *.ko .tmp_versions