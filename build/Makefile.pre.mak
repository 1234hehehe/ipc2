
# <定义时间>-----------------------------------------------------
export buildyear?=$(shell date +%Y)
export buildmonth?=$(shell date +%m)
export buildday?=$(shell date +%d)
export buildhour?=$(shell date +%H)
export buildmin?=$(shell date +%M)
export buildsec?=$(shell date +%S)
# </定义时间>-----------------------------------------------------

# <定义一般环境变量>-----------------------------------------------------
export CONFIG?=${ROOT_PATH}/build.config
ifeq ($(shell test -f ${CONFIG} && echo true), true)
export CPUFAMILY_NAME?=$(shell grep "IPC_CPUFAMILY=" ${CONFIG} | cut -d '"' -f 2)
export PLATFORM_NAME?=$(shell grep "IPC_PLATFORM=" ${CONFIG} | cut -d '"' -f 2)
export COMPILER_NAME?=$(shell grep "IPC_COMPILER=" ${CONFIG} | cut -d '"' -f 2)
export SDKVERSION_NAME?=$(shell grep "IPC_SDKVERSION=" ${CONFIG} | cut -d '"' -f 2)
export HARDWARE_NAME?=$(shell grep "IPC_HARDWARE=" ${CONFIG} | cut -d '"' -f 2)
export HWID_NAME?=$(shell grep "IPC_HWID=" ${CONFIG} | cut -d '"' -f 2)
export HWID_NAME1?=$(shell echo ${HWID_NAME} | sed 's/ /_/g')
#export FLASHTSM_NAME?=$(shell grep "IPC_FLASHTSM=" ${CONFIG} | cut -d '"' -f 2)
export FLASHMEDIA_NAME?=$(shell grep "IPC_FLASHMEDIA=" ${CONFIG} | cut -d '"' -f 2)
export FLASHBSK_NAME?=$(shell grep "IPC_FLASHBSK=" ${CONFIG} | cut -d '"' -f 2)
export FLASHPSK_NAME?=$(shell grep "IPC_FLASHPSK=" ${CONFIG} | cut -d '"' -f 2)
export FLASHSSB_NAME?=$(shell grep "IPC_FLASHSSB=" ${CONFIG} | cut -d '"' -f 2)
export CUSTOMER_NAME?=$(shell grep "IPC_CUSTOMER=" ${CONFIG} | cut -d '"' -f 2)
export SHORTNAME_NAME?=$(shell grep "IPC_SHORTNAME=" ${CONFIG} | cut -d '"' -f 2)
export SUBVERSION_NAME?=$(shell grep "IPC_SUBVERSION=" ${CONFIG} | cut -d '"' -f 2)
export PROJECT_NAME?=$(shell grep "IPC_PROJECT=" ${CONFIG} | cut -d '"' -f 2)
export SENSORMODEL_NAME?=$(shell grep "IPC_SENSORMODEL=" ${CONFIG} | cut -d '"' -f 2)
export AUTOLENS_DRIVE?=$(shell grep "IPC_AUTOLENS_DRIVER=" ${CONFIG} | cut -d '"' -f 2)
export AUTOLENS_SUPPORT?=$(shell grep "IPC_AUTOLENS_SUPPORT=" ${CONFIG} | cut -d '"' -f 2)
export AUTOLENS_TYPE?=$(shell grep "IPC_AUTOLENS_MODEL=" ${CONFIG} | cut -d '"' -f 2)
export RLF_UPDATE?=$(shell grep "IPC_RLF_UPDATE=" ${CONFIG} | cut -d '=' -f 2)
export RLF_FLASH?=$(shell grep "IPC_RLF_FLASH=" ${CONFIG} | cut -d '=' -f 2)
endif
# </定义一般环境变量>-----------------------------------------------------

# <定义一般路径名称>-----------------------------------------------------
export BUILDP_PATH?=${ROOT_PATH}/build
export APP_PATH?=${ROOT_PATH}/app
export APPFS_PATH?=${ROOT_PATH}/appfs
export IMAGE_PATH?=${ROOT_PATH}/image
export RELEASE_PATH?=${ROOT_PATH}/release
export RELEASE_COPY_PATH?=${RELEASE_PATH}/${buildyear}${buildmonth}${buildday}-${SHORTNAME_NAME}-$(HWID_NAME1)-${PROJECT_NAME}-${HARDWARE_NAME}-${SENSORMODEL_NAME}
export RELEASE_COPY_ARCHIVE?=${RELEASE_COPY_PATH}/archive.zip
# </定义一般路径名称>-----------------------------------------------------

