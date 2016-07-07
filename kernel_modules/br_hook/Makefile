ifneq ($(KERNELRELEASE),)
	obj-m := br_filter.o
	
else
	KERNELDIR ?= /usr/src/redhat/BUILD/kernel-2.6.9/linux-2.6.9
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif
