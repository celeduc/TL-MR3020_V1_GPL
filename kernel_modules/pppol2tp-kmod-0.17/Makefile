ifneq ($(KERNELRELEASE),)
	obj-m := pppol2tp.o
	
else
	KERNELDIR ?= /usr/src/redhat/BUILD/kernel-2.6.9/linux-2.6.9
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

clean:
	rm -f pppol2tp.tmp* pppol2tp.ksyms
	rm -fr *.mod.c .*cmd *.o *.ko .tmp_versions
