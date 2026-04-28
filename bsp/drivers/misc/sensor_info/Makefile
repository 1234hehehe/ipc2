CROSS_COMPILE ?= mips-t33-linux-uclibc-gnu-

ifeq (${TASSADAR_ENV_KERNEL_DIR}, )
	TASSADAR_ENV_KERNEL_DIR = $(PWD)/../../../kernel
endif
KDIR := ${TASSADAR_ENV_KERNEL_DIR}
OPENSOURCE_DIR = $(TASSADAR_ENV_OPENSOURCE_DIR)

MODULE_NAME := sinfo

all: modules

.PHONY: modules clean

$(MODULE_NAME)-objs := sensor_info.o
obj-m := $(MODULE_NAME).o

modules:
	@if test "$(KDIR)" = ""; \
	then \
		echo "**************************************"; \
		echo "Please run env_setup.sh at top of isvp"; \
		echo "**************************************"; \
	fi
	@$(MAKE) -C $(KDIR) M=$(shell pwd) $@
sample_sinfo:
	mips-linux-uclibc-gnu-gcc sample_sinfo.c -o sample_sinfo
clean:
	@rm -rf *.o *~ .depend .*.cmd  *.mod *.mod.c .tmp_versions *.ko *.symvers modules.order
	@rm -f sample_sinfo
