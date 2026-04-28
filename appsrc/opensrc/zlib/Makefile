
# 必须指定 TARGET_NAME 和 TARGET_TYPE,表示编译目标的名称和类型.类型可以是 none,libs,libd,lib,exe.
TARGET_NAME?=none
TARGET_TYPE?=none

# 指定当前路径CUR_PATH和整个工程的根路径.一般需要指定子模块的Makefile中的根路径.
CUR_PATH:=.
ROOT_PATH?=${CUR_PATH}/..

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
# LIB_FILE_OWN+=$(foreach libpath,${LIB_PATH_OWN},$(wildcard ${libpath}/libbmcast.a))
# LNK_FLAGS_OWN,表示需要链接的动态库,包含-l部分.
# LNK_FLAGS_OWN+=-lpthread -lm -ldl -lrt -lstdc++
# LNK_FLAGS_OWN+=$(foreach libfile,${LIB_FILE_OWN},$(patsubst lib%.so,-l%,$(shell basename ${libfile})))
# INCLUDE_OBJS,表示仅编译指定的obj文件.
# INCLUDE_OBJS+=
# EXCLUDE_OBJS,表示不需要编译的obj文件.
# EXCLUDE_OBJS+=

# 包含Makefile.main.mak.
#include ${ROOT_PATH}/Makefile.main.mak


OPENUP_PATH:=objs/${PLATFORM_EIGEN}
ARCHIVE_FILE:=zlib-1.3.2.tar.gz
WORKING_PATH:=zlib-1.3.2

.PHONY:build clean distclean openup config compile finish

build: openup config compile finish

distclean:
	-rm -rf objs;
clean:
	#-rm -rf ${WORKING_PATH};
	@cd ${WORKING_PATH} && make clean && rm -rf zlib.pc
openup:${WORKING_PATH}
${WORKING_PATH}:
	#@mkdir -p ${OPENUP_PATH} && tar -xzf ${ARCHIVE_FILE} -C ${OPENUP_PATH};
	
config: openup ${WORKING_PATH}/zlib.pc
${WORKING_PATH}/zlib.pc:
	@cd ${WORKING_PATH} && sed -i -e "s/-soname,lib.*\.so\.[0-9],//g" configure && \
	CC=${COMPILER_NAME}-gcc CFLAGS="${CFLAGS}" LDFLAGS="${CFLAGS}" CPPFLAGS="${CFLAGS}" CXXFLAGS="${CFLAGS}" ./configure;
	
compile: config ${WORKING_PATH}/libz.so ${WORKING_PATH}/libz.a
${WORKING_PATH}/libz.so ${WORKING_PATH}/libz.a:
	make -C ${WORKING_PATH};
	${STRIP} ${WORKING_PATH}/libz.so;
	
finish: compile ${OUT_PATH}/lib/libz.so ${OUT_PATH}/lib/libz.a ${OUT_PATH}/inc/zlib.h ${OUT_PATH}/inc/zconf.h
${OUT_PATH}/lib/libz.so: ${WORKING_PATH}/libz.so
	@mkdir -p $(dir $@);
	cp -rLpT $? $@;
${OUT_PATH}/lib/libz.a: ${WORKING_PATH}/libz.a
	@mkdir -p $(dir $@);
	cp -rLpT $? $@;
${OUT_PATH}/inc/zconf.h: ${WORKING_PATH}/zconf.h
	@mkdir -p $(dir $@);
	cp -rLpT $? $@;
${OUT_PATH}/inc/zlib.h: ${WORKING_PATH}/zlib.h
	@mkdir -p $(dir $@);
	cp -rLpT $? $@;
	
