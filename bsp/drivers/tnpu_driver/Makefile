
KERNELDIR := ../../kernel

obj-m := tnpu.o

tnpu-objs := \
		tnpu_core.o \
		tnpu_mem_bo.o \
		tnpu_drv.o \


all default: modules
install: modules_install

modules modules_install help clean:
	$(MAKE) -C $(KERNELDIR) M=$(shell pwd) $@
