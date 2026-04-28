
# 必须指定PLATFORM_NAME,表示CPU平台.这将决定选择哪种编译工具链.
#export PLATFORM_NAME?=host
#export PLATFORM_NAME?=HI3516A
#export PLATFORM_NAME?=HI3516AV080
#export PLATFORM_NAME?=HI3519
#export PLATFORM_NAME?=HI3516CV300
#export PLATFORM_NAME?=HI3516E
#export PLATFORM_NAME?=MS316
export PLATFORM_NAME?=JZT30
#export PLATFORM_NAME?=HI3516CV500
#export PLATFORM_NAME?=HI3516EV200

# 是否输出冗余打印信息,CC,CFLAGS等.
export VERBOSE?=y
# 是否strip
export STRIPED?=y

# 打印输出颜色的定义,使用echo -e.
NORMAL=\e[0;39m
RED=\e[1;31m
GREEN=\e[1;32m
YELLOW=\e[1;33m
BLUE=\e[1;34m
MAGENTA=\e[1;35m
CYAN=\e[1;36m
WHITE=\e[1;37m

# 推断,编译工具链,编译选项.
PLATFORM_EIGEN=$(shell echo ${PLATFORM_NAME} | tr "[A-Z]" "[a-z]")

ifeq (${PLATFORM_NAME}, HI3516A)
export COMPILER_NAME=arm-hisiv300-linux
export COMPILER_PREFIX=arm-hisiv300-linux-
CPU_FLAGS  +=-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4

else ifeq (${PLATFORM_NAME}, HI3516AV080)
export COMPILER_NAME=arm-hisiv510-linux
export COMPILER_PREFIX=arm-hisiv510-linux-
CPU_FLAGS  +=-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations

else ifeq (${PLATFORM_NAME}, HI3519)
export COMPILER_NAME=arm-hisiv500-linux
export COMPILER_PREFIX=arm-hisiv500-linux-
CPU_FLAGS  +=-mcpu=cortex-a17.cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations

else ifeq (${PLATFORM_NAME}, HI3516CV300)
export COMPILER_NAME=arm-hisiv500-linux
export COMPILER_PREFIX=arm-hisiv500-linux-
CPU_FLAGS  +=-mcpu=arm926ej-s -mno-unaligned-access -fno-aggressive-loop-optimizations

else ifeq (${PLATFORM_NAME}, HI3516E)
export COMPILER_NAME=arm-hisiv500-linux
export COMPILER_PREFIX=arm-hisiv500-linux-
CPU_FLAGS  +=-mcpu=arm926ej-s -mno-unaligned-access -fno-aggressive-loop-optimizations -Os

else ifeq (${PLATFORM_NAME}, HI3516EV200)
export COMPILER_NAME=arm-himix100-linux
export COMPILER_PREFIX=arm-himix100-linux-
CPU_FLAGS  +=-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -fno-aggressive-loop-optimizations -Os

else ifeq (${PLATFORM_NAME}, MS316)
export COMPILER_NAME=arm-mstarv100-linux
export COMPILER_PREFIX=arm-mstarv100-linux-

else ifeq (${PLATFORM_NAME}, JZT30)
export COMPILER_NAME=mips-linux-gnu
export COMPILER_PREFIX=mips-linux-gnu-
CPU_FLAGS  +=-muclibc -Os

else ifeq (${PLATFORM_NAME}, JZT32)
export COMPILER_NAME=mips-t32-linux-gnu
export COMPILER_PREFIX=mips-t32-linux-gnu-
CPU_FLAGS  +=-muclibc -Os

else ifeq (${PLATFORM_NAME}, JZT33)
export COMPILER_NAME=mips-t33-linux-gnu
export COMPILER_PREFIX=mips-t33-linux-gnu-
CPU_FLAGS  +=-muclibc -Os -fstack-protector-all 

else ifeq (${PLATFORM_NAME}, JZT40)
export COMPILER_NAME=mips-t40-linux-gnu
export COMPILER_PREFIX=mips-t40-linux-gnu-
CPU_FLAGS  +=-muclibc -EL -mips32r2 -mfp64 -mmsa -mabs=2008 -mnan=2008

else ifeq (${PLATFORM_NAME}, JZT41)
export COMPILER_NAME=mips-t41-linux-gnu
export COMPILER_PREFIX=mips-t41-linux-gnu-
CPU_FLAGS  +=-muclibc -EL -march=mips32r2 -mfp64 -mmsa -mabs=2008 -mnan=2008

else ifeq (${PLATFORM_NAME}, SSC335)
export COMPILER_NAME=arm-mstarv300-linux
export COMPILER_PREFIX=arm-mstarv300-linux-
CPU_FLAGS  += -DLINUX_OS

else ifeq (${PLATFORM_NAME}, HI3559A)
export COMPILER_NAME=aarch64-himix100-linux
export COMPILER_PREFIX=aarch64-himix100-linux-

else ifeq (${PLATFORM_NAME}, HI3516CV500)
export COMPILER_NAME=arm-himix200-linux
export COMPILER_PREFIX=arm-himix200-linux-
CPU_FLAGS  +=-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -fno-aggressive-loop-optimizations

else ifeq (${PLATFORM_NAME}, host)
export COMPILER_NAME=
export COMPILER_PREFIX=

else 
$(error Unknown platform ${PLATFORM_NAME})

endif

# 导出编译工具链.
export CC=${COMPILER_PREFIX}gcc
export CXX=${COMPILER_PREFIX}g++
export AR=${COMPILER_PREFIX}ar
export LINK=${COMPILER_PREFIX}ar cqs
ifeq (${STRIPED},y)
export STRIP=${COMPILER_PREFIX}strip -p
else
export STRIP=ls -l
endif

# 增加芯片平台相关的宏定义
MAC_FLAGS:=-DPLATFORM_${PLATFORM_NAME} -DLINUX_${PLATFORM_NAME} -DSDKVER_${SDKVER_NAME}
ifeq (${DBG},y)
OPT_FLAGS:=-lpthread -ldl -g -fno-omit-frame-pointer -rdynamic -Wall -fPIC -pthread
STRIPED=n
else
OPT_FLAGS:=-Os -Wall -fPIC -pthread
endif
#OPT_FLAGS+=-fexceptions -funwind-tables -finstrument-functions -fasynchronous-unwind-tables -ffunction-sections -fdata-sections
#OPT_FLAGS+=-ffunction-sections -fdata-sections
# ARM: -mapcs-frame 
CFLAGS:=${CPU_FLAGS} ${MAC_FLAGS} ${OPT_FLAGS}

ifneq (${VERBOSE},y)
define COMPILE_AND_PRINT
	@echo -en "Compiling $@\r";
	@($(1) && echo -e "${GREEN}Completed $@${NORMAL}") || echo -e "${RED}ERROR ${YELLOW}$(1) ${NORMAL}"
endef
else
define COMPILE_AND_PRINT
	$(1)
endef
endif

# Define some usual paths.
#/
#|---inc
#|   |---common
#|   |---extern
#|---lib
#|   |---${PLATFORM_EIGEN}
#|---out
#|   |---${PLATFORM_EIGEN}
#|        |---bin
#|        |---lib
#|---modx
#|   |---inc
#|   |---lib
#|   |*.c/*.h/*.cpp

IN_PATH:=${ROOT_PATH}/in/${PLATFORM_EIGEN}
OUT_PATH:=${ROOT_PATH}/out/${PLATFORM_EIGEN}
PUB_PATH:=${ROOT_PATH}/../pub/${PLATFORM_EIGEN}
INC_PATH:=
LIB_PATH:=
