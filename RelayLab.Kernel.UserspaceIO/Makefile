# SPDX-License-Identifier: GPL-2.0
# Makefile for the Hyper-V VMBus Userspace I/O drivers.

ifeq ($(KERNELRELEASE), )
KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD :=$(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR)  M=$(PWD) modules
clean:
	rm -rf .tmp_versions Module.symvers *.mod *.mod.c .*.o *.o *.ko .*.cmd Module.markers modules.order
load:
	/sbin/insmod uio.ko
	/sbin/insmod uio_hv_generic.ko
unload:
	/sbin/rmmod uio_hv_generic
	/sbin/rmmod uio
install:
	cp uio.ko /lib/modules/$(shell uname -r)/kernel/drivers/uio/uio.ko
	cp uio_hv_generic.ko /lib/modules/$(shell uname -r)/kernel/drivers/uio/uio_hv_generic.ko
else
	obj-m = uio.o uio_hv_generic.o
endif
