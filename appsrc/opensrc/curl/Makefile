
# 必须指定 TARGET_NAME 和 TARGET_TYPE,表示编译目标的名称和类型.类型可以是 none,libs,libd,lib,exe.
TARGET_NAME?=none
TARGET_TYPE?=none

# 指定当前路径CUR_PATH和整个工程的根路径.一般需要指定子模块的Makefile中的根路径.
CUR_PATH:=$(shell pwd)
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


OPENUP_PATH_FULL:=objs/${PLATFORM_EIGEN}/full
ARCHIVE_FILE:=curl-7.60.0.tar.gz
WORKING_PATH_FULL:=curl-8.19.0

.PHONY:build clean distclean build_full openup_full config_full compile_full finish_full

build: build_full
build_full: openup_full config_full compile_full finish_full

distclean:
	-rm -rf objs;
clean:
	#-rm -rf ${WORKING_PATH_FULL} ${WORKING_PATH_MINI};
	cd ${WORKING_PATH_FULL} && make clean && rm -rf *.pc

openup_full:${WORKING_PATH_FULL}
${WORKING_PATH_FULL}:
	#@mkdir -p ${OPENUP_PATH_FULL} && tar -xzf ${ARCHIVE_FILE} -C ${OPENUP_PATH_FULL};

config_full: openup_full ${WORKING_PATH_FULL}/libcurl.pc
${WORKING_PATH_FULL}/libcurl.pc:
	cd ${WORKING_PATH_FULL} && ./configure CFLAGS="${CFLAGS} -DHAVE_CONFIG_H -DUSE_OPENSSL -I${OUT_PATH}/inc" \
	CPPFLAGS="${CFLAGS} -DHAVE_CONFIG_H -DUSE_OPENSSL -I${OUT_PATH}/inc" \
	LDFLAGS="${CFLAGS} -DHAVE_CONFIG_H -DUSE_OPENSSL -I${OUT_PATH}/inc" \
	CXXFLAGS="${CFLAGS} -DHAVE_CONFIG_H -DUSE_OPENSSL -I${OUT_PATH}/inc" \
	--host=${COMPILER_NAME} --disable-rtsp --disable-gopher --disable-manual --with-zlib=${OUT_PATH}/inc  --with-openssl=${OUT_PATH}/ssl --without-libpsl
	
compile_full: config_full ${WORKING_PATH_FULL}/lib/.libs/libcurl.so ${WORKING_PATH_FULL}/lib/.libs/libcurl.a
${WORKING_PATH_FULL}/lib/.libs/libcurl.so ${WORKING_PATH_FULL}/lib/.libs/libcurl.a:
	make -C ${WORKING_PATH_FULL}/lib libcurl.la;
	${STRIP} -p ${WORKING_PATH_FULL}/lib/.libs/libcurl.so

finish_full: compile_full ${OUT_PATH}/lib/libcurl.so ${OUT_PATH}/lib/libcurl.a ${OUT_PATH}/inc/curl
${OUT_PATH}/lib/libcurl.so: ${WORKING_PATH_FULL}/lib/.libs/libcurl.so
	@mkdir -p $(dir $@);
	cp -rLpT $? $@;
${OUT_PATH}/lib/libcurl.a: ${WORKING_PATH_FULL}/lib/.libs/libcurl.a
	@mkdir -p $(dir $@);
	cp -rLpT $? $@;
${OUT_PATH}/inc/curl: ${WORKING_PATH_FULL}/include/curl
	@mkdir -p $(dir $@);
	cp -rLpT $? $@;
