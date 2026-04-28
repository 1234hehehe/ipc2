
# 必须指定 TARGET_NAME 和 TARGET_TYPE,表示编译目标的名称和类型.类型可以是 none,libs,libd,lib,exe.
#TARGET_NAME:=$(shell pwd | xargs basename)
TARGET_NAME:=ovfs_core_2063
TARGET_TYPE:=exe

# 指定当前路径CUR_PATH和整个工程的根路径.一般需要指定子模块的Makefile中的根路径.
CUR_PATH:=.
ROOT_PATH?=${CUR_PATH}/../..

# 包含Makefile.pre.mak.
include ${ROOT_PATH}/Makefile.pre.mak

# 子系统差异化.
# [NOTE]本代码位置应该在包含Makefile.pre.mak之后,在包含Makefile.main.mak之前.
# OPT_FLAGS_OWN,表示编译选项,比如指定cpu属性.
# OPT_FLAGS_OWN+=
# MAC_FLAGS_OWN,表示宏定义,包含-D部分.
# MAC_FLAGS_OWN+=
# INC_PATH_OWN,表示头文件的搜索路径,不包含-I部分.
# INC_PATH_OWN+=${ROOT_PATH}/inc/common ${CUR_PATH}/inc
# LIB_PATH_OWN,表示库文件的搜索路径,不包含-L部分.
# LIB_PATH_OWN+=${ROOT_PATH}/lib/${PLATFORM_EIGEN} ${OUT_PATH}/lib
# INC_FILE_EXPORT,表示需要导出到公共目录的头文件,以便引用此模块接口使用.
# INC_FILE_EXPORT:=
# LIB_FILE_OWN,表示需要静态链接的库文件的路径和文件名.
LIB_FILE_OWN+=${PUB_PATH}/lib/librtc.a ${PUB_PATH}/lib/libwtdg.a
# LNK_FLAGS_OWN,表示需要链接的动态库,包含-l部分.
LNK_FLAGS_OWN+=-lmodule -lstreamqueue -lcommon -ldl -laccess_sdk
# LNK_FLAGS_OWN+=-lpthread -lm -ldl -lrt -lstdc++
# LNK_FLAGS_OWN+=$(foreach libfile,${LIB_FILE_OWN},$(patsubst lib%.so,-l%,$(shell basename ${libfile})))
LNK_FLAGS_OWN+=-lcurl -lssl -lcrypto -lz -lalarm_sdk_t41

# 包含Makefile.main.mak.
include ${ROOT_PATH}/Makefile.main.mak
