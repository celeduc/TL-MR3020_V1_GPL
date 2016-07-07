# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.

ifeq ($(KERNELPATH), ) 
	KERNELPATH := ../../linux 
else 

endif

obj-m += tp_domain.o

PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELPATH) M=$(PWD) modules

.PHONY:clean
clean:
	$(MAKE) -C $(KERNELPATH) M=$(PWD) clean