# <定义一般命令>-----------------------------------------------------
SVN_EXPORT?=svn export --force
GIT_EXPORT?=cp -afT
UPX=${BUILDP_PATH}/bin/upx
MKUPDATE=${BUILDP_PATH}/bin/updatedemo
MKFLASHFS=${BUILDP_PATH}/bin/mkflashfs
MKYAFFS2IMG=${BUILDP_PATH}/bin/mkyaffs2image
MKJFFS2IMG=${BUILDP_PATH}/bin/mkfs.jffs2
MKCRAMFSIMG=${BUILDP_PATH}/bin/mkcramfs
MKSQUASHFSIMG=${BUILDP_PATH}/bin/mksquashfs
MKUBIFS=${BUILDP_PATH}/bin/mkfs.ubifs
MKUBIIMG=${BUILDP_PATH}/bin/ubinize
MKNANDPART=${BUILDP_PATH}/bin/nand_product
OUTPUTFF=${BUILDP_PATH}/bin/outputff.sh

CROSS_COMPILE_PREFIX=${COMPILER_NAME}
CC=$(CROSS_COMPILE_PREFIX)gcc
CXX=$(CROSS_COMPILE_PREFIX)g++
STRIP=$(CROSS_COMPILE_PREFIX)strip
AR=$(CROSS_COMPILE_PREFIX)ar cr
GLINK=$(CROSS_COMPILE_PREFIX)g++
# </定义一般命令>-----------------------------------------------------

# <定义服务器路径>-----------------------------------------------------
export GIT_RSERVER?=http://192.168.0.202/gitlab
export GIT_LSERVER?=/home/public/git
export GIT_RBUILDP_PATH?=${GIT_RSERVER}/ovfs_build.git
export GIT_LBUILDP_PATH?=${GIT_LSERVER}/ovfs_build/build2020

SDKVER_EIGEN=$(shell echo ${SDKVERSION_NAME} | tr [A-Z] [a-z])
COMPILER_EIGEN=$(shell echo ${COMPILER_NAME} | cut -d '-' -f 2)
CPUFAMILY_EIGEN=$(shell echo ${CPUFAMILY_NAME} | tr [A-Z] [a-z])
PLATFORM_EIGEN=$(shell echo ${PLATFORM_NAME} | tr [A-Z] [a-z])
CUSTOMER_EIGEN=$(shell echo ${CUSTOMER_NAME} | tr [A-Z] [a-z])
PROJECT_EIGEN=$(shell echo ${PROJECT_NAME} | tr [A-Z] [a-z])
HARDWARE_EIGEN=$(shell echo ${HARDWARE_NAME} | tr [A-Z] [a-z])
GIT_LOWERSYS_PATH=${GIT_LSERVER}/ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}
#$(warning GIT_LOWERSYS_PATH=${GIT_LOWERSYS_PATH})
GIT_UPPERSYS_PATH=${GIT_LSERVER}/ovfs_uppersys
#$(warning GIT_UPPERSYS_PATH=${GIT_UPPERSYS_PATH})
GIT_THIRDPART_PATH=${GIT_LSERVER}/ovfsthirdpart
#$(warning GIT_THIRDPART_PATH=${GIT_THIRDPART_PATH})
GIT_PCEND_PATH=${GIT_LSERVER}/ovfs_pcend
#$(warning GIT_PCEND_PATH=${GIT_PCEND_PATH})

# </定义服务器路径>-----------------------------------------------------

