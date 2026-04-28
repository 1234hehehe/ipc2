

# <定义当前的路径>-----------------------------------------------------
CUR_PATH=$(shell pwd)
export ROOT_PATH?=${CUR_PATH}/..
# </定义当前的路径>-----------------------------------------------------

# 包含Makefile.pre.mak.
#$(warning include ${ROOT_PATH}/Makefile.pre.mak)
include ${ROOT_PATH}/Makefile.pre.mak

# 根据规则推测要引用的打包脚本.
INCLUDE_SCRIPT_1=$(shell echo build/${CUSTOMER_NAME}.${PROJECT_EIGEN}.${CPUFAMILY_NAME}.${HARDWARE_NAME}.image.inc | tr [A-Z] [a-z])

INCLUDE_SCRIPT_2=$(shell echo build/${CUSTOMER_NAME}.${PROJECT_EIGEN}.${CPUFAMILY_NAME}.defhw.image.inc | tr [A-Z] [a-z])
INCLUDE_SCRIPT_3=$(shell echo build/${CUSTOMER_NAME}.defproj.${CPUFAMILY_NAME}.${HARDWARE_NAME}.image.inc | tr [A-Z] [a-z])
INCLUDE_SCRIPT_4=$(shell echo build/defcust.${PROJECT_EIGEN}.${CPUFAMILY_NAME}.${HARDWARE_NAME}.image.inc | tr [A-Z] [a-z])

INCLUDE_SCRIPT_5=$(shell echo build/${CUSTOMER_NAME}.defproj.${CPUFAMILY_NAME}.defhw.image.inc | tr [A-Z] [a-z])
INCLUDE_SCRIPT_6=$(shell echo build/defcust.${PROJECT_EIGEN}.${CPUFAMILY_NAME}.defhw.image.inc | tr [A-Z] [a-z])
INCLUDE_SCRIPT_7=$(shell echo build/defcust.defproj.${CPUFAMILY_NAME}.${HARDWARE_NAME}.image.inc | tr [A-Z] [a-z])

INCLUDE_SCRIPT_8=$(shell echo build/defcust.defproj.${CPUFAMILY_NAME}.defhw.image.inc | tr [A-Z] [a-z])

INCLUDE_SCRIPT_9=$(shell echo build/defcust.defproj.${PLATFORM_NAME}.defhw.image.inc | tr [A-Z] [a-z])

EXIST_SCRIPT_ID:=0

ifeq (${EXIST_SCRIPT_ID}, 0)
ifeq ($(shell test -f ${INCLUDE_SCRIPT_1} && echo true), true)
EXIST_SCRIPT_ID=1
INCLUDE_SCRIPT:=${INCLUDE_SCRIPT_1}
else 
$(warning No such script ${INCLUDE_SCRIPT_1})
endif
endif

ifeq (${EXIST_SCRIPT_ID}, 0)
ifeq ($(shell test -f ${INCLUDE_SCRIPT_2} && echo true), true)
EXIST_SCRIPT_ID=2
INCLUDE_SCRIPT:=${INCLUDE_SCRIPT_2}
else 
$(warning No such script ${INCLUDE_SCRIPT_2})
endif
endif

ifeq (${EXIST_SCRIPT_ID}, 0)
ifeq ($(shell test -f ${INCLUDE_SCRIPT_3} && echo true), true)
EXIST_SCRIPT_ID=3
INCLUDE_SCRIPT:=${INCLUDE_SCRIPT_3}
else 
$(warning No such script ${INCLUDE_SCRIPT_3})
endif
endif

ifeq (${EXIST_SCRIPT_ID}, 0)
ifeq ($(shell test -f ${INCLUDE_SCRIPT_4} && echo true), true)
EXIST_SCRIPT_ID=4
INCLUDE_SCRIPT:=${INCLUDE_SCRIPT_4}
else 
$(warning No such script ${INCLUDE_SCRIPT_4})
endif
endif

ifeq (${EXIST_SCRIPT_ID}, 0)
ifeq ($(shell test -f ${INCLUDE_SCRIPT_5} && echo true), true)
EXIST_SCRIPT_ID=5
INCLUDE_SCRIPT:=${INCLUDE_SCRIPT_5}
else 
$(warning No such script ${INCLUDE_SCRIPT_5})
endif
endif

ifeq (${EXIST_SCRIPT_ID}, 0)
ifeq ($(shell test -f ${INCLUDE_SCRIPT_6} && echo true), true)
EXIST_SCRIPT_ID=6
INCLUDE_SCRIPT:=${INCLUDE_SCRIPT_6}
else 
$(warning No such script ${INCLUDE_SCRIPT_6})
endif
endif

