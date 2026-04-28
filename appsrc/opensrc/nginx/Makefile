
# 必须指定 TARGET_NAME 和 TARGET_TYPE,表示编译目标的名称和类型.类型可以是 none,libs,libd,lib,exe.
TARGET_NAME:=none
TARGET_TYPE:=none

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
OPENUP_PATH_MINI:=objs/${PLATFORM_EIGEN}/mini
ARCHIVE_FILE:=nginx-1.14.0.tar.gz
WORKING_PATH_FULL:=nginx-1.28.2
WORKING_PATH_MINI:=${OPENUP_PATH_MINI}/nginx-1.28.2

.PHONY:build clean distclean build_full build_mini openup_full openup_mini config_full config_mini compile_full compile_mini finish_full finish_mini

build:build_full

build_full: openup_full config_full compile_full finish_full
build_mini: openup_mini config_mini compile_mini finish_mini

distclean:
	-rm -rf objs;
clean:
	#-rm -rf ${WORKING_PATH_FULL} ${WORKING_PATH_MINI};
	cd ${WORKING_PATH_FULL} && make clean && rm -rf *.pc
	
openup_full: ${WORKING_PATH_FULL}
${WORKING_PATH_FULL}:
	#mkdir -p ${OPENUP_PATH_FULL} && tar -xzf ${ARCHIVE_FILE} -C ${OPENUP_PATH_FULL};
openup_mini: ${WORKING_PATH_MINI}
${WORKING_PATH_MINI}:
	mkdir -p ${OPENUP_PATH_MINI} && tar -xzf ${ARCHIVE_FILE} -C ${OPENUP_PATH_MINI};
	
config_full: openup_full ${WORKING_PATH_FULL}/Makefile
${WORKING_PATH_FULL}/Makefile:
	
config_mini: openup_mini ${WORKING_PATH_MINI}/Makefile
${WORKING_PATH_MINI}/Makefile:
	@cd ${WORKING_PATH_MINI} && patch -p1 <${CUR_PATH}/nginx-1.14.0-mini.patch && \
	OUT_PATH=${OUT_PATH} ./configure --with-pcre=${OUT_PATH} --with-zlib=${OUT_PATH} \
	--http-proxy-temp-path=/tmp/var/nginx \
	--error-log-path=/tmp/var/nginx \
	--without-select_module \
	--without-mail_pop3_module --without-mail_imap_module --without-mail_smtp_module \
	--without-http_upstream_hash_module --without-http_upstream_ip_hash_module --without-http_upstream_least_conn_module --without-http_upstream_zone_module \
	--without-http_ssi_module --without-http_userid_module --without-http_autoindex_module --without-http_geo_module --without-http_uwsgi_module --without-http_scgi_module --without-http_empty_gif_module --without-http_browser_module \
	--without-stream_limit_conn_module --without-stream_access_module --without-stream_upstream_hash_module --without-stream_upstream_least_conn_module --without-stream_upstream_zone_module \
	--with-cc=${COMPILER_PREFIX}gcc --with-cpp=${COMPILER_PREFIX}g++ \
	--with-cc-opt="${CFLAGS} -L${OUT_PATH}/lib" --with-ld-opt="${CFLAGS} -L${OUT_PATH}/lib"; \
	sed -i -e "s/[^ ]*\/libz.a/-lz/g" \
	-e "s/inc\/.libs\/libpcre.a/lib\/libpcre.a/g" \
	objs/Makefile;
	
compile_full: config_full ${WORKING_PATH_FULL}/objs/nginx
${WORKING_PATH_FULL}/objs/nginx:
	make -C ${WORKING_PATH_FULL};
	${STRIP} ${WORKING_PATH_FULL}/objs/nginx;
compile_mini: config_mini ${WORKING_PATH_MINI}/objs/nginx
${WORKING_PATH_MINI}/objs/nginx:
	make -C ${WORKING_PATH_MINI};
	${STRIP} ${WORKING_PATH_MINI}/objs/nginx;
	
finish_full: compile_full ${OUT_PATH}/bin/nginx
${OUT_PATH}/bin/nginx: ${WORKING_PATH_FULL}/objs/nginx
	@mkdir -p $(dir $@);
	cp -rLpT $? $@;

finish_mini: compile_mini ${OUT_PATH}/bin/nginx_mini
${OUT_PATH}/bin/nginx_mini: ${WORKING_PATH_MINI}/objs/nginx
	@mkdir -p $(dir $@);
	cp -rLpT $? $@;
	
