.PHONY: uboot kernel driver

none:help

help:
	@printf "\n"
	@printf "make uboot .\n"
	@printf "make kernel .\n"
	@printf "make driver .\n"
	@printf "make install .\n"
	@printf "make all .\n"
	@printf "make clean .\n"

all: uboot kernel driver install

uboot:
	@if [ ! -d uboot ] ; then tar -xvf uboot.tar.gz; fi
	@cp patch/PRJ.h uboot/include/configs/;
	make -C uboot/ stqc_custom;

kernel:
	@if [ ! -d kernel ] ; then tar -xvf kernel.tar.gz;	fi
	@cp patch/defconfig kernel/arch/mips/configs/stqc_defconfig;
	make -C kernel/ stqc_defconfig;
	make -C kernel/ uImage;

driver:
	@if [ ! -d drivers ] ; then		tar -xvf drivers.tar.gz;	fi
	@cp patch/firmware.a drivers/isp/tx-isp/libPRJ008-firmware-310-540.a;
	make -C drivers/ all;

install:
	mkdir -p out/ko;
	@cp uboot/u-boot-with-spl.bin out/uboot.bin;
	@cp kernel/arch/mips/boot/uImage out/kernel.bin;
	@cp rootfs/rootfs.bin out/rootfs.bin;
	@cp drivers/out/* out/ko/


clean: 
	rm -fr kernel
	rm -fr uboot
	rm -fr drivers
	rm -fr out