ifeq (${EXIST_SCRIPT_ID}, 0)
ifeq ($(shell test -f ${INCLUDE_SCRIPT_7} && echo true), true)
EXIST_SCRIPT_ID=7
INCLUDE_SCRIPT:=${INCLUDE_SCRIPT_7}
else 
$(warning No such script ${INCLUDE_SCRIPT_7})
endif
endif

ifeq (${EXIST_SCRIPT_ID}, 0)
ifeq ($(shell test -f ${INCLUDE_SCRIPT_8} && echo true), true)
EXIST_SCRIPT_ID=8
INCLUDE_SCRIPT:=${INCLUDE_SCRIPT_8}
else 
$(warning No such script ${INCLUDE_SCRIPT_8})
endif
endif

ifeq (${EXIST_SCRIPT_ID}, 0)
ifeq ($(shell test -f ${INCLUDE_SCRIPT_9} && echo true), true)
EXIST_SCRIPT_ID=9
INCLUDE_SCRIPT:=${INCLUDE_SCRIPT_9}
else 
$(warning No such script ${INCLUDE_SCRIPT_9})
endif
endif

ifeq (${EXIST_SCRIPT_ID}, 0)
INCLUDE_SCRIPT:=build/dummy.appfs.inc
else
CUSTOMER_EIGEN=$(shell basename "${INCLUDE_SCRIPT}" | cut -d '.' -f 1)
PROJECT_EIGEN=$(shell basename "${INCLUDE_SCRIPT}" | cut -d '.' -f 2)
CPUFAMILY_EIGEN=$(shell basename "${INCLUDE_SCRIPT}" | cut -d '.' -f 3)
HARDWARE_EIGEN=$(shell basename "${INCLUDE_SCRIPT}" | cut -d '.' -f 4)
endif

ifeq ($(shell test -f ${INCLUDE_SCRIPT} && echo true), true)
$(warning include ${INCLUDE_SCRIPT})
include ${INCLUDE_SCRIPT}

ifeq ($(shell test -f Makefile.nandvar.mak && echo true), true)
include Makefile.nandvar.mak
endif
endif


.PHONY: 

none:printenv

printenv:
	@printf "CPUFAMILY_NAME=${CPUFAMILY_NAME}\n"
	@printf "PLATFORM_NAME=${PLATFORM_NAME}\n"
	@printf "COMPILER_NAME=${COMPILER_NAME}\n"
	@printf "SDKVERSION_NAME=${SDKVERSION_NAME}\n"
	@printf "HARDWARE_NAME=${HARDWARE_NAME}\n"
	@printf "HWID_NAME=${HWID_NAME}\n"
	@printf "CUSTOMER_NAME=${CUSTOMER_NAME}\n"
	@printf "SHORTNAME_NAME=${SHORTNAME_NAME}\n"
	@printf "PROJECT_NAME=${PROJECT_NAME}\n"
	@printf "SENSORMODEL_NAME=${SENSORMODEL_NAME}\n"

help:
	@printf "\n"
	@printf "make all \n\t Start packaging the version by one command.\n"
	@printf "make offline \n\t Package version without updating files from server.\n"

testconfig:
	@if [ -z "${PLATFORM_NAME}" -o -z "${SDKVERSION_NAME}" -o -z "${HARDWARE_NAME}" -o -z "${CUSTOMER_NAME}" ] ; then \
	make menuconfig; echo "Stoped. Please 'make' once more."; exit 1; \
	fi

openup: 
	@rm -rf build && mkdir -p build && ${GIT_EXPORT} ${GIT_LBUILDP_PATH}/image/build build && ${GIT_EXPORT} ${GIT_LBUILDP_PATH}/image/Makefile.nandvar.mak Makefile.nandvar.mak;
	
distclean: 
	@echo "Cleaning ";
	@rm -rf fs;

	
# 被引用的打包脚本需要按照Makefile语法实现这些目标.
# update_fs -- 更新文件系统中的文件.
# mkimg_fs -- 制作文件系统的镜像文件.

all: update_fs mkimg_fs archive

offline: mkimg_fs archive

static: mkimg_fs archive

archive:
ifneq (${EXIST_SCRIPT_ID}, 0)
	@mkdir -p ${RELEASE_COPY_PATH};\
	cd ${ROOT_PATH}; \
	zip -qyr ${RELEASE_COPY_ARCHIVE} .config build.config Makefile Makefile.pre.mak build image/Makefile image/Makefile.nandvar.mak image/${INCLUDE_SCRIPT};\
	[ -d image/fs ] && zip -qyr ${RELEASE_COPY_ARCHIVE} image/fs/*.bin; \
	cd -;
endif
	
