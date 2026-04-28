

# <定义当前的路径>-----------------------------------------------------
CUR_PATH=$(shell pwd)
export ROOT_PATH?=${CUR_PATH}
# </定义当前的路径>-----------------------------------------------------

# 包含Makefile.pre.mak.
include ${ROOT_PATH}/Makefile.pre.mak


.PHONY: appfs image

none:printenv

printenv:
	@printf "CPUFAMILY_NAME=${CPUFAMILY_NAME}\n"
	@printf "PLATFORM_NAME=${PLATFORM_NAME}\n"
	@printf "COMPILER_NAME=${COMPILER_NAME}\n"
	@printf "SDKVERSION_NAME=${SDKVERSION_NAME}\n"
	@printf "HARDWARE_NAME=${HARDWARE_NAME}\n"
	@printf "HWID_NAME=${HWID_NAME}\n"
	@printf "FLASHMEDIA_NAME=${FLASHMEDIA_NAME}\n"
ifeq (${FLASHMEDIA_NAME},NAND)
	@printf "FLASHBSK_NAME=${FLASHBSK_NAME}\n"
	@printf "FLASHPSK_NAME=${FLASHPSK_NAME}\n"
	@printf "FLASHSSB_NAME=${FLASHSSB_NAME}\n"
endif
	@printf "CUSTOMER_NAME=${CUSTOMER_NAME}\n"
	@printf "PROJECT_NAME=${PROJECT_NAME}\n"
	@printf "SENSORMODEL_NAME=${SENSORMODEL_NAME}\n"
	@printf "RLF_UPDATE=${RLF_UPDATE}\n"
	@printf "RLF_FLASH=${RLF_FLASH}\n"

help:
	@printf "\n"
	@printf "make pull \n\t Pull files from remote git server.\n"
	@printf "make openup \n\t Download or update scripts for packaging version.\n"
	@printf "make menuconfig \n\t Set configurations for this version manually.\n"
	@printf "make all \n\t Start packaging the version by one command.\n"
	@printf "make appfs \n\t Prepare application file system.\n"
	@printf "make appfs_offline \n\t Prepare application file system without updating files from server.\n"
	@printf "make image \n\t Prepare all file system, in flash and update package.\n"
	@printf "make image_offline \n\t Prepare all file system without updating files from server.\n"
	@printf "make offline \n\t Package version without updating files from server.\n"

all: release_distclean appfs image

clean: release_distclean image_distclean appfs_distclean
	@rm -fr release;

testconfig:
	@if [ -z "${PLATFORM_NAME}" -o -z "${SDKVERSION_NAME}" -o -z "${HARDWARE_NAME}" -o -z "${CUSTOMER_NAME}" ] ; then \
	make menuconfig; echo "Stoped. Please 'make' once more."; exit 1; \
	fi

menuconfig:
	@if [ ! -f build/bin/mconf ] ; then mkdir -p build && ${GIT_EXPORT} ${GIT_LBUILDP_PATH}/build build; fi;
	@if [ -d build/bin ] ; then chmod a+x build/bin/*; fi;
	@if [ -f build.config ] ; then cp -af build.config .config; fi;
	@./build/bin/mconf ./build/menu/Config.in && cp .config build.config;

openup: self_openup appfs_openup image_openup release_openup build_openup

pull:
	@echo -e "\n\n===================================================================" && \
	cd ${GIT_LSERVER}/ovfs_build && pwd && git pull && git log -3 && cd - > /dev/null;
	@echo -e "\n\n===================================================================" && \
	cd ${GIT_LSERVER}/ovfs_lowersys && pwd && git pull && git log -3 && cd - > /dev/null
	@echo -e "\n\n===================================================================" && \
	cd ${GIT_LSERVER}/ovfs_uppersys && pwd && git pull && git log -3 && cd - > /dev/null;
	@echo -e "\n\n===================================================================" && \
	cd ${GIT_LSERVER}/ovfs_thirdpart && pwd && git pull && git log -3 && cd - > /dev/null;
	@echo -e "\n\n===================================================================" && \
	cd ${GIT_LSERVER}/ovfs_pcend && pwd && git pull && git log -3 && cd - > /dev/null;
	
self_openup:
	${GIT_EXPORT} ${GIT_LBUILDP_PATH}/Makefile Makefile;
	${GIT_EXPORT} ${GIT_LBUILDP_PATH}/Makefile.pre.mak Makefile.pre.mak;
	@rm -rf build && mkdir -p build && ${GIT_EXPORT} ${GIT_LBUILDP_PATH}/build build;
	@chmod a+x -R build/bin;
	
appfs_openup:
	mkdir -p ${APPFS_PATH} && ${GIT_EXPORT} ${GIT_LBUILDP_PATH}/appfs/Makefile appfs/Makefile && make CUSTOMER_NAME=DEFAULT -C appfs openup;
	
image_openup:
	mkdir -p ${IMAGE_PATH} && ${GIT_EXPORT} ${GIT_LBUILDP_PATH}/image/Makefile image/Makefile && make -C image openup;
	
release_openup:
	mkdir -p ${RELEASE_PATH}
	
build_openup:
	@if [ ! -f build/bin/mconf ] ; then mkdir -p build && ${GIT_EXPORT} ${GIT_LBUILDP_PATH}/build build; fi;
	@if [ -d build/bin ] ; then chmod a+x build/bin/*; fi;
	
distclean: image_distclean appfs_distclean
	rm -rf ${CONFIG}.old;

offline:appfs_offline image_offline
	
appfs: testconfig printenv
	make -C ${APPFS_PATH} all
	
appfs_distclean:
	make -C ${APPFS_PATH} distclean
	
appfs_offline: testconfig printenv
	make -C ${APPFS_PATH} offline
	
update_app_version: testconfig
	make -C ${APPFS_PATH} update_app_version
	
image: testconfig printenv
	make -C ${IMAGE_PATH} all 

image_offline: testconfig printenv
	make -C ${IMAGE_PATH} offline

image_distclean:
	make -C ${IMAGE_PATH} distclean

release_distclean:
	@rm -rf ${RELEASE_COPY_PATH}/archive.zip ${RELEASE_COPY_PATH}/*.update ${RELEASE_COPY_PATH}/*.flash*