NORMAL=\e[0;39m
RED=\e[1;31m
GREEN=\e[1;32m
YELLOW=\e[1;33m
BLUE=\e[1;34m
MAGENTA=\e[1;35m
CYAN=\e[1;36m
WHITE=\e[1;37m

define GIT_EXPORT_LOWF
	@[ -d $(shell dirname ${2}) ] || mkdir -p $(shell dirname ${2}); \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${1} ${2} 2>/dev/null && echo -e "  ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${1} -> ${2}") || \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/${CUSTOMER_EIGEN}/defproj/${1} ${2} 2>/dev/null && echo -e "  ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/${CUSTOMER_EIGEN}/defproj/${1} -> ${2}") || \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/defcust/defproj/${1} ${2} 2>/dev/null && echo -e "  ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/defcust/defproj/${1} -> ${2}") || \
	(echo -e "${RED}Not found file [${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/defcust/defproj/${1}]${NORMAL}")
endef

define GIT_EXPORT_LOWF_ALLOF
	@[ -d $(shell dirname ${3}) ] || mkdir -p $(shell dirname ${3}); \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/defcust/defproj/${1} ${2} 2>/dev/null && echo -e "  ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/defcust/defproj/${1} -> ${2}"); \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/${CUSTOMER_EIGEN}/defproj/${1} ${2} 2>/dev/null && echo -e "  ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/${CUSTOMER_EIGEN}/defproj/${1} -> ${2}"); \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${1} ${2} 2>/dev/null && echo -e "  ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${1} -> ${2}"); \
	([ -e ${GIT_LSERVER}/ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/defcust/defproj/${1} ] || [ -e ${GIT_LSERVER}/ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/${CUSTOMER_EIGEN}/defproj/${1} ] || [ -e ${GIT_LSERVER}/ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${1} ] || echo -e "${RED}Not found file [${1}]${NORMAL}")
endef

#define GIT_EXPORT_LOWF_DEFPROJ
#	@[ -d $(shell dirname ${2}) ] || mkdir -p $(shell dirname ${2}); \
#	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/${CUSTOMER_EIGEN}/defproj/${1} ${2} 2>/dev/null && echo -e "  ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/${CUSTOMER_EIGEN}/defproj/${1} -> ${2}") || \
#	echo -e "${RED}Not found file [${1}]${NORMAL}"
#endef

#define GIT_EXPORT_LOWF_DEFCUST_DEFPROJ
#	@[ -d $(shell dirname ${2}) ] || mkdir -p $(shell dirname ${2}); \
#	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/defcust/defproj/${1} ${2} 2>/dev/null && echo -e "  ovfs_lowersys/${CPUFAMILY_EIGEN}/${SDKVER_EIGEN}/defcust/defproj/${1} -> ${2}") || \
#	echo -e "${RED}Not found file [${1}]${NORMAL}"
#endef

define GIT_EXPORT_PCEF
	@[ -d $(shell dirname ${3}) ] || mkdir -p $(shell dirname ${3}); \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${PLATFORM_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${PLATFORM_EIGEN}/${2} -> ${3}") || \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/defplat/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/defplat/${2} -> ${3}") || \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/${CUSTOMER_EIGEN}/defproj/${PLATFORM_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/${CUSTOMER_EIGEN}/defproj/${PLATFORM_EIGEN}/${2} -> ${3}") || \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/defcust/${PROJECT_EIGEN}/${PLATFORM_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/defcust/${PROJECT_EIGEN}/${PLATFORM_EIGEN}/${2} -> ${3}") || \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/${CUSTOMER_EIGEN}/defproj/defplat/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/${CUSTOMER_EIGEN}/defproj/defplat/${2} -> ${3}") || \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/defcust/defproj/defplat/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/defcust/defproj/defplat/${2} -> ${3}") || \
	(echo -e "${RED}Not found file [${1}]/[${2}]${NORMAL}")
endef

define GIT_EXPORT_PCEF_ALLOF
	@[ -d $(shell dirname ${3}) ] || mkdir -p $(shell dirname ${3}); \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/defcust/defproj/defplat/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/defcust/defproj/defplat/${2} -> ${3}"); \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/${CUSTOMER_EIGEN}/defproj/defplat/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/${CUSTOMER_EIGEN}/defproj/defplat/${2} -> ${3}"); \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/defcust/${PROJECT_EIGEN}/${PLATFORM_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/defcust/${PROJECT_EIGEN}/${PLATFORM_EIGEN}/${2} -> ${3}"); \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/${CUSTOMER_EIGEN}/defproj/${PLATFORM_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/${CUSTOMER_EIGEN}/defproj/${PLATFORM_EIGEN}/${2} -> ${3}"); \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/defplat/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/defplat/${2} -> ${3}"); \
	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${PLATFORM_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${PLATFORM_EIGEN}/${2} -> ${3}"); \
	([ -e ${GIT_LSERVER}/ovfs_pcend/${1}/defcust/defproj/defplat/${2} ] || [ -e ${GIT_LSERVER}/ovfs_pcend/${1}/${CUSTOMER_EIGEN}/defproj/defplat/${2} ] || [ -e ${GIT_LSERVER}/ovfs_pcend/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/defplat/${2} ] || [ -e ${GIT_LSERVER}/ovfs_pcend/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${PLATFORM_EIGEN}/${2} ] || echo -e "${RED}Not found file [${1}]/[${2}]${NORMAL}")
endef

#define GIT_EXPORT_PCEF_DEFPROJ
#	@[ -d $(shell dirname ${3}) ] || mkdir -p $(shell dirname ${3}); \
#	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/${CUSTOMER_EIGEN}/defproj/defplat/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/${CUSTOMER_EIGEN}/defproj/defplat/${2} -> ${3}") || \
#	echo -e "${RED}Not found file [${1}]/[${2}]${NORMAL}"
#endef

