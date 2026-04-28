export ROOT := $(shell pwd)

.PHONY: bsp app build

none:help

help:
	@printf "\n"
	@printf "make bsp : making uboot kernel rootfs and drivers.\n"
	@printf "make app : making software application.\n"
	@printf "make build : packaging upgrade file and flash bin.\n"
	@printf "make install : generate release file.\n"
	@printf "make all .\n"
	@printf "make clean .\n"

all: bsp app build install

bsp:
	make -C bsp all;

app:
	make -C appsrc;

build:
	ROOT=$(pwd)
	echo "${ROOT}"
	make -C build all;

install:
	@cp -a build/release .


clean: 
	make -C bsp clean;
	make -C appsrc clean;
	make -C build clean;
	rm -fr release