#define GIT_EXPORT_PCEF_DEFCUST_DEFPROJ
#	@[ -d $(shell dirname ${3}) ] || mkdir -p $(shell dirname ${3}); \
#	(${GIT_EXPORT} ${GIT_LSERVER}/ovfs_pcend/${1}/defcust/defproj/defplat/${2} ${3} 2>/dev/null && echo -e "  ovfs_pcend/${1}/defcust/defproj/defplat/${2} -> ${3}") || \
#	echo -e "${RED}Not found file [${1}]/[${2}]${NORMAL}"
#endef

define GIT_EXPORT_FILE
	@[ -d $(shell dirname ${3}) ] || mkdir -p $(shell dirname ${3}); \
	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${CPUFAMILY_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${CPUFAMILY_EIGEN}/${2} -> ${3}") || \
	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/defplat/${2} ${3} 2>/dev/null && echo -e "  ${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/defplat/${2} -> ${3}") || \
	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/${CUSTOMER_EIGEN}/defproj/${CPUFAMILY_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ${1}/${CUSTOMER_EIGEN}/defproj/${CPUFAMILY_EIGEN}/${2} -> ${3}") || \
	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/defcust/${PROJECT_EIGEN}/${CPUFAMILY_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ${1}/defcust/${PROJECT_EIGEN}/${CPUFAMILY_EIGEN}/${2} -> ${3}") || \
	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/defcust/defproj/${CPUFAMILY_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ${1}/defcust/defproj/${CPUFAMILY_EIGEN}/${2} -> ${3}") || \
	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/defcust/defproj/defplat/${2} ${3} 2>/dev/null && echo -e "  ${1}/defcust/defproj/defplat/${2} -> ${3}") || \
	(echo -e "${RED}Not found file [${CPUFAMILY_EIGEN}/${1}]/[${2}]${NORMAL}")
endef

define GIT_EXPORT_FILE_ALLOF
	@[ -d $(shell dirname ${3}) ] || mkdir -p $(shell dirname ${3}); \
	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/defcust/defproj/defplat/${2} ${3} 2>/dev/null && echo -e "  ${1}/defcust/defproj/defplat/${2} -> ${3}"); \
	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/defcust/defproj/${CPUFAMILY_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ${1}/defcust/defproj/${CPUFAMILY_EIGEN}/${2} -> ${3}"); \
	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/${CUSTOMER_EIGEN}/defproj/defplat/${2} ${3} 2>/dev/null && echo -e "  ${1}/${CUSTOMER_EIGEN}/defproj/defplat/${2} -> ${3}"); \
	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/${CUSTOMER_EIGEN}/defproj/${CPUFAMILY_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ${1}/${CUSTOMER_EIGEN}/defproj/${CPUFAMILY_EIGEN}/${2} -> ${3}"); \
	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/defplat/${2} ${3} 2>/dev/null && echo -e "  ${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/defplat/${2} -> ${3}"); \
	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${CPUFAMILY_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${CPUFAMILY_EIGEN}/${2} -> ${3}"); \
	([ -e ${GIT_LSERVER}/${1}/defcust/defproj/${CPUFAMILY_EIGEN}/${2} ] || [ -e ${GIT_LSERVER}/${1}/${CUSTOMER_EIGEN}/defproj/${CPUFAMILY_EIGEN}/${2} ] || [ -e ${GIT_LSERVER}/${1}/${CUSTOMER_EIGEN}/${PROJECT_EIGEN}/${CPUFAMILY_EIGEN}/${2} ] || echo -e "${RED}Not found file [${1}]/[${2}]${NORMAL}")
endef

#define GIT_EXPORT_FILE_DEFPROJ
#	@[ -d $(shell dirname ${3}) ] || mkdir -p $(shell dirname ${3}); \
#	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/${CUSTOMER_EIGEN}/defproj/${CPUFAMILY_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ${1}/${CUSTOMER_EIGEN}/defproj/${CPUFAMILY_EIGEN}/${2} -> ${3}") || \
#	echo -e "${RED}Not found file [${1}]/[${2}]${NORMAL}"
#endef

#define GIT_EXPORT_FILE_DEFCUST_DEFPROJ
#	@[ -d $(shell dirname ${3}) ] || mkdir -p $(shell dirname ${3}); \
#	(${GIT_EXPORT} ${GIT_LSERVER}/${1}/defcust/defproj/${CPUFAMILY_EIGEN}/${2} ${3} 2>/dev/null && echo -e "  ${1}/defcust/defproj/${CPUFAMILY_EIGEN}/${2} -> ${3}") || \
#	echo -e "${RED}Not found file [${1}]/[${2}]${NORMAL}"
#endef
